/*
 * fir.c
 *
 *  Created on: 01 Apr 2026
 *      Author: edwar
 */

#include "fir.h"
#include <math.h>

#define M_PI 3.14159265358979323846f
#define N_MINUS_1 (FIR_ORDER - 1)

// --- Private State Variables ---
// Using static keeps these hidden from the rest of the program
float fir_coeffs[FIR_ORDER];
static float sample_history[FIR_ORDER];

/**
 * @brief Calculates the Hamming windowed sinc function coefficients
 * and clears the state buffer. Run once at startup.
 */
void FIR_Init(void) {
    float wc = 2.0f * M_PI * (FIR_FC / FIR_FS);

    for (int i = 0; i < FIR_ORDER; i++) {
        int n = i - (N_MINUS_1 / 2);

        // 1. Calculate Ideal Impulse Response
        float h_d;
        if (n == 0) {
            h_d = wc / M_PI;
        } else {
            h_d = sinf(wc * n) / (M_PI * n);
        }

        // 2. Apply Hamming Window
        float w = 0.54f + 0.46f * cosf((2.0f * M_PI * n) / N_MINUS_1);

        // 3. Store in LUT
        fir_coeffs[i] = h_d * w;
    }

    // Clear the history buffer
    for (int i = 0; i < FIR_ORDER; i++) {
        sample_history[i] = 0.0f;
    }
}

/**
 * @brief Performs the discrete convolution on an entire block of audio.
 */
void FIR_ProcessBlock(float* input_block, float* output_block, uint32_t block_size) {
    for (uint32_t i = 0; i < block_size; i++) {

        // 1. Shift history down
        for (int j = FIR_ORDER - 1; j > 0; j--) {
            sample_history[j] = sample_history[j - 1];
        }

        // 2. Insert newest sample
        sample_history[0] = input_block[i];

        // 3. Convolve
        float y = 0.0f;
        for (int k = 0; k < FIR_ORDER; k++) {
            y += fir_coeffs[k] * sample_history[k];
        }

        // 4. Store result
        output_block[i] = y;
    }
}

