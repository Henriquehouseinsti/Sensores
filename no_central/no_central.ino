// Coleta dados sensor e transmite
// ESP8266 433MHz transmitter test 1
#include "tasktimer.h"

RH_ASK rf_driver(2000, 3, 17); // Velocidade, RX, TX

#define configTICK_RATE_HZ 1000

QueueHandle_t xQueue;
TimerHandle_t coleta_dados;

TaskHandle_t xtask_transmissaoHandle = NULL;
TaskHandle_t xtask_receptorHandle = NULL;

// Criando o handle do semáforo
SemaphoreHandle_t xSemaforo;

bool dado_true_rtc_false = true;

void setup() {
  Serial.begin(115200); // Ajusta a entrada serial
  
  // Cria semáforo
  xSemaforo = xSemaphoreCreateBinary();
  xSemaphoreGive(xSemaforo); // libera o semáforo pela primeira vez

  // Configura PWM
  ledcSetup(0, 125000, 8);
  ledcAttachPin(pinmodulado, 0);

  // Inicializa driver RF
  if (!rf_driver.init()) {
    Serial.println("init failed");
    while (1) delay(10000);
  }
  Serial.println("Transmitter: rf_driver initialised");
  rf_driver.setModeIdle();         // Define modo do driver (idle/transmissor/receptor)
  rf_driver.setThisAddress(2);    // Define endereço próprio
  rf_driver.setHeaderFrom(2);     // Define endereço de origem das mensagens
  
  // Cria fila para dados coletados
  xQueue = xQueueCreate(50, 64); // 50 itens, 64 bytes cada

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
        rf_driver.setHeaderTo(1);    // Destinatário (transmissor)
        rf_driver.send((uint8_t*)ack, strlen(ack));
        rf_driver.waitPacketSent();

        Serial.println("ACK enviado.");
        rtc_sincronizado = true; // Sai do loop
        xQueueSend(xQueue, &buf, portMAX_DELAY);
      }
    }

    memset(buf, 0, sizeof(buf));
    buflen = sizeof(buf);

    vTaskDelay(pdMS_TO_TICKS(100));  // Espera antes de nova tentativa
  }

  // Prepara parâmetros para passar às tasks
  static Params_t params;
  params.rf_driver = &rf_driver;
  params.xQueue = xQueue;

  // Cria task para coleta de dados
  xTaskCreate(
    dados_task,
    "task_dados_rodando",
    4000,
    (void *)&params,
    2,
    NULL
  );

  // Cria task para transmissão dos dados
  xTaskCreate(
    transmissor,
    "task_transmissor_rodando",
    4000,
    (void *)&params,
    2,
    NULL
  );

  // Cria task para recepção e ACK
  xTaskCreate(
    recepcao,
    "task_recepcao_rodando",
    8000,
    (void *)&params,
    1,
    NULL
  );
}

void loop() {
  disableCore0WDT();       // Apenas para debugging
  vTaskSuspend(NULL);      // Suspende a task loop() permanentemente
}
