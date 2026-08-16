/**
 * Teste de Força / Rastreio de Fraqueza no Braço (indício de AVC)
 * BitDogLab - RP2040
 *
 * Combina:
 *  - Botão A (GPIO5): inicia o teste
 *  - Joystick analógico: VRX = GPIO27 (ADC1) / VRY = GPIO26 (ADC0)
 *  - Matriz de LEDs NeoPixel 5x5 (GPIO7): mostra a seta da direção e o
 *    resultado (verde = sucesso, vermelho = falha) em animação tremulante.
 *  - Display OLED SSD1306 (I2C1, SDA=GPIO14 / SCL=GPIO15): mostra a
 *    direção sendo testada, instruções e a contagem regressiva dos 10s.
 *  - Buzzer (GPIO21): toca um jingle de sucesso ou um som de falha ao
 *    final de cada direção.
 *
 * Máquina de estados:
 *   IDLE -> aguarda o botão A
 *   SETA_DIREITA -> SETA_ESQUERDA -> SETA_CIMA -> SETA_BAIXO
 *   (cada etapa: mostra a seta, mede se o usuário consegue empurrar o
 *    joystick totalmente na direção pedida e SEGURAR por 10s sem a força cair)
 *   RESUMO -> mostra/pisca o resultado final e volta para IDLE
 *
 * OBS: os limiares (THRESHOLD_ALTO / THRESHOLD_BAIXO) e o mapeamento de qual
 * eixo/sentido corresponde a cada direção física do joystick dependem da
 * orientação do módulo na placa. Teste no hardware real e ajuste os
 * #define abaixo se "direita" no código não corresponder à direita física.
 *
 * OBS 2: a fonte usada pelo driver do OLED (ssd1306_font.h) só tem letras
 * MAIÚSCULAS e dígitos — qualquer outro caractere (acentos, ":", etc.)
 * aparece como espaço em branco. Por isso os textos abaixo evitam acentos
 * e pontuação.
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/i2c.h"

#include "ws2818b.pio.h"
#include "ssd1306.h"
#include "som.h"

// ---------------------- Pinos ----------------------
#define BOTAO_A       5
#define JOY_PIN_X     27   // VRX -> ADC1 -> eixo X (esquerda/direita)
#define JOY_PIN_Y     26   // VRY -> ADC0 -> eixo Y (cima/baixo)
#define MATRIZ_PIN    7
#define I2C_SDA_PIN   14
#define I2C_SCL_PIN   15

// ---------------------- Matriz de LEDs ----------------------
#define LED_COUNT 25

typedef struct {
    uint8_t G, R, B;
} pixel_t;

static pixel_t leds[LED_COUNT];
static PIO np_pio;
static uint np_sm;

// ---------------------- OLED ----------------------
static struct render_area frame_area;
static uint8_t oled_buf[SSD1306_BUF_LEN];

// ---------------------- Parâmetros do teste ----------------------
#define ADC_MAX            4095
#define THRESHOLD_ALTO     3700   // considera "totalmente empurrado" para um lado
#define THRESHOLD_BAIXO    400    // considera "totalmente empurrado" para o outro lado
#define TEMPO_SEGURAR_MS   10000  // 10 segundos segurando
#define TEMPO_LIMITE_MS    20000  // tempo máximo para tentar alcançar o limiar
#define DEBOUNCE_MS        200

typedef enum {
    DIR_DIREITA,
    DIR_ESQUERDA,
    DIR_CIMA,
    DIR_BAIXO
} direcao_t;

static const char *nome_direcao[4] = {"DIREITA", "ESQUERDA", "CIMA", "BAIXO"};

// ---------------------- Funções da matriz de LEDs ----------------------

// Converte (linha, coluna) físicas -> índice no buffer, respeitando a
// fiação em serpentina da matriz 5x5 (linha 0 = base/DIN, linha 4 = topo/DOUT).
static inline uint getIndex(uint linha, uint coluna) {
    if (linha % 2 == 0)
        return linha * 5 + (4 - coluna);
    else
        return linha * 5 + coluna;
}

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    np_sm = pio_claim_unused_sm(np_pio, false);
    if (np_sm < 0) {
        np_pio = pio1;
        np_sm = pio_claim_unused_sm(np_pio, true);
    }

    ws2818b_program_init(np_pio, np_sm, offset, pin, 800000.f);

    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear(void) {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

void npWrite(void) {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, np_sm, leds[i].G);
        pio_sm_put_blocking(np_pio, np_sm, leds[i].R);
        pio_sm_put_blocking(np_pio, np_sm, leds[i].B);
    }
    sleep_us(100);
}

// Desenha um padrão 5x5 (1 = aceso) na cor (r,g,b).
// padrao[0] é a linha de cima (topo físico), padrao[4] é a linha de baixo.
void npDesenhaPadrao(const uint8_t padrao[5][5], uint8_t r, uint8_t g, uint8_t b) {
    npClear();
    for (int linhaTopo = 0; linhaTopo < 5; ++linhaTopo) {
        uint linhaFisica = 4 - linhaTopo;
        for (int coluna = 0; coluna < 5; ++coluna) {
            if (padrao[linhaTopo][coluna]) {
                npSetLED(getIndex(linhaFisica, coluna), r, g, b);
            }
        }
    }
    npWrite();
}

// ---------------------- Padrões de seta (5x5) ----------------------
static const uint8_t SETA_DIREITA[5][5] = {
    {0,0,1,0,0},
    {0,0,0,1,0},
    {1,1,1,1,1},
    {0,0,0,1,0},
    {0,0,1,0,0},
};

static const uint8_t SETA_ESQUERDA[5][5] = {
    {0,0,1,0,0},
    {0,1,0,0,0},
    {1,1,1,1,1},
    {0,1,0,0,0},
    {0,0,1,0,0},
};

static const uint8_t SETA_CIMA[5][5] = {
    {0,0,1,0,0},
    {0,1,1,1,0},
    {1,0,1,0,1},
    {0,0,1,0,0},
    {0,0,1,0,0},
};

static const uint8_t SETA_BAIXO[5][5] = {
    {0,0,1,0,0},
    {0,0,1,0,0},
    {1,0,1,0,1},
    {0,1,1,1,0},
    {0,0,1,0,0},
};

static const uint8_t QUADRO_CHEIO[5][5] = {
    {1,1,1,1,1},
    {1,1,1,1,1},
    {1,1,1,1,1},
    {1,1,1,1,1},
    {1,1,1,1,1},
};

void mostraSeta(direcao_t dir) {
    switch (dir) {
        case DIR_DIREITA:  npDesenhaPadrao(SETA_DIREITA,  0, 0, 40); break; // azul fraco = aguardando
        case DIR_ESQUERDA: npDesenhaPadrao(SETA_ESQUERDA, 0, 0, 40); break;
        case DIR_CIMA:     npDesenhaPadrao(SETA_CIMA,     0, 0, 40); break;
        case DIR_BAIXO:    npDesenhaPadrao(SETA_BAIXO,    0, 0, 40); break;
    }
}

// Pisca a matriz inteira "tremulando" na cor indicada (feedback de sucesso/falha).
void piscaResultado(bool sucesso) {
    uint8_t r = sucesso ? 0  : 60;
    uint8_t g = sucesso ? 60 : 0;
    uint8_t b = 0;

    for (int i = 0; i < 6; ++i) {
        npDesenhaPadrao(QUADRO_CHEIO, r, g, b);
        sleep_ms(180);
        npClear();
        npWrite();
        sleep_ms(120);
    }
}

// ---------------------- OLED ----------------------
void oledInit(void) {
    i2c_init(i2c1, SSD1306_I2C_CLK * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    SSD1306_init();

    frame_area = (struct render_area){
        start_col : 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };
    calc_render_area_buflen(&frame_area);

    memset(oled_buf, 0, SSD1306_BUF_LEN);
    render(oled_buf, &frame_area);
}

// Mostra até 4 linhas de texto (uma por "página" de 8px). Passe NULL para
// deixar uma linha em branco.
void oledMostra4Linhas(const char *l1, const char *l2, const char *l3, const char *l4) {
    memset(oled_buf, 0, SSD1306_BUF_LEN);
    const char *linhas[4] = {l1, l2, l3, l4};
    int y = 0;
    for (int i = 0; i < 4; ++i) {
        if (linhas[i] != NULL) {
            WriteString(oled_buf, 0, y, (char *)linhas[i]);
        }
        y += 8;
    }
    render(oled_buf, &frame_area);
}

void oledMostraIdle(void) {
    oledMostra4Linhas("TESTE DE FORCA", "", "APERTE O", "BOTAO A");
}

// ---------------------- Botão ----------------------
bool botaoAPressionado(void) {
    return gpio_get(BOTAO_A) == 0; // pull-up: pressionado = nível baixo
}

void aguardaBotaoA(void) {
    // espera soltar, caso ainda esteja pressionado de uma leitura anterior
    while (botaoAPressionado()) sleep_ms(10);

    while (!botaoAPressionado()) {
        sleep_ms(10);
    }
    sleep_ms(DEBOUNCE_MS); // debounce simples
}

// ---------------------- Leitura do joystick ----------------------
uint16_t leEixoX(void) {
    adc_select_input(1); // GPIO27
    return adc_read();
}

uint16_t leEixoY(void) {
    adc_select_input(0); // GPIO26
    return adc_read();
}

// Retorna true se a leitura atual do eixo correspondente já ultrapassou o
// limiar de "totalmente empurrado" para a direção pedida.
bool passouLimiar(direcao_t dir) {
    switch (dir) {
        case DIR_DIREITA:  return leEixoX() >= THRESHOLD_ALTO;
        case DIR_ESQUERDA: return leEixoX() <= THRESHOLD_BAIXO;
        case DIR_CIMA:     return leEixoY() >= THRESHOLD_ALTO;
        case DIR_BAIXO:    return leEixoY() <= THRESHOLD_BAIXO;
    }
    return false;
}

/**
 * Executa o teste de uma direção:
 *  1. Mostra a seta e as instruções (aguardando o usuário alcançar o limiar).
 *  2. Assim que o limiar é alcançado, começa a contar os 10s, atualizando
 *     a contagem regressiva no OLED a cada segundo.
 *  3. Se a leitura cair abaixo do limiar antes de completar os 10s,
 *     o teste falha (a "força caiu antes do tempo").
 *  4. Se não alcançar o limiar dentro de TEMPO_LIMITE_MS, também falha.
 */
bool testaDirecao(direcao_t dir) {
    printf("\n== Teste: %s ==\n", nome_direcao[dir]);
    mostraSeta(dir);

    char linha_dir[17];
    snprintf(linha_dir, sizeof(linha_dir), "DIRECAO %s", nome_direcao[dir]);
    oledMostra4Linhas("TESTE DE FORCA", linha_dir, "EMPURRE O", "JOYSTICK");

    absolute_time_t inicio_tentativa = get_absolute_time();
    bool segurando = false;
    absolute_time_t inicio_segurando;
    int ultimo_segundo_exibido = -1;

    while (true) {
        bool no_limiar = passouLimiar(dir);
        int64_t decorrido_tentativa_ms = absolute_time_diff_us(inicio_tentativa, get_absolute_time()) / 1000;

        if (no_limiar) {
            if (!segurando) {
                segurando = true;
                inicio_segurando = get_absolute_time();
                ultimo_segundo_exibido = -1;
                printf("Limiar alcancado, segure por %d s...\n", TEMPO_SEGURAR_MS / 1000);
            }

            int64_t decorrido_segurando_ms = absolute_time_diff_us(inicio_segurando, get_absolute_time()) / 1000;
            int segundos_restantes = (TEMPO_SEGURAR_MS - (int)decorrido_segurando_ms + 999) / 1000;
            if (segundos_restantes < 0) segundos_restantes = 0;

            // Só re-escreve o OLED quando o segundo mostrado muda (evita I2C excessivo).
            if (segundos_restantes != ultimo_segundo_exibido) {
                ultimo_segundo_exibido = segundos_restantes;
                char linha_tempo[17];
                snprintf(linha_tempo, sizeof(linha_tempo), "FALTAM %2d S", segundos_restantes);
                oledMostra4Linhas("TESTE DE FORCA", linha_dir, "SEGURE FIRME", linha_tempo);
                printf("Segurando %s: faltam %d s\n", nome_direcao[dir], segundos_restantes);
            }

            if (decorrido_segurando_ms >= TEMPO_SEGURAR_MS) {
                printf("Sucesso: segurou %s por %d ms\n", nome_direcao[dir], (int)decorrido_segurando_ms);
                oledMostra4Linhas("TESTE DE FORCA", linha_dir, "RESULTADO", "SUCESSO");
                return true;
            }
        } else {
            if (segurando) {
                // a força caiu antes de completar os 10s -> falha
                printf("Falha: forca caiu antes de completar o tempo em %s\n", nome_direcao[dir]);
                oledMostra4Linhas("TESTE DE FORCA", linha_dir, "RESULTADO", "FALHOU");
                return false;
            }
        }

        if (decorrido_tentativa_ms >= TEMPO_LIMITE_MS) {
            printf("Falha: tempo esgotado tentando alcancar %s\n", nome_direcao[dir]);
            oledMostra4Linhas("TESTE DE FORCA", linha_dir, "RESULTADO", "FALHOU TEMPO");
            return false;
        }

        sleep_ms(20);
    }
}

int main() {
    stdio_init_all();

    // Botão A
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    // ADC do joystick
    adc_init();
    adc_gpio_init(JOY_PIN_X);
    adc_gpio_init(JOY_PIN_Y);

    // Matriz de LEDs
    npInit(MATRIZ_PIN);
    npClear();
    npWrite();

    // Display OLED
    oledInit();
    oledMostraIdle();

    // Buzzer (sons de sucesso/falha)
    setup_som();

    const direcao_t ordem[4] = {DIR_DIREITA, DIR_ESQUERDA, DIR_CIMA, DIR_BAIXO};

    while (true) {
        printf("\nAguardando botao A para iniciar o teste...\n");
        aguardaBotaoA();

        bool resultado[4];
        bool todos_ok = true;

        for (int i = 0; i < 4; ++i) {
            resultado[i] = testaDirecao(ordem[i]);

            // Feedback simultâneo: LEDs piscando + som de sucesso ou falha.
            if (resultado[i]) {
                toca_som_sucesso();
            } else {
                toca_som_falha();
                todos_ok = false;
            }
            piscaResultado(resultado[i]);
        }

        printf("\n===== Resumo do teste =====\n");
        for (int i = 0; i < 4; ++i) {
            printf("%s: %s\n", nome_direcao[i], resultado[i] ? "OK" : "FALHOU");
        }
        printf("Resultado geral: %s\n", todos_ok ? "SEM INDICIOS DE FRAQUEZA" : "POSSIVEL FRAQUEZA DETECTADA");

        // Resumo no OLED: uma linha por direção testada.
        char linhas_resumo[4][17];
        for (int i = 0; i < 4; ++i) {
            snprintf(linhas_resumo[i], sizeof(linhas_resumo[i]), "%s %s",
                      nome_direcao[i], resultado[i] ? "OK" : "FALHOU");
        }
        oledMostra4Linhas(linhas_resumo[0], linhas_resumo[1], linhas_resumo[2], linhas_resumo[3]);
        sleep_ms(3000);

        oledMostra4Linhas("RESULTADO GERAL", "",
                           todos_ok ? "SEM INDICIOS" : "POSSIVEL",
                           todos_ok ? "DE FRAQUEZA" : "FRAQUEZA");

        // Feedback final mais longo, reforçando o resultado geral
        for (int i = 0; i < 3; ++i) {
            piscaResultado(todos_ok);
        }
        if (todos_ok) {
            toca_som_sucesso();
        } else {
            toca_som_falha();
        }

        sleep_ms(2000);
        npClear();
        npWrite();
        oledMostraIdle();
    }
}
