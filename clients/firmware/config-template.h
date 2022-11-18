#include "Inkplate.h"

// Fill out the marked variables and save this file as config.h

const char SSID[] = "";                           // Your WiFi SSID
const char *PASSWORD = "";                        // Your WiFi password
const char HOST[] = "http://192.168.2.104:8000/"; // Printer host

IPAddress local_ip = IPAddress(192, 168, 2, 103); // IP of this device
IPAddress gateway = IPAddress(192, 168, 2, 1);
IPAddress subnet = IPAddress(255, 255, 255, 0);
IPAddress dns1 = IPAddress(1, 1, 1, 1);
IPAddress dns2 = IPAddress(1, 0, 0, 1);

// DISPLAY - config for Inkplate 10

const int DPI = 145;
const int COLOR_DEPTH = 4;
const int DISPLAY_WIDTH = 825;
const int DISPLAY_HEIGHT = 1200;