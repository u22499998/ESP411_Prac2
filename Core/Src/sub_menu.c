/*
 * sub_menu.c
 *
 *  Created on: 31 Mar 2026
 *      Author: edward
 */

#include "sub_menu.h"
#include "stm32f429i_discovery_lcd.h"

void SubMenu_Draw(const char* title) {
    BSP_LCD_Clear(COLOR_BG);
    HAL_Delay(2);

    BSP_LCD_SetTextColor(0xFFFFD700);
    BSP_LCD_SetBackColor(COLOR_BG);
    BSP_LCD_DisplayStringAt(0, 20, (uint8_t*)title, CENTER_MODE);

    UI_Draw3DButton(40, 80, 160, 50, "Time Analysis", false);
    UI_Draw3DButton(40, 160, 160, 50, "Freq Analysis", false);

    // Small Back Button
    UI_Draw3DButton(80, 280, 80, 30, "BACK", false);
}

DisplayState_t SubMenu_HandleTouch(uint16_t x, uint16_t y, DisplayState_t currentState) {
    // Back Button hit logic (applies to both sub-menus)
    if ((x > 75 && x < 165) && y > 280) {
        UI_Draw3DButton(80, 280, 80, 30, "BACK", true);
        return STATE_MAIN_MENU;
    }

    // Route based on WHICH sub-menu we are currently in
    if (currentState == STATE_SAMPLED) {
        if (y > 80 && y < 130) return STATE_PLOT_RAW_TIME;
        if (y > 160 && y < 210) return STATE_PLOT_RAW_FFT;
    }
    else if (currentState == STATE_SUB_HM) {
        if (y > 80 && y < 130) return STATE_PLOT_HM_TIME;
        if (y > 160 && y < 210) return STATE_PLOT_HM_FREQ;
    }
    else if (currentState == STATE_SUB_BUFFER) {
        if (y > 80 && y < 130) return STATE_PLOT_BUFFER_TIME;
        if (y > 160 && y < 210) return STATE_PLOT_BUFFER_FREQ;
    }
    return currentState;
}
