#include "config.h"

#include "Inkplate.h"
#include "SdFat.h" 
#include <HTTPClient.h>
#include <WiFi.h>


Inkplate display(INKPLATE_3BIT);
String mac_addr;

enum {
    tp_none = 0,
    tp_left = 1,
    tp_middle = 2,
    tp_right = 3
} touchpad_pressed;

enum {
    page_view,
    doc_view
} view_mode;

long prev_millis = 0;
bool check_new_doc_while_idle = true;
int check_docs_interval_ms = 30 * 1000;

bool touchpad_released = true;
int touchpad_cooldown_ms = 1000;
long touchpad_released_time = 0;

int cur_page_num = 1;
char cur_doc_name[255];

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

void save_img_buff_to_sd(uint8_t *buf, String &filename)
{
    SdFile file;
    String bmp_filename = filename + ".bmp";

    //make sure file can be created, otherwise print error
    if (!file.open(bmp_filename.c_str(), O_RDWR | O_CREAT | O_AT_END))
    {
        Serial.println("Error creating file!");
    }
    
    int file_size = READ32(buf + 2);
    file.write(buf, file_size);
    file.flush();
    file.close();                                        // close file when done writing

    Serial.println("file saved");
}

uint8_t* download_file(String &doc_name, int &page_num)
{
    String url = String(HOST)
                + "/img"
                + "?client="
                + mac_addr
                + "&doc_name="
                + doc_name
                + "&page_num="
                + page_num;

    int len = 1200*825; // display resolution
    Serial.println("Downloading...");
    uint8_t *img_buf = display.downloadFile(url.c_str(), &len);
    // uint8_t *img_buf = display.downloadFile("https://people.math.sc.edu/Burkardt/data/bmp/all_gray.bmp", &len);
    Serial.println("image downloaded");
    return img_buf;
}

bool server_has_new_file(String &doc_name,int &num_pages)
{
    HTTPClient http;
    uint16_t port = 5000;
    String server_addr = String(HOST)
                        + "?client"
                        + mac_addr;
    http.begin(server_addr);

    int response_code = http.GET();
    if(response_code == 200)
    {
        String payload = http.getString();
        Serial.print("payload:\t");
        Serial.println(payload);
        int index = payload.indexOf("_");
        doc_name = payload.substring(0, index);
        num_pages = payload.substring(index + 1).toInt();

        Serial.print("server has new file with name: ");
        Serial.print(doc_name);
        Serial.print("\tpages:");
        Serial.println(num_pages);
        return true;
    }
    else if(response_code == 201)
    {
        Serial.println("No new image available.");
        return false;
    }
    else
    {
        Serial.println("HTTP Error");
        return false;
    }
    return false;
}

void request_doc_routine()
{
    int num_pages;
    String doc_name;
    if(!server_has_new_file(doc_name, num_pages)) return;

    for(int cur_page=1; cur_page<=num_pages; ++cur_page)
    {
        uint8_t* img_buf = download_file(doc_name, cur_page);
        Serial.println("file downloaded");
        if(img_buf == nullptr)
        {
            Serial.println("Error downloading file.");
            return;
        }

        if(cur_page == 1)    // immediately display the first page
        {
            if(display.drawBitmapFromBuffer(img_buf, 0, 0, 0, 0))
            {
                Serial.println("drawImage returned 1");
                display.display();
            }
            else Serial.println("drawImage failed");
        }

        save_img_buff_to_sd(img_buf, doc_name + "_" + cur_page);
    }

    strcpy(cur_doc_name, doc_name.c_str());
    cur_page_num = 1;
}

void prev_page()
{
    if(cur_page_num <= 1) return;

    String page_name = String(cur_doc_name) + "_" + (cur_page_num - 1) + ".bmp";
    if(display.drawImage(page_name, display.BMP, 0, 0))
    {
        cur_page_num--;
        display.display();
    }
    else 
    {
        Serial.print("cannot get previous page with num");
        Serial.println(cur_page_num-1);
    }
}

void next_page()
{
    String page_name = String(cur_doc_name) + "_" + (cur_page_num + 1) + ".bmp";
    if(display.drawImage(page_name, display.BMP, 0, 0))
    {
        cur_page_num++;
        display.display();
    }
    else 
    {
        Serial.print("cannot get next page with num: ");
        Serial.print(cur_page_num+1);
        Serial.print(" for doc: ");
        Serial.println(page_name);
    }
}

void prev_doc()
{

}

void next_doc()
{

}

int read_touchpads()
{
    if(display.readTouchpad(PAD1)) touchpad_pressed = tp_left;
    else if(display.readTouchpad(PAD2)) touchpad_pressed = tp_middle;
    else if(display.readTouchpad(PAD3)) touchpad_pressed = tp_right;
    else touchpad_pressed = tp_none;
}

void touchpad_routine()
{
    read_touchpads();
    Serial.println(touchpad_pressed);
    if(touchpad_pressed == tp_none && !touchpad_released)
    {
        Serial.println("nothing pressed");
        touchpad_released = true;
        touchpad_released_time = millis();
        return;
    }

    if(!touchpad_released || touchpad_released_time + touchpad_cooldown_ms > millis())
    {
        // Serial.println("not released or on cooldown");
        return;
    }
    if(touchpad_pressed > 0) touchpad_released = false;
    
    switch(touchpad_pressed)
    {
        case tp_left:
            Serial.println("TP left");
            if(view_mode == page_view) prev_page();
            else prev_doc();
            break;
        case tp_middle:
            // view_mode = !view_mode;
            break;
        case tp_right:
            Serial.println("TP right");
            if(view_mode == page_view) next_page();
            else next_doc();
            break;
    }
}

void setup()
{
    Serial.begin(115200);
    display.begin();
    view_mode = page_view;
    String foo = "job-1";
    strcpy(cur_doc_name, foo.c_str());

    setup_wifi();
    mac_addr = WiFi.macAddress();

    // Init SD card. Display if SD card is init propery or not.
    if (display.sdCardInit())
    {
        display.println("SD Card OK!");
    }

    //request_doc_routine();
    //enter_deep_sleep();
}

void loop()
{
    touchpad_routine();

    if(check_new_doc_while_idle)
    {
        if(prev_millis + check_docs_interval_ms > millis())
        {
            prev_millis = millis();
            //request_doc_routine();
        }
    }
}