/*
 * fir.h
 *
 *  Created on: 01 Apr 2026
 *      Author: edward
 */

#ifndef INC_FIR_H_
#define INC_FIR_H_

#include <stdint.h>

// --- FIR Filter Specifications ---
#define FIR_FS       176470.59f  // Sampling rate
#define FIR_FP       1500.0f   // Passband edge
#define FIR_F_STOP   3000.0f   // Stopband edge
#define FIR_FC       ((FIR_FP + FIR_F_STOP) / 2.0f) // Cutoff frequency

// Calculated Order: 3.3 * (48000 / 1500) = 105.6 -> round to odd 107
#define FIR_ORDER    389

// --- Public Function Prototypes ---
void FIR_Init(void);
void FIR_ProcessBlock(float* input_block, float* output_block, uint32_t block_size);
extern float fir_coeffs[FIR_ORDER];

#endif /* INC_FIR_H_ */
