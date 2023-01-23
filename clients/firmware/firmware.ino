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

#include "./socket_controller.h"
#include "./touchpad_controller.h"
#include "./view_controller.h"
#include "./state.h"

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

Inkplate display(INKPLATE_1BIT);
ViewController view_controller;

State state;

// ====================================================== //
// ====================== Constants ===================== //
// ====================================================== //

int DPI = 145;
int COLOR_DEPTH = 1;

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

// ====================================================== //
// ====================== Variables ===================== //
// ====================================================== //

// ~~~~~~~~~~~~~~ Touchpads ~~~~~~~~~~~~~~ //

TouchpadController touchpadController;

// ~~~~~~~~~~~~~~~~ State ~~~~~~~~~~~~~~~~ //

// --------- Modes -------- //

enum DisplayMode
{
  blank = 0,
  displaying = 1,
  paired = 2,
} display_mode;

int pairing_index = -1;

// ~~~~~~~~~~~~~~~ Network ~~~~~~~~~~~~~~~ //

String device_id;
SocketIOclient socketIO;

SocketController socketController;

boolean is_registered = false;

// ====================================================== //
// ======================= General ====================== //
// ====================================================== //

boolean do_go_to_sleep()
{
  // return false;
  // @ToDo: Fix sleep button
  return (millis() - state.last_interaction_ts) > AWAKE_TIME * 1000;
}

void enter_deep_sleep()
{
  Serial.println("Going to sleep");
  view_controller.hide_gui();

  // Only enable TP wakeup, see this issue:
  // https://github.com/SolderedElectronics/Inkplate-Arduino-library/issues/119
  // esp_sleep_enable_ext1_wakeup((1ULL << 36), ESP_EXT1_WAKEUP_ALL_LOW);

  esp_sleep_enable_ext1_wakeup(TOUCHPAD_WAKE_MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
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
// ======================= Network ====================== //
// ====================================================== //

void download_and_save_page(int page_index)
{
  USE_SERIAL.println("downloading page " + String(page_index));
  state.last_interaction_ts = millis(); // avoid sleep during download
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
  WiFi.mode(WIFI_STA);

  // TODO: Remove?
  // WiFi.config(local_ip, gateway, subnet, dns1, dns2);

  Serial.println("Setup: WiFi config");

  Serial.println(SSID.c_str());
  Serial.println(PASSWORD.c_str());

  Serial.println(WiFi.status());

  if (WiFi.status() == 255)
  {
    Serial.println("NO MODULE!!!!");
    return;
  }

  WiFi.begin(SSID.c_str(), PASSWORD.c_str());

  Serial.println("Setup: WiFi begin ");

  while (!is_wifi_connected() && millis() - setup_begin < WIFI_CONNECTION_TIMEOUT * 1000)
  {
    delay(250);
    Serial.print(".");
  }
  Serial.println("Setup: WiFi complete");
  state.s_info.is_wifi_setup = true;
}

bool is_wifi_connected()
{
  state.s_info.is_wifi_connected = WiFi.status() == WL_CONNECTED;
  return state.s_info.is_wifi_connected;
}

// ====================================================== //
// ======================= Network ====================== //
// ====================================================== //

class SocketEventHandler : virtual public SocketEventCallback
{
public:
  void handle_connected_message()
  {
    socketController.send_register_message(device_id, COLOR_DEPTH, DPI, display.width(), display.height());
  };
  void handle_disconnected_message()
  {
    USE_SERIAL.printf("[IOc] Disconnected!\n");
    if (is_registered)
    {
      // set unregistered
      is_registered = false;
      view_controller.draw_connection_status();
      if (state.d_info.device_index != -1)
      {
        // reset device index
        state.d_info.device_index = -1;
        view_controller.draw_device_index_info();
      }
      view_controller.refresh_display();
    }
  };
  void handle_registered_message(DynamicJsonDocument data)
  {
    USE_SERIAL.println("Handling registered message");
    if (data["wasSuccessful"].as<bool>())
    {
      USE_SERIAL.println("Registered successfully");
      is_registered = true;
      view_controller.draw_connection_status();
      view_controller.refresh_display();
      // handle_middle_tp_pressed(); // DEBUG: Auto enqueue on connect
    }
    else
    {
      USE_SERIAL.println("Registered unsuccessfully");
      is_registered = false;
    }
  };
  void handle_update_device_index_message(DynamicJsonDocument data)
  {
    USE_SERIAL.print("Handling update device index message, new index:");
    state.d_info.device_index = data["deviceIndex"].as<int>();
    view_controller.draw_device_index_info();
    view_controller.refresh_display();
  };
  void handle_show_page_message(DynamicJsonDocument data)
  {
    // TODO: Handle null / -1
    USE_SERIAL.println("Handling show page message");
    int new_page_index = data["pageIndex"].as<int>();

    state.set_page_index(new_page_index);
    // set_display_mode(displaying);
    state.save();
  };
  void handle_pages_ready_message(DynamicJsonDocument data)
  {
    USE_SERIAL.println("Handling pages ready message");
    int new_page_count = data["pageCount"].as<int>();
    state.set_page_count(new_page_count);
    int new_page_index = state.d_info.device_index <= new_page_count ? state.d_info.device_index : new_page_count; // avoid going over max
    USE_SERIAL.print("New page index: " + String(new_page_index));
    state.set_page_index(new_page_index);

    state.is_downloading = true;
    view_controller.draw_loading_icon(tp_middle);
    view_controller.refresh_display();

    clear_stored_pages();

    // Download the initial page & display it
    download_and_save_page(new_page_index);
    view_controller.show_page(new_page_index, true, true);

    // Download the rest of the pages
    for (int i = 1; i <= state.p_info.page_count; i++)
    {
      if (i != new_page_index)
      {
        download_and_save_page(i);
        socketController.loop(); // necessary to avoid DC during long downloads, doesn't work yet
      }
    }

    USE_SERIAL.println("Downloaded all pages.");
    state.d_info.device_index = -1;
    state.is_downloading = false;
    view_controller.draw_gui();
    view_controller.refresh_display();
  };
};

void setup_socket()
{
  Serial.println("Setup: Socket begin");
  SocketEventHandler *handler = new SocketEventHandler();
  USE_SERIAL.println(HOST);
  USE_SERIAL.println(PORT);
  socketController.setup(HOST, PORT, handler);
  state.s_info.is_socket_setup = true;
  Serial.println("Setup: Socket complete");
}

// ~~~~~~~~~~~~~~~ Messages ~~~~~~~~~~~~~~ //

// ====================================================== //
// ====================== Touchpads ===================== //
// ====================================================== //

class TouchpadEventHandler : virtual public TouchpadEventCallback
{
public:
  void handle_left_tp_pressed()
  {
    USE_SERIAL.println("left tp pressed");
    view_controller.prev_page();
  }
  void handle_middle_tp_pressed()
  {
    USE_SERIAL.println("middle tp pressed");
    if (state.d_info.device_index == -1)
    {
      socketController.send_enqueue_message();
    }
    else
    {
      socketController.send_dequeue_message();
    }
  }
  void handle_right_tp_pressed()
  {
    USE_SERIAL.println("right tp pressed");
    view_controller.next_page();
  }
};

void setup_touchpads()
{
  TouchpadEventHandler *handler = new TouchpadEventHandler();
  touchpadController.setup(&display, handler);
}

// ====================================================== //
// ======================= Arduino ====================== //
// ====================================================== //

void setup()
{
  USE_SERIAL.begin(115200);
  USE_SERIAL.setDebugOutput(true);

  state.last_interaction_ts = millis();
  view_controller.setup(&display, &state, 145, 1); // TODO: use values from config

  // USE_SERIAL.println("W, H " + String(DISPLAY_WIDTH) + " x " + String(DISPLAY_HEIGHT));

  setup_touchpads();
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

  if (!state.load())
  {
    USE_SERIAL.println("Failed to load state, please make sure the state.json file is valid. If you are unsure, delete it and restart the device.");
    return;
  }

  view_controller.draw_connection_status();
  view_controller.show_gui();

  setup_wifi();
  view_controller.refresh_connection_status();

  setup_socket();
  view_controller.refresh_connection_status();

  USE_SERIAL.println("Setup done.");
  state.is_setup = true;
}

int last_socket_routine = 0;

void loop()
{
  if (do_go_to_sleep())
  {
    enter_deep_sleep();
    return;
  }

  if (state.is_setup)
  {
    touchpadController.loop();
    // run socket routine every 1s @todo find a better way to unblock loop
    if (millis() - last_socket_routine > 50)
    {
      socketController.loop();
      last_socket_routine = millis();
    }
  }
  else
  {
    USE_SERIAL.println("Error during setup, entering deep sleep");
    enter_deep_sleep();
  }
}