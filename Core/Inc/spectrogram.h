#ifndef __SPECTROGRAM_H
#define __SPECTROGRAM_H

#include "st7789.h"
#include <stdint.h>

#define SPEC_NUM_BARS     22
#define SPEC_HDR_HEIGHT   50
#define SPEC_BAR_AREA_H   (ST7789_HEIGHT - SPEC_HDR_HEIGHT)
#define SPEC_BAR_W        10
#define SPEC_BAR_GAP      2
#define SPEC_SAMPLE_RATE  8000   // Hz (ajuste conforme seu ADC)

void Spectrogram_Init(void);
void Spectrogram_Update(uint16_t *magnitudes, uint16_t num_bins);
void Spectrogram_SimulatedTest(void);

#endif
