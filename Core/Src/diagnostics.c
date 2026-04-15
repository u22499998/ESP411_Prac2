/*
 * diagnostics.c
 *
 * Created on: 02 Apr 2026
 * Author: edwar
 */

#include "diagnostics.h"
#include "ui_types.h" // For COLOR_BG, COLOR_TEXT, etc.
#include "stm32f429i_discovery_lcd.h"
#include "fonts.h"    // Required for the sFONT structure and font tables
#include <math.h>
#include <string.h>   // Added for memset to clear memory arrays
#include <stdio.h>    // Added for snprintf label formatting

// ============================================================================
// 0. GLOBAL GRAPHICS MEMORY (Moved here so they can be reset on screen init)
// ============================================================================
static uint16_t previous_heights[256] = {0};          // Raw FFT memory
static uint16_t previous_y[256] = {0};                // Time Graph memory
static uint16_t previous_filtered_heights[256] = {0}; // Filtered FFT memory
static uint16_t prev_current[256] = {0};              // Persist FFT memory
static float peak_heights[256] = {0};                 // Persist FFT memory
static uint16_t prev_peaks[256] = {0};                // Persist FFT memory

// ============================================================================
// 1. LOW LEVEL LANDSCAPE ENGINE
// ============================================================================

void DrawPixelLandscape(uint16_t cart_X, uint16_t cart_Y, uint32_t color) {
    if (cart_X > 319 || cart_Y > 239) return;

    uint16_t phys_X = cart_Y;
    uint16_t phys_Y = cart_X;

    BSP_LCD_DrawPixel(phys_X, phys_Y, color);
}

static void UI_DrawRotatedChar(uint16_t x, uint16_t y, uint8_t c) {
    sFONT *font = BSP_LCD_GetFont();
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
                DrawPixelLandscape(x + col, y - line, fg_color);
            } else {
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
        x += font->Width;
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
    memset(previous_heights, 0, sizeof(previous_heights));

    BSP_LCD_Clear(COLOR_BG);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    BSP_LCD_SetBackColor(COLOR_BG);
    BSP_LCD_SetFont(&Font12);

    float x_spacing = (float)x_length / num_x_indices;
    float y_spacing = (float)y_length / num_y_indices;

    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

    // Draw the main solid axes
    for (int i = origin_x; i <= origin_x + x_length; i += 1) {
        BSP_LCD_DrawPixel(origin_x, i, LCD_COLOR_CYAN);
    }
    for (int i = origin_y; i <= origin_y + y_length; i += 1) {
        BSP_LCD_DrawPixel(i, origin_y, LCD_COLOR_CYAN);
    }

    // X-Axis Grid and Labels (Decluttered to every 2nd index, scaled to 7.34 kHz)
    char binBuf[16];
    UI_DrawRotatedString(22, 15, "0");
    for (int i = 2; i <= num_x_indices; i += 2) {
        uint16_t current_y = origin_y + (uint16_t)(i * x_spacing);

        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
        BSP_LCD_DrawHLine(origin_x, current_y, 7);

        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
        BSP_LCD_DrawHLine(origin_x + 7, current_y, y_length - 7);

        // Safe float calculation for nano.specs
        float freq_val = ((float)i / (float)num_x_indices) * 7.34f;
        int f_int = (int)freq_val;
        int f_frac = (int)((freq_val - f_int) * 10.0f);

        if (i == num_x_indices) {
            snprintf(binBuf, sizeof(binBuf), "%d.%dkHz", f_int, f_frac);
            UI_DrawRotatedString(current_y - 20, 15, binBuf);
        } else {
            snprintf(binBuf, sizeof(binBuf), "%d.%d", f_int, f_frac);
            UI_DrawRotatedString(current_y - 8, 15, binBuf);
        }
    }

    // Y-axis Grid
    for (int i = 1; i <= num_y_indices; i++) {
        uint16_t current_x = origin_x + (uint16_t)(i * y_spacing);

        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
        BSP_LCD_DrawVLine(current_x, origin_y, 7);

        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
        BSP_LCD_DrawVLine(current_x, origin_y + 7, x_length - 7);
    }

    // Y-axis Labels (-70 to 0 dB)
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    UI_DrawRotatedString(3, 210, "dB");
    UI_DrawRotatedString(17, 205, "0");
    UI_DrawRotatedString(2, 167, "-14");
    UI_DrawRotatedString(2, 132, "-28");
    UI_DrawRotatedString(2,  100, "-42");
    UI_DrawRotatedString(2,  62, "-56");
    UI_DrawRotatedString(2,  30, "-70");

    // Main Title
    BSP_LCD_SetFont(&Font16);
    UI_DrawRotatedString(100, 230, "RAW ADC FFT");
}

void Diagnostics_UpdateRawFFT(float* fft_magnitudes) {
    for (int i = 1; i < 256; i++) {
        uint16_t x = 25 + i;

        float mag = fft_magnitudes[i];
        if (mag < 1.0f) mag = 1.0f;

        float db = 20.0f * log10f(mag / 1048576.0f);
        int magnitude = 174 + (int)(db * 2.5f);

        if (magnitude < 0) magnitude = 0;
        if (magnitude > 174) magnitude = 174;

        uint16_t new_height = (uint16_t)magnitude;
        uint16_t old_height = previous_heights[i];

        if (new_height > old_height) {
            uint16_t start_y = 26 + old_height;
            uint16_t length = (25 + new_height) - start_y + 1;

            BSP_LCD_SetTextColor(LCD_COLOR_ORANGE);
            BSP_LCD_DrawHLine(start_y, x, length);
        }
        else if (new_height < old_height) {
            uint32_t bg_color = COLOR_BG;
            // Adjusted to 64 to match the every-second-index grid
            if ((x - 25) % 64 == 0 && (x - 25) > 0) {
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
    memset(previous_y, 0, sizeof(previous_y));

    BSP_LCD_Clear(COLOR_BG);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    BSP_LCD_SetBackColor(COLOR_BG);

    float x_spacing = (float)x_length / num_x_indices;
    float y_spacing = (float)y_length / num_y_indices;

    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

    for (int i = origin_x; i <= origin_x + x_length; i += 1) {
        BSP_LCD_DrawPixel(origin_x, i, LCD_COLOR_CYAN);
    }
    for (int i = origin_y; i <= origin_y + y_length; i += 1) {
        BSP_LCD_DrawPixel(i, origin_y, LCD_COLOR_CYAN);
    }

    for (int i = 1; i <= num_x_indices; i++) {
        uint16_t current_y = origin_y + (uint16_t)(i * x_spacing);
        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
        BSP_LCD_DrawHLine(origin_x, current_y, 7);
        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
        BSP_LCD_DrawHLine(origin_x + 7, current_y, y_length - 7);
    }

    for (int i = 1; i <= num_y_indices; i++) {
        uint16_t current_x = origin_x + (uint16_t)(i * y_spacing);
        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
        BSP_LCD_DrawVLine(current_x, origin_y, 7);
        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
        BSP_LCD_DrawVLine(current_x, origin_y + 7, x_length - 7);
    }

    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    UI_DrawRotatedString(3, 215, "Volt");
    UI_DrawRotatedString(5, 203, "3.3");
    UI_DrawRotatedString(2, 156, "2.475");
    UI_DrawRotatedString(2, 115, "1.65");
    UI_DrawRotatedString(2,  69, "0.825");
    UI_DrawRotatedString(5,  26, "0.0");

    BSP_LCD_SetFont(&Font12);
    UI_DrawRotatedString(120, 15, "Time (Samples)");

    BSP_LCD_SetFont(&Font16);
    UI_DrawRotatedString(60, 230, "FILTERED TIME DOMAIN");
}

void Diagnostics_UpdateTimeGraph(float* time_buffer, uint16_t buffer_size) {
    for (int i = 0; i < 256; i++) {
        if (i >= buffer_size) break;

        uint16_t x = 26 + i;

        float voltage = (time_buffer[i] / 4096.0f) * 3.3f;
        if (voltage < 0.0f) voltage = 0.0f;
        if (voltage > 3.3f) voltage = 3.3f;

        int y_val = 26 + (int)((voltage / 3.3f) * 175.0f);
        uint16_t new_y = (uint16_t)y_val;

        if (new_y != previous_y[i]) {
            if (previous_y[i] != 0) {
                uint32_t bg_color = COLOR_BG;
                if ((previous_y[i]- 25) % 44 == 0) bg_color = LCD_COLOR_LIGHTGRAY;
                if ((x - 25) % 32 == 0) bg_color = LCD_COLOR_LIGHTGRAY;

                DrawPixelLandscape(x, previous_y[i], bg_color);
            }
            DrawPixelLandscape(x, new_y, LCD_COLOR_YELLOW);
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
    memset(previous_filtered_heights, 0, sizeof(previous_filtered_heights));
    memset(prev_current, 0, sizeof(prev_current));
    memset(peak_heights, 0, sizeof(peak_heights));
    memset(prev_peaks, 0, sizeof(prev_peaks));

    BSP_LCD_Clear(COLOR_BG);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    BSP_LCD_SetBackColor(COLOR_BG);
    BSP_LCD_SetFont(&Font12);

    float x_spacing = (float)x_length / num_x_indices;
    float y_spacing = (float)y_length / num_y_indices;

    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);

    // Draw the main solid axes
    for (int i = origin_x; i <= origin_x + x_length; i += 1) {
        BSP_LCD_DrawPixel(origin_x, i, LCD_COLOR_CYAN);
    }
    for (int i = origin_y; i <= origin_y + y_length; i += 1) {
        BSP_LCD_DrawPixel(i, origin_y, LCD_COLOR_CYAN);
    }

    // X-Axis Grid and Labels (Decluttered to every 2nd index, scaled to 7.34 kHz)
    char binBuf[16];
    UI_DrawRotatedString(22, 15, "0");
    for (int i = 2; i <= num_x_indices; i += 2) {
        uint16_t current_y = origin_y + (uint16_t)(i * x_spacing);

        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
        BSP_LCD_DrawHLine(origin_x, current_y, 7);

        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
        BSP_LCD_DrawHLine(origin_x + 7, current_y, y_length - 7);

        float freq_val = ((float)i / (float)num_x_indices) * 7.34f;
        int f_int = (int)freq_val;
        int f_frac = (int)((freq_val - f_int) * 10.0f);

        if (i == num_x_indices) {
            snprintf(binBuf, sizeof(binBuf), "%d.%dkHz", f_int, f_frac);
            UI_DrawRotatedString(current_y - 20, 15, binBuf);
        } else {
            snprintf(binBuf, sizeof(binBuf), "%d.%d", f_int, f_frac);
            UI_DrawRotatedString(current_y - 8, 15, binBuf);
        }
    }

    // Y-axis Grid
    for (int i = 1; i <= num_y_indices; i++) {
        uint16_t current_x = origin_x + (uint16_t)(i * y_spacing);

        BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
        BSP_LCD_DrawVLine(current_x, origin_y, 7);

        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTGRAY);
        BSP_LCD_DrawVLine(current_x, origin_y + 7, x_length - 7);
    }

    // Y-axis Labels (-70 to 0 dB)
    BSP_LCD_SetFont(&Font8);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    UI_DrawRotatedString(3, 210, "dB");
    UI_DrawRotatedString(17, 205, "0");
    UI_DrawRotatedString(2, 167, "-14");
    UI_DrawRotatedString(2, 132, "-28");
    UI_DrawRotatedString(2,  100, "-42");
    UI_DrawRotatedString(2,  62, "-56");
    UI_DrawRotatedString(2,  30, "-70");

    // --- ADDED: -3dB Threshold Red Line ---
    BSP_LCD_SetTextColor(LCD_COLOR_RED);
    BSP_LCD_DrawVLine(191, origin_y, x_length);

    // Main Title
    BSP_LCD_SetFont(&Font16);
    UI_DrawRotatedString(70, 230, "OUTPUT BUFFER FFT");
}

void Diagnostics_UpdateFilteredFFT(float* fft_magnitudes) {
    for (int i = 1; i < 256; i++) {
        uint16_t x = 25 + i;

        float mag = fft_magnitudes[i];
        if (mag < 1.0f) mag = 1.0f;

        float db = 20.0f * log10f(mag / 1048576.0f);
        int magnitude = 174 + (int)(db * 2.5f);

        if (magnitude < 0) magnitude = 0;
        if (magnitude > 174) magnitude = 174;

        uint16_t new_height = (uint16_t)magnitude;
        uint16_t old_height = previous_filtered_heights[i];

        if (new_height > old_height) {
            uint16_t start_y = 26 + old_height;
            uint16_t length = (25 + new_height) - start_y + 1;

            BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
            BSP_LCD_DrawHLine(start_y, x, length);
        }
        else if (new_height < old_height) {
            uint32_t bg_color = COLOR_BG;
            if ((x - 25) % 64 == 0 && (x - 25) > 0) {
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

            // Redraw the -3dB Red Line if erased
            if (191 >= (26 + new_height) && 191 <= (25 + old_height)) {
                DrawPixelLandscape(x, 191, LCD_COLOR_RED);
            }
        }
        previous_filtered_heights[i] = new_height;
    }
}

void Diagnostics_UpdateFilteredFFTPersist(float* fft_magnitudes) {
    for (int i = 1; i < 256; i++) {
        uint16_t x = 25 + i;

        float mag = fft_magnitudes[i];
        if (mag < 1.0f) mag = 1.0f;

        float db = 20.0f * log10f(mag / 1048576.0f);
        int magnitude = 174 + (int)(db * 2.5f);

        if (magnitude < 0) magnitude = 0;
        if (magnitude > 174) magnitude = 174;

        uint16_t curr_h = (uint16_t)magnitude;

        if (magnitude > peak_heights[i]) {
            peak_heights[i] = (float)magnitude;
        }

        uint16_t peak_h = (uint16_t)peak_heights[i];
        uint16_t old_curr_h = prev_current[i];
        uint16_t old_peak_h = prev_peaks[i];

        if (curr_h > old_curr_h) {
            BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
            BSP_LCD_DrawHLine(26 + old_curr_h, x, curr_h - old_curr_h);
        }
        else if (curr_h < old_curr_h) {
            BSP_LCD_SetTextColor(LCD_COLOR_MAGENTA);
            BSP_LCD_DrawHLine(26 + curr_h, x, old_curr_h - curr_h);
        }

        if (peak_h < old_peak_h) {
            uint32_t bg_color = COLOR_BG;
            if ((x - 25) % 64 == 0 && (x - 25) > 0) bg_color = LCD_COLOR_LIGHTGRAY;

            BSP_LCD_SetTextColor(bg_color);
            BSP_LCD_DrawHLine(26 + peak_h, x, old_peak_h - peak_h);

            for (int grid = 1; grid <= 5; grid++) {
                uint16_t tick_y = 25 + (grid * 35);
                if (tick_y >= (26 + peak_h) && tick_y <= (26 + old_peak_h)) {
                    DrawPixelLandscape(x, tick_y, LCD_COLOR_LIGHTGRAY);
                }
            }

            // Redraw the -3dB Red Line if erased
            if (191 >= (26 + peak_h) && 191 <= (26 + old_peak_h)) {
                DrawPixelLandscape(x, 191, LCD_COLOR_RED);
            }
        }

        prev_current[i] = curr_h;
        prev_peaks[i] = peak_h;
    }
}

/**
 * @brief Draws the static Filter Coefficients h[m] with dynamic scaling and axis labels
 */
void Diagnostics_DrawHMPlot(const float* coefficients, uint16_t order) {
    BSP_LCD_Clear(COLOR_BG);

    // 1. Auto-Scale Calculation
    float max_val = 0.0001f;
    for (uint16_t i = 0; i < order; i++) {
        if (fabsf(coefficients[i]) > max_val) {
            max_val = fabsf(coefficients[i]);
        }
    }
    float scale = 80.0f / max_val; // Scale so max value is exactly 80 pixels high

    // 2. Draw Axis Lines
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_DrawLine(120, 40, 120, 280); // X-axis (Center)
    BSP_LCD_DrawLine(20, 40, 220, 40);   // Y-axis (Left)

    // 3. Add Axis Headings (Titles)
    BSP_LCD_SetFont(&Font16);
    BSP_LCD_SetTextColor(COLOR_TEXT);
    UI_DrawRotatedString(80, 230, "Impulse Response h[m]");

    BSP_LCD_SetFont(&Font12);
    UI_DrawRotatedString(10, 140, "Amp");
    UI_DrawRotatedString(260, 110, "m");

    // 4. Add Axis Scale Values
    BSP_LCD_SetFont(&Font8);
    char labelBuf[16];

    // Float printing workaround for nano.specs without forcing linker flags
    int max_int = (int)max_val;
    int max_frac = (int)((max_val - max_int) * 100.0f);

    // Y-Axis Values (Amplitude)
    snprintf(labelBuf, sizeof(labelBuf), "%d.%02d", max_int, max_frac);
    UI_DrawRotatedString(5, 205, labelBuf); // Max Positive

    UI_DrawRotatedString(25, 123, "0");      // Center Zero

    snprintf(labelBuf, sizeof(labelBuf), "-%d.%02d", max_int, max_frac);
    UI_DrawRotatedString(5, 45, labelBuf);  // Max Negative

    // X-Axis Values (Sample Index)
    UI_DrawRotatedString(40, 105, "0");      // Start

    snprintf(labelBuf, sizeof(labelBuf), "%d", order / 2);
    UI_DrawRotatedString(150, 105, labelBuf);// Middle

    snprintf(labelBuf, sizeof(labelBuf), "%d", order - 1);
    UI_DrawRotatedString(265, 105, labelBuf);// End

    // 5. Draw the actual scaled sinc data
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    for (int i = 0; i < order - 1; i++) {
        int y1 = 120 + (int)(coefficients[i] * scale);
        int y2 = 120 + (int)(coefficients[i+1] * scale);

        int x1 = 40 + (i * 240 / order);
        int x2 = 40 + ((i + 1) * 240 / order);

        BSP_LCD_DrawLine(y1, x1, y2, x2);
    }
}
