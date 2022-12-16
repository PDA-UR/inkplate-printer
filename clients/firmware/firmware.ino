#include <Arduino.h>

#include "Inkplate.h"
#include "./libraries/InkplateLibrary/Fonts/FreeMono9pt7b.h"
#include "./libraries/InkplateLibrary/Fonts/FreeMonoBold12pt7b.h"
#include "./libraries/InkplateLibrary/Fonts/FreeMono24pt7b.h"
#include "./libraries/InkplateLibrary/Fonts/FreeMonoBold24pt7b.h"

#include <ArduinoJson.h>
#include "SdFat.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include "socket_messages.h"
#include "./icons/arrow_left_58.h"
#include "./icons/arrow_right_58.h"
#include "./icons/enqueue_58.h"

// ##################################################################### //
// ############################## Firmware ############################# //
// ##################################################################### //

// ~~~~~~~~~~~~~ Definitions ~~~~~~~~~~~~~ //

#define USE_SERIAL Serial
#define formatBool(b) ((b) ? "true" : "false")

Inkplate display(INKPLATE_3BIT);

// ====================================================== //
// ====================== Constants ===================== //
// ====================================================== //

const int AWAKE_TIME = 5; // seconds

// ~~~~~~~~~~~~~ File System ~~~~~~~~~~~~~ //

const String PAGE_CHAIN_DIR = "/page_chain/";
const String CONFIG_FILE = "/config.json";
const String STATE_FILE = "/state.json";

// ~~~~~~~~~~~~~~~~~ GUI ~~~~~~~~~~~~~~~~~ //
const int ARROW_HEAD_HEIGHT = 25;
const int ARROW_HEAD_WIDTH = 20;

// ~~~~~~~~~~~~~~~~~ WiFi ~~~~~~~~~~~~~~~~ //

String SSID;
String PASSWORD;
String HOST;
int PORT;

IPAddress local_ip;
IPAddress gateway;
IPAddress subnet;
IPAddress dns1;
IPAddress dns2;

// ~~~~~~~~~~~~~~~~ Display ~~~~~~~~~~~~~~~ //

int DPI;
int COLOR_DEPTH;
int DISPLAY_WIDTH;
int DISPLAY_HEIGHT;

// ====================================================== //
// ====================== Variables ===================== //
// ====================================================== //

bool is_setup = false;

// ~~~~~~~~~~~~~~~ Display ~~~~~~~~~~~~~~~ //

int wake_up_timestamp = 0;

// ~~~~~~~~~~~~~~ Touchpads ~~~~~~~~~~~~~~ //

bool touchpad_released = true;
int touchpad_cooldown_ms = 1000;
long touchpad_released_time = 0;

enum TP_PRESSED
{
  tp_none = 0,
  tp_left = 1,
  tp_middle = 2,
  tp_right = 3
} touchpad_pressed;

// ~~~~~~~~~~~~~~~~ State ~~~~~~~~~~~~~~~~ //

// ------- Document ------- //

int page_index = -1;
int page_count = -1;

// --------- Modes -------- //

enum DisplayMode
{
  blank = 0,
  displaying = 1,
  paired = 2,
} display_mode;

// -------- Device -------- //

int device_index = -1;
int pairing_index = -1;

// ~~~~~~~~~~~~~~~ Network ~~~~~~~~~~~~~~~ //

String mac_addr;
SocketIOclient socketIO;

boolean is_registered = false;

// ====================================================== //
// ======================= General ====================== //
// ====================================================== //

boolean do_go_to_sleep()
{
  return (millis() - wake_up_timestamp) > AWAKE_TIME * 1000;
}

void enter_deep_sleep()
{
  Serial.println("Going to sleep");
  hide_gui();
  esp_deep_sleep_start();
}

// ====================================================== //
// ======================= Storage ====================== //
// ====================================================== //

bool setup_storage()
{
  // Init SD card. Display if SD card is init properly or not.
  USE_SERIAL.println("Setup: Storage");

  if (display.sdCardInit())
  {
    USE_SERIAL.println("SD Card init ok!");

    SdFile root;
    USE_SERIAL.println("Checking " + PAGE_CHAIN_DIR);
    if (root.open("/", O_READ))
    {
      USE_SERIAL.println("SD Card Accessible");
      root.close();
      return true;
    }
    else
      USE_SERIAL.println("Failed to open root dir of SD Card");
  }
  else
    USE_SERIAL.println("SD Card init ERROR!");

  return false;
}

void save_page(int &page_index, uint8_t *img_download_buffer)
{
  SdFile file;
  String bmp_filename = get_page_filepath(page_index);

  // make sure file can be created, otherwise print error

  USE_SERIAL.println(bmp_filename.c_str());

  if (file.open(bmp_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC))
  {
    int file_size = READ32(img_download_buffer + 2);
    file.truncate(0);
    file.write(img_download_buffer, file_size);
    file.flush();
  }
  else
    USE_SERIAL.println("Error opening bmp file");

  file.close();
}

uint8_t *get_page(int page_index)
{
  SdFile file;
  String filepath = get_page_filepath(page_index);
  uint8_t *img_buffer = NULL;

  USE_SERIAL.println(filepath.c_str());

  if (file.open(filepath.c_str(), O_READ))
  {
    int file_size = file.fileSize();
    img_buffer = (uint8_t *)malloc(file_size);
    file.read(img_buffer, file_size);
  }
  else
    USE_SERIAL.println("Error opening bmp file");

  file.close();
  return img_buffer;
}

void clear_stored_pages()
{
  USE_SERIAL.println("Clearing stored images");
  SdFile dir;
  SdFile file;
  if (dir.open(PAGE_CHAIN_DIR.c_str()))
  {
    while (file.openNext(&dir, O_READ))
    {
      file.remove();
      file.close();
    }
    dir.close();
  }
  else
    Serial.println("Error opening page chain directory");
}

int get_num_stored_pages()
{
  File dir;
  SdFile file;
  int count = 0;
  if (dir.open(PAGE_CHAIN_DIR.c_str()))
  {
    while (file.openNext(&dir, O_READ))
    {
      count++;
      file.close();
    }
    dir.close();
  }
  else
    Serial.println("Error opening page chain directory");

  return count;
}

String get_page_filename(int page_index)
{
  String filename = String(page_index) + ".bmp";
  USE_SERIAL.println("filename " + filename);
  return filename;
}

String get_page_filepath(int page_index)
{
  String filepath = PAGE_CHAIN_DIR + get_page_filename(page_index);
  USE_SERIAL.println("filepath " + filepath);
  return filepath;
}

// ~~~~~~~~~~~~~~~~ Config ~~~~~~~~~~~~~~~ //

String get_host_url()
{
  return "http://" + String(HOST) + ":" + String(PORT);
}

bool load_config()
{
  USE_SERIAL.println("Setup: Config");

  File configFile;
  if (!configFile.open(CONFIG_FILE.c_str(), O_READ))
  {
    USE_SERIAL.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    USE_SERIAL.println("Config file size is too large");
    return false;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  if (error)
  {
    USE_SERIAL.println("Failed to parse config file");
    return false;
  }

  // get wifi sub property
  JsonObject wifi = doc["wifi"];
  if (!wifi.isNull())
  {
    SSID = wifi["ssid"].as<String>();
    PASSWORD = wifi["password"].as<String>();
  }
  else
    return false;

  // get api sub property
  JsonObject api = doc["api"];
  if (!api.isNull())
  {
    HOST = api["host"].as<String>();
    PORT = api["port"].as<int>();
  }
  else
    return false;

  JsonObject network = doc["network"];
  if (!network.isNull())
  {
    JsonArray LOCAL_IP = network["local_ip"].as<JsonArray>();
    local_ip = parse_ip_json(LOCAL_IP);
    JsonArray GATEWAY = network["gateway"].as<JsonArray>();
    gateway = parse_ip_json(GATEWAY);
    JsonArray SUBNET = network["subnet"].as<JsonArray>();
    subnet = parse_ip_json(SUBNET);
    JsonArray DNS1 = network["dns1"].as<JsonArray>();
    dns1 = parse_ip_json(DNS1);
    JsonArray DNS2 = network["dns2"].as<JsonArray>();
    dns2 = parse_ip_json(DNS2);

    // free up memeory
    LOCAL_IP.clear();
    GATEWAY.clear();
    SUBNET.clear();
    DNS1.clear();
    DNS2.clear();
  }
  else
    return false;

  JsonObject display = doc["display"];
  if (!display.isNull())
  {
    DPI = display["dpi"].as<int>();
    COLOR_DEPTH = display["color_depth"].as<int>();
  }
  else
    return false;

  return true;
}

IPAddress parse_ip_json(JsonArray ip_array)
{
  if (ip_array.isNull() || ip_array.size() != 4)
  {
    USE_SERIAL.println("Error parsing ip address");
    return IPAddress(0, 0, 0, 0);
  }
  return IPAddress(ip_array[0], ip_array[1], ip_array[2], ip_array[3]);
}

// ====================================================== //
// ======================== State ======================= //
// ====================================================== //

bool state_file_exists()
{
  SdFile file;
  if (file.open("state.json", O_RDWR))
  {
    file.close();
    return true;
  }
  else
    return false;
}

bool load_state()
{
  // TODO: Load state from SD card
  USE_SERIAL.println("Loading State");

  if (state_file_exists())
  {
    USE_SERIAL.println("State file exists");

    SdFile file;
    if (file.open("state.json", O_READ))
    {
      USE_SERIAL.println("State file opened");

      // read from file
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, file);
      if (!error)
      {
        USE_SERIAL.println("State file read");

        // read state
        page_index = doc["page_index"];
        page_count = doc["page_count"];
        display_mode = doc["display_mode"];

        file.close();
        return true;
      }
      else
      {
        USE_SERIAL.println("State file read error");
        file.close();
        return false;
      }
    }
    else
    {
      USE_SERIAL.println("Error opening state file!");
      return false;
    }
  }
  else
  {
    USE_SERIAL.println("State file does not exist, creating...");
    return save_state();
  }
}

bool save_state()
{
  USE_SERIAL.println("Saving State");

  // create json
  StaticJsonDocument<200> doc;
  doc["page_index"] = page_index;
  doc["page_count"] = page_count;
  doc["display_mode"] = display_mode;

  // serialize json
  String json;
  serializeJson(doc, json);

  // write to file
  SdFile file;

  if (file.open("state.json", O_RDWR | O_CREAT | O_AT_END))
  {
    // Clear the contents of the file
    file.truncate(0);
    file.seekSet(0);

    file.write(json.c_str(), json.length());
    file.flush();
    file.close();
    return true;
  }
  else
    Serial.println("Error creating file!");
  return false;
}

// ~~~~~~~~~~~~~~~~ Setter ~~~~~~~~~~~~~~~ //

void set_page_index(int index)
{
  page_index = index;
  save_state();
}

void set_page_count(int count)
{
  Serial.printf("Setting page count to %d \n", count);
  page_count = count;
  save_state();
}

void set_display_mode(DisplayMode mode)
{
  display_mode = mode;
  save_state();
}

// ====================================================== //
// ======================= Network ====================== //
// ====================================================== //

void download_and_save_page(int page_index)
{
  uint8_t *img_buf = download_page(page_index);
  if (img_buf != nullptr)
  {
    save_page(page_index, img_buf);
    // free memory
    free(img_buf);
  }
}

uint8_t *download_page(int &page_num)
{
  String url = get_host_url() + "/api/img" + "?client=" + mac_addr + "&page_num=" + page_num;

  int len = DISPLAY_WIDTH * DISPLAY_HEIGHT; // display resolution
  Serial.println("Downloading...");
  uint8_t *downloaded_image = display.downloadFile(url.c_str(), &len);

  Serial.println("Page downloaded!");
  return downloaded_image;
}

// ====================================================== //
// ======================== Wifi ======================== //
// ====================================================== //

void setup_wifi()
{
  Serial.println("Setup: WiFi");

  WiFi.config(local_ip, gateway, subnet, dns1, dns2);

  WiFi.begin(SSID.c_str(), PASSWORD.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Setup: WiFi connected");
}

// ====================================================== //
// ======================= Socket ======================= //
// ====================================================== //

void setup_socket()
{
  socketIO.begin(HOST, PORT, "/socket.io/?EIO=4");
  socketIO.onEvent(on_socket_event);
}

void socket_routine()
{
  socketIO.loop();
}

// ~~~~~~~~~~~~~~~ Messages ~~~~~~~~~~~~~~ //

// --------- Send --------- //

void send_message(String name, DynamicJsonDocument *data)
{
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.to<JsonArray>();

  arr.add(name);
  if (data != nullptr)
    arr.add(*data);

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.printf("Sent message '%s' with data", name.c_str());
  Serial.println(output);
}

void send_example_message()
{
  DynamicJsonDocument data(1024);
  data["message"] = "Hello from Arduino!";
  send_message("example", &data);
}

void send_register_message()
{
  DynamicJsonDocument data(1024);
  data["uuid"] = mac_addr;

  JsonObject screen_info = data.createNestedObject("screenInfo");
  screen_info["colorDepth"] = COLOR_DEPTH;
  screen_info["dpi"] = DPI;

  JsonObject resolution = screen_info.createNestedObject("resolution");
  resolution["width"] = DISPLAY_WIDTH;
  resolution["height"] = DISPLAY_HEIGHT;

  data["isBrowser"] = false;

  send_message(REGISTER_MESSAGE, &data);
}

void send_enqueue_message()
{
  send_message(ENQUEUE_MESSAGE, nullptr);
}

void send_dequeue_message()
{
  send_message(DEQUEUE_MESSAGE, nullptr);
}

// -------- Receive ------- //

// Code (modified) from
// // https://github.com/Links2004/arduinoWebSockets/blob/master/examples/esp32/WebSocketClientSocketIOack/WebSocketClientSocketIOack.ino
void on_socket_event(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case sIOtype_DISCONNECT:
    USE_SERIAL.printf("[IOc] Disconnected!\n");
    is_registered = false;
    break;
  case sIOtype_CONNECT:
    USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");
    send_register_message();
    break;
  case sIOtype_EVENT:
  {
    char *sptr = NULL;
    int id = strtol((char *)payload, &sptr, 10);
    // USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
    if (id)
    {
      payload = (uint8_t *)sptr;
    }
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error)
    {
      USE_SERIAL.print(F("deserializeJson() failed: "));
      USE_SERIAL.println(error.c_str());
      return;
    }

    String eventName = doc[0];
    DynamicJsonDocument data = doc[1];

    handle_socket_messages(eventName, data);
  }
  break;
  case sIOtype_ACK:
    USE_SERIAL.printf("[IOc] get ack: %u\n", length);
    break;
  case sIOtype_ERROR:
    USE_SERIAL.printf("[IOc] get error: %u\n", length);
    break;
  case sIOtype_BINARY_EVENT:
    USE_SERIAL.printf("[IOc] get binary: %u\n", length);
    break;
  case sIOtype_BINARY_ACK:
    USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
    break;
  }
}

void handle_socket_messages(
    String name,
    DynamicJsonDocument data)
{
  USE_SERIAL.printf("Handling socket message '%s' with data: ", name.c_str());

  // print the data
  String output;
  serializeJson(data, output);
  USE_SERIAL.println(output);

  if (name == REGISTERED_MESSAGE)
    handle_registered_message(data);
  else if (name == UPDATE_DEVICE_INDEX_MESSAGE)
    handle_update_device_index_message(data);
  else if (name == SHOW_PAGE_MESSAGE)
    handle_show_page_message(data);
  else if (name == PAGES_READY_MESSAGE)
    handle_pages_ready_message(data);
}

void handle_registered_message(DynamicJsonDocument data)
{
  USE_SERIAL.println("Handling registered message");
  if (data["wasSuccessful"].as<bool>())
  {
    USE_SERIAL.println("Registered successfully");
    is_registered = true;
    // handle_middle_tp_pressed(); // DEBUG: Auto enqueue on connect
  }
  else
  {
    USE_SERIAL.println("Registered unsuccessfully");
    is_registered = false;
  }
}

void handle_update_device_index_message(DynamicJsonDocument data)
{
  USE_SERIAL.print("Handling update device index message, new index:");
  device_index = data["deviceIndex"].as<int>();
  draw_device_index_info();
  display.display();
}

void handle_show_page_message(DynamicJsonDocument data)
{
  // TODO: Handle null / -1
  USE_SERIAL.println("Handling show page message");
  int new_page_index = data["pageIndex"].as<int>();

  set_page_index(new_page_index);
  set_display_mode(displaying);
  save_state();
}

void handle_pages_ready_message(DynamicJsonDocument data)
{
  USE_SERIAL.println("Handling pages ready message");
  int new_page_count = data["pageCount"].as<int>();
  set_page_count(new_page_count);
  set_page_index(device_index);

  USE_SERIAL.println(page_count);
  clear_stored_pages();

  // Download the initial page & display it
  download_and_save_page(device_index);
  show_page(device_index, true);

  // Download the rest of the pages
  for (int i = 1; i <= page_count; i++)
  {
    if (i != device_index)
    {
      download_and_save_page(i);
      socket_routine(); // necessary to avoid DC during long downloads, doesn't work yet
    }
  }

  USE_SERIAL.println("Downloaded all pages.");
  device_index = -1;
}

// ====================================================== //
// ====================== Display ======================= //
// ====================================================== //

bool show_page(int page_index, bool do_show_gui)
{
  USE_SERIAL.print("Showing page ");
  String filepath = get_page_filepath(page_index);

  if (display.drawBitmapFromSd(filepath.c_str(), 0, 0, 0, 0))
    draw_page_index();
  else
  {
    Serial.println("Failed to show page");
    return false;
  }

  if (do_show_gui)
    draw_gui();

  display.display();

  return true;
}

void navigate_page(int page_change)
{
  int new_page_index = page_index + page_change;
  if (new_page_index <= page_count && new_page_index > 0)
  {
    show_page(new_page_index, true);
    set_page_index(new_page_index);
    USE_SERIAL.println("Showing page " + page_index);
  }
  else
  {
    USE_SERIAL.printf("at page limit %d of %d \n", new_page_index, page_count);
  }
}

void prev_page()
{
  navigate_page(-1);
}

void next_page()
{
  navigate_page(1);
}

void show_gui()
{
  show_page(page_index, true);
}

void hide_gui()
{
  show_page(page_index, false);
}

void draw_gui()
{
  draw_back_button();
  draw_next_button();
  draw_device_index_info();
}

void draw_page_index()
{
  USE_SERIAL.println("drawing page index");
  String page_info = "[" + String(page_index) + "/" + String(page_count) + "]";
  int cursor_x = 0;
  int cursor_y = DISPLAY_WIDTH - 12;
  const GFXfont *text1_font = &FreeMono9pt7b;
  display.setFont(text1_font);
  display.setTextColor(0, 7);
  display.setTextSize(1);
  display.setCursor(cursor_x, cursor_y);
  display.print(page_info);
}

void draw_next_button()
{
  int icon_size = sizeof(arrow_right_icon);
  display.drawJpegFromBuffer(arrow_right_icon, icon_size, 0, get_button_y(tp_right, 56), true, false);
}

void draw_back_button()
{
  int icon_size = sizeof(arrow_left_icon);
  display.drawJpegFromBuffer(arrow_left_icon, icon_size, 0, get_button_y(tp_left, 56), true, false);
}

void draw_device_index_info()
{
  if (device_index == -1)
  {
    draw_enqueue_button();
  }
  else
  {
    draw_device_index();
  }
}

void draw_device_index()
{
  USE_SERIAL.println("drawing index");
  int icon_size = sizeof(enqueue_icon);
  String device_index_string = device_index == -1 ? "0" : String(device_index);
  int cursor_x = 4;
  int cursor_y = get_button_y(tp_middle, 4);
  const GFXfont *font = &FreeMono24pt7b;
  display.setFont(font);
  display.setTextColor(0, 7);
  display.setTextSize(1);
  display.setCursor(cursor_x, cursor_y);
  display.print(device_index_string);
}

void draw_enqueue_button()
{
  USE_SERIAL.println("draw enqueue");
  int icon_size = sizeof(enqueue_icon);
  display.drawJpegFromBuffer(enqueue_icon, icon_size, 0, get_button_y(tp_middle, 56), true, false);
}

int get_button_spacing()
{
  return DISPLAY_WIDTH / 10 - 8; // dont ask me why
}

int get_button_y(TP_PRESSED button, int button_height)
{
  int middle_button_x = DISPLAY_WIDTH / 2;
  int button_offset = button_height / 2;
  switch (button)
  {
  case tp_left:
    return middle_button_x - get_button_spacing() - button_offset;
  case tp_right:
    return middle_button_x + get_button_spacing() - button_offset;
  default:
    return middle_button_x - button_offset;
  }
}

// ====================================================== //
// ====================== Touchpads ===================== //
// ====================================================== //

void touchpad_routine()
{
  read_touchpads();
  if (touchpad_pressed == tp_none && !touchpad_released)
  {
    Serial.println("nothing pressed");
    touchpad_released = true;
    touchpad_released_time = millis();
    return;
  }

  if (!touchpad_released || touchpad_released_time + touchpad_cooldown_ms > millis())
  {
    // Serial.println("not released or on cooldown");
    return;
  }
  if (touchpad_pressed > 0)
    touchpad_released = false;

  switch (touchpad_pressed)
  {
  case tp_left:
    handle_left_tp_pressed();
    break;
  case tp_middle:
    handle_middle_tp_pressed();
    break;
  case tp_right:
    handle_right_tp_pressed();
    break;
  case tp_none:
    // no button pressed, ignore
    // Serial.println("TP none");
    break;
  }
}

void read_touchpads()
{
  // print_touchpad_status();
  if (display.readTouchpad(PAD1))
    touchpad_pressed = tp_left;
  else if (display.readTouchpad(PAD2))
    touchpad_pressed = tp_middle;
  else if (display.readTouchpad(PAD3))
    touchpad_pressed = tp_right;
  else
    touchpad_pressed = tp_none;
}

void handle_left_tp_pressed()
{
  USE_SERIAL.println("LEFT tp pressed");
  draw_back_button();
  prev_page();
}

void handle_middle_tp_pressed()
{
  USE_SERIAL.println("MIDDLE tp pressed");
  if (device_index == -1)
  {
    send_enqueue_message();
  }
  else
  {
    send_dequeue_message();
  }
}

void handle_right_tp_pressed()
{
  USE_SERIAL.println("RIGHT tp pressed");
  next_page();
}

void print_touchpad_status()
{
  Serial.printf("triggered touchpads: left: %s middle: %s right: %s \n",
                formatBool(display.readTouchpad(PAD1)),
                formatBool(display.readTouchpad(PAD2)),
                formatBool(display.readTouchpad(PAD3)));
}

// ====================================================== //
// ======================= Arduino ====================== //
// ====================================================== //

void setup()
{
  USE_SERIAL.begin(115200);
  USE_SERIAL.setDebugOutput(true);

  wake_up_timestamp = millis();

  display.begin();

  // display.setDisplayMode(INKPLATE_1BIT);

  display.setRotation(3); // Portrait mode

  DISPLAY_WIDTH = display.height(); // due to portrait mode
  DISPLAY_HEIGHT = display.width();

  if (!setup_storage())
  {
    USE_SERIAL.println("Failed to setup storage");
    return;
  }

  if (!load_config())
  {
    USE_SERIAL.println("Failed to load config");
    return;
  }

  if (!load_state())
  {
    USE_SERIAL.println("Failed to load state");
    return;
  }

  setup_wifi();
  mac_addr = WiFi.macAddress();
  setup_socket();

  show_gui();
  is_setup = true;
}

int last_socket_routine = 0;

void loop()
{
  if (do_go_to_sleep())
  {
    enter_deep_sleep();
    return;
  }

  if (is_setup)
  {
    touchpad_routine();
    // run socket routine every 1s @todo find a better way to unblock loop
    if (millis() - last_socket_routine > 50)
    {
      socket_routine();
      last_socket_routine = millis();
    }
  }
  else
  {
    USE_SERIAL.println("Error during setup, entering deep sleep");
    enter_deep_sleep();
  }
}