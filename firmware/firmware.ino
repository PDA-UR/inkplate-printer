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


// Next 3 lines are a precaution, you can ignore those, and the example would also work without them


#include "config.h"

Inkplate display(INKPLATE_3BIT);

void setup()
{
    Serial.begin(115200);
    display.begin();

    // Welcome screen
    display.setCursor(100, 100);
    display.setTextSize(5);
    display.setTextColor(BLACK);
    display.print("Booting...");
    display.display();

    delay(2000);

    WiFi.config(local_ip, gateway, subnet, dns1, dns2);

    // Connect to Wi-Fi network with SSID and password
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    display.setCursor(100, 300);
    display.print("Connected to: " + WiFi.SSID());
    display.setCursor(100, 400);
    display.print("Downloading image...");
    display.display();

    // Download and display image
    Serial.println(display.drawImage(HOST, display.PNG, 0, 0));
    display.display();

    Serial.println("Going to sleep");
    delay(100);
    esp_sleep_enable_timer_wakeup(15ll * 60 * 1000 * 1000);
    esp_deep_sleep_start();
}

void loop()
{
    // Never here, as deepsleep restarts esp32
}