#ifndef config_h
#define config_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SdFat.h"

#include "./util.h"

const String CONFIG_FILE = "/config.json";

class Config
{
public:
    String device_id;

    int DPI;
    int COLOR_DEPTH;

    String SSID;
    String PASSWORD;
    String HOST;
    int PORT;

    IPAddress local_ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;

    Config(){};
    bool load()
    {
        File configFile;
        if (!configFile.open(CONFIG_FILE.c_str(), O_READ))
        {
            Serial.println("Failed to open config file");
            return false;
        }

        size_t size = configFile.size();
        if (size > 1024)
        {
            Serial.println("Config file size is too large");
            return false;
        }

        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, configFile);
        if (error)
        {
            Serial.println("Failed to parse config file");
            return false;
        }

        JsonObject wifi = doc["wifi"];
        if (!wifi.isNull())
        {
            SSID = wifi["ssid"].as<String>();
            PASSWORD = wifi["password"].as<String>();
        }
        else
            return false;

        String id = doc["id"].as<String>();
        if (id != "null")
            device_id = id;
        else
            return false;

        Serial.println(id);

        JsonObject api = doc["api"];
        if (!api.isNull())
        {
            HOST = api["host"].as<String>();
            PORT = api["port"].as<int>();
        }
        else
            return false;

        JsonObject network = doc["network"];
        if (!network.isNull())
        {
            JsonArray LOCAL_IP = network["local_ip"].as<JsonArray>();
            local_ip = parse_ip_json(LOCAL_IP);
            JsonArray GATEWAY = network["gateway"].as<JsonArray>();
            gateway = parse_ip_json(GATEWAY);
            JsonArray SUBNET = network["subnet"].as<JsonArray>();
            subnet = parse_ip_json(SUBNET);
            JsonArray DNS1 = network["dns1"].as<JsonArray>();
            dns1 = parse_ip_json(DNS1);
            JsonArray DNS2 = network["dns2"].as<JsonArray>();
            dns2 = parse_ip_json(DNS2);

            // free up memeory
            LOCAL_IP.clear();
            GATEWAY.clear();
            SUBNET.clear();
            DNS1.clear();
            DNS2.clear();
        }
        else
            return false;

        JsonObject display = doc["display"];
        if (!display.isNull())
        {
            DPI = display["dpi"].as<int>();
            COLOR_DEPTH = display["color_depth"].as<int>();
        }
        else
            return false;

        return true;
    }

private:
    IPAddress parse_ip_json(JsonArray ip_array)
    {
        if (ip_array.isNull() || ip_array.size() != 4)
        {
            Serial.println("Error parsing ip address");
            return IPAddress(0, 0, 0, 0);
        }
        return IPAddress(ip_array[0], ip_array[1], ip_array[2], ip_array[3]);
    }
};

#endif