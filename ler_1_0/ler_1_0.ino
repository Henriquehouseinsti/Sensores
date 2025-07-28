
//Código de recepção e transmissão
// ESP32 433MHz receiver test 1
#include <RH_ASK.h> 
#include <SPI.h> 

RH_ASK rf_driver(1000, 3, 17); 

void setup() {
  Serial.begin(115200);
  delay(4000);
  Serial.println("ESP32 433MHz receiver");
  if (RH_PLATFORM == RH_PLATFORM_ESP32)
  Serial.println("RH_PLATFORM_ESP32");
  delay(5000);
  Serial.println("Receiver: rf_driver initialising");
  if (!rf_driver.init()) {
    Serial.println("init failed");
    while (1) delay(1000);
  }
  Serial.println("Receiver: rf_driver initialised");
}

void loop() {
  if (digitalRead(18) == HIGH) {
    Serial.println("1");
  } else {
    Serial.println("0");
  }
  delay(1); // 0.5 ms

}
