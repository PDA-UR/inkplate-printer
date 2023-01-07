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
#include "./icons/hourglass_58.h"

// ##################################################################### //
// ############################## Firmware ############################# //
// ##################################################################### //

// helper struct for image download that contains buffer and size
struct ImageBuffer
{
  uint8_t *buffer;
  int size;
};

// ~~~~~~~~~~~~~ Definitions ~~~~~~~~~~~~~ //

#define USE_SERIAL Serial
#define formatBool(b) ((b) ? "true" : "false")

Inkplate display(INKPLATE_3BIT);

// ====================================================== //
// ====================== Constants ===================== //
// ====================================================== //

const int AWAKE_TIME = 600;            // seconds
const int WIFI_CONNECTION_TIMEOUT = 4; // seconds
const int C_BLACK = 1;
const int C_WHITE = 0;

// ~~~~~~~~~~~~~ File System ~~~~~~~~~~~~~ //

const String PAGE_CHAIN_DIR = "/page_chain/";
const String CONFIG_FILE = "/config.json";
const String STATE_FILE = "/state.json";

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
int touchpad_cooldown_ms = 500;
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

String device_id;
SocketIOclient socketIO;

boolean is_registered = false;

// ====================================================== //
// ======================= General ====================== //
// ====================================================== //

boolean do_go_to_sleep()
{
  return (millis() - wake_up_timestamp) > AWAKE_TIME * 1000;
}

void enter_deep_sleep(bool do_hide_gui)
{
  Serial.println("Going to sleep");
  if (do_hide_gui)
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

void save_page(int &page_index, ImageBuffer *img)
{
  SdFile file;
  String bmp_filename = get_page_filepath(page_index);

  // make sure file can be created, otherwise print error

  USE_SERIAL.println(bmp_filename.c_str());

  // create file

  if (file.open(bmp_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC))
  {
    // opened file
    // clear contents
    file.truncate(0);
    file.seekSet(0);

    USE_SERIAL.println("writing to file with size " + String(img->size));

    file.write(img->buffer, img->size);
    USE_SERIAL.println("write");
    file.flush();
    USE_SERIAL.println("flush");
  }
  else
    USE_SERIAL.println("Error opening page file");

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
  String filename = String(page_index) + ".jpeg";
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

  // get id property
  String id = doc["id"].as<String>();
  if (id != "null")
    device_id = id;
  else
    return false;

  USE_SERIAL.println(id);

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
  USE_SERIAL.println("downloading page " + String(page_index));
  ImageBuffer *img = download_page(page_index);

  // if image size is 0, then there was an error
  if (img != nullptr)
  {
    USE_SERIAL.println("saving page " + String(page_index));
    save_page(page_index, img);
    // free memory
    free(img->buffer);
    free(img);
  }
}

ImageBuffer *download_page(int &page_num)
{
  String url = get_host_url() + "/api/img" + "?client=" + device_id + "&page_num=" + page_num;

  // download the image via http request
  HTTPClient http;

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    USE_SERIAL.println("Downloading image " + String(page_num));
    int size = http.getSize();

    // get stream
    WiFiClient *stream = http.getStreamPtr();
    // download image
    uint8_t *buffer = display.downloadFile(stream, size);

    http.end();

    USE_SERIAL.println("Downloaded image");
    return new ImageBuffer{buffer, size};
  }
  else
  {
    USE_SERIAL.println("Error downloading page " + String(page_num));
    http.end();
    return nullptr;
  }
}

// ====================================================== //
// ======================== Wifi ======================== //
// ====================================================== //

void setup_wifi()
{
  int setup_begin = millis();
  Serial.println("Setup: WiFi");

  WiFi.config(local_ip, gateway, subnet, dns1, dns2);

  WiFi.begin(SSID.c_str(), PASSWORD.c_str());
  while (!is_wifi_connected() && millis() - setup_begin < WIFI_CONNECTION_TIMEOUT * 1000)
  {
    delay(250);
    Serial.print(".");
  }

  draw_status_bar(); // TODO: Remove?
  Serial.println("Setup: WiFi complete");
}

bool is_wifi_connected()
{
  return WiFi.status() == WL_CONNECTED;
}

// ====================================================== //
// ======================= Socket ======================= //
// ====================================================== //

void setup_socket()
{
  if (is_wifi_connected())
  {
    socketIO.begin(HOST, PORT, "/socket.io/?EIO=4");
    socketIO.onEvent(on_socket_event);
  }
}

void socket_routine()
{
  if (is_wifi_connected())
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
  data["uuid"] = device_id;

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
    if (is_registered)
    {
      // set unregistered
      is_registered = false;
      draw_connection_status();
      if (device_index != -1)
      {
        // reset device index
        device_index = -1;
        draw_device_index_info();
      }
      refresh_display();
    }

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
    draw_connection_status();
    refresh_display();
    handle_middle_tp_pressed(); // DEBUG: Auto enqueue on connect
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
  refresh_display();
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

void refresh_display()
{
  display.partialUpdate();
}

bool show_page(int page_index, bool do_show_gui)
{
  USE_SERIAL.print("Showing page ");
  String filepath = get_page_filepath(page_index);

  if (display.drawJpegFromSd(filepath.c_str(), 0, 0, 0, 0))
    draw_status_bar();
  else
  {
    Serial.println("Failed to show page");
    return false;
  }

  if (do_show_gui)
    draw_gui();

  refresh_display();

  return true;
}

void navigate_page(int page_change)
{
  draw_loading_icon(page_change > 0 ? tp_right : tp_left);
  refresh_display();

  int new_page_index = page_index + page_change;
  if (new_page_index <= page_count && new_page_index > 0)
  {
    set_page_index(new_page_index);
    draw_gui();
    show_page(new_page_index, true);
    USE_SERIAL.println("Showing page " + page_index);
  }
  else
  {
    draw_gui();
    refresh_display();
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
  draw_gui_bg();
  draw_back_button();
  draw_next_button();
  draw_device_index_info();
}

void draw_gui_bg()
{
  int padding = 4;
  int border_width = 2;

  int width = 56 + padding;
  int height = get_button_y(tp_right, width) - get_button_y(tp_left, width) + 56 + padding;
  int x = 0;
  int y = get_button_y(tp_left, width);

  // black border
  display.fillRect(x, y - border_width, width + border_width, height + border_width * 2, C_BLACK);
  // white background
  display.fillRect(x, y, width, height, C_WHITE);
}

void draw_loading_icon(TP_PRESSED tp)
{
  int icon_size = sizeof(hourglass_icon);
  switch (tp)
  {
  case tp_left:
    display.drawJpegFromBuffer(hourglass_icon, icon_size, 0, get_button_y(tp_left, 56), true, false);
    break;

  case tp_right:
    display.drawJpegFromBuffer(hourglass_icon, icon_size, 0, get_button_y(tp_right, 56), true, false);
    break;

  case tp_middle:
    display.drawJpegFromBuffer(hourglass_icon, icon_size, 0, get_button_y(tp_middle, 56), true, false);
    break;
  }
}

void draw_status_bar()
{
  USE_SERIAL.println("drawing page index");

  int cursor_x = 0;
  int cursor_y = DISPLAY_HEIGHT - 12;

  // white bg
  display.fillRect(cursor_x, cursor_y - 12, DISPLAY_WIDTH, 12 * 2, C_WHITE);

  String page_info = "[" + String(page_index) + "/" + String(page_count) + "]";
  String wifi_status = is_wifi_connected() ? "O" : "X";
  String server_status = is_registered ? "O" : "X";
  String info = " Page: " + page_info + " | Wifi: [" + wifi_status + "] | Server: [" + server_status + "]";

  const GFXfont *text1_font = &FreeMono9pt7b;
  display.setFont(text1_font);
  display.setTextColor(C_BLACK, C_WHITE);
  display.setTextSize(1);
  display.setCursor(cursor_x, cursor_y);
  display.print(info);
}

void draw_connection_status()
{
  draw_status_bar();
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

  // white bg
  int bg_x = 0;
  int bg_y = get_button_y(tp_middle, 56);
  display.fillRect(bg_x, bg_y, 56, 56, C_WHITE);

  // index number
  int icon_size = sizeof(enqueue_icon);
  String device_index_string = device_index == -1 ? "0" : String(device_index);
  int cursor_x = 6;
  if (page_index < 10)
    cursor_x += 9; // center it if its only 1 char
  int cursor_y = get_button_y(tp_middle, 0) + 7;

  const GFXfont *font = &FreeMono24pt7b;
  display.setFont(font);
  display.setTextColor(C_BLACK, C_WHITE);
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
  return DISPLAY_HEIGHT / 10 - 8; // dont ask me why
}

int get_button_y(TP_PRESSED button, int button_height)
{
  int middle_button_x = DISPLAY_HEIGHT / 2;
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
  // display.selectDisplayMode(INKPLATE_1BIT);

  display.setDisplayMode(INKPLATE_1BIT);

  display.setRotation(3); // Portrait mode

  DISPLAY_WIDTH = display.width(); // due to portrait mode
  DISPLAY_HEIGHT = display.height();

  USE_SERIAL.println("W, H " + String(DISPLAY_WIDTH) + " x " + String(DISPLAY_HEIGHT));

  draw_status_bar();  

  if (!setup_storage())
  {
    USE_SERIAL.println("Failed to setup storage");
    return;
  }

  if (!load_config())
  {
    USE_SERIAL.println("Failed to load config, please make sure a valid config.json is present on the SD card (root directory).");
    return;
  }

  if (!load_state())
  {
    USE_SERIAL.println("Failed to load state, please make sure the state.json file is valid. If you are unsure, delete it and restart the device.");
    return;
  }

  setup_wifi();
  USE_SERIAL.println("Setup done: Wifi");

  setup_socket();
  USE_SERIAL.println("Setup done: Socket");

  show_gui();
  // display.display(); // initial refresh

  USE_SERIAL.println("Setup done.");
  is_setup = true;

  // download_and_save_page(1);
}

int last_socket_routine = 0;

void loop()
{
  // if (do_go_to_sleep())
  // {
  //   enter_deep_sleep();
  //   return;
  // }

  if (is_setup)
  {
    touchpad_routine();
    // socket_routine();
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
    enter_deep_sleep(false);
  }
}