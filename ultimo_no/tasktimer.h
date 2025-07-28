#ifndef TASKTIMER_H
#define TASKTIMER_H

#include <RH_ASK.h>            // Biblioteca para comunicação via RF 433MHz
#include <SPI.h>               // Necessário para o RH_ASK
#include <Arduino.h>
#include <ArduinoJson.h>       // Manipulação de mensagens JSON
#include <WiFi.h>
#include <time.h>              // Controle de tempo e RTC
#include "driver/ledc.h"       // Controle de PWM (ESP32)
#include <stdio.h>
#include <string.h>            // Para strcat(), strcpy(), etc.

// Define o pino utilizado para saída PWM modulada
#define pinmodulado 17         // Ajuste conforme necessidade

// Handles globais para controle das tasks
extern TaskHandle_t xtask_transmissaoHandle;
extern TaskHandle_t xtask_receptorHandle;

// Semáforo global para controle do driver RF
extern SemaphoreHandle_t xSemaforo;

// Variável de controle de tipo de dado (não utilizada diretamente aqui)
extern bool dado_true_rtc_false;

// Estrutura para passar parâmetros compartilhados entre tasks
typedef struct {
  RH_ASK *rf_driver;           // Ponteiro para o driver RF
  QueueHandle_t xQueue;        // Fila de mensagens
} Params_t;

// Prototipagem das funções de task

// Função da task transmissora
// - Lê dados (fila/sensor), transmite via RF e aguarda ACK
void transmissor(void *pvParameters);

// Função da task receptora
// - Recebe dados via RF e envia ACK de confirmação
void recepcao(void *pvParameters);

#endif
