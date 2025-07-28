#include "tasktimer.h"

void transmissor(void *pvParameters) {
  Serial.println("Task transmissao rodando");

  // Recebe os valores dos objetos do driver e da fila
  Params_t *params = (Params_t *)pvParameters;
  RH_ASK &rf_driver = *params->rf_driver;
  QueueHandle_t xQueue = params->xQueue;

  char valor_transmitindo[256];
  const uint8_t maxTentativas = 3;

  while (true) {
    time_t now = time(nullptr);
    int segundos = now;

    // Calcula o tempo até o próximo múltiplo de 100 segundos
    int delaySegundos = 99 - ((segundos+50) % 100);
    vTaskDelay(pdMS_TO_TICKS(delaySegundos * 1000));

    // Verifica se há mensagem na fila (não bloqueante)
    if (xQueueReceive(xQueue, &valor_transmitindo, 0) == pdPASS) {
      if (xSemaphoreTake(xSemaforo, portMAX_DELAY)) {
        bool ackReceived = false;
        int tentativa = 0;

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, valor_transmitindo);

        if (!error) {
          // Mensagem JSON válida - envia dados
          while (!ackReceived && tentativa < maxTentativas) {
            rf_driver.setHeaderId(1);  // mensagem tipo dados
            rf_driver.setHeaderTo(1);  // destinatário
            rf_driver.send((uint8_t *)valor_transmitindo, strlen(valor_transmitindo) + 1);
            rf_driver.waitPacketSent();

            Serial.println("Mensagem enviada:");
            Serial.println(valor_transmitindo);

            // Aguarda ACK por até 3 segundos
            uint32_t tempoInicio = millis();
            uint8_t buf[256];
            uint8_t buflen = sizeof(buf);
            unsigned long start = millis();

            while (millis() - tempoInicio < 3000) {
              if (rf_driver.recv(buf, &buflen)) {
                if (rf_driver.headerId() == 2 && rf_driver.headerFrom() == 1) {
                  Serial.println("ACK recebido. Sincronização confirmada.");
                  ackReceived = true;
                  break;  // Sai da espera e do laço de tentativas
                }
              }
            }

            if (!ackReceived) {
              Serial.println("ACK não recebido. Reenviando...");
              tentativa++;
            }
          }

          if (!ackReceived) {
            Serial.println("Falha ao enviar mensagem após 3 tentativas.");
          }

        } else {
          // Mensagem não é JSON, assume sincronização RTC
          // Reenvia para fila para garantir que sincronização será tentada
          //xQueueSend(xQueue, &valor_transmitindo, portMAX_DELAY);
          
          rf_driver.setHeaderTo(3);
          // Tenta enviar timestamp para sincronização RTC
          for (uint8_t tentativa = 0; tentativa < maxTentativas; tentativa++) {
            now = time(nullptr);

            rf_driver.setHeaderId(3); // mensagem atualização RTC

            uint8_t timestamp_bytes[4];
            timestamp_bytes[0] = (now >> 24) & 0xFF;
            timestamp_bytes[1] = (now >> 16) & 0xFF;
            timestamp_bytes[2] = (now >> 8) & 0xFF;
            timestamp_bytes[3] = now & 0xFF;

            rf_driver.send(timestamp_bytes, 4);
            rf_driver.waitPacketSent();

            Serial.printf("Timestamp enviado (tentativa %d): %u\n", tentativa + 1, now);

            uint32_t tempoInicio = millis();
            uint8_t buf[10];
            uint8_t buflen = sizeof(buf);
            bool ackRtcReceived = false;

            while (millis() - tempoInicio < 3000) {
              if (rf_driver.recv(buf, &buflen)) {
                if (rf_driver.headerId() == 2 && rf_driver.headerFrom() == 3) {
                  Serial.println("ACK recebido. Sincronização confirmada.");
                  ackRtcReceived = true;
                  break;  // Sai da espera e do laço de tentativas
                }
              }
            }

            if (ackRtcReceived) {
              break;
            } else {
              Serial.println("confirmação não recebida. Reenviando...");
            }
          }
        }

        xSemaphoreGive(xSemaforo);
      }
    }
  }
}
