#ifndef view_controller_h
#define view_controller_h

#include "Inkplate.h"
#include "./state.h"
#include "./config.h"

// bitmask for GPIO_34 which is connected to MCP INTB
#define TOUCHPAD_WAKE_MASK (int64_t(1) << GPIO_NUM_34)

const int AWAKE_TIME = 60;             // seconds
const int WIFI_CONNECTION_TIMEOUT = 5; // seconds
const int C_BLACK = 1;
const int C_WHITE = 0;

class ViewController
{

private:
    Inkplate *display;
    int DISPLAY_WIDTH;
    int DISPLAY_HEIGHT;

    State *state;

    String get_page_filename(int page_index)
    {
        String filename = String(page_index) + ".jpeg";
        // USE_SERIAL.println("filename " + filename);
        return filename;
    }

    String get_page_filepath(int page_index)
    {
        String filepath = "/page_chain/" + get_page_filename(page_index);
        // USE_SERIAL.println("filepath " + filepath);
        return filepath;
    }

public:
    ViewController(){};
    void setup(Inkplate *display, State *state)
    {
        this->display = display;
        this->state = state;

        display->begin();
        // Setup mcp interrupts
        display->setIntOutputInternal(MCP23017_INT_ADDR, display->mcpRegsInt, 1, false, false, HIGH);
        display->setIntPinInternal(MCP23017_INT_ADDR, display->mcpRegsInt, PAD1, RISING);
        display->setIntPinInternal(MCP23017_INT_ADDR, display->mcpRegsInt, PAD2, RISING);
        display->setIntPinInternal(MCP23017_INT_ADDR, display->mcpRegsInt, PAD3, RISING);

        display->setDisplayMode(INKPLATE_1BIT);

        display->setRotation(3); // Portrait mode

        DISPLAY_WIDTH = display->width();
        DISPLAY_HEIGHT = display->height();
    }
    void refresh_display()
    {
        display->partialUpdate();
    }

    bool show_page(int page_index, bool do_show_gui, bool do_show_connection)
    {
        // // USE_SERIAL.print("Showing page ");
        String filepath = get_page_filepath(page_index);

        if (display->drawJpegFromSd(filepath.c_str(), 0, 0, 0, 0))
            draw_status_bar(do_show_connection);
        else
        {
            // serial.println("Failed to show page");
            return false;
        }

        if (do_show_gui)
            draw_gui();

        refresh_display();

        return true;
    }

    void navigate_page(int page_change)
    {
        // TODO: Private
        draw_loading_icon(page_change > 0 ? tp_right : tp_left);
        refresh_display();

        int new_page_index = state->p_info.page_index + page_change;
        if (new_page_index <= state->p_info.page_count && new_page_index > 0)
        {

            state->set_page_index(new_page_index);
            // state->p_info.page_index = new_page_index; // TODO: state obj, NOT SAVED RIGHT NOW!!!!
            draw_gui();
            show_page(new_page_index, true, true);
        }
        else
        {
            draw_gui();
            refresh_display();
        }
    }

    void prev_page()
    {
        navigate_page(-1);
    }

    void next_page()
    {
        navigate_page(1);
    }

    void show_gui()
    {
        show_page(state->p_info.page_index, true, true);
    }

    void hide_gui()
    {
        show_page(state->p_info.page_index, false, false);
    }

    void draw_gui()
    {
        draw_gui_bg();
        draw_back_button();
        draw_next_button();
        draw_device_index_info();
    }

    void draw_gui_bg()
    {
        // TODO: Private
        int padding = 4;
        int border_width = 2;

        int width = 56 + padding;
        int height = get_button_y(tp_right, width) - get_button_y(tp_left, width) + 56 + padding;
        int x = 0;
        int y = get_button_y(tp_left, width);

        // black border
        display->fillRect(x, y - border_width, width + border_width, height + border_width * 2, C_BLACK);
        // white background
        display->fillRect(x, y, width, height, C_WHITE);
    }

    void draw_loading_icon(TP_PRESSED tp)
    {
        int icon_size = sizeof(hourglass_icon);
        switch (tp)
        {
        case tp_left:
            display->drawJpegFromBuffer(hourglass_icon, icon_size, 0, get_button_y(tp_left, 56), true, false);
            break;

        case tp_right:
            display->drawJpegFromBuffer(hourglass_icon, icon_size, 0, get_button_y(tp_right, 56), true, false);
            break;

        case tp_middle:
            display->drawJpegFromBuffer(hourglass_icon, icon_size, 0, get_button_y(tp_middle, 56), true, false);
            break;
        }
    }

    void refresh_connection_status()
    {
        draw_connection_status();
        refresh_display();
    }

    void draw_status_bar(bool do_show_connection)
    {
        // // USE_SERIAL.println("drawing status bar");

        // serial.println("[APP] Free memory: " + String(esp_get_free_heap_size()) + " bytes");

        int cursor_x = 0;
        int cursor_y = DISPLAY_HEIGHT - 12;

        // white bg
        display->fillRect(cursor_x, cursor_y - 12, DISPLAY_WIDTH, 12 * 2, C_WHITE);

        String page_info = "[" + String(state->p_info.page_index) + "/" + String(state->p_info.page_count) + "]";
        String wifi_status = state->s_info.is_wifi_setup ? (state->s_info.is_wifi_connected ? "O" : "X") : "...";
        String server_status = state->s_info.is_socket_setup ? (state->s_info.is_socket_registered ? "O" : "X") : "...";
        String info_page = " Page: " + page_info;
        String info_conn = info_page + " | Wifi: [" + wifi_status + "] | Server: [" + server_status + "]";

        const GFXfont *text1_font = &FreeMono9pt7b;
        display->setFont(text1_font);
        display->setTextColor(C_BLACK, C_WHITE);
        display->setTextSize(1);
        display->setCursor(cursor_x, cursor_y);

        if (do_show_connection)
            display->print(info_conn);
        else
            display->print(info_page);
    }

    void draw_connection_status()
    {
        draw_status_bar(true);
    }

    void draw_next_button()
    {
        int icon_size = sizeof(arrow_right_icon);
        display->drawJpegFromBuffer(arrow_right_icon, icon_size, 0, get_button_y(tp_right, 56), true, false);
    }

    void draw_back_button()
    {
        int icon_size = sizeof(arrow_left_icon);
        display->drawJpegFromBuffer(arrow_left_icon, icon_size, 0, get_button_y(tp_left, 56), true, false);
    }

    void draw_device_index_info()
    {
        if (state->is_downloading)
        {
            draw_loading_icon(tp_middle);
        }
        else if (state->d_info.device_index == -1)
        {
            draw_enqueue_button();
        }
        else
        {
            draw_device_index();
        }
    }

    void draw_device_index()
    {
        // // USE_SERIAL.println("drawing index");

        // white bg
        int bg_x = 0;
        int bg_y = get_button_y(tp_middle, 56);
        display->fillRect(bg_x, bg_y, 56, 56, C_WHITE);

        // index number
        int icon_size = sizeof(enqueue_icon);
        String device_index_string = state->d_info.device_index == -1 ? "0" : String(state->d_info.device_index);
        int cursor_x = 6;
        if (state->p_info.page_count < 10)
            cursor_x += 9; // center it if its only 1 char
        int cursor_y = get_button_y(tp_middle, 0) + 7;

        const GFXfont *font = &FreeMono24pt7b;
        display->setFont(font);
        display->setTextColor(C_BLACK, C_WHITE);
        display->setTextSize(1);
        display->setCursor(cursor_x, cursor_y);
        display->print(device_index_string);
    }

    void draw_enqueue_button()
    {
        // // USE_SERIAL.println("draw enqueue");
        int icon_size = sizeof(enqueue_icon);
        display->drawJpegFromBuffer(enqueue_icon, icon_size, 0, get_button_y(tp_middle, 56), true, false);
    }

    int get_button_spacing()
    {
        // TODO: private
        return DISPLAY_HEIGHT / 10 - 8; // dont ask me why
    }

    int get_button_y(TP_PRESSED button, int button_height)
    {
        // TODO: private
        int middle_button_x = DISPLAY_HEIGHT / 2;
        int button_offset = button_height / 2;
        switch (button)
        {
        case tp_left:
            return middle_button_x - get_button_spacing() - button_offset;
        case tp_right:
            return middle_button_x + get_button_spacing() - button_offset;
        default:
            return middle_button_x - button_offset;
        }
    }
};

#endif /* view_controller_h */