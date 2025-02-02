/* Programa de capacitação EMBARCATECH
 * Unidade 4 - Microcontroladores / Capítulo 4 - Interrupções
 * Tarefa 1 - Aula Síncrona 27/01/2025
 *
 * Programa desenvolvido por:
 *      - Lucas Meira de Souza Leite
 * 
 * Objetivos: 
 *      - Usar interrupções no microcontrolador RP2040 e placa BitDogLab
 *      - Controlar LEDs comuns e LEDs endereçáveis WS2812 
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

#define LED_VERMELHO 13
#define NUM_LEDS 25
#define WS2812_PIN 7
#define BOTAO_A 5
#define BOTAO_B 6
#define TEMPO_DEBOUNCE 200
#define TEMPO to_ms_since_boot(get_absolute_time())
volatile unsigned long TEMPODEBOUNCE; 

// Definição de pixel GRB
struct pixel_t {
  uint8_t G, R, B;};  // Três valores de 8-bits compõem um pixel.

typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[NUM_LEDS];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;
uint PosicaoAtual = 0;

//Arrays contendo os LEDS que irão compor a visualização de cada número 
const uint8_t Zero[12] = {1, 2, 3, 6, 8, 11, 13, 16, 18, 21, 22, 23};
const uint8_t Um[5] = {1, 8, 11, 18, 21};
const uint8_t Dois[11] = {1, 2, 3, 6, 11, 12, 13, 18, 21, 22, 23};
const uint8_t Tres[11] = {1, 2, 3, 8, 11, 12, 13, 18, 21, 22, 23};
const uint8_t Quatro[9] = {1, 8, 11, 12, 13, 16, 18, 21, 23};
const uint8_t Cinco[11] = {1, 2, 3, 8, 11, 12, 13, 16, 21, 22, 23};
const uint8_t Seis[12] = {1, 2, 3, 6, 8, 11, 12, 13, 16, 21, 22, 23};
const uint8_t Sete[7] = {1, 8, 11, 18, 21, 22, 23};
const uint8_t Oito[13] = {1, 2, 3, 6, 8, 11, 12, 13, 16, 18, 21, 22, 23};
const uint8_t Nove[12] ={1, 2, 3, 8, 11, 12, 13, 16, 18, 21, 22, 23};
const uint8_t *Fila[10] = {Zero, Um, Dois, Tres, Quatro, Cinco, Seis, Sete, Oito, Nove};

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < NUM_LEDS; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
  for (uint i = 0; i < NUM_LEDS; ++i)
    npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < NUM_LEDS; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
}

// Prototipo da função de interrupção
static void gpio_irq(uint gpio, uint32_t events);

void main()
{
    stdio_init_all();
    TEMPODEBOUNCE = TEMPO;
    
    npInit(WS2812_PIN);
    
    gpio_init(LED_VERMELHO); //Inicializa o led
    gpio_set_dir(LED_VERMELHO, GPIO_OUT); //Define o led como saida

    gpio_init(BOTAO_A);                    // Inicializa o botão
    gpio_set_dir(BOTAO_A, GPIO_IN);        // Configura o pino como entrada
    gpio_pull_up(BOTAO_A);                 // Habilita o pull-up interno

    gpio_init(BOTAO_B);                    // Inicializa o botão
    gpio_set_dir(BOTAO_B, GPIO_IN);        // Configura o pino como entrada
    gpio_pull_up(BOTAO_B);                 // Habilita o pull-up interno
    
    npClear(); // Inicia os leds com o número 0
    for (int i = 0; i < 12; ++i){
        npSetLED(Zero[i], 0, 0, 100);
    }    
    npWrite();

    // Configuração da interrupção com callback
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_RISE, true, &gpio_irq);    
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_RISE, true, &gpio_irq);
    
    while (true) {
        //Laço responsável por ficar piscando o LED RGB
        gpio_put(LED_VERMELHO, true);
        sleep_ms(100);
        gpio_put(LED_VERMELHO, false);
        sleep_ms(100);
    }
}

void gpio_irq(uint gpio, uint32_t events)
{
    int tamanho_array;    
    switch (gpio)
    {
    case BOTAO_A: //Botão responsável por incrementar o número
        if ((PosicaoAtual != 9) && (TEMPO - TEMPODEBOUNCE > TEMPO_DEBOUNCE)){
            PosicaoAtual++;
            switch (PosicaoAtual) { //Obtenção do tamanho do array de cada número
                case 0: tamanho_array = 12; break;
                case 1: tamanho_array = 5; break;
                case 2: tamanho_array = 11; break;
                case 3: tamanho_array = 11; break;
                case 4: tamanho_array = 9; break;
                case 5: tamanho_array = 11; break;
                case 6: tamanho_array = 12; break;
                case 7: tamanho_array = 7; break;
                case 8: tamanho_array = 13; break;
                case 9: tamanho_array = 12; break;
            }                                
            // Seta os LEDs conforme cada número
            npClear();
            for (int i = 0; i < tamanho_array; ++i){
                npSetLED(Fila[PosicaoAtual][i], 0, 0, 100);
            }    
            npWrite();
        }
        break;
    case BOTAO_B: //Botão responsável por decrementar o número
        if ((PosicaoAtual != 0) && (TEMPO - TEMPODEBOUNCE > TEMPO_DEBOUNCE)){                        
            PosicaoAtual--;
            switch (PosicaoAtual) { //Obtenção do tamanho do array de cada número
                case 0: tamanho_array = 12; break;
                case 1: tamanho_array = 5; break;
                case 2: tamanho_array = 11; break;
                case 3: tamanho_array = 11; break;
                case 4: tamanho_array = 9; break;
                case 5: tamanho_array = 11; break;
                case 6: tamanho_array = 12; break;
                case 7: tamanho_array = 7; break;
                case 8: tamanho_array = 13; break;
                case 9: tamanho_array = 12; break;
            }
            // Seta os LEDs conforme cada número
            npClear();
            for (int i = 0; i < tamanho_array; ++i){
                npSetLED(Fila[PosicaoAtual][i], 0, 0, 100);
            }    
            npWrite();
        }        
        break;  
    }  
    TEMPODEBOUNCE = TEMPO;    
}