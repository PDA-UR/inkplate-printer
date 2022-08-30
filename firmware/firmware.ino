#include "config.h"

Inkplate display(INKPLATE_3BIT);
String mac_addr;

enum {
    none = 0,
    left = 1,
    middle = 2,
    right = 3
} touchpad_pressed;

enum {
    page_view,
    doc_view
} view_mode;

bool check_new_doc_while_idle = true;
long prev_millis = 0;
int check_docs_interval_ms = 30 * 1000;
bool touchpad_released = true;
int touchpad_cooldown_ms = 1000;
long touchpad_released_time = 0;

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
}

int read_touchpads()
{
    if(display.readTouchpad(PAD1)) return 1;
    if(display.readTouchpad(PAD2)) return 2;
    if(display.readTouchpad(PAD3)) return 3;
    return 0;
}

void setup()
{
    Serial.begin(115200);
    display.begin();

    setup_wifi();
    mac_addr = WiFi.macAddress();

    // Init SD card. Display if SD card is init propery or not.
    if (display.sdCardInit())
    {
        display.println("SD Card OK!");
    }

    request_doc_routine();
    //enter_deep_sleep();
}

void touchpad_routine()
{
    touchpad_pressed = read_touchpads();
    if(touchpad_pressed == none)
    {
        touchpad_released = true;
        touchpad_released_time = millis();
        return;
    }

    if(!touchpad_released || touchpad_released_time + touchpad_cooldown_ms > millis())
    {
        return;
    }
    touchpad_released = false;

    switch(touchpad_pressed)
    {
        case left:
            if(view_mode == doc_view) prev_page();
            else prev_doc();
            break;
        case middle:
            view_mode = !view_mode;
            break;
        case right:
            if(view_mode == doc_view) next_page();
            else next_doc();
            break;
    }
}

void loop()
{
    touchpad_routine();

    if(check_new_doc_while_idle)
    {
        if(prev_millis + check_docs_interval_ms > millis())
        {
            prev_millis = millis();
            request_doc_routine();
        }
    }
}