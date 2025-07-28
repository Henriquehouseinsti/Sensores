// ESP8266 - Teste de transmissor 433MHz (Versão 1)
#include "tasktimer.h"

// Inicializa o driver RF: velocidade de transmissão, pino RX, pino TX
RH_ASK rf_driver(2000, 3, 17);

#define configTICK_RATE_HZ 1000

// Fila para troca de mensagens entre tarefas
QueueHandle_t xQueue;

// Handle da task de recepção
TaskHandle_t xtask_receptorHandle = NULL;

// Handle do semáforo
SemaphoreHandle_t xSemaforo;

void setup() {
  Serial.begin(115200); // Inicializa a comunicação serial

  // Cria o semáforo binário
  xSemaforo = xSemaphoreCreateBinary();
  xSemaphoreGive(xSemaforo); // Libera o semáforo inicialmente

  // Configura o PWM (canal 0, frequência 125 kHz, resolução 8 bits)
  ledcSetup(0, 125000, 8);
  ledcAttachPin(pinmodulado, 0);

  // Inicializa o driver RF
  if (!rf_driver.init()) {
    Serial.println("Falha na inicialização do driver RF");
    while (1) delay(10000); // Loop infinito em caso de falha
  }
  Serial.println("Transmissor: driver RF inicializado");

  rf_driver.setModeIdle();      // Define o driver como inativo inicialmente
  rf_driver.setThisAddress(3);  // Define o endereço do transmissor (identificação própria)
  rf_driver.setHeaderFrom(3);   // Define o campo "de" nos pacotes enviados

  // Cria a fila de transmissão (50 itens de 64 bytes cada)
  xQueue = xQueueCreate(50, 64);

    // Buffer para receber mensagens
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN] = {0};
  uint8_t buflen = sizeof(buf);

  // Aguarda sincronização RTC antes de continuar (com timeout se desejar)
  bool rtc_sincronizado = false;
  while (!rtc_sincronizado) {
    if (rf_driver.recv(buf, &buflen) && rf_driver.headerId() != 2) {
      Serial.print("Mensagem recebida (RTC): ");
      for (int i = 0; i < buflen; i++) {
        Serial.printf("%02X ", buf[i]);
      }
      Serial.println();

      uint8_t id = rf_driver.headerId();
      if (id == 3 && buflen >= 4) {  // Mensagem de sincronização RTC
        // Converte bytes para time_t
        time_t novo_rtc =
          ((uint32_t)buf[0] << 24) |
          ((uint32_t)buf[1] << 16) |
          ((uint32_t)buf[2] << 8) |
          (uint32_t)buf[3];

        struct timeval tv = { .tv_sec = novo_rtc, .tv_usec = 0 };
        settimeofday(&tv, nullptr);

        Serial.print("RTC sincronizado para: ");
        Serial.println(novo_rtc);

        // Envia ACK
        const char* ack = "ACK";
        rf_driver.setHeaderId(2);    // ID reservado para ACK
        rf_driver.setHeaderTo(2);    // Destinatário (transmissor)
        rf_driver.send((uint8_t*)ack, strlen(ack));
        rf_driver.waitPacketSent();

        Serial.println("ACK enviado.");
        rtc_sincronizado = true; // Sai do loop
      }
    }

    memset(buf, 0, sizeof(buf));
    buflen = sizeof(buf);

    vTaskDelay(pdMS_TO_TICKS(100));  // Espera antes de nova tentativa
  }

  // Estrutura estática para armazenar parâmetros compartilhados entre tasks
  static Params_t params;
  params.rf_driver = &rf_driver;
  params.xQueue = xQueue;

  // Cria a task de recepção
  xTaskCreate(
    recepcao,                    // Função da task
    "task_recepcao_rodando",     // Nome da task (para depuração)
    8000,                        // Tamanho da pilha (em palavras)
    (void *)&params,             // Parâmetros passados
    1,                           // Prioridade da task
    NULL                         // Handle da task (não usado aqui)
  );

  // Cria a task de transmissão
  xTaskCreate(
    transmissor,                 // Função da task
    "task_transmissor_rodando",  // Nome da task (para depuração)
    8000,                        // Tamanho da pilha (em palavras)
    (void *)&params,             // Parâmetros passados
    1,                           // Prioridade da task
    NULL                         // Handle da task (não usado aqui)
  );
}

void loop() {
  disableCore0WDT();    // Desativa o watchdog do núcleo 0 (usar apenas para depuração)
  vTaskSuspend(NULL);   // Suspende permanentemente a task loop()
}
