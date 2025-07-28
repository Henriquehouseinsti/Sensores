#include "tasktimer.h"

void dados_task(void *pvParameters) {
  // Recupera o driver RF e a fila a partir dos parâmetros compartilhados
  Params_t *params = (Params_t *)pvParameters;
  RH_ASK &rf_driver = *params->rf_driver;  // (não usado nesta task diretamente)
  QueueHandle_t xQueue = params->xQueue;

  while (true) {
    time_t now = time(nullptr);
    int segundos = now % 60;

    // Calcula o tempo restante até o próximo múltiplo de 200 segundos
    int delaySegundos = (199 - (segundos % 200)) % 200;
    if (delaySegundos == 0) {
      delaySegundos = 200;
    }

    // Aguarda até o próximo ciclo de envio de dados
    vTaskDelay(pdMS_TO_TICKS(delaySegundos * 1000));

    Serial.println("Task de coleta de dados rodando");

    // Atualiza o timestamp atual
    now = time(nullptr);

    // Leitura do sensor (ex.: sensor de luz no pino 4)
    char jsonBuffer[256];
    StaticJsonDocument<256> json_mensagem;

    json_mensagem["sensor"] = "luz";
    json_mensagem["valor"] = analogRead(4);  // Leitura do sensor
    json_mensagem["now"] = now;              // Timestamp atual

    // Converte o JSON em string
    serializeJson(json_mensagem, jsonBuffer);

    // Envia o JSON para a fila (para que outra task envie via RF)
    xQueueSend(xQueue, jsonBuffer, 0);
  }
}
