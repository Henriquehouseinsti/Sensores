#include "tasks.h"

void onda125k(void *pvParameters) {
  pinMode(leitorpin, INPUT);

  // Configura canal PWM 0 com frequência 125kHz e resolução 8 bits
  const int canalPWM = 0;
  const int frequencia = 125000;
  const int resolucao = 8;

  ledcSetup(canalPWM, frequencia, resolucao);
  ledcAttachPin(saidapin, canalPWM);

  while (true) {
    if (digitalRead(leitorpin) == HIGH) {
      // Ativa a onda quadrada (duty cycle 50% = 127)
      ledcWrite(canalPWM, 127);
    } else {
      // Desliga a saída
      ledcWrite(canalPWM, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // Pequeno atraso para não sobrecarregar o loop
  }
}