#include "Inkplate.h"

const char SSID[] = "";    // Your WiFi SSID
const char *PASSWORD = ""; // Your WiFi password
const char HOST[] = "";

IPAddress local_ip  =   IPAddress(192, 168, 0, 2);
IPAddress gateway   =   IPAddress(192, 168, 0, 1);
IPAddress subnet    =   IPAddress(255, 255, 255, 0);
IPAddress dns1      =   IPAddress(1, 1, 1, 1);
IPAddress dns2      =   IPAddress(1, 0, 0, 1);
