#ifndef tp_controller_h
#define tp_controller_h

#include "Inkplate.h"

enum TP_PRESSED
{
    tp_none = 0,
    tp_left = 1,
    tp_middle = 2,
    tp_right = 3
} touchpad_pressed;

class TouchpadEventCallback
{
public:
    virtual void handle_left_tp_pressed() = 0;
    virtual void handle_middle_tp_pressed() = 0;
    virtual void handle_right_tp_pressed() = 0;
};

class TouchpadController
{
private:
    Inkplate *display;
    TouchpadEventCallback *callback;

    bool touchpad_released = true;
    int touchpad_cooldown_ms = 500;
    long touchpad_released_time = 0;
    int last_interaction_ts = 0;

    void read()
    {
        if (display->readTouchpad(PAD1))
            touchpad_pressed = tp_left;
        else if (display->readTouchpad(PAD2))
            touchpad_pressed = tp_middle;
        else if (display->readTouchpad(PAD3))
            touchpad_pressed = tp_right;
        else
            touchpad_pressed = tp_none;
    }

    void handle_left_tp_pressed()
    {
        last_interaction_ts = millis();
        callback->handle_left_tp_pressed();
    }

    void handle_middle_tp_pressed()
    {
        last_interaction_ts = millis();
        callback->handle_middle_tp_pressed();
    }

    void handle_right_tp_pressed()
    {
        last_interaction_ts = millis();
        callback->handle_right_tp_pressed();
    }

public:
    void setup(Inkplate *display, TouchpadEventCallback *callback)
    {
        this->display = display;
        this->callback = callback;
        this->last_interaction_ts = millis();
    }
    void loop()
    {
        read();
        if (touchpad_pressed == tp_none && !touchpad_released)
        {
            touchpad_released = true;
            touchpad_released_time = millis();
            return;
        }

        if (!touchpad_released || touchpad_released_time + touchpad_cooldown_ms > millis())
        {
            // not released or on cooldown
            return;
        }
        if (touchpad_pressed > 0)
            touchpad_released = false;

        switch (touchpad_pressed)
        {
        case tp_left:
            handle_left_tp_pressed();
            break;
        case tp_middle:
            handle_middle_tp_pressed();
            break;
        case tp_right:
            handle_right_tp_pressed();
            break;
        case tp_none:
            // no button pressed, ignore
            break;
        }
    }
};

#endif