#include "tasktimer.h"

void recepcao(void *pvParameters) {

  // Recupera os objetos do driver e da fila a partir dos parâmetros
  Params_t *params = (Params_t *)pvParameters;
  RH_ASK &rf_driver = *params->rf_driver;
  QueueHandle_t xQueue = params->xQueue;

  // Formato padrão do JSON (não utilizado diretamente neste trecho)
  const char *json = "{\"id\": , \"valor\": , \"now\": }";

  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN] = {0};
  uint8_t buflen = sizeof(buf);
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
      
      Serial.print("Lendo ");

      start = millis();
      while (millis() - start < 5000) {  // Janela de 5 segundos para leitura de mensagens

        // Verifica se uma mensagem foi recebida
        if (rf_driver.recv(buf, &buflen) && rf_driver.headerId() != 2) {

          buf[buflen] = '\0';  // Garante que o buffer seja tratado como string
          Serial.print("Mensagem recebida: ");
          Serial.println((char*)buf);

          uint8_t id = rf_driver.headerId();  // ID da mensagem recebida

          // Configura o driver para envio de ACK
          rf_driver.setHeaderId(2);  // Define ID 2 como ACK

          if (id == 3) {  // Caso a mensagem seja de atualização de RTC

            // Extrai novo RTC (timestamp) dos primeiros 4 bytes recebidos
            time_t novo_rtc =
              ((uint32_t)buf[0] << 24) |
              ((uint32_t)buf[1] << 16) |
              ((uint32_t)buf[2] << 8) |
              (uint32_t)buf[3];

            // Atualiza o RTC do dispositivo
            struct timeval tv = { .tv_sec = novo_rtc, .tv_usec = 0 };
            settimeofday(&tv, nullptr);

            Serial.print("RTC sincronizado para: ");
            Serial.println(novo_rtc);

            // Envia mensagem de ACK
            const char* ack = "ACK";
            rf_driver.setHeaderId(2);   // ID reservado para ACK
            rf_driver.setHeaderTo(2);   // Define o destinatário correto (ID do transmissor)
            rf_driver.send((uint8_t*)ack, strlen(ack));
            rf_driver.waitPacketSent();

            Serial.println("ACK enviado.");
          }
        }
      }

      // Libera o semáforo após finalizar a janela de leitura
      xSemaphoreGive(xSemaforo);

      Serial.println("Parou de ler.");

      // Limpa o buffer para a próxima leitura
      memset(buf, 0, sizeof(buf));
    
    }
  }
}
