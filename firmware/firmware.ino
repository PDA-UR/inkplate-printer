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

void update_image()
{
    HTTPClient http;
    uint16_t port = 5000;
    String server_addr = "";
    server_addr.concat(HOST);
    server_addr.concat("?client=");
    server_addr.concat(mac_addr);
    Serial.println(server_addr);
    if(display.drawImage(server_addr, display.PNG, 0, 0))
    {
        display.display();
    }
    // http.begin(server_addr);
    // int status_code = http.GET();
    // if(status_code == 200)
    // {
    //     display.drawImage(server_addr, display.PNG, 0, 0);
    //     display.display();
    // }
    // else if(status_code == 201)
    // {
    //     Serial.println("No new image available.");
    //     return;
    // }
    // else
    // {
    //     Serial.println("HTTP Error");
    //     return;
    // }
}

void setup()
{
    Serial.begin(115200);
    display.begin();

    setup_wifi();
    mac_addr = WiFi.macAddress();

    update_image();

    enter_deep_sleep();
}

void loop()
{
    // not used, because of deep sleep
}