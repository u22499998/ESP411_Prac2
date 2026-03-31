/*
 * ui_types.c
 *
 *  Created on: 31 Mar 2026
 *      Author: edward
 */

#include "ui_types.h"
#include "stm32f429i_discovery_lcd.h" // Replace with your exact BSP header

void UI_Draw3DButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* text, bool isPressed) {

    // 1. Choose surface color based on state
    uint32_t surfaceColor = isPressed ? COLOR_BTN_PRESS : COLOR_BTN_IDLE;

    // 2. Draw the main button body
    BSP_LCD_SetTextColor(surfaceColor);
    BSP_LCD_FillRect(x, y, width, height);

    // 3. Draw the 3D Bevel Borders
    if (!isPressed) {
        // Raised effect: Light on top/left, Dark on bottom/right
        BSP_LCD_SetTextColor(COLOR_HIGHLIGHT);
        BSP_LCD_DrawHLine(x, y, width);               // Top edge
        BSP_LCD_DrawVLine(x, y, height);              // Left edge

        BSP_LCD_SetTextColor(COLOR_SHADOW);
        BSP_LCD_DrawHLine(x, y + height - 1, width);  // Bottom edge
        BSP_LCD_DrawVLine(x + width - 1, y, height);  // Right edge
    } else {
        // Sunken effect (Pressed): Dark on top/left, Light on bottom/right
        BSP_LCD_SetTextColor(COLOR_SHADOW);
        BSP_LCD_DrawHLine(x, y, width);
        BSP_LCD_DrawVLine(x, y, height);

        BSP_LCD_SetTextColor(COLOR_HIGHLIGHT);
        BSP_LCD_DrawHLine(x, y + height - 1, width);
        BSP_LCD_DrawVLine(x + width - 1, y, height);
    }

    // 4. Draw the Text (Centered)
    BSP_LCD_SetBackColor(surfaceColor); // Make text background match button
    BSP_LCD_SetFont(&Font12);
    BSP_LCD_SetTextColor(COLOR_TEXT);

    // Y-offset math: Center the text vertically.
    // Assuming a standard BSP font height of ~16 pixels.
    BSP_LCD_DisplayStringAt(0, y + (height / 2) - 8, (uint8_t*)text, CENTER_MODE);

    // Reset background color to the standard app background
    BSP_LCD_SetBackColor(COLOR_BG);
}

