#pragma once

#include "VVect2.h"

NV_NAMESPACE_BEGIN

enum JoyButton
{
    BUTTON_A				= 1<<0,
    BUTTON_B				= 1<<1,
    BUTTON_X				= 1<<2,
    BUTTON_Y				= 1<<3,
    BUTTON_START			= 1<<4,
    BUTTON_BACK				= 1<<5,
    BUTTON_SELECT			= 1<<6,
    BUTTON_MENU				= 1<<7,
    BUTTON_RIGHT_TRIGGER	= 1<<8,
    BUTTON_LEFT_TRIGGER		= 1<<9,
    BUTTON_DPAD_UP			= 1<<10,
    BUTTON_DPAD_DOWN		= 1<<11,
    BUTTON_DPAD_LEFT		= 1<<12,
    BUTTON_DPAD_RIGHT		= 1<<13,

    // The analog sticks can also be treated as binary buttons
    BUTTON_LSTICK_UP		= 1<<14,
    BUTTON_LSTICK_DOWN		= 1<<15,
    BUTTON_LSTICK_LEFT		= 1<<16,
    BUTTON_LSTICK_RIGHT		= 1<<17,

    BUTTON_RSTICK_UP		= 1<<18,
    BUTTON_RSTICK_DOWN		= 1<<19,
    BUTTON_RSTICK_LEFT		= 1<<20,
    BUTTON_RSTICK_RIGHT		= 1<<21,

    BUTTON_TOUCH			= 1<<22,		// finger on touchpad

    // Touch gestures recognized by app
    BUTTON_SWIPE_UP			= 1<<23,
    BUTTON_SWIPE_DOWN		= 1<<24,
    BUTTON_SWIPE_FORWARD	= 1<<25,
    BUTTON_SWIPE_BACK		= 1<<26,

    BUTTON_TOUCH_WAS_SWIPE	= 1<<27,		// after a swipe, we still have a touch release coming when the user stops touching the touchpad.
                                                                                    // this bit is set until the frame following the touch release to allow apps to ignore the touch
                                                                                    // if necessary.

    BUTTON_TOUCH_SINGLE		= 1<<28,
    BUTTON_TOUCH_DOUBLE		= 1<<29
};

enum JoyEvent
{
    BUTTON_JOYPAD_FLAG = 0x10000,

    KEYCODE_GAME = 0 | BUTTON_JOYPAD_FLAG,
    KEYCODE_A = 96 | BUTTON_JOYPAD_FLAG,
    KEYCODE_B = 97 | BUTTON_JOYPAD_FLAG,
    KEYCODE_X = 99 | BUTTON_JOYPAD_FLAG,
    KEYCODE_Y = 100 | BUTTON_JOYPAD_FLAG,
    KEYCODE_START = 108 | BUTTON_JOYPAD_FLAG,
    KEYCODE_BACK = 4 | BUTTON_JOYPAD_FLAG,
    KEYCODE_SELECT = 109 | BUTTON_JOYPAD_FLAG,
    KEYCODE_MENU = 82 | BUTTON_JOYPAD_FLAG,
    KEYCODE_RIGHT_TRIGGER = 103 | BUTTON_JOYPAD_FLAG,
    KEYCODE_LEFT_TRIGGER = 102 | BUTTON_JOYPAD_FLAG,
    KEYCODE_DPAD_UP = 19 | BUTTON_JOYPAD_FLAG,
    KEYCODE_DPAD_DOWN = 20 | BUTTON_JOYPAD_FLAG,
    KEYCODE_DPAD_LEFT = 21 | BUTTON_JOYPAD_FLAG,
    KEYCODE_DPAD_RIGHT = 22 | BUTTON_JOYPAD_FLAG,

    KEYCODE_LSTICK_UP = 200 | BUTTON_JOYPAD_FLAG,
    KEYCODE_LSTICK_DOWN = 201 | BUTTON_JOYPAD_FLAG,
    KEYCODE_LSTICK_LEFT = 202 | BUTTON_JOYPAD_FLAG,
    KEYCODE_LSTICK_RIGHT = 203 | BUTTON_JOYPAD_FLAG,

    KEYCODE_RSTICK_UP = 204 | BUTTON_JOYPAD_FLAG,
    KEYCODE_RSTICK_DOWN = 205 | BUTTON_JOYPAD_FLAG,
    KEYCODE_RSTICK_LEFT = 206 | BUTTON_JOYPAD_FLAG,
    KEYCODE_RSTICK_RIGHT = 207 | BUTTON_JOYPAD_FLAG
};

struct VInput
{
    VInput()
        : buttonState(0)
        , buttonPressed(0)
        , buttonReleased(0)
        , swipeFraction(0.0f)
    {
        sticks[0][0] = sticks[0][1] = sticks[1][0] = sticks[1][1] = 0.0f;
    }

    // [0][] = left pad
    // [1][] = right pad
    // [][0] : -1 = left, 1 = right
    // [][1] = -1 = up, 1 = down
    float sticks[2][2];

    V2Vectf touch;
    V2Vectf touchRelative;

    uint buttonState;

    uint buttonPressed;
    uint buttonReleased;

    float swipeFraction;
};

NV_NAMESPACE_END
