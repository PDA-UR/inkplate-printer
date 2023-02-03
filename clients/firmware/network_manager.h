#ifndef network_manager_h
#define network_manager_h

#include "./config.h"
#include "./util.h"

class NetworkManager
{
public:
    bool setup(Inkplate *display, Config *config)
    {
        this->display = display;
        this->config = config;
        return true; // for consistency
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