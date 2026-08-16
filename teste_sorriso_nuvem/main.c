#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"

#include "ssd1306.h"
#include "icons.h"
#include "config.h"

#define RED_LED_PIN   13
#define GREEN_LED_PIN 11
#define BLUE_LED_PIN  12

// Botao A (BitDogLab default wiring). Ativo em nivel baixo: pressionado = 0.
#define BUTTON_A_PIN  5

#define HAPPY_DISPLAY_MS 5000  // how long the "feliz" message stays up

static ssd1306_t display;
static absolute_time_t happy_until;
static bool showing_happy = false;

static void leds_init(void) {
    gpio_init(RED_LED_PIN);
    gpio_init(GREEN_LED_PIN);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    gpio_put(RED_LED_PIN, 0);
    gpio_put(GREEN_LED_PIN, 0);
    gpio_put(BLUE_LED_PIN, 0);
}

static void button_a_init(void) {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
}

static bool button_a_pressed(void) {
    return !gpio_get(BUTTON_A_PIN); // ativo em nivel baixo
}

static void show_start_screen(void) {
    ssd1306_clear(&display);
    ssd1306_draw_rect(&display, 0, 0, OLED_WIDTH, OLED_HEIGHT); // borda
    ssd1306_draw_string(&display, 18, 5, "NEUROTRIAGEM.AI", 1);
    // Centraliza o icone de play (24x24) no meio do display (128x64).
    int icon_x = (OLED_WIDTH - PLAY_ICON_WIDTH) / 2;
    int icon_y = (OLED_HEIGHT - PLAY_ICON_HEIGHT) / 2;
    ssd1306_draw_bitmap(&display, icon_x, icon_y, PLAY_ICON_BITMAP,PLAY_ICON_WIDTH, PLAY_ICON_HEIGHT);
    ssd1306_draw_string(&display, 37, 54, "INICIAR A", 1); // mais abaixo, centralizado
    ssd1306_show(&display);
}

static void wait_for_button_a(void) {
    show_start_screen();
    // Espera o botao ser pressionado (com debounce simples).
    while (true) {
        if (button_a_pressed()) {
            sleep_ms(30); // debounce
            if (button_a_pressed()) break;
        }
        tight_loop_contents();
    }
    // Espera soltar antes de seguir, para nao "vazar" o clique para outra logica.
    while (button_a_pressed()) {
        tight_loop_contents();
    }
}

static void show_idle_screen(void) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, "AGUARDANDO...", 1);
    ssd1306_draw_string(&display, 0, 40, "SORRIA :)", 1);
    ssd1306_show(&display);
    gpio_put(GREEN_LED_PIN, 0);
}

static void show_happy_screen(void) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 16, "VOCE ESTA", 2);
    ssd1306_draw_string(&display, 0, 40, "FELIZ :)", 2);
    ssd1306_show(&display);
    gpio_put(GREEN_LED_PIN, 1);

    showing_happy = true;
    happy_until = make_timeout_time_ms(HAPPY_DISPLAY_MS);
}

static err_t on_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        // remote closed the connection
        tcp_close(tpcb);
        return ERR_OK;
    }

    char payload[64] = {0};
    uint16_t len = p->len < sizeof(payload) - 1 ? p->len : sizeof(payload) - 1;
    memcpy(payload, p->payload, len);

    if (strstr(payload, "SMILE") != NULL) {
        show_happy_screen();
        tcp_write(tpcb, "OK\n", 3, TCP_WRITE_FLAG_COPY);
    } else {
        tcp_write(tpcb, "IGNORED\n", 8, TCP_WRITE_FLAG_COPY);
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    tcp_close(tpcb);
    return ERR_OK;
}

static err_t on_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, on_recv);
    return ERR_OK;
}

static bool start_server(uint16_t port) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return false;

    if (tcp_bind(pcb, IP_ADDR_ANY, port) != ERR_OK) {
        tcp_close(pcb);
        return false;
    }

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, on_accept);
    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000); // da tempo do monitor serial conectar antes dos primeiros prints
    leds_init();
    button_a_init();

    printf("Boot: iniciando OLED...\n");
    ssd1306_init(&display);
    printf("Boot: OLED ok.\n");

    printf("Boot: aguardando Botao A...\n");
    wait_for_button_a();
    printf("Boot: Botao A pressionado, iniciando...\n");

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 24, "CONECTANDO", 1);
    ssd1306_draw_string(&display, 0, 40, "WIFI...", 1);
    ssd1306_show(&display);

    printf("Boot: cyw43_arch_init()...\n");
    if (cyw43_arch_init()) {
        printf("Boot: cyw43_arch_init() FALHOU.\n");
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 24, "ERRO WIFI", 1);
        ssd1306_show(&display);
        while (true) tight_loop_contents();
    }
    printf("Boot: cyw43_arch_init() ok.\n");

    cyw43_arch_enable_sta_mode();
    printf("Boot: conectando em SSID '%s'...\n", WIFI_SSID);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        printf("Boot: falha ao conectar no Wi-Fi (timeout ou senha errada).\n");
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 24, "FALHA WIFI", 1);
        ssd1306_show(&display);
        while (true) tight_loop_contents();
    }
    printf("Boot: Wi-Fi conectado!\n");

    // O modo de economia de energia padrao do radio cyw43 deixa o chip
    // "cochilando" entre os beacons do Wi-Fi para economizar energia, o que
    // atrasa a resposta a conexoes TCP recebidas (mais perceptivel em redes
    // com sinal mais fraco/distante, como o Wi-Fi de casa vs. um hotspot
    // proximo). Desligamos isso para minimizar a latencia do servidor.
    cyw43_wifi_pm(&cyw43_state, CYW43_NO_POWERSAVE_MODE);

    start_server(SMILE_SERVER_PORT);

    const char *ip_str = ip4addr_ntoa(netif_ip4_addr(netif_list));

    printf("Conectado! IP: %s  porta: %d\n", ip_str, SMILE_SERVER_PORT);
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 8, "CONECTADO!", 1);
    ssd1306_draw_string(&display, 0, 24, "IP:", 1);
    ssd1306_draw_string(&display, 0, 40, ip_str, 1);
    ssd1306_show(&display);
    sleep_ms(8000);

    show_idle_screen();

    while (true) {
        cyw43_arch_poll(); // drives the lwIP raw-API state machine
        if (showing_happy && absolute_time_diff_us(get_absolute_time(), happy_until) < 0) {
            showing_happy = false;
            show_idle_screen();
        }

        cyw43_arch_wait_for_work_until(make_timeout_time_ms(50));
    }
}