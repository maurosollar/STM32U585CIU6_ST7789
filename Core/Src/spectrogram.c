#include "spectrogram.h"
#include <stdio.h>
#include <math.h>

/* Guarda a última magnitude de cada barra para apagar só o que mudou */
static uint16_t prev_bar_h[SPEC_NUM_BARS] = {0};

/* Retorna a cor RGB565 conforme intensidade normalizada 0..255 */
static uint16_t bar_color(uint8_t intensity)
{
    if (intensity > 191) return RED;      // > 75%
    if (intensity > 127) return YELLOW;   // > 50%
    if (intensity >  63) return GREEN;    // > 25%
    return BLUE;
}

/* Desenha o cabeçalho fixo (fundo + título). Chame uma vez. */
void Spectrogram_Init(void)
{
    ST7789_Fill_Color(BLACK);

    /* Linha divisória */
    ST7789_DrawLine(0, SPEC_HDR_HEIGHT - 1,
                    ST7789_WIDTH - 1, SPEC_HDR_HEIGHT - 1, GRAY);

    /* Título fixo */
    ST7789_WriteString(25, 5, "Espectrograma",
                       Font_7x10, WHITE, BLACK);
    ST7789_WriteString(220, 25, "FFT",
                       Font_11x18, WHITE, BLACK);
}

/*
 * Atualiza o espectrograma com um novo frame de magnitudes.
 * magnitudes[] : array de num_bins valores 0..65535
 * num_bins     : quantos bins da FFT (usa os primeiros SPEC_NUM_BARS)
 */
void Spectrogram_Update(uint16_t *magnitudes, uint16_t num_bins)
{
    /* --- 1. Calcula estatísticas do frame --- */
    uint16_t peak_idx  = 0;
    uint32_t max_mag   = 0;
    uint16_t bins_used = (num_bins < SPEC_NUM_BARS) ? num_bins : SPEC_NUM_BARS;

    for (uint16_t i = 0; i < bins_used; i++) {
        if (magnitudes[i] > max_mag) {
            max_mag   = magnitudes[i];
            peak_idx  = i;
        }
    }

    uint32_t freq_peak = ((uint32_t)peak_idx * SPEC_SAMPLE_RATE) / SPEC_NUM_BARS;
    uint8_t  amp_pct   = (uint8_t)((max_mag * 100UL) / 65535UL);

    /* --- 2. Atualiza cabeçalho (só as áreas de valor, não o título) --- */
    char buf[24];

    /* Frequência dominante */
    snprintf(buf, sizeof(buf), "Freq: %4lu Hz", freq_peak);
    ST7789_WriteString(25, 20, buf, Font_7x10, CYAN, BLACK);

    /* Amplitude */
    snprintf(buf, sizeof(buf), "Amp:  %3d%%  ", amp_pct);
    uint16_t amp_color = (amp_pct > 75) ? RED :
                         (amp_pct > 50) ? YELLOW : GREEN;
    ST7789_WriteString(25, 36, buf, Font_7x10, amp_color, BLACK);

    /* --- 3. Atualiza as barras (apaga só a diferença) --- */
    for (uint16_t i = 0; i < bins_used; i++) {

        /* Normaliza magnitude para altura em pixels */
        uint16_t bar_h = (uint16_t)(
            ((uint32_t)magnitudes[i] * SPEC_BAR_AREA_H) / 65535UL
        );
        if (bar_h > SPEC_BAR_AREA_H) bar_h = SPEC_BAR_AREA_H;
        if (bar_h < 1)               bar_h = 1;

        uint16_t x   = i * (SPEC_BAR_W + SPEC_BAR_GAP);
        uint16_t col = bar_color((uint8_t)((magnitudes[i] >> 8) & 0xFF));

        if (bar_h == prev_bar_h[i]) continue; /* nada mudou, pula */

        if (bar_h < prev_bar_h[i]) {
            /* Barra encolheu: apaga parte do topo */
            uint16_t erase_top = SPEC_HDR_HEIGHT + (SPEC_BAR_AREA_H - prev_bar_h[i]);
            uint16_t erase_bot = SPEC_HDR_HEIGHT + (SPEC_BAR_AREA_H - bar_h) - 1;
            ST7789_Fill(x, erase_top, x + SPEC_BAR_W - 1, erase_bot, BLACK);
        } else {
            /* Barra cresceu: desenha a parte nova no topo */
            uint16_t draw_top = SPEC_HDR_HEIGHT + (SPEC_BAR_AREA_H - bar_h);
            uint16_t draw_bot = SPEC_HDR_HEIGHT + (SPEC_BAR_AREA_H - prev_bar_h[i]) - 1;
            ST7789_Fill(x, draw_top, x + SPEC_BAR_W - 1, draw_bot, col);
        }

        prev_bar_h[i] = bar_h;
    }
}

/* Gera dados simulados e chama Spectrogram_Update em loop.
 * Útil para testar sem ADC/FFT. Chame no seu while(1). */
void Spectrogram_SimulatedTest(void)
{
    static float phase = 0.0f;
    uint16_t magnitudes[SPEC_NUM_BARS];

    phase += 0.08f;
    if (phase > 2.0f * 3.14159f) phase -= 2.0f * 3.14159f;

    /* Simula um pico varrendo o espectro */
    float center = (sinf(phase) * 0.5f + 0.5f); /* 0..1 */

    for (uint16_t i = 0; i < SPEC_NUM_BARS; i++) {
        float f    = (float)i / SPEC_NUM_BARS;
        float dist = f - center;
        float val  = expf(-(dist * dist) * 18.0f); /* gaussiana */
        val += 0.05f * ((float)(HAL_GetTick() ^ (i * 31)) / 65535.0f); /* ruído */
        if (val > 1.0f) val = 1.0f;
        magnitudes[i] = (uint16_t)(val * 65535.0f);
    }

    Spectrogram_Update(magnitudes, SPEC_NUM_BARS);
    HAL_Delay(30); /* ~30 fps */
}
