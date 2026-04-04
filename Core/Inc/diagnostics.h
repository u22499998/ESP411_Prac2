/*
 * diagnostics.h
 *
 *  Created on: 02 Apr 2026
 *      Author: edwar
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>
#include <stdbool.h>

// --- LOW LEVEL DRAWING ---
// Maps Cartesian coordinates (Origin Bottom-Left) to the Portrait LCD
void DrawPixelLandscape(uint16_t cart_X, uint16_t cart_Y, uint32_t color);

// Custom Font Engine for landscape strings
void UI_DrawRotatedString(uint16_t x, uint16_t y, const char* str);

// --- GRAPHING FUNCTIONS ---
// Draws the static axes and labels once for live data
void Diagnostics_InitLandscapeGraph(const char* title);

// Fast-updates the live data without clearing the screen
void Diagnostics_UpdateGraph(float* oldData, float* newData, uint16_t length);

// Initializes the static background for the FFT
void Diagnostics_InitRawFFTGraph(uint16_t origin_x, uint16_t origin_y,
        uint16_t x_length, uint16_t y_length,
        uint8_t num_x_indices, uint8_t num_y_indices);

// Fast-updates only the changed pixels on the graph
void Diagnostics_UpdateRawFFT(float* fft_magnitudes);
void Diagnostics_ResetFFTMemory(void);

// plot 2. h[m] functions
void Diagnostics_DrawHMPlot(float* coefficients, uint16_t order);
void Diagnostics_UpdateTimeDomain(float* input, float* output);
void Diagnostics_InitFilteredFFTGraph(void);

#endif // DIAGNOSTICS_H
