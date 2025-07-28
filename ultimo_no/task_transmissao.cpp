#include "tasktimer.h"

void transmissor(void *pvParameters) {
  Serial.println("Task de transmissão iniciada");

  // Recupera os objetos do driver e da fila a partir dos parâmetros
  Params_t *params = (Params_t *)pvParameters;
  RH_ASK &rf_driver = *params->rf_driver;
  QueueHandle_t xQueue = params->xQueue;

  // Variáveis auxiliares
  char valor_transmitindo[256];
  int count = 0;
  const uint8_t maxTentativas = 3;

  while (true) {
    time_t now = time(nullptr);
    int segundos = now;

    // Calcula o tempo restante até o próximo múltiplo de 200 segundos
    int delaySegundos = (200 - (segundos % 200)) % 200;

    vTaskDelay(pdMS_TO_TICKS(delaySegundos * 1000));


    Serial.println("Preparando dados para transmissão...");

    // Atualiza o horário atual
    now = time(nullptr);

    // Lê o sensor (pino 4) e monta a mensagem JSON
    char jsonBuffer[256];
    StaticJsonDocument<256> json_mensagem;

    json_mensagem["sensor"] = "luz";
    json_mensagem["valor"] = analogRead(4);
    json_mensagem["now"] = now;

    // Converte o JSON para string
    serializeJson(json_mensagem, jsonBuffer);

    // Prepara a mensagem a ser transmitida
    strcpy(valor_transmitindo, jsonBuffer);

    // Aguarda o semáforo para controlar o acesso ao driver RF
    if (xSemaphoreTake(xSemaforo, portMAX_DELAY)) {
      bool ackReceived = false;
      count = 0;

      // Loop de retransmissão (caso ACK não seja recebido)
      while (!ackReceived) {
        // Configura cabeçalhos do pacote
        rf_driver.setHeaderId(1);  // ID da mensagem: 1 = dados
        rf_driver.setHeaderTo(2);  // Destinatário (por exemplo, ID do receptor)

        // Envia a mensagem
        rf_driver.send((uint8_t *)valor_transmitindo, strlen(valor_transmitindo) + 1);
        rf_driver.waitPacketSent();
        Serial.println("Mensagem enviada:");
        Serial.println(valor_transmitindo);

        // Aguarda ACK (até 3000 ms)
        uint8_t buf[RH_ASK_MAX_MESSAGE_LEN] = {0};
        uint8_t buflen = sizeof(buf);
        unsigned long start = millis();

        while (millis() - start < 3000) {
          buflen = sizeof(buf);
          if (rf_driver.recv(buf, &buflen)) {
            if (rf_driver.headerId() == 2 && rf_driver.headerFrom() == 2) {
              Serial.println((char*)buf);
              ackReceived = true;
            }
          }
        }

        if (!ackReceived) {
          Serial.println("ACK não recebido. Reenviando...");
          count++;
        }

        if (count >= maxTentativas) {
          Serial.println("Falha após 3 tentativas de envio.");
          break;
        }
      }

      // Libera o semáforo após o envio/retransmissão
      xSemaphoreGive(xSemaforo);    
    }
  }
}
