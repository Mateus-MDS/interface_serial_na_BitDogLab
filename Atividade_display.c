#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "animacoes_led.pio.h"

//numero de LEDs
#define NUM_PIXELS 25

//pino de saí­da
#define matriz_leds 7

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

//Variáveis Globais
PIO pio;        // Controlador PIO
uint sm;        // State Machine do PIO
uint numero = 0;  // variavel para indicar o numero a ser exibido na matriz de leds
ssd1306_t ssd; // Variável global para o display

// definição dos pinos dos leds e botôes a serem utilizados
const uint led_verde = 11;       
const uint led_azul = 12;        
const uint botao_A = 5;
const uint botao_B = 6;

// indicando o estados dos leds, inicialmente desligados
bool led_verde_on = false;
bool led_azul_on = false;

// Função para converter intensidade em valor RGB
uint32_t matrix_rgb(double intensity) {
    unsigned char value = intensity * 255;
    return (value << 16) | (value << 8) | value;
}

// desenhos dos números de 0 a 9, na matriz de leds 5x5
double numeros[10][25] = {
    {0, 1, 1, 1, 0,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1,
    0, 1, 1, 1, 0}, //0

    {0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 1,
    0, 1, 1, 0, 0,
    0, 0, 1, 0, 0}, // 1

    {1, 1, 1, 1, 1,
    0, 1, 0, 0, 0,
    0, 1, 0, 0, 0,
    1, 0, 0, 0, 1,
    0, 1, 1, 1, 0}, // 2

    {1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    0, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    1, 1, 1, 1, 1}, // 3

    {1, 0, 0, 0, 0,
    0, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 0, 0, 0, 1}, // 4

    {1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 1}, // 5

     {1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 0,
    1, 1, 1, 1, 1}, // 6

    {0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 1, 1, 1}, // 7

    {1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1}, // 8

    {1, 1, 1, 1, 1,
    0, 0, 0, 0, 1,
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1}  // 9
    };

// Função de interrupção para os botões
void gpio_irq_handler(uint gpio, uint32_t events);

int main() {
    
    stdio_init_all(); // Inicializa o sistema, para funções como printf

    sleep_ms(2000);  // Aguarda 2 segundos antes de continuar

    // Inicializa os LEDs e botões, e os direciona como saída e entrada digitais
    gpio_init(led_verde);
    gpio_set_dir(led_verde, GPIO_OUT);
    gpio_init(led_azul);
    gpio_set_dir(led_azul, GPIO_OUT);
    gpio_init(botao_A);
    gpio_set_dir(botao_A, GPIO_IN);
    gpio_pull_up(botao_A);             // aciona o resistor de pull up, para que fique em nível alto quando do botão for precionado
    gpio_init(botao_B);
    gpio_set_dir(botao_B, GPIO_IN);
    gpio_pull_up(botao_B);             // aciona o resistor de pull up, para que fique em nível alto quando do botão for precionado

    // Inicialização do PIO para controlar a matriz de LEDs
    pio = pio0;
    uint offset = pio_add_program(pio, &animacoes_led_program);
    sm = pio_claim_unused_sm(pio, true);
    animacoes_led_program_init(pio, sm, offset, matriz_leds);

    // Configuração do I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        if (stdio_usb_connected())
        { // Certifica-se de que o USB está conectado
            char c;
            if (scanf("%c", &c) == 1)
            { // Ler caractere da entrada padrão
                printf("Recebido: '%c'\n", c);

                switch (c)
                {
                // temos os casos para exibir no display as letras de a a z, minúsculas e maiúsculas, e os números de 0 a 9, que também são exibidos na matriz de leds
                    
                // letras minúsculas de a a z
                case 'a':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "a", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'b':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "b", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'c':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "c", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'd':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "d", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'e':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "e", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'f':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "f", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'g':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "g", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'h':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "h", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                // Letras de 'i' a 'z' e números de '0' a '9'
                case 'i':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "i", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'j':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "j", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'k':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "k", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'l':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "l", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'm':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "m", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'n':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "n", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'o':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "o", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'p':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "p", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'q':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "q", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'r':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "r", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 's':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "s", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 't':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "t", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'u':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "u", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'v':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "v", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'w':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "w", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'x':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "x", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'y':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "y", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                case 'z':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "z", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                // Letras maiúsculas de 'A' a 'Z'
                case 'A':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "A", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'B':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "B", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'C':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "C", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'D':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "D", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'E':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "E", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'F':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "F", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'G':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "G", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'H':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "H", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'I':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "I", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'J':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "J", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'K':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "K", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'L':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "L", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'M':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "M", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'N':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "N", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'O':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "O", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'P':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "P", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'Q':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "Q", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'R':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "R", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'S':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "S", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'T':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "T", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'U':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "U", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'V':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "V", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'W':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "W", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'X':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "X", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'Y':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "Y", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;

                case 'Z':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "Z", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                    break;
                
                // Números de 0 a 9, que além do display, são exibidos na matriz de leds
                case '0':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "0", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display
                     
                    // numero a ser exibido na matriz de leds
                    numero = 0; 
                    
                    // interação para acendimento de todos os 25 leds da matriz
                    for (int i = 0; i < NUM_PIXELS; i++) {
                        uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                        pio_sm_put_blocking(pio, sm, valor_led);
                        
                    }
                    break;

                case '1':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "1", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 1; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '2':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "2", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 2; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '3':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "3", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 3; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '4':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "4", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 4; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '5':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "5", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 5; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '6':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "6", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 6; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '7':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "7", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 7; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '8':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "8", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 8; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;
                
                case '9':
                    ssd1306_fill(&ssd, false);                // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "9", 30, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                  // envia os dados do buffer da memória para o display

                     // numero a ser exibido na matriz de leds
                     numero = 9; 
                    
                     // interação para acendimento de todos os 25 leds da matriz
                     for (int i = 0; i < NUM_PIXELS; i++) {
                         uint32_t valor_led = matrix_rgb(numeros[numero][i]);
                         pio_sm_put_blocking(pio, sm, valor_led);
                         
                     }
                    break;

                default:
                    printf("string inválida: '%c'\n", c);
                    ssd1306_fill(&ssd, false);                             // apaga o conteudo do display
                    ssd1306_draw_string(&ssd, "string invalida", 1, 30);   // desenha a string no buffer da memória
                    ssd1306_send_data(&ssd);                               // envia os dados do buffer da memória para o display
                }
            }
        }
    }
}

// função de interrupção quando um dos botões forem precionados
void gpio_irq_handler(uint gpio, uint32_t events) {

    static uint32_t last_time = 0;
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Debouncing de 300ms
    if (current_time - last_time > 300000) {
        last_time = current_time;

        // verifica se o botão A foi precionado
        if (gpio == botao_A && !gpio_get(botao_A)) {
            
            // inverte o estado do led verde, 
            led_verde_on = !led_verde_on;

            // apaga ou acente o led verde
            gpio_put(led_verde, led_verde_on);

            // Apaga o contéudo do display
            ssd1306_fill(&ssd, false);

            // condição de estados dos leds, para exibir no display
            if ((led_verde_on == true) && (led_azul_on == false)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Ligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Desligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde ligado\n");
                printf("LED azul desligado\n");

            } 
            
            else if ((led_verde_on == true) && (led_azul_on == true)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Ligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Ligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde ligado\n");
                printf("LED azul ligado\n");
            }

            else if ((led_verde_on == false) && (led_azul_on == true)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Desligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Ligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde desligado\n");
                printf("LED azul ligado\n");
            }

            else if ((led_verde_on == false) && (led_azul_on == false)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Desligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Desligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde desligado\n");
                printf("LED azul desligado\n");
            }

            ssd1306_send_data(&ssd);              // envia os dados do buffer da memória para o display
        }

        // verifica se o botão B foi precionado
        if (gpio == botao_B && !gpio_get(botao_B)) {

            // inverte o estado do led azul
            led_azul_on = !led_azul_on;

            // liga ou desliga o led azul
            gpio_put(led_azul, led_azul_on);

            // Apaga o contéudo do display
            ssd1306_fill(&ssd, false);
            
            // Condições dos estados dos leds para exibir no display
            if ((led_verde_on == true) && (led_azul_on == false)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Ligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Desligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde ligado\n");
                printf("LED azul desligado\n");

            } 
            
            else if ((led_verde_on == true) && (led_azul_on == true)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Ligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Ligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde ligado\n");
                printf("LED azul ligado\n");
            }

            else if ((led_verde_on == false) && (led_azul_on == true)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Desligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Ligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde desligado\n");
                printf("LED azul ligado\n");
            }

            else if ((led_verde_on == false) && (led_azul_on == false)){

                // desenha as strings no buffer da memória
                ssd1306_draw_string(&ssd, "Led Verde", 10, 10);
                ssd1306_draw_string(&ssd, "Desligado", 10, 20);
                ssd1306_draw_string(&ssd, "Led Azul", 10, 40);
                ssd1306_draw_string(&ssd, "Desligado", 10, 50);

                // exibe no monitor serial o estado dos leds
                printf("LED verde desligado\n");
                printf("LED azul desligado\n");
            }

            ssd1306_send_data(&ssd); // envia os dados do buffer da memória para o display
        }
    }
}
