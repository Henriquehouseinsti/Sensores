#include "tasktimer.h"

void recepcao(void *pvParameters) {
  // Recupera os parâmetros compartilhados
  Params_t *params = (Params_t *)pvParameters;
  RH_ASK &rf_driver = *params->rf_driver;
  QueueHandle_t xQueue = params->xQueue;

  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN + 1];  // Buffer para mensagem recebida (+1 para '\0')
  uint8_t buflen;
  unsigned long start = millis();

  while (true) {
    time_t now = time(nullptr);
    int segundos = now;

    // Calcula tempo restante até o próximo múltiplo de 100 segundos (ajustado com +50)
    int delaySegundos = (100 - ((segundos + 50) % 100)) % 100;
    if (delaySegundos == 0) {
      delaySegundos = 100;  // Garante execução apenas uma vez por ciclo
    }

    // Aguarda o momento adequado para ativar a recepção
    vTaskDelay(pdMS_TO_TICKS(delaySegundos * 1000));

    // Tenta adquirir o semáforo antes de acessar o driver RF
    if (xSemaphoreTake(xSemaforo, portMAX_DELAY)) {
      Serial.print("Lendo... ");
      start = millis();

      // Janela de 5 segundos para escutar mensagens
      while (millis() - start < 5000) {
        buflen = RH_ASK_MAX_MESSAGE_LEN;

        // Verifica se uma mensagem foi recebida
        if (rf_driver.recv(buf, &buflen) && rf_driver.headerId() != 2) {
          buf[buflen] = '\0';  // Garante que o buffer seja tratado como string
          Serial.print("Mensagem recebida: ");
          Serial.println((char*)buf);

          delay(50);
          const char* ack = "ACK";
          rf_driver.setHeaderId(2);    // ID reservado para ACK
          rf_driver.setHeaderTo(2);
          rf_driver.send((uint8_t*)ack, strlen(ack));
          rf_driver.waitPacketSent();

          Serial.println("ACK enviado.");

          // Processamento da mensagem recebida (espera-se JSON)
          if (rf_driver.headerId() == 1) {  // Verifica se a mensagem é de dados
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

              // Se necessário, envie os dados para outra task via fila:
              // xQueueSend(xQueue, &doc, portMAX_DELAY);
            }
          }

          // Limpa o buffer para próxima leitura
          memset(buf, 0, sizeof(buf));
        }
      }

      // Libera o semáforo após a janela de recepção
      xSemaphoreGive(xSemaforo);
      Serial.println("Parou de ler.");
    }
  }
}
