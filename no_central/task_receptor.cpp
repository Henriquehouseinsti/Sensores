#include "tasktimer.h"

void recepcao(void *pvParameters) {
  Params_t *params = (Params_t *)pvParameters;
  RH_ASK &rf_driver = *params->rf_driver;
  QueueHandle_t xQueue = params->xQueue;


  unsigned long start = millis();

  while (true) {
    
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN] = {0};
    uint8_t buflen = sizeof(buf);

    time_t now = time(nullptr);
    int segundos = now;

    int delaySegundos = 99 - ((segundos) % 100);
    if (delaySegundos == 0) {
      delaySegundos = 100;
    }

    vTaskDelay(pdMS_TO_TICKS(delaySegundos * 1000));

    if (xSemaphoreTake(xSemaforo, portMAX_DELAY)) {
      Serial.print("Lendo... ");
      start = millis();

      while (millis() - start < 8000) {
        if (rf_driver.recv(buf, &buflen) && rf_driver.headerId() != 2) {
          buf[buflen] = '\0';
          Serial.print("Mensagem recebida: ");
          Serial.println((char*)buf);

          uint8_t id = rf_driver.headerId();


          if (id == 1) {
            // ACK para mensagem tipo 1 (dados JSON)
            rf_driver.setHeaderTo(3);

            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, buf);
            if (error) {
              Serial.print("Erro ao desserializar JSON: ");
              Serial.println(error.c_str());
            } else {
              const char* sensor = doc["sensor"];
              int valor = doc["valor"];
              int now = doc["now"];

              Serial.print("Sensor: ");
              Serial.print(sensor);
              Serial.print(" | Valor: ");
              Serial.print(valor);
              Serial.print(" | Now: ");
              Serial.println(now);

              xQueueSend(xQueue, &buf, portMAX_DELAY);
              dado_true_rtc_false = true;
            }

          } else if (id == 3) {
            // ACK para mensagem tipo 3 (RTC)
            rf_driver.setHeaderTo(1);

            time_t novo_rtc =
              ((uint32_t)buf[0] << 24) |
              ((uint32_t)buf[1] << 16) |
              ((uint32_t)buf[2] << 8) |
              (uint32_t)buf[3];

            struct timeval tv = { .tv_sec = novo_rtc, .tv_usec = 0 };
            settimeofday(&tv, nullptr);

            Serial.print("RTC sincronizado para: ");
            Serial.println(novo_rtc);

            dado_true_rtc_false = false;
            xQueueSend(xQueue, &buf, portMAX_DELAY);
          }

          delay(50);
          const char* ack = "ACK";
          rf_driver.setHeaderId(2);    // ID reservado para ACK
          rf_driver.send((uint8_t*)ack, strlen(ack));
          rf_driver.waitPacketSent();

          Serial.println("ACK recebido.");
          memset(buf, 0, sizeof(buf));
        }
      }

      xSemaphoreGive(xSemaforo);
      Serial.println("Parou de ler.");
    }
  }
}
