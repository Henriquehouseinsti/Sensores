#include "tasktimer.h"

void transmissor(void *pvParameters) {
  // Recupera os objetos compartilhados (driver RF e fila)
  Params_t *params = (Params_t *)pvParameters;
  RH_ASK &rf_driver = *params->rf_driver;
  QueueHandle_t xQueue = params->xQueue;

  const uint8_t maxTentativas = 3;  // Número máximo de tentativas de envio

  while (true) {
    time_t now = time(nullptr);
    int segundos = now;

    // Calcula o tempo restante até o próximo múltiplo de 100 segundos
    int delaySegundos = (600 - (segundos % 600)) % 600;

    vTaskDelay(pdMS_TO_TICKS(delaySegundos * 1000));

    // Tenta adquirir o semáforo antes de usar o driver RF
    if (xSemaphoreTake(xSemaforo, portMAX_DELAY)) {
      Serial.println("Task transmissao_rtc rodando");

      bool ackRecebido = false;  // Redefine o status do ACK antes de iniciar as tentativas

      // Tenta enviar e sincronizar o RTC até atingir o limite de tentativas
      for (uint8_t tentativa = 0; tentativa < maxTentativas; tentativa++) {
        // Obtém o horário atual
        time_t now = time(nullptr);

        // Converte o timestamp em 4 bytes
        uint8_t timestamp_bytes[4];
        timestamp_bytes[0] = (now >> 24) & 0xFF;
        timestamp_bytes[1] = (now >> 16) & 0xFF;
        timestamp_bytes[2] = (now >> 8) & 0xFF;
        timestamp_bytes[3] = now & 0xFF;

        // Configura os cabeçalhos da mensagem
        rf_driver.setHeaderId(3);  // ID 3 indica mensagem de atualização de RTC
        rf_driver.setHeaderTo(2);  // ID do receptor

        // Envia o timestamp
        rf_driver.send(timestamp_bytes, 4);
        rf_driver.waitPacketSent();

        Serial.print("Timestamp enviado (tentativa ");
        Serial.print(tentativa + 1);
        Serial.print("): ");
        Serial.println(now);

        // Espera até 3 segundos por um ACK
        uint32_t tempoInicio = millis();
        uint8_t buf[10];
        uint8_t buflen = sizeof(buf);

        while (millis() - tempoInicio < 3000) {
          if (rf_driver.recv(buf, &buflen)) {
            Serial.println((char*)buf);
            if (rf_driver.headerId() == 2 && rf_driver.headerFrom() == 2) {
              Serial.println("ACK recebido. Sincronização confirmada.");
              ackRecebido = true;
              break;  // Sai da espera e do laço de tentativas
            }
          }
        }

        if (ackRecebido) break;  // Envio confirmado, encerra o laço de tentativas
      }

      if (!ackRecebido) {
        Serial.println("Falha ao sincronizar o RTC após 3 tentativas.");
      }

      vTaskDelay(pdMS_TO_TICKS(1000));  // Pausa antes do próximo ciclo de envio

      // Libera o semáforo após uso do driver RF
      xSemaphoreGive(xSemaforo);
    }
  }
}
