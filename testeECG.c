#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/stdio_usb.h"
#include "pico/time.h"
#include "hardware/adc.h"

// =====================================================================
//  ECG com AD8232 + Raspberry Pi Pico W
//
//  Ligações:
//    AD8232 OUTPUT -> GPIO28 (ADC2)
//    AD8232 LO+    -> GPIO9  (leads-off +)
//    AD8232 LO-    -> GPIO8  (leads-off -)
//
//  Saída USB serial (CSV), uma linha por amostra:
//    t_us,adc,adc_filtrado,leads_off
//    t_us          : tempo desde o início, em microssegundos
//    adc           : valor bruto (0..4095)
//    adc_filtrado  : valor após o filtro passa-baixas
//    leads_off     : 1 = algum eletrodo desconectado, 0 = ok
// =====================================================================

// ---------- Pinos ----------
#define ECG_ADC_GPIO         28    // AD8232 OUTPUT (ADC2)
#define ECG_ADC_CHANNEL      2
#define LEAD_OFF_PLUS_GPIO   9     // LO+
#define LEAD_OFF_MINUS_GPIO  8     // LO-
#define SDN 16

// ---------- Amostragem ----------
#define SAMPLE_RATE_HZ       200
#define SAMPLE_PERIOD_US     (1000000 / SAMPLE_RATE_HZ)

// ---------- Filtro passa-baixas (1ª ordem, IIR) ----------
// y[n] = y[n-1] + alpha * (x[n] - y[n-1])
// alpha = 1 - exp(-2*pi*fc/fs). Para fc = 40 Hz e fs = 200 Hz: alpha ≈ 0,71
#define LP_ALPHA             0.71f

int main(void)
{
    // stdio_init_all();

    // // Leads-off: entradas com pull-down (nível alto = eletrodo solto)
    // gpio_init(LEAD_OFF_PLUS_GPIO);
    // gpio_set_dir(LEAD_OFF_PLUS_GPIO, GPIO_IN);
    // gpio_pull_down(LEAD_OFF_PLUS_GPIO);

    // gpio_init(SDN);
    // gpio_set_dir(SDN, GPIO_OUT);
    // gpio_put(SDN, 1);

    // gpio_init(LEAD_OFF_MINUS_GPIO);
    // gpio_set_dir(LEAD_OFF_MINUS_GPIO, GPIO_IN);
    // gpio_pull_down(LEAD_OFF_MINUS_GPIO);

    // // ADC no GPIO 28 (canal 2)
    // adc_init();
    // adc_gpio_init(ECG_ADC_GPIO);
    // adc_select_input(ECG_ADC_CHANNEL);

    // // Espera o computador abrir a porta USB (no máximo 5 s),
    // // para não perder o cabeçalho e as primeiras amostras
    // absolute_time_t limite = make_timeout_time_ms(5000);
    // while (!stdio_usb_connected() && !time_reached(limite)) {
    //     sleep_ms(100);
    // }
    // sleep_ms(500);

    // printf("t_us,adc,adc_filtrado,leads_off\n");

    // uint64_t t0 = to_us_since_boot(get_absolute_time());
    // absolute_time_t proximo = get_absolute_time();

    // float filtrado = 0.0f;
    // bool  filtro_iniciado = false;

    // while (true) {
    //     // Agenda a próxima amostra sem acumular deriva
    //     proximo = delayed_by_us(proximo, SAMPLE_PERIOD_US);
    //     sleep_until(proximo);

    //     // Se atrasou demais (ex.: USB lento), ressincroniza
    //     if (absolute_time_diff_us(proximo, get_absolute_time()) > 5 * SAMPLE_PERIOD_US) {
    //         proximo = get_absolute_time();
    //     }

    //     // Amostra e timestamp
    //     uint64_t agora = to_us_since_boot(get_absolute_time());
    //     uint16_t adc = adc_read();
    //     bool leads_off = gpio_get(LEAD_OFF_PLUS_GPIO) || gpio_get(LEAD_OFF_MINUS_GPIO);

    //     // Filtro passa-baixas
    //     if (!filtro_iniciado) {
    //         filtrado = (float)adc;
    //         filtro_iniciado = true;
    //     }
    //     filtrado += LP_ALPHA * ((float)adc - filtrado);

    //     printf("%llu,%u,%d,%d\n",
    //            (unsigned long long)(agora - t0),
    //            adc,
    //            (int)(filtrado + 0.5f),
    //            leads_off ? 1 : 0);
    // }

    return 0;
}
