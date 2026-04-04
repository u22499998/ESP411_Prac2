/*
 * fir.h
 *
 * Created on: 01 Apr 2026
 * Author: edward
 */

#ifndef INC_FIR_H_
#define INC_FIR_H_

#include <stdint.h>

// --- FIR Filter Specifications ---
#define FIR_FS       176470.59f  // Sampling rate
#define FIR_FP       1500.0f     // Passband edge
#define FIR_F_STOP   3000.0f     // Stopband edge
#define FIR_FC       ((FIR_FP + FIR_F_STOP) / 2.0f) // Cutoff frequency

// Calculated Order based on transition width and window type
#define FIR_ORDER    389

// --- Public Function Prototypes ---

/**
 * @brief Initializes filter coefficients and clear state buffers.
 */
void FIR_Init(void);

/**
 * @brief Processes a block of samples through the FIR filter.
 * @note This function uses uint16_t for the length to match the implementation in fir.c.
 */
void FIR_ProcessBlock(float* input, float* output, uint16_t length);

/**
 * @brief Externally accessible coefficient array for plotting in diagnostics.c
 */
extern float fir_coeffs[FIR_ORDER];

#endif /* INC_FIR_H_ */
