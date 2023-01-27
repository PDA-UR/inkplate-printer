#ifndef storage_manager_h
#define storage_manager_h

#include <Arduino.h>
#include "SdFat.h"

const String PAGE_CHAIN_DIR = "/page_chain/";

class StorageManager
{
private:
    Inkplate *display;

    String get_page_filename(int page_index)
    {
        String filename = String(page_index) + ".jpeg";
        Serial.println("filename " + filename);
        return filename;
    };

    String get_page_filepath(int page_index)
    {
        String filepath = PAGE_CHAIN_DIR + get_page_filename(page_index);
        Serial.println("filepath " + filepath);
        return filepath;
    };

public:
    bool setup(Inkplate *display)
    {
        this->display = display;
        // Init SD card. Display if SD card is init properly or not.
        Serial.println("Setup: Storage");

        if (display->sdCardInit())
        {
            Serial.println("SD Card init ok!");

            SdFile root;
            Serial.println("Checking " + PAGE_CHAIN_DIR);
            if (root.open("/", O_READ))
            {
                Serial.println("SD Card Accessible");
                root.close();
                return true;
            }
            else
                Serial.println("Failed to open root dir of SD Card");
        }
        else
            Serial.println("SD Card init ERROR!");
        return false;
    };

    void save_page(int &page_index, ImageBuffer *img)
    {
        SdFile file;
        String bmp_filename = get_page_filepath(page_index);

        // make sure file can be created, otherwise print error
        Serial.println(bmp_filename.c_str());

        // create file
        if (file.open(bmp_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC))
        {
            // opened file
            // clear contents
            file.truncate(0);
            file.seekSet(0);

            Serial.println("writing to file with size " + String(img->size));

            file.write(img->buffer, img->size);
            Serial.println("write");
            file.flush();
            Serial.println("flush");
        }
        else
            Serial.println("Error opening page file");

        file.close();
    };

    // @ToDo Remove unused?
    uint8_t *get_page(int page_index)
    {
        SdFile file;
        String filepath = get_page_filepath(page_index);
        uint8_t *img_buffer = NULL;

        Serial.println(filepath.c_str());

        if (file.open(filepath.c_str(), O_READ))
        {
            int file_size = file.fileSize();
            img_buffer = (uint8_t *)malloc(file_size);
            file.read(img_buffer, file_size);
        }
        else
            Serial.println("Error opening bmp file");

        file.close();
        return img_buffer;
    };

    void clear_stored_pages()
    {
        Serial.println("Clearing stored images");
        SdFile dir;
        SdFile file;
        if (dir.open(PAGE_CHAIN_DIR.c_str()))
        {
            while (file.openNext(&dir, O_READ))
            {
                file.remove();
                file.close();
            }
            dir.close();
        }
        else
            Serial.println("Error opening page chain directory");
    };

    // @ToDo: Remove unused?
    int get_num_stored_pages()
    {
        File dir;
        SdFile file;
        int count = 0;
        if (dir.open(PAGE_CHAIN_DIR.c_str()))
        {
            while (file.openNext(&dir, O_READ))
            {
                count++;
                file.close();
            }
            dir.close();
        }
        else
            Serial.println("Error opening page chain directory");

        return count;
    };
};

#endif