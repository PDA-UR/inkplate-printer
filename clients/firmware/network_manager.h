#ifndef network_manager_h
#define network_manager_h

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "./config.h"
#include "./util.h"

class NetworkManager
{
public:
    bool setup(Inkplate *display, Config *config)
    {
        this->display = display;
        this->config = config;
        return true; // kinda useless
    };

    bool connect_wifi()
    {
        int setup_begin = millis();
        Serial.println("Setup: WiFi");
        WiFi.mode(WIFI_STA);

        // WiFi.config(config->local_ip, config->gateway, config->subnet, config->dns1, config->dns2);

        if (WiFi.status() == 255)
        {
            Serial.println("NO MODULE!!!!");
            return false;
        }

        WiFi.begin(config->SSID.c_str(), config->PASSWORD.c_str());

        Serial.println("Setup: WiFi begin ");

        while (!is_wifi_connected() && millis() - setup_begin < WIFI_CONNECTION_TIMEOUT * 1000)
        {
            delay(250);
            Serial.print(".");
        }

        return is_wifi_connected();
    };

    bool is_wifi_connected()
    {
        return WiFi.status() == WL_CONNECTED;
    };

    ImageBuffer *download_page(int &page_num)
    {
        String url = get_host_url() + "/api/img" + "?client=" + config->device_id + "&page_num=" + page_num;

        // download the image via http request
        HTTPClient http;

        http.begin(url);
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK)
        {
            Serial.println("Downloading image " + String(page_num));
            int size = http.getSize();

            WiFiClient *stream = http.getStreamPtr();
            uint8_t *buffer = display->downloadFile(stream, size);

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
    };

private:
    Inkplate *display;
    Config *config;
    String get_host_url()
    {
        return "http://" + String(config->HOST) + ":" + String(config->PORT);
    };
};

#endif