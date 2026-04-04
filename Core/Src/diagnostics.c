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

static uint16_t previous_heights[256] = {0};
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

void Diagnostics_ResetFFTMemory(void) {
    for (int i = 0; i < 256; i++) {
        previous_heights[i] = 0;
    }
}

void Diagnostics_UpdateRawFFT(float* fft_magnitudes) {


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
// 3. ADDITIONAL PRACTICAL 2 PLOTTERS
// ============================================================================

/**
 * @brief Draws the static Filter Coefficients h[m]
 */
void Diagnostics_DrawHMPlot(float* coefficients, uint16_t order) {
    BSP_LCD_Clear(COLOR_BG);

    // 1. Draw Axis Lines
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_DrawLine(120, 40, 120, 280); // X-axis (Center)
    BSP_LCD_DrawLine(20, 40, 220, 40);   // Y-axis (Left)

    // 2. Add Axis Headings (Titles)
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    // Main Title
    UI_DrawRotatedString(80, 230, "Impulse Response h[m]");

    // 3. Add Axis Labels
    BSP_LCD_SetFont(&Font12);
    // Y-Axis Label (Amplitude) - Near the vertical axis
    UI_DrawRotatedString(10, 140, "Amp");

    // X-Axis Label (Sample Index) - At the far end of the horizontal axis
    UI_DrawRotatedString(260, 110, "m");

    // 4. Draw the actual sinc data...
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    for (int i = 0; i < order - 1; i++) {
            // Find center (120) and scale (800.0f).
            // Increase 800.0f if the line is too flat.
    	int y1 = 120 + (int)(coefficients[i] * 800.0f);
    	int y2 = 120 + (int)(coefficients[i+1] * 800.0f);

            // Map index to the 240-pixel wide plot area (from x=40 to x=280)
            int x1 = 40 + (i * 240 / order);
            int x2 = 40 + ((i + 1) * 240 / order);

            // Map Cartesian to Physical coordinates:
            // Physical X = Cartesian Y, Physical Y = Cartesian X
            BSP_LCD_DrawLine(y1, x1, y2, x2);
        }
}

/**
 * @brief Continuous Time Domain plotter (Input vs Filtered Output)
 */
void Diagnostics_UpdateTimeDomain(float* input, float* output) {
    static uint16_t prev_in[250] = {0};
    static uint16_t prev_out[250] = {0};

    for (int i = 0; i < 249; i++) {
        uint16_t x = 40 + i;

        // Map ADC 0-4095 to 0-100 pixels (Top half)
        uint16_t in_y = 60 - (uint16_t)((input[i] - 2048.0f) / 40.0f);
        // Map Filtered output to 0-100 pixels (Bottom half)
        uint16_t out_y = 180 - (uint16_t)(output[i] / 40.0f);

        // Erase old pixels
        DrawPixelLandscape(x, prev_in[i], COLOR_BG);
        DrawPixelLandscape(x, prev_out[i], COLOR_BG);

        // Draw new pixels
        DrawPixelLandscape(x, in_y, LCD_COLOR_BLUE);   // Input (Blue)
        DrawPixelLandscape(x, out_y, LCD_COLOR_ORANGE); // Output (Orange)

        prev_in[i] = in_y;
        prev_out[i] = out_y;
    }
}

/**
 * @brief Reuses the FFT Graph logic for the Filtered Frequency Spectrum
 */
void Diagnostics_InitFilteredFFTGraph(void) {
    Diagnostics_InitRawFFTGraph(25, 25, 256, 175, 8, 5);
    BSP_LCD_SetFont(&Font16);
    UI_DrawRotatedString(100, 230, "FILTERED SPECTRUM");
}

