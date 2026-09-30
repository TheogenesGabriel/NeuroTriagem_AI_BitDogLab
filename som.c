#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "notes.h"
#include "som.h"

// Pino do buzzer (mesmo usado como BUZZER_A no play_audio.c original da BitDogLab)
#define BUZZER_PIN 21
#define DIVISOR_CLK_PWM 16.0f

// Toca uma nota (wrap definido em notes.h) por 'duracao_ms' com 50% de duty
// cycle e depois silencia o buzzer.
static void tocaNota(uint16_t wrap, uint32_t duracao_ms) {
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice, wrap);
    pwm_set_gpio_level(BUZZER_PIN, wrap / 2);
    pwm_set_enabled(slice, true);
    sleep_ms(duracao_ms);
    pwm_set_enabled(slice, false);
    sleep_ms(20); // pequeno silêncio entre notas
}

void setup_som(void) {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(slice, DIVISOR_CLK_PWM);
}

// Jingle curto e ascendente (Dó-Mi-Sol-Dó agudo) = sensação de "sucesso".
void toca_som_sucesso(void) {
    tocaNota(NOTE_C4, 100);
    tocaNota(NOTE_E4, 100);
    tocaNota(NOTE_G4, 100);
    tocaNota(NOTE_C5, 180);
}

// Som curto e grave/descendente = sensação de "erro".
void toca_som_falha(void) {
    tocaNota(NOTE_C4, 150);
    tocaNota(NOTE_AS3, 300);
}
