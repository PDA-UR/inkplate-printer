#include <Arduino.h>

#include "Inkplate.h"

#include <ArduinoJson.h>
#include "SdFat.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <WebSocketsClient.h>
#include <SocketIOclient.h>

#include "config.h"

// ##################################################################### //
// ############################## Firmware ############################# //
// ##################################################################### //

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

enum {
  tp_none = 0,
  tp_left = 1,
  tp_middle = 2,
  tp_right = 3
} touchpad_pressed;

// ~~~~~~~~~~~~~~~ Document ~~~~~~~~~~~~~~ //

int cur_page_num = 1;
char cur_doc_name[255];

// ~~~~~~~~~~~~~~~ Network ~~~~~~~~~~~~~~~ //

String mac_addr;
SocketIOclient socketIO;

// ====================================================== //
// ======================= General ====================== //
// ====================================================== //


void enter_deep_sleep() {
  Serial.println("Going to sleep");
  delay(100);
  esp_sleep_enable_timer_wakeup(15ll * 1000 * 1000);
  esp_deep_sleep_start();
}

void save_img_buff_to_sd(uint8_t *buf, String &filename) {
  SdFile file;
  String bmp_filename = filename + ".bmp";

  //make sure file can be created, otherwise print error
  if (!file.open(bmp_filename.c_str(), O_RDWR | O_CREAT | O_AT_END)) {
    Serial.println("Error creating file!");
  }

  int file_size = READ32(buf + 2);
  file.write(buf, file_size);
  file.flush();
  file.close();  // close file when done writing

  Serial.println("file saved");
}

// ====================================================== //
// ======================= Network ====================== //
// ====================================================== //

uint8_t *download_file(String &doc_name, int &page_num) {
  String url = String(HOST)
               + "/img"
               + "?client="
               + mac_addr
               + "&doc_name="
               + doc_name
               + "&page_num="
               + page_num;

  int len = 1200 * 825;  // display resolution
  Serial.println("Downloading...");
  uint8_t *img_buf = display.downloadFile(url.c_str(), &len);
  Serial.println("image downloaded");
  return img_buf;
}

bool server_has_new_file(String &doc_name, int &num_pages) {
  HTTPClient http;
  uint16_t port = 5000;
  String server_addr = String(HOST)
                       + "?client"
                       + mac_addr;
  http.begin(server_addr);

  Serial.println("Requesting from server address: " + server_addr);
  int response_code = http.GET();
  if (response_code == 200) {
    String payload = http.getString();
    Serial.printf("payload:\t %s\n", payload.c_str());
    int index = payload.indexOf("_");                  // payload containing job name & num pages, e.g. job-1_3
    doc_name = payload.substring(0, index);            // job name, e.g. job-1
    num_pages = payload.substring(index + 1).toInt();  // number of pages, e.g. 3

    Serial.printf("server has new job %s (%i pages)\n", doc_name.c_str(), num_pages);
    return true;
  } else if (response_code == 201) {
    Serial.println("No new image available.");
    return false;
  } else {
    Serial.println("HTTP Error");
    return false;
  }
  return false;
}

// ====================================================== //
// ======================== Wifi ======================== //
// ====================================================== //

void setup_wifi() {
  Serial.println("Setup: WiFi");
  WiFi.config(local_ip, gateway, subnet, dns1, dns2);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Setup: WiFi connected");
}

// ====================================================== //
// ======================= Socket ======================= //
// ====================================================== //

void setup_socket() {
  socketIO.begin("192.168.2.104", 8000, "/socket.io/?EIO=4");  // TODO: Replace with config
  socketIO.onEvent(on_socket_event);
}

void loop_socket() {
  socketIO.loop();
}

// ~~~~~~~~~~~~~~~ Messages ~~~~~~~~~~~~~~ //

// --------- Send --------- //

void send_message(String name, DynamicJsonDocument *data) {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.to<JsonArray>();

  arr.add(name);
  if (data != nullptr) arr.add(*data);

  String output;
  serializeJson(doc, output);
  socketIO.sendEVENT(output);

  Serial.println(output);
}

// Example
void send_example_message() {
  DynamicJsonDocument data(1024);
  data["message"] = "Hello from Arduino!";
  send_message("example", &data);
}

// Register
void send_register_message() {
  DynamicJsonDocument data(1024);
  data["uuid"] = "MAC_HERE";

  JsonObject screen_info = data.createNestedObject("screenInfo");
  screen_info["colorDepth"] = 8;
  screen_info["dpi"] = 128;

  JsonObject resolution = screen_info.createNestedObject("resolution");
  resolution["width"] = 825;
  resolution["height"] = 1200;

  data["isBrowser"] = false;

  send_message("register", &data);
}

// -------- Receive ------- //

// Code (modified) from
// // https://github.com/Links2004/arduinoWebSockets/blob/master/examples/esp32/WebSocketClientSocketIOack/WebSocketClientSocketIOack.ino
void on_socket_event(socketIOmessageType_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
      {
        char *sptr = NULL;
        int id = strtol((char *)payload, &sptr, 10);
        USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
        if (id) {
          payload = (uint8_t *)sptr;
        }
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
          USE_SERIAL.print(F("deserializeJson() failed: "));
          USE_SERIAL.println(error.c_str());
          return;
        }

        String eventName = doc[0];
        USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());

        // Message Includes a ID for a ACK (callback)
        if (id) {
          // creat JSON message for Socket.IO (ack)
          DynamicJsonDocument docOut(1024);
          JsonArray array = docOut.to<JsonArray>();

          // add payload (parameters) for the ack (callback function)
          JsonObject param1 = array.createNestedObject();
          param1["now"] = millis();

          // JSON to String (serializion)
          String output;
          output += id;
          serializeJson(docOut, output);

          // Send event
          socketIO.send(sIOtype_ACK, output);
        }
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


// ====================================================== //
// ====================== Document ====================== //
// ====================================================== //

void request_doc_routine() {
  int num_pages;
  String doc_name;
  return;  // TODO: Remove
  if (!server_has_new_file(doc_name, num_pages)) return;

  for (int cur_page = 1; cur_page <= num_pages; ++cur_page) {
    uint8_t *img_buf = download_file(doc_name, cur_page);
    Serial.println("file downloaded");
    if (img_buf == nullptr) {
      Serial.println("Error downloading file.");
      return;
    }

    if (cur_page == 1)  // immediately display the first page
    {
      if (display.drawBitmapFromBuffer(img_buf, 0, 0, 0, 0)) {
        Serial.println("drawImage returned 1");
        display.display();
      } else Serial.println("drawImage failed");
    }

    save_img_buff_to_sd(img_buf, doc_name + "_" + cur_page);
  }

  strcpy(cur_doc_name, doc_name.c_str());
  cur_page_num = 1;
}

void save_cur_doc_info() {
  Serial.println("Saving current doc info");
  // TODO: Save current doc info to SD from variables
}

void load_cur_doc_info() {
  Serial.println("Loading current doc info");
  // TODO: Load current doc info from SD into variables
  String foo = "job-1";  // harcoded for now
  strcpy(cur_doc_name, foo.c_str());
  cur_page_num = 1;
}

// ====================================================== //
// ====================== Touchpads ===================== //
// ====================================================== //

void touchpad_routine() {
  read_touchpads();
  // Serial.println(touchpad_pressed);
  if (touchpad_pressed == tp_none && !touchpad_released) {
    Serial.println("nothing pressed");
    touchpad_released = true;
    touchpad_released_time = millis();
    return;
  }

  if (!touchpad_released || touchpad_released_time + touchpad_cooldown_ms > millis()) {
    // Serial.println("not released or on cooldown");
    return;
  }
  if (touchpad_pressed > 0) touchpad_released = false;

  switch (touchpad_pressed) {
    case tp_left:
      Serial.println("TP left");
      prev_page();
      break;
    case tp_middle:
      Serial.println("TP middle");
      break;
    case tp_right:
      Serial.println("TP right");
      next_page();
      break;
    case tp_none:
      // no button pressed, ignore
      // Serial.println("TP none");
      break;
  }
}

void read_touchpads() {
  // print_touchpad_status();
  if (display.readTouchpad(PAD1)) touchpad_pressed = tp_left;
  else if (display.readTouchpad(PAD2)) touchpad_pressed = tp_middle;
  else if (display.readTouchpad(PAD3)) touchpad_pressed = tp_right;
  else touchpad_pressed = tp_none;
}

void prev_page() {
  if (cur_page_num <= 1) return;

  String page_name = String(cur_doc_name) + "_" + (cur_page_num - 1) + ".bmp";
  if (display.drawImage(page_name, display.BMP, 0, 0)) {
    cur_page_num--;
    display.display();
  } else {
    Serial.print("cannot get previous page with num");
    Serial.println(cur_page_num - 1);
  }
}

void next_page() {
  String page_name = String(cur_doc_name) + "_" + (cur_page_num + 1) + ".bmp";
  if (display.drawImage(page_name, display.BMP, 0, 0)) {
    cur_page_num++;
    display.display();
  } else {
    Serial.print("cannot get next page with num: ");
    Serial.print(cur_page_num + 1);
    Serial.print(" for doc: ");
    Serial.println(page_name);
  }
}

void print_touchpad_status() {
  Serial.printf("triggered touchpads: left: %s middle: %s right: %s \n",
                formatBool(display.readTouchpad(PAD1)),
                formatBool(display.readTouchpad(PAD2)),
                formatBool(display.readTouchpad(PAD3)));
}

// ====================================================== //
// ======================= Arduino ====================== //
// ====================================================== //

void setup() {
  USE_SERIAL.begin(115200);
  USE_SERIAL.setDebugOutput(true);

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  display.begin();
  load_cur_doc_info();

  setup_wifi();
  mac_addr = WiFi.macAddress();

  setup_socket();

  // Init SD card. Display if SD card is init properly or not.
  if (display.sdCardInit()) {
    display.println("SD Card OK!");
  } else {
    display.println("SD Card ERROR!");
  }
  //request_doc_routine();
  //enter_deep_sleep();
}

unsigned long message_timestamp = 0;

void loop() {
  //touchpad_routine();
  loop_socket();

  uint64_t now = millis();
  if (now - message_timestamp > 2000) {
    send_register_message();
    message_timestamp = now;
  }
  return;

  if (check_new_doc_while_idle) {
    if (prev_millis + check_docs_interval_ms > millis()) {
      prev_millis = millis();
      //request_doc_routine();
    }
  }
}