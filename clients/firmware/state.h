#ifndef state_h
#define state_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SdFat.h"

const String STATE_FILE = "state.json";

struct StatusInfo
{
    bool is_wifi_setup = false;
    bool is_wifi_connected = false;
    bool is_socket_setup = false;
    bool is_socket_registered = false;
};

struct PageInfo
{
    int page_index = -1;
    int page_count = -1;
};

struct DeviceInfo
{
    int device_index = -1;
};

class State
{
public:
    bool is_setup = false;
    bool is_downloading = false;
    bool last_interaction_ts = 0;
    StatusInfo s_info;
    PageInfo p_info;
    DeviceInfo d_info;

    State(){};
    bool load()
    {
        Serial.println("Loading State");
        if (state_file_exists())
        {
            Serial.println("State file exists");
            SdFile file;
            if (file.open(STATE_FILE.c_str(), O_READ))
            {
                Serial.println("State file opened");
                // read from file
                StaticJsonDocument<200> doc;
                DeserializationError error = deserializeJson(doc, file);
                if (!error)
                {
                    Serial.println("State file read");
                    p_info.page_index = doc["page_index"];
                    p_info.page_count = doc["page_count"];
                    file.close();
                    return true;
                }
                else
                {
                    Serial.println("State file read error");
                    file.close();
                    return false;
                }
            }
            else
            {
                Serial.println("Error opening state file!");
                return false;
            }
        }
        else
        {
            Serial.println("State file does not exist, creating...");
            return save();
        }
    }

    bool save()
    {
        // create json
        StaticJsonDocument<200> doc;
        doc["page_index"] = p_info.page_index;
        doc["page_count"] = p_info.page_count;
        // serialize json
        String json;
        serializeJson(doc, json);

        // write to file
        SdFile file;

        if (file.open(STATE_FILE.c_str(), O_RDWR | O_CREAT | O_AT_END))
        {
            // overwrite file with new json
            file.seekSet(0);
            file.truncate(0);
            file.write(json.c_str());
            file.close();

            Serial.println("State file saved");
            return true;
        }

        Serial.println("Error saving state file!");
        return false;
    }

    void set_page_index(int index)
    {
        p_info.page_index = index;
        save();
    }

    void set_page_count(int count)
    {
        p_info.page_count = count;
        save();
    }

private:
    bool state_file_exists()
    {
        SdFile file;
        if (file.open(STATE_FILE.c_str(), O_RDWR))
        {
            file.close();
            return true;
        }
        else
            return false;
    }
};

#endif