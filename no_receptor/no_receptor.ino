// ESP8266 - Teste Transmissor 433MHz (Versão 1)
#include "tasktimer.h"

// Inicialização do driver RF (velocidade, pino RX, pino TX)
RH_ASK rf_driver(2000, 3, 17);

#define configTICK_RATE_HZ 1000  // Taxa de ticks do sistema (se necessário para ajustes)

QueueHandle_t xQueue;  // Fila para comunicação entre tasks

// Handles globais
TaskHandle_t xtask_receptorHandle = NULL;
SemaphoreHandle_t xSemaforo;  // Controle de acesso ao driver RF

void setup() {
  Serial.begin(115200);  // Inicializa a porta serial

  // Cria o semáforo binário e libera inicialmente
  xSemaforo = xSemaphoreCreateBinary();
  xSemaphoreGive(xSemaforo);

  // Configura o PWM (canal 0, 125 kHz, 8 bits)
  ledcSetup(0, 125000, 8);
  ledcAttachPin(pinmodulado, 0);

  // Inicializa o driver RF
  if (!rf_driver.init()) {
    Serial.println("Falha ao inicializar o driver RF");
    while (1) delay(10000);  // Loop infinito em caso de falha
  }
  Serial.println("Transmissor: driver RF inicializado");

  rf_driver.setModeIdle();      // Define o driver em modo ocioso inicialmente
  rf_driver.setThisAddress(1);  // Define o endereço do dispositivo (transmissor)
  rf_driver.setHeaderFrom(1);   // Define o campo "de" nos pacotes enviados

  // Cria a fila de transmissão (50 itens de 64 bytes cada)
  xQueue = xQueueCreate(50, 64);

  // Parâmetros compartilhados entre as tasks
  static Params_t params;
  params.rf_driver = &rf_driver;
  params.xQueue = xQueue;

  // Criação da task receptora
  xTaskCreate(
    recepcao,                   // Função da task
    "task_recepcao_rodando",    // Nome da task (para debug)
    8000,                       // Tamanho da pilha (em palavras)
    (void *)&params,            // Parâmetros passados
    1,                          // Prioridade da task
    NULL                        // Handle da task (não usado)
  );

  // Criação da task transmissora
  xTaskCreate(
    transmissor,                // Função da task
    "task_transmissor_rodando", // Nome da task (para debug)
    8000,                       // Tamanho da pilha (em palavras)
    (void *)&params,            // Parâmetros passados
    1,                          // Prioridade da task
    NULL                        // Handle da task (não usado)
  );
}

void loop() {
  disableCore0WDT();    // Desativa o watchdog do Core 0 (usar apenas para debug)
  vTaskSuspend(NULL);   // Suspende permanentemente o loop() (controle entregue às tasks)
}
