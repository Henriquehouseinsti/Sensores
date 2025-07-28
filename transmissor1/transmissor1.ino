#include "tasks.h"


RH_ASK rf_driver(1000, 3, 17); // Velocidade, RX, TX

TaskHandle_t onda125kHandle = NULL;



void setup() {

  Serial.begin(115200);
  Serial.println("Esperando o sensor aquecer um pouco...");
  delay(5000);
  Serial.println("Fim do setup()");

  Serial.println("ESP8266 433MHz transmitter");
  if (!rf_driver.init()) {
  Serial.println("init failed");
  while (1) delay(10000);
  }
  Serial.println("Transmitter: rf_driver initialised");
  rf_driver.setModeIdle();
  rf_driver.setHeaderId(1);
  rf_driver.setHeaderTo(2);
  rf_driver.setHeaderFrom(1);

  xTaskCreate(
    onda125k,         // função da task
    "onda_125kHz",    // nome da task
    2048,             // stack size
    NULL,             // parâmetros (nenhum)
    1,                // prioridade
    &onda125kHandle              // handle (não utilizado)
  );
}
int cont = 10;
void loop() {  
  digitalWrite(17, HIGH);
  delay(1); // 0.5 ms
  digitalWrite(17, LOW);
  delay(1); // 0.5 ms
}