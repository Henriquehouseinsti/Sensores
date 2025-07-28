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
  int nivelLuz = analogRead(pinSensorLuz); // Ler o nível de luz
  if (nivelLuz > 2000) { // Ajuste o valor conforme necessário
  Serial.println("Ambiente claro!");
  } else {
  Serial.println("Ambiente escuro!");
  }

  Serial.print("Nivel analógico:");
  Serial.println(nivelLuz);

  delay(1000); // Aguardar 1 segundo antes de repetir

  // Calcular a Escala China
  int EscalaChina;
  EscalaChina = (nivelLuz - 4000) / (-40);
  Serial.print("Escala China:");
  Serial.println(EscalaChina);

  // Converta EscalaChina para string
  char str[20];
  char str2[20];
  sprintf(str, "%d", EscalaChina);
  sprintf(str2, "%d", cont);

  // Concatene a string "Escala china:" com o valor de EscalaChina
  char msg[100]; // Buffer para a mensagem completa
  snprintf(msg, sizeof(msg), "E%s%s0", str, str2);



  // Envia a mensagem via RF
  //Serial.println("Transmitting packet");
  vTaskResume(onda125kHandle);
  rf_driver.send((uint8_t *)msg, strlen(msg) + 1); // +1 para incluir o terminador nulo '\0'
  rf_driver.waitPacketSent();
  vTaskSuspend(onda125kHandle);
  digitalWrite(saidapin, LOW);  // Garante nível baixo
  
  cont++;
  Serial.println(cont);
  if(cont>99) {
    cont=10;
  }
  delay(1000); // Aguardar 1 segundos antes de enviar novamente
}