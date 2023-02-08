#ifndef storage_manager_h
#define storage_manager_h

#include <Arduino.h>
#include "SdFat.h"

const String PAGE_CHAIN_DIR = "/page_chain/";

class StorageManager
{
private:
    Inkplate *display;
    State *state;
    ImageBuffer next_page_cache;

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
    bool setup(Inkplate *display, State *state)
    {
        this->display = display;
        this->state = state;
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

        if (file.open(bmp_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC))
        {
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
    ImageBuffer get_page(int page_index)
    {
        SdFile file;
        String filepath = get_page_filepath(page_index);
        ImageBuffer img_buffer;

        Serial.println(filepath.c_str());

        if (file.open(filepath.c_str(), O_READ))
        {
            int file_size = file.fileSize();
            img_buffer.buffer = (uint8_t *)malloc(file_size);
            img_buffer.size = file_size;
            file.read(img_buffer.buffer, file_size);
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

    void cache_page(bool is_next)
    {
        Serial.println("Caching next page");
        int next_page_index = state->p_info.page_index + (is_next ? 1 : -1);
        if (next_page_index < 0 || next_page_index >= state->p_info.page_count)
            return;
        Serial.println("next page index " + String(next_page_index));
        next_page_cache = get_page(next_page_index);
        Serial.println("next page cache size " + String(next_page_cache.size));

        Serial.printf("[APP] Free memory: %d bytes \n", esp_get_free_heap_size());
    };

    bool has_cached_page(bool is_next)
    {
        int next_page_index = state->p_info.page_index + (is_next ? 1 : -1);
        if (next_page_index < 0 || next_page_index >= state->p_info.page_count)
            return false;
        return true;
    };

    ImageBuffer *get_cached_page()
    {
        return &next_page_cache;
    }
};

#endif