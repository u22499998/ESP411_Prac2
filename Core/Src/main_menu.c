/*
 * main_menu.c
 *
 *  Created on: 31 Mar 2026
 *      Author: edward
 */

#include "main_menu.h"
#include "stm32f429i_discovery_lcd.h"

void MainMenu_Draw(void) {
    BSP_LCD_Clear(COLOR_BG);

    HAL_Delay(2);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    BSP_LCD_SetBackColor(COLOR_BG);
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_DisplayStringAt(0, 20, (uint8_t*)"SYSTEM MONITOR", CENTER_MODE);

    // Draw the two main buttons
    UI_Draw3DButton(40, 80, 160, 50, "1. Calculate h[m]", false);
    UI_Draw3DButton(40, 160, 160, 50, "2. Output Buffer", false);
}

DisplayState_t MainMenu_HandleTouch(uint16_t x, uint16_t y, DisplayState_t currentState) {
    // Check Button 1 (h[m])
    if (x > 40 && x < 200 && y > 80 && y < 130) {
        UI_Draw3DButton(40, 80, 160, 50, "1. Calculate h[m]", true); // Press effect
        return STATE_SUB_HM;
    }
    // Check Button 2 (Buffer)
    if (x > 40 && x < 200 && y > 160 && y < 210) {
        UI_Draw3DButton(40, 160, 160, 50, "2. Output Buffer", true); // Press effect
        return STATE_SUB_BUFFER;
    }
    return currentState; // No valid button pressed
}

