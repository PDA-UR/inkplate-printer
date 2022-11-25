#include <Arduino.h>

#include "Inkplate.h"

#include <ArduinoJson.h>
#include "SdFat.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include "config.h"
#include "socket_messages.h"

// ##################################################################### //
// ############################## Firmware ############################# //
// ##################################################################### //

// ====================================================== //
// ====================== Constants ===================== //
// ====================================================== //

// ~~~~~~~~~~~~~ File System ~~~~~~~~~~~~~ //

const String PAGE_CHAIN_DIR = "/page_chain/";

// ====================================================== //
// ====================== Variables ===================== //
// ====================================================== //

// ~~~~~~~~~~~~~ Definitions ~~~~~~~~~~~~~ //

#define USE_SERIAL Serial
#define formatBool(b) ((b) ? "true" : "false")

// ~~~~~~~~~~~~~~~ Display ~~~~~~~~~~~~~~~ //

Inkplate display(INKPLATE_3BIT);

long prev_millis = 0;
bool check_new_doc_while_idle = true;
int check_docs_interval_ms = 30 * 1000;

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
char doc_name[255];

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

void enter_deep_sleep()
{
  Serial.println("Going to sleep");
  delay(100);
  esp_sleep_enable_timer_wakeup(15ll * 1000 * 1000);
  esp_deep_sleep_start();
}

// ====================================================== //
// ======================= Storage ====================== //
// ====================================================== //

void setup_storage()
{
  // Init SD card. Display if SD card is init properly or not.
  USE_SERIAL.println("Setup: Storage");

  if (display.sdCardInit())
  {
    USE_SERIAL.println("SD Card OK!");

    SdFile root;
    USE_SERIAL.println("Checking " + PAGE_CHAIN_DIR);
    if (root.open(PAGE_CHAIN_DIR.c_str(), O_RDWR | O_CREAT))
    {
      USE_SERIAL.println("Page chain dir initialized");
      root.close();
    }
    else
      USE_SERIAL.println("Failed to open root");
  }
  else
    USE_SERIAL.println("SD Card ERROR!");
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

void load_state()
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
        strcpy(doc_name, doc["doc_name"]);
        display_mode = doc["display_mode"];
      }
      else
        USE_SERIAL.println("State file read error");
      file.close();
    }
    else
      USE_SERIAL.println("Error opening state file!");
  }
  else
  {
    USE_SERIAL.println("State file does not exist, creating...");
    save_state();
  }
}

void save_state()
{
  // TODO: Save state to SD card
  USE_SERIAL.println("Saving State");

  // create json
  StaticJsonDocument<200> doc;
  doc["page_index"] = page_index;
  doc["page_count"] = page_count;
  doc["doc_name"] = doc_name;
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
  }
  else
    Serial.println("Error creating file!");
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

void set_doc_name(char *name)
{
  strcpy(doc_name, name);
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
  String url = String(HOST) + "api/img" + "?client=" + mac_addr + "&page_num=" + page_num;

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

  WiFi.begin(SSID, PASSWORD);
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

void socket_setup()
{
  // TODO: Replace with config data
  socketIO.begin("192.168.2.104", 8000, "/socket.io/?EIO=4");
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

// Example
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
  USE_SERIAL.println(device_index);
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
  show_page(device_index);

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

bool show_page(int page_index)
{
  USE_SERIAL.print("Showing page ");
  String filepath = get_page_filepath(page_index);

  if (display.drawBitmapFromSd(filepath.c_str(), 0, 0, 0, 0))
  {
    display.display();
    return true;
  }
  else
    Serial.println("Failed to show page");
  return false;
}

void navigate_page(int page_change)
{
  int new_page_index = page_index + page_change;
  if (new_page_index <= page_count && new_page_index > 0)
  {
    show_page(new_page_index);
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
    send_enqueue_message();
  else
    send_dequeue_message();
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

  for (uint8_t t = 4; t > 0; t--)
  {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  display.begin();

  setup_storage();

  load_state();

  setup_wifi();
  mac_addr = WiFi.macAddress();

  socket_setup();

  // enter_deep_sleep();
}

void loop()
{
  touchpad_routine();
  socket_routine();

  if (check_new_doc_while_idle)
  {
    if (prev_millis + check_docs_interval_ms > millis())
    {
      prev_millis = millis();
    }
  }
}