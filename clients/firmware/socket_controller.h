#ifndef socket_controller_h
#define socket_controller_h

#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <ArduinoJson.h>

class SocketEventCallback
{
public:
  virtual void handle_connected_message() = 0;
  virtual void handle_disconnected_message() = 0;
  virtual void handle_registered_message(DynamicJsonDocument data) = 0;
  virtual void handle_update_device_index_message(DynamicJsonDocument data) = 0;
  virtual void handle_show_page_message(DynamicJsonDocument data) = 0;
  virtual void handle_pages_ready_message(DynamicJsonDocument data) = 0;
};

class SocketController
{
private:
  SocketIOclient socketIO;
  String host;
  int port;
  SocketEventCallback *callback;
  void on_socket_event(socketIOmessageType_t type, uint8_t *payload, size_t length)
  {
    switch (type)
    {
    case sIOtype_DISCONNECT:
      callback->handle_disconnected_message();
      break;
    case sIOtype_CONNECT:
      socketIO.send(sIOtype_CONNECT, "/"); // join default namespace
      callback->handle_connected_message();
      break;
    case sIOtype_EVENT:
    {
      char *sptr = NULL;
      int id = strtol((char *)payload, &sptr, 10);
      if (id)
      {
        payload = (uint8_t *)sptr;
      }
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload, length);
      if (error)
        return;
      String eventName = doc[0];
      DynamicJsonDocument data = doc[1];
      this->handle_custom_event(eventName, data);
      break;
    }

    case sIOtype_ACK:
      break;
    case sIOtype_ERROR:;
      break;
    case sIOtype_BINARY_EVENT:
      break;
    case sIOtype_BINARY_ACK:
      break;
    }
  };
  void handle_custom_event(
      String name,
      DynamicJsonDocument data)
  {
    String output;
    serializeJson(data, output);
    if (name == REGISTERED_MESSAGE)
      callback->handle_registered_message(data);
    else if (name == UPDATE_DEVICE_INDEX_MESSAGE)
      callback->handle_update_device_index_message(data);
    else if (name == SHOW_PAGE_MESSAGE)
      callback->handle_show_page_message(data);
    else if (name == PAGES_READY_MESSAGE)
      callback->handle_pages_ready_message(data);
  };
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
  };

public:
  SocketController(){};
  bool setup(String host, int port, SocketEventCallback *callback)
  {
    this->host = host;
    this->port = port;
    this->callback = callback;
    if (WiFi.status() != WL_CONNECTED)
      return false;

    socketIO.begin(host, port, "/socket.io/?EIO=4");
    socketIO.onEvent(
        [this](socketIOmessageType_t type, uint8_t *payload, size_t length)
        {
          this->on_socket_event(type, payload, length);
        });
    return true;
  };
  void loop()
  {
    if (WiFi.status() == WL_CONNECTED)
      socketIO.loop();
  };

  void send_example_message()
  {
    DynamicJsonDocument data(1024);
    data["message"] = "Hello from Arduino!";
    this->send_message("example", &data);
  };

  void send_register_message(String device_id, int color_depth, int dpi, int display_width, int display_height)
  {
    DynamicJsonDocument data(1024);
    data["uuid"] = device_id;

    JsonObject screen_info = data.createNestedObject("screenInfo");
    screen_info["colorDepth"] = color_depth;
    screen_info["dpi"] = dpi;

    JsonObject resolution = screen_info.createNestedObject("resolution");
    resolution["width"] = display_width;
    resolution["height"] = display_height;

    data["isBrowser"] = false;

    this->send_message(REGISTER_MESSAGE, &data);
  };

  void send_enqueue_message()
  {
    this->send_message(ENQUEUE_MESSAGE, nullptr);
  };

  void send_dequeue_message()
  {
    this->send_message(DEQUEUE_MESSAGE, nullptr);
  };
};
#endif