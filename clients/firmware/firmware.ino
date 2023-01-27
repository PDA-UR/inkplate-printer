#include <Arduino.h>
#include "Inkplate.h"
// @ToDo: Clean up imports
#include <ArduinoJson.h>
#include "SdFat.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include "./socket_controller.h"
#include "./touchpad_controller.h"
#include "./view_controller.h"
#include "./storage_manager.h"
#include "./state.h"
#include "./config.h"
#include "./util.h"

// ##################################################################### //
// ############################## Firmware ############################# //
// ##################################################################### //

// ~~~~~~~~~~~~~ Definitions ~~~~~~~~~~~~~ //

Inkplate display(INKPLATE_1BIT);

TouchpadController touchpadController;
SocketController socketController;
ViewController view_controller;

StorageManager storage_manager;

State state;
Config config;

// System Functionality

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
// ======================= Network ====================== //
// ====================================================== //

String get_host_url()
{
  return "http://" + String(config.HOST) + ":" + String(config.PORT);
}

void download_and_save_page(int page_index)
{
  Serial.println("downloading page " + String(page_index));
  state.last_interaction_ts = millis(); // avoid sleep during download
  ImageBuffer *img = download_page(page_index);

  // if image size is 0, then there was an error
  if (img != nullptr)
  {
    Serial.println("saving page " + String(page_index));
    storage_manager.save_page(page_index, img);
    // free memory
    free(img->buffer);
    free(img);
  }
}

ImageBuffer *download_page(int &page_num)
{
  String url = get_host_url() + "/api/img" + "?client=" + config.device_id + "&page_num=" + page_num;

  // download the image via http request
  HTTPClient http;

  http.begin(url);
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    Serial.println("Downloading image " + String(page_num));
    int size = http.getSize();

    WiFiClient *stream = http.getStreamPtr();
    uint8_t *buffer = display.downloadFile(stream, size);

    http.end();

    Serial.println("Downloaded image");
    return new ImageBuffer{buffer, size};
  }
  else
  {
    Serial.println("Error downloading page " + String(page_num));
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
  // WiFi.config(config.local_ip, config.gateway, config.subnet, config.dns1, config.dns2);

  Serial.println("Setup: WiFi config");

  Serial.println(config.SSID.c_str());
  Serial.println(config.PASSWORD.c_str());

  Serial.println(WiFi.status());

  if (WiFi.status() == 255)
  {
    Serial.println("NO MODULE!!!!");
    return;
  }

  WiFi.begin(config.SSID.c_str(), config.PASSWORD.c_str());

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
// ======================= Callbacks ==================== //
// ====================================================== //

// ~~~~~~~~~~~~~~~~ Socket ~~~~~~~~~~~~~~~ //

class SocketEventHandler : virtual public SocketEventCallback
{
public:
  void handle_connected_message()
  {
    socketController.send_register_message(config.device_id, config.COLOR_DEPTH, config.DPI, display.width(), display.height());
  };
  void handle_disconnected_message()
  {
    Serial.printf("[IOc] Disconnected!\n");
    if (state.s_info.is_socket_registered)
    {
      // set unregistered
      state.s_info.is_socket_registered = false;
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
    Serial.println("Handling registered message");
    if (data["wasSuccessful"].as<bool>())
    {
      Serial.println("Registered successfully");
      state.s_info.is_socket_registered = true;
      view_controller.draw_connection_status();
      view_controller.refresh_display();
      // handle_middle_tp_pressed(); // DEBUG: Auto enqueue on connect
    }
    else
    {
      Serial.println("Registered unsuccessfully");
      state.s_info.is_socket_registered = false;
    }
  };
  void handle_update_device_index_message(DynamicJsonDocument data)
  {
    Serial.print("Handling update device index message, new index:");
    state.d_info.device_index = data["deviceIndex"].as<int>();
    view_controller.draw_device_index_info();
    view_controller.refresh_display();
  };
  void handle_show_page_message(DynamicJsonDocument data)
  {
    // TODO: Handle null / -1
    Serial.println("Handling show page message");
    int new_page_index = data["pageIndex"].as<int>();

    state.set_page_index(new_page_index);
    // set_display_mode(displaying);
    state.save();
  };
  void handle_pages_ready_message(DynamicJsonDocument data)
  {
    Serial.println("Handling pages ready message");
    int new_page_count = data["pageCount"].as<int>();
    state.set_page_count(new_page_count);
    int new_page_index = state.d_info.device_index <= new_page_count ? state.d_info.device_index : new_page_count; // avoid going over max
    Serial.print("New page index: " + String(new_page_index));
    state.set_page_index(new_page_index);

    state.is_downloading = true;
    view_controller.draw_loading_icon(tp_middle);
    view_controller.refresh_display();

    storage_manager.clear_stored_pages();

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

    Serial.println("Downloaded all pages.");
    state.d_info.device_index = -1;
    state.is_downloading = false;
    view_controller.draw_gui();
    view_controller.refresh_display();
  };
};

// ~~~~~~~~~~~~~~ Touchpads ~~~~~~~~~~~~~~ //

class TouchpadEventHandler : virtual public TouchpadEventCallback
{
public:
  void handle_left_tp_pressed()
  {
    Serial.println("left tp pressed");
    view_controller.prev_page();
  }
  void handle_middle_tp_pressed()
  {
    Serial.println("middle tp pressed");
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
    Serial.println("right tp pressed");
    view_controller.next_page();
  }
};

// ====================================================== //
// ======================= Setup ======================== //
// ====================================================== //

void setup_touchpads()
{
  TouchpadEventHandler *handler = new TouchpadEventHandler();
  touchpadController.setup(&display, handler);
}

void setup_socket()
{
  Serial.println("Setup: Socket begin");
  SocketEventHandler *handler = new SocketEventHandler();
  state.s_info.is_socket_setup = socketController.setup(config.HOST, config.PORT, handler);
  Serial.println("Setup: Socket completed, was successful: " + String(state.s_info.is_socket_setup));
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  state.last_interaction_ts = millis();
  view_controller.setup(&display, &state);

  setup_touchpads();
  if (!storage_manager.setup(&display))
  {
    Serial.println("Failed to setup storage");
    return;
  }

  if (!config.load())
  {
    Serial.println("Failed to load config, please make sure a valid config.json is present on the SD card (root directory).");
    return;
  }

  if (!state.load())
  {
    Serial.println("Failed to load state, please make sure the state.json file is valid. If you are unsure, delete it and restart the device.");
    return;
  }

  view_controller.draw_connection_status();
  view_controller.show_gui();

  setup_wifi();
  view_controller.refresh_connection_status();

  setup_socket();
  view_controller.refresh_connection_status();

  Serial.println("Setup done.");
  state.is_setup = true;
}

// ====================================================== //
// ======================== Loop ======================== //
// ====================================================== //

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
    socketController.loop();
  }
  else
  {
    Serial.println("Error during setup, entering deep sleep");
    enter_deep_sleep();
  }
}