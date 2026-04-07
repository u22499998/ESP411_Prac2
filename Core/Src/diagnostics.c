/*
 * diagnostics.c
 *
 *  Created on: 02 Apr 2026
 *      Author: edwar
 */

#include "diagnostics.h"
#include "ui_types.h" // For COLOR_BG, COLOR_TEXT, etc.
#include "stm32f429i_discovery_lcd.h"
#include "fonts.h"    // Required for the sFONT structure and font tables
#include <math.h>

// ============================================================================
// 1. LOW LEVEL LANDSCAPE ENGINE
// ============================================================================

void DrawPixelLandscape(uint16_t cart_X, uint16_t cart_Y, uint32_t color) {
    // Safety boundary check for a 320x240 landscape screen
    if (cart_X > 319 || cart_Y > 239) return;

    // New 90-degree Counter-Clockwise Rotation Math (USB port on the LEFT)
    // Maps Cartesian (0,0) to the physical Top-Left of the portrait screen
    uint16_t phys_X = cart_Y;
    uint16_t phys_Y = cart_X;

    BSP_LCD_DrawPixel(phys_X, phys_Y, color);
}

static void UI_DrawRotatedChar(uint16_t x, uint16_t y, uint8_t c) {
    sFONT *font = BSP_LCD_GetFont();

    // Calculate memory offset for the specific character in the font table
    uint32_t char_offset = (c - ' ') * font->Height * ((font->Width + 7) / 8);
    const uint8_t *ptr = &font->table[char_offset];

    uint32_t bytes_per_line = (font->Width + 7) / 8;
    uint32_t fg_color = BSP_LCD_GetTextColor();
    uint32_t bg_color = BSP_LCD_GetBackColor();

    for (uint16_t line = 0; line < font->Height; line++) {
        for (uint16_t col = 0; col < font->Width; col++) {
            uint8_t byte_idx = col / 8;
            uint8_t bit_idx = 7 - (col % 8);

            if (ptr[byte_idx] & (1 << bit_idx)) {
                // Foreground pixel. Y subtracts 'line' because Cartesian Y goes UP,
                // but reading a font goes DOWN.
                DrawPixelLandscape(x + col, y - line, fg_color);
            } else {
                // Background pixel
                DrawPixelLandscape(x + col, y - line, bg_color);
            }
        }
        ptr += bytes_per_line;
    }
}

void UI_DrawRotatedString(uint16_t x, uint16_t y, const char* str) {
    sFONT *font = BSP_LCD_GetFont();
    while (*str) {
        UI_DrawRotatedChar(x, y, *str);
        x += font->Width; // Move right for the next character
        str++;
    }
}

// ============================================================================
// 2. SPECIFIC GRAPH PLOTTERS
// ============================================================================

void Diagnostics_InitRawFFTGraph(uint16_t origin_x, uint16_t origin_y,
        uint16_t x_length, uint16_t y_length,
        uint8_t num_x_indices, uint8_t num_y_indices)
{
		BSP_LCD_Clear(COLOR_BG);
	    BSP_LCD_SetTextColor(COLOR_TEXT);
	    BSP_LCD_SetBackColor(COLOR_BG);


	    BSP_LCD_SetFont(&Font12);

	    // Calculate the exact pixel spacing for the grid lines
	       float x_spacing = (float)x_length / num_x_indices;
	       float y_spacing = (float)y_length / num_y_indices;

	       BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

	       // Draw the main solid axes
	       // X-Axis (Frequency)
	           for (int i = origin_x; i <= origin_x + x_length; i += 1) {
	               BSP_LCD_DrawPixel(origin_x, i, LCD_COLOR_CYAN);
	           }

	       // Y-Axis (Amplitude) - Drawn horizontally in your rotated layout
	           for (int i = origin_y; i <= origin_y + y_length; i += 1) {
	                   BSP_LCD_DrawPixel(i, origin_y, LCD_COLOR_CYAN);
	               }

	       // Draw X-axis (Frequency) indices and grid lines
	       for (int i = 1; i <= num_x_indices; i++) {
	           uint16_t current_y = origin_y + (uint16_t)(i * x_spacing);

	           // Draw the Cyan tick mark (7 pixels long)
	           BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
	           BSP_LCD_DrawHLine(origin_x, current_y, 7);

	           // Draw the Light Gray grid line extending across the graph
	           BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
	           BSP_LCD_DrawHLine(origin_x + 7, current_y, y_length - 7);
	       }

	       // Draw Y-axis (Amplitude) indices and grid lines
	       for (int i = 1; i <= num_y_indices; i++) {
	           uint16_t current_x = origin_x + (uint16_t)(i * y_spacing);

	           // Draw the Cyan tick mark (7 pixels long)
	           BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
	           BSP_LCD_DrawVLine(current_x, origin_y, 7);

	           // Draw the Light Gray grid line extending across the graph
	           BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
	           BSP_LCD_DrawVLine(current_x, origin_y + 7, x_length - 7);
	       }

	    // Draw Y-axis Labels based on your manual code (-70 to 0 dB)
	    	BSP_LCD_SetFont(&Font8);
	        UI_DrawRotatedString(3, 210, "dB");
	        UI_DrawRotatedString(17, 205, "0");
	        UI_DrawRotatedString(2, 167, "-14");
	        UI_DrawRotatedString(2, 132, "-28");
	        UI_DrawRotatedString(2,  100, "-42");
	        UI_DrawRotatedString(2,  62, "-56");
	        UI_DrawRotatedString(2,  30, "-70");


	    // 4. Draw Titles
	    BSP_LCD_SetFont(&Font12);
	    UI_DrawRotatedString(100, 15, "Frequency (Bins)");
	    BSP_LCD_SetFont(&Font16);
	    UI_DrawRotatedString(100, 230, "RAW ADC FFT");
}

void Diagnostics_UpdateRawFFT(float* fft_magnitudes) {
    static uint16_t previous_heights[256] = {0};

    for (int i = 1; i < 256; i++) {
        uint16_t x = 25 + i; // Exactly 1 bin per pixel

        // --- FIXED-SCALE MATH ---
        float mag = fft_magnitudes[i];
        if (mag < 1.0f) mag = 1.0f;

        float db = 20.0f * log10f(mag / 1048576.0f);
        int magnitude = 174 + (int)(db * 2.5f);

        if (magnitude < 0) magnitude = 0;
        if (magnitude > 174) magnitude = 174;

        uint16_t new_height = (uint16_t)magnitude;
        uint16_t old_height = previous_heights[i];
        // ------------------------

        // DELTA DRAWING LOGIC (OPTIMIZED WITH FAST HARDWARE LINES)
        if (new_height > old_height) {
            // Signal got louder! Draw green block upward
            uint16_t start_y = 26 + old_height;
            uint16_t length = (25 + new_height) - start_y + 1;

            BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
            // Because Cartesian Y is Physical X, and Cartesian X is Physical Y:
            BSP_LCD_DrawHLine(start_y, x, length);
        }
        else if (new_height < old_height) {
            // Signal got quieter! Erase old block.
            uint32_t bg_color = COLOR_BG;
            if ((x - 25) % 32 == 0) {
                bg_color = LCD_COLOR_LIGHTGRAY; // Keep vertical grid intact
            }

            uint16_t start_y = 26 + new_height;
            uint16_t length = (25 + old_height) - start_y + 1;

            BSP_LCD_SetTextColor(bg_color);
            BSP_LCD_DrawHLine(start_y, x, length);

            // Redraw the faint horizontal grid dots we crossed while erasing
            for (int grid = 1; grid <= 5; grid++) {
                uint16_t tick_y = 25 + (grid * 35);
                if (tick_y >= (26 + new_height) && tick_y <= (25 + old_height)) {
                    // Just one pixel, so the pixel wrapper is fine here
                    DrawPixelLandscape(x, tick_y, LCD_COLOR_LIGHTGRAY);
                }
            }
        }

        previous_heights[i] = new_height;
    }
}

// ============================================================================
// 3. TIME DOMAIN OUTPUT PLOT (Oscilloscope View)
// ============================================================================

void Diagnostics_InitTimeGraph(uint16_t origin_x, uint16_t origin_y,
        uint16_t x_length, uint16_t y_length,
        uint8_t num_x_indices, uint8_t num_y_indices)
{
	// 1. Clear the screen
	    BSP_LCD_Clear(COLOR_BG);
	    BSP_LCD_SetTextColor(COLOR_TEXT);
	    BSP_LCD_SetBackColor(COLOR_BG);

	    // Calculate the exact pixel spacing for the grid lines
	    float x_spacing = (float)x_length / num_x_indices;
	    float y_spacing = (float)y_length / num_y_indices;

	    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

	    // 2. Draw the main solid axes
	    // X-Axis (Time / Samples)
	    for (int i = origin_x; i <= origin_x + x_length; i += 1) {
	        BSP_LCD_DrawPixel(origin_x, i, LCD_COLOR_CYAN);
	    }

	    // Y-Axis (Voltage) - Drawn horizontally in your rotated layout
	    for (int i = origin_y; i <= origin_y + y_length; i += 1) {
	        BSP_LCD_DrawPixel(i, origin_y, LCD_COLOR_CYAN);
	    }

	    // 3. Draw X-axis (Time) indices and grid lines
	    for (int i = 1; i <= num_x_indices; i++) {
	        uint16_t current_y = origin_y + (uint16_t)(i * x_spacing);

	        // Draw the Cyan tick mark (7 pixels long)
	        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
	        BSP_LCD_DrawHLine(origin_x, current_y, 7);

	        // Draw the Light Gray grid line extending across the graph
	        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
	        BSP_LCD_DrawHLine(origin_x + 7, current_y, y_length - 7);
	    }

	    // 4. Draw Y-axis (Voltage) indices and grid lines
	    for (int i = 1; i <= num_y_indices; i++) {
	        uint16_t current_x = origin_x + (uint16_t)(i * y_spacing);

	        // Draw the Cyan tick mark (7 pixels long)
	        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
	        BSP_LCD_DrawVLine(current_x, origin_y, 7);

	        // Draw the Light Gray grid line extending across the graph
	        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
	        BSP_LCD_DrawVLine(current_x, origin_y + 7, x_length - 7);
	    }

	    // 5. Draw Y-axis Labels for Voltage (0.0V to 3.3V spread across the Y axis)
	    BSP_LCD_SetFont(&Font8);
	    BSP_LCD_SetTextColor(COLOR_TEXT);
	    UI_DrawRotatedString(3, 215, "Volt");

	    // Assuming origin_y is ~25 and max Y is ~199
	    UI_DrawRotatedString(5, 203, "3.3");
	    UI_DrawRotatedString(2, 156, "2.475");
	    UI_DrawRotatedString(2, 115, "1.65");
	    UI_DrawRotatedString(2,  69, "0.825");
	    UI_DrawRotatedString(5,  26, "0.0");

	    // 6. Draw Titles
	    BSP_LCD_SetFont(&Font12);
	    UI_DrawRotatedString(120, 15, "Time (Samples)");

	    BSP_LCD_SetFont(&Font16);
	    UI_DrawRotatedString(60, 230, "FILTERED TIME DOMAIN");
}

void Diagnostics_UpdateTimeGraph(float* time_buffer, uint16_t buffer_size) {
    static uint16_t previous_y[256] = {0};

    // We only loop up to 256 to fit the screen width, acting as our "window"
    for (int i = 0; i < 256; i++) {
        // Safety check in case buffer is unexpectedly small
        if (i >= buffer_size) break;

        uint16_t x = 26 + i;

        // --- Y-AXIS SCALING (0.0V to 3.3V) ---
        // 1. Convert raw 12-bit ADC value (0-4095) to Voltage (0.0-3.3)
        // (If your buffer is already in volts, skip this and just use time_buffer[i])
        float voltage = (time_buffer[i] / 4095.0f) * 3.3f;

        // 2. Clamp voltage for safety to prevent drawing outside the graph area
        if (voltage < 0.0f) voltage = 0.0f;
        if (voltage > 3.3f) voltage = 3.3f;

        // 3. Map 0.0V - 3.3V to the physical Y-pixels.
        // Origin Y is 26. Max Y is 199. Total drawing height is 173 pixels.
        int y_val = 26 + (int)((voltage / 3.3f) * 175.0f);

        uint16_t new_y = (uint16_t)y_val;

        // --- DELTA DRAWING LOGIC ---
        if (new_y != previous_y[i]) {
            // 1. Erase the old pixel
            if (previous_y[i] != 0) {
                uint32_t bg_color = COLOR_BG;
                // Leave grid lines intact if we erase over them
                if ((previous_y[i]- 25) % 44 == 0) bg_color = LCD_COLOR_LIGHTGRAY;
                if ((x - 25) % 32 == 0) bg_color = LCD_COLOR_LIGHTGRAY;

                DrawPixelLandscape(x, previous_y[i], bg_color);
            }

            // 2. Draw the new pixel
            DrawPixelLandscape(x, new_y, LCD_COLOR_YELLOW);

            // 3. Update memory
            previous_y[i] = new_y;
        }
    }
}

// ============================================================================
// 4. FILTERED FFT PLOT (Output Spectrum)
// ============================================================================

void Diagnostics_InitFilteredFFTGraph(uint16_t origin_x, uint16_t origin_y,
        uint16_t x_length, uint16_t y_length,
        uint8_t num_x_indices, uint8_t num_y_indices)
{
	BSP_LCD_Clear(COLOR_BG);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    BSP_LCD_SetBackColor(COLOR_BG);


    BSP_LCD_SetFont(&Font12);

    // Calculate the exact pixel spacing for the grid lines
       float x_spacing = (float)x_length / num_x_indices;
       float y_spacing = (float)y_length / num_y_indices;

       BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

       // Draw the main solid axes
       // X-Axis (Frequency)
           for (int i = origin_x; i <= origin_x + x_length; i += 1) {
               BSP_LCD_DrawPixel(origin_x, i, LCD_COLOR_CYAN);
           }

       // Y-Axis (Amplitude) - Drawn horizontally in your rotated layout
           for (int i = origin_y; i <= origin_y + y_length; i += 1) {
                   BSP_LCD_DrawPixel(i, origin_y, LCD_COLOR_CYAN);
               }

       // Draw X-axis (Frequency) indices and grid lines
       for (int i = 1; i <= num_x_indices; i++) {
           uint16_t current_y = origin_y + (uint16_t)(i * x_spacing);

           // Draw the Cyan tick mark (7 pixels long)
           BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
           BSP_LCD_DrawHLine(origin_x, current_y, 7);

           // Draw the Light Gray grid line extending across the graph
           BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
           BSP_LCD_DrawHLine(origin_x + 7, current_y, y_length - 7);
       }

       // Draw Y-axis (Amplitude) indices and grid lines
       for (int i = 1; i <= num_y_indices; i++) {
           uint16_t current_x = origin_x + (uint16_t)(i * y_spacing);

           // Draw the Cyan tick mark (7 pixels long)
           BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
           BSP_LCD_DrawVLine(current_x, origin_y, 7);

           // Draw the Light Gray grid line extending across the graph
           BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
           BSP_LCD_DrawVLine(current_x, origin_y + 7, x_length - 7);
       }

    // Draw Y-axis Labels based on your manual code (-70 to 0 dB)
    	BSP_LCD_SetFont(&Font8);
        UI_DrawRotatedString(3, 210, "dB");
        UI_DrawRotatedString(17, 205, "0");
        UI_DrawRotatedString(2, 167, "-14");
        UI_DrawRotatedString(2, 132, "-28");
        UI_DrawRotatedString(2,  100, "-42");
        UI_DrawRotatedString(2,  62, "-56");
        UI_DrawRotatedString(2,  30, "-70");


    // 4. Draw Titles
    BSP_LCD_SetFont(&Font12);
    UI_DrawRotatedString(100, 15, "Frequency (Bins)");
    BSP_LCD_SetFont(&Font16);
    UI_DrawRotatedString(70, 230, "OUTPUT BUFFER FFT");
}

void Diagnostics_UpdateFilteredFFT(float* fft_magnitudes) {
    // We can use the EXACT same fast-hardware line drawing logic as the raw FFT.
    // However, we need a separate static array so it doesn't conflict with the Raw FFT's memory!
    static uint16_t previous_filtered_heights[256] = {0};

    for (int i = 1; i < 256; i++) {
        uint16_t x = 25 + i;

        // --- FIXED-SCALE MATH ---
        float mag = fft_magnitudes[i];
        if (mag < 1.0f) mag = 1.0f;

        float db = 20.0f * log10f(mag / 1048576.0f);
        int magnitude = 174 + (int)(db * 2.5f);

        if (magnitude < 0) magnitude = 0;
        if (magnitude > 174) magnitude = 174;

        uint16_t new_height = (uint16_t)magnitude;
        uint16_t old_height = previous_filtered_heights[i];
        // ------------------------

        // DELTA DRAWING LOGIC
        if (new_height > old_height) {
            // Signal got louder! Draw purple/magenta block upward to distinguish from raw FFT
            uint16_t start_y = 26 + old_height;
            uint16_t length = (25 + new_height) - start_y + 1;

            BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
            BSP_LCD_DrawHLine(start_y, x, length);
        }
        else if (new_height < old_height) {
            // Signal got quieter! Erase old block.
            uint32_t bg_color = COLOR_BG;
            if ((x - 25) % 32 == 0) {
                bg_color = LCD_COLOR_LIGHTGRAY;
            }

            uint16_t start_y = 26 + new_height;
            uint16_t length = (25 + old_height) - start_y + 1;

            BSP_LCD_SetTextColor(bg_color);
            BSP_LCD_DrawHLine(start_y, x, length);

            for (int grid = 1; grid <= 5; grid++) {
                uint16_t tick_y = 25 + (grid * 35);
                if (tick_y >= (26 + new_height) && tick_y <= (25 + old_height)) {
                    DrawPixelLandscape(x, tick_y, LCD_COLOR_LIGHTGRAY);
                }
            }
        }
        previous_filtered_heights[i] = new_height;
    }
}


