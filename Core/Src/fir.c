/*
 * fir.c
 *
 * Created on: 01 Apr 2026
 * Author: edward
 */

#include "fir.h"
#include <math.h>

// Handle potential redefinition of M_PI from math.h
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define N_MINUS_1 (FIR_ORDER - 1)

// --- Private State Variables ---
float fir_coeffs[FIR_ORDER];
static float state_buffer[FIR_ORDER] = {0};
static uint16_t state_idx = 0;

/**
 * @brief Calculates Blackman-windowed sinc coefficients.
 * Use Blackman to ensure >50dB stopband attenuation.
 */
void FIR_Init(void) {
    // Normalized cutoff frequency (radians/sample)
    float wc = 2.0f * M_PI * (FIR_FC / FIR_FS);

    for (int i = 0; i < FIR_ORDER; i++) {
        // Shift index to center the sinc at (N-1)/2
        float n = (float)i - (N_MINUS_1 / 2.0f);

        // 1. Calculate Ideal Impulse Response (Sinc)
        float h_d;
        if (fabsf(n) < 1e-6f) {
            h_d = wc / M_PI; // L'Hopital's rule for the center peak
        } else {
            h_d = sinf(wc * n) / (M_PI * n);
        }

        // 2. Apply Blackman Window
        // w(i) = 0.42 - 0.5*cos(2*pi*i/M) + 0.08*cos(4*pi*i/M)
        float w = 0.42f - 0.5f * cosf((2.0f * M_PI * (float)i) / N_MINUS_1) +
                  0.08f * cosf((4.0f * M_PI * (float)i) / N_MINUS_1);

        // 3. Store the windowed coefficient
        fir_coeffs[i] = h_d * w;
    }

    // Clear the state/history buffer
    for (int i = 0; i < FIR_ORDER; i++) {
        state_buffer[i] = 0.0f;
    }
    state_idx = 0;
}

/**
 * @brief Real-time convolution using a circular buffer.
 * Expects input to be already zero-centered (DC offset removed in main.c).
 */
void FIR_ProcessBlock(float* input, float* output, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        // 1. Store the new incoming sample into the circular buffer
        state_buffer[state_idx] = input[i];

        // 2. Perform Multiply-Accumulate (MAC)
        float acc = 0.0f;
        uint16_t coeff_ptr = 0;

        // Multiply coefficients by the history buffer, starting from current index
        for (int j = state_idx; j >= 0; j--) {
            acc += fir_coeffs[coeff_ptr++] * state_buffer[j];
        }
        // Wrap around to the end of the buffer
        for (int j = FIR_ORDER - 1; j > state_idx; j--) {
            acc += fir_coeffs[coeff_ptr++] * state_buffer[j];
        }

        // 3. Advance the circular index
        state_idx++;
        if (state_idx >= FIR_ORDER) {
            state_idx = 0;
        }

        // 4. Output the filtered sample
        output[i] = acc;
    }
}
