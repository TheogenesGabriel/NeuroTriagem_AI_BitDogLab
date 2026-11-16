#ifndef TESTE_ECG_H
#define TESTE_ECG_H

#include <stdint.h>
#include <stdbool.h>

// =====================================================================
//  ECG com AD8232 (módulo integrado ao teste de AVC)
//
//  Ligações (pinos definidos em teste_avc.c e passados para initECG):
//    AD8232 OUTPUT -> GPIO28 (ADC2)
//    AD8232 LO+    -> GPIO9  (leads-off +)
//    AD8232 LO-    -> GPIO8  (leads-off -)
//    AD8232 SDN    -> GPIO16 (nível alto = módulo ligado)
//
//  Saída USB serial (CSV), uma linha por amostra:
//    t_us,adc,adc_filtrado,leads_off
// =====================================================================

// ---------- Amostragem ----------
#define SAMPLE_RATE_HZ       200
#define SAMPLE_PERIOD_US     (1000000 / SAMPLE_RATE_HZ)

// ---------- Filtro passa-baixas (1ª ordem, IIR) ----------
// y[n] = y[n-1] + alpha * (x[n] - y[n-1])
// alpha = 1 - exp(-2*pi*fc/fs). Para fc = 40 Hz e fs = 200 Hz: alpha ≈ 0,71
#define LP_ALPHA             0.71f

// Resumo de uma coleta de ECG.
typedef struct {
    uint32_t amostras;        // total de amostras coletadas
    uint32_t amostras_lead_off; // amostras com eletrodo solto
    uint16_t adc_min;         // menor valor filtrado
    uint16_t adc_max;         // maior valor filtrado
    bool     eletrodos_ok;    // true se (quase) toda a coleta teve contato
} ecg_resultado_t;

// Configura GPIOs (LO+, LO-, SDN) e o pino analógico do AD8232.
// Pressupõe stdio_init_all() e adc_init() já chamados pelo programa principal.
void initECG(int LEAD_OFF_PLUS_GPIO, int LEAD_OFF_MINUS_GPIO,
             int ECG_ADC_GPIO, int SDN);

// Coleta ECG por duracao_ms, enviando o CSV pela serial USB.
// Seleciona o canal ADC por amostra (o joystick usa outros canais).
// Se res != NULL, preenche o resumo da coleta.
void coletaECG(int ECG_ADC_CHANNEL, int LEAD_OFF_PLUS_GPIO,
               int LEAD_OFF_MINUS_GPIO, uint32_t duracao_ms,
               ecg_resultado_t *res);

#endif // TESTE_ECG_H
