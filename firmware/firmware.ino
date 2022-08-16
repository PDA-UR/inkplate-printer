/*
   Image frame example for e-radionica.com Inkplate 10
   For this example you will need only USB cable and Inkplate 10.
   Select "Inkplate 10(ESP32)" from Tools -> Board menu.
   Don't have "Inkplate 10(ESP32)" option? Follow our tutorial and add it:
   https://e-radionica.com/en/blog/add-inkplate-6-to-arduino-ide/

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: http://forum.e-radionica.com/en/
   28 July 2020 by e-radionica.com
*/

#include "config.h"

Inkplate display(INKPLATE_3BIT);
String last_hash;
String mac_addr;

void setup_wifi()
{
    // Connect to Wi-Fi network with SSID and password
    WiFi.config(local_ip, gateway, subnet, dns1, dns2);

    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

void enter_deep_sleep()
{
    Serial.println("Going to sleep");
    delay(100);
    esp_sleep_enable_timer_wakeup(15ll * 1000 * 1000);
    esp_deep_sleep_start();
}

void save_img_buff_to_sd(uint8_t *buf, string filename)
{
    SdFile file;

    //make sure file can be created, otherwise print error
    if (!file.open(filename.c_str(), O_RDWR | O_CREAT | O_AT_END))
    {
        sd.errorHalt("opening file for write failed");
    }

    int file_size = READ32(buf + 2);

    file.write(buf, file_size);

    file.flush();
    file.close();                                        // close file when done writing

    Serial.println("file saved");
}

uint8_t* download_file()
{
    HTTPClient http;
    uint16_t port = 5000;
    String server_addr = "";
    server_addr.concat(HOST);
    server_addr.concat("?client=");
    server_addr.concat(mac_addr);
    Serial.println(server_addr);

    // Initialize SdFat or print a detailed error message and halt
    // Use half speed like the native library.
    // change to SPI_FULL_SPEED for more performance.
    // if (!sd.begin(chipSelect, SPI_HALF_SPEED)) sd.initErrorHalt();
    if(!display.sdCardInit()) return nullptr;

    http.begin("https://people.math.sc.edu/Burkardt/data/bmp/blackbuck.bmp");
    int status_code = http.GET();
    if(status_code == 200)
    {
        int len = 1200*825; // display resolution
        uint8_t *img_buf = display.downloadFile("https://people.math.sc.edu/Burkardt/data/bmp/blackbuck.bmp", &len); 
        Serial.println("image downloaded");
        return img_buf;
    }
    else if(status_code == 201)
    {
        Serial.println("No new image available.");
        return nullptr;
    }
    else
    {
        Serial.println("HTTP Error");
        return nullptr;
    }
}

void setup()
{
    Serial.begin(115200);
    display.begin();

    setup_wifi();
    mac_addr = WiFi.macAddress();

    uint8_t *buf = download_file();
    if(buf == nullptr) return;

    save_img_buff_to_sd(buf);
    free(buf);

    //enter_deep_sleep();
}

void loop()
{
    // if (display.readTouchpad(PAD3))
    // {
        
    // }
    // not used, because of deep sleep
}