#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#include "testeECG.h"

// Fração máxima de amostras com leads-off tolerada para considerar a coleta válida.
#define LEAD_OFF_TOLERANCIA_PCT 10

void initECG(int LEAD_OFF_PLUS_GPIO, int LEAD_OFF_MINUS_GPIO,
             int ECG_ADC_GPIO, int SDN) {
    // Leads-off: entradas com pull-down (nível alto = eletrodo solto)
    gpio_init(LEAD_OFF_PLUS_GPIO);
    gpio_set_dir(LEAD_OFF_PLUS_GPIO, GPIO_IN);
    gpio_pull_down(LEAD_OFF_PLUS_GPIO);

    gpio_init(LEAD_OFF_MINUS_GPIO);
    gpio_set_dir(LEAD_OFF_MINUS_GPIO, GPIO_IN);
    gpio_pull_down(LEAD_OFF_MINUS_GPIO);

    // SDN em nível alto mantém o AD8232 ligado
    gpio_init(SDN);
    gpio_set_dir(SDN, GPIO_OUT);
    gpio_put(SDN, 1);

    // Pino analógico do ECG (GPIO28 -> canal 2)
    adc_gpio_init(ECG_ADC_GPIO);
}

void coletaECG(int ECG_ADC_CHANNEL, int LEAD_OFF_PLUS_GPIO,
               int LEAD_OFF_MINUS_GPIO, uint32_t duracao_ms,
               ecg_resultado_t *res) {
    ecg_resultado_t r = {0, 0, 4095, 0, false};

    printf("t_us,adc,adc_filtrado,leads_off\n");

    absolute_time_t inicio = get_absolute_time();
    absolute_time_t proximo = inicio;
    uint64_t t0 = to_us_since_boot(inicio);

    float filtrado = 0.0f;
    bool filtro_iniciado = false;

    while (absolute_time_diff_us(inicio, get_absolute_time()) < (int64_t)duracao_ms * 1000) {
        // Agenda a próxima amostra sem acumular deriva
        proximo = delayed_by_us(proximo, SAMPLE_PERIOD_US);
        sleep_until(proximo);

        // Se atrasou demais (ex.: USB lento), ressincroniza
        if (absolute_time_diff_us(proximo, get_absolute_time()) > 5 * SAMPLE_PERIOD_US) {
            proximo = get_absolute_time();
        }

        uint64_t agora = to_us_since_boot(get_absolute_time());
        adc_select_input(ECG_ADC_CHANNEL); // o joystick troca o canal entre leituras
        uint16_t adc = adc_read();
        bool leads_off = gpio_get(LEAD_OFF_PLUS_GPIO) || gpio_get(LEAD_OFF_MINUS_GPIO);

        // Filtro passa-baixas
        if (!filtro_iniciado) {
            filtrado = (float)adc;
            filtro_iniciado = true;
        }
        filtrado += LP_ALPHA * ((float)adc - filtrado);
        uint16_t filt = (uint16_t)(filtrado + 0.5f);

        printf("%llu,%u,%u,%d\n",
               (unsigned long long)(agora - t0), adc, filt, leads_off ? 1 : 0);

        r.amostras++;
        if (leads_off) r.amostras_lead_off++;
        if (filt < r.adc_min) r.adc_min = filt;
        if (filt > r.adc_max) r.adc_max = filt;
    }

    r.eletrodos_ok = r.amostras > 0 &&
                     (r.amostras_lead_off * 100 / r.amostras) <= LEAD_OFF_TOLERANCIA_PCT;

    if (res) *res = r;
}
