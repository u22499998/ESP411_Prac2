/*
 * main_menu.h
 *
 *  Created on: 31 Mar 2026
 *      Author: edward
 */

#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

//Dark colour palette
#define COLOR_BG          0xFF1E1E1E
#define COLOR_BTN_IDLE    0xFF3A3D41
#define COLOR_BTN_PRESS   0xFF252729
#define COLOR_HIGHLIGHT   0xFF5C6370
#define COLOR_SHADOW      0xFF111111
#define COLOR_TEXT        0xFFE0E0E0

// The Global State Machine
typedef enum {
    STATE_MAIN_MENU,
    STATE_SUB_HM,
    STATE_SUB_BUFFER,
    STATE_PLOT_HM_TIME,
    STATE_PLOT_HM_FREQ,
    STATE_PLOT_BUFFER_TIME,
    STATE_PLOT_BUFFER_FREQ
} DisplayState_t;

// A shared utility function for 3D buttons
void UI_Draw3DButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* text, bool isPressed);

#endif // UI_TYPES_H
