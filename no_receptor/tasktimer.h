#ifndef TASKTIMER_H
#define TASKTIMER_H

#include <RH_ASK.h>        // Biblioteca para comunicação RF 433MHz (VirtualWire)
#include <SPI.h>           // Necessário para o RH_ASK
#include <Arduino.h>       // Funções principais do ESP32/Arduino
#include <ArduinoJson.h>   // Manipulação de JSON para troca de dados
#include <WiFi.h>          // (Se necessário para outras funções)
#include <time.h>          // Controle do tempo e RTC
#include "driver/ledc.h"   // Controle PWM no ESP32
#include <stdio.h>
#include <string.h>        // Funções de manipulação de strings (strcat, strcpy etc.)

// Pino usado para saída PWM modulada (se aplicável ao sistema RF)
#define pinmodulado 17

// Handles de tasks globais (opcionalmente usados para controle externo)
extern TaskHandle_t xtask_transmissaoHandle;
extern TaskHandle_t xtask_receptorHandle;

// Semáforo global para controle de acesso ao driver RF (evita colisões)
extern SemaphoreHandle_t xSemaforo;

// Estrutura para passar múltiplos parâmetros entre as tasks
typedef struct {
  RH_ASK *rf_driver;       // Ponteiro para o driver de rádio
  QueueHandle_t xQueue;    // Fila de mensagens entre tasks (se necessário)
} Params_t;

// Prototipação das tasks

// Task do transmissor
// - Envia dados via RF (pode ser um JSON ou timestamp)
// - Aguarda confirmação via ACK
void transmissor(void *pvParameters);

// Task do receptor
// - Recebe mensagens via RF
// - Envia ACK como resposta
// - (Opcionalmente processa JSON recebido)
void recepcao(void *pvParameters);

#endif
