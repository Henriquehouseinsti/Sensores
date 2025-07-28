#ifndef TASKTIMER_H
#define TASKTIMER_H

#include <RH_ASK.h> 
#include <SPI.h> 
#define pinmodulado 17 // Definir um pino analógico adequado
#include <stdio.h>
#include <string.h> // Para usar strcat()
#include <Arduino.h>
#include "driver/ledc.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <time.h>

extern TaskHandle_t xtask_transmissaoHandle; 
extern TaskHandle_t xtask_receptorHandle; 

// Handle do semáforo para sincronização entre tasks
extern SemaphoreHandle_t xSemaforo;

extern bool dado_true_rtc_false;

typedef struct {
  RH_ASK *rf_driver;
  QueueHandle_t xQueue;
} Params_t;

// Task que lê dados periodicamente, cria JSON, serializa e coloca na fila para transmissão
void dados_task(void *pvParameters);

// Task que pega dados da fila e envia, aguardando confirmação via ACK
void transmissor(void *pvParameters);

// Task que recebe mensagens e envia confirmação ACK
void recepcao(void *pvParameters);

#endif
