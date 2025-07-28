
//Código de recepção e transmissão
// ESP32 433MHz receiver test 1
#include <RH_ASK.h> 
#include <SPI.h> 

RH_ASK rf_driver(2000, 3, 17); 

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
  rf_driver.setHeaderTo(3);
  rf_driver.setHeaderId(2);
  rf_driver.setThisAddress(2);
  rf_driver.setHeaderFrom(2);
}

void loop() {
  uint8_t buf[50]={0}; 
  uint8_t buflen = sizeof(buf);
  if (rf_driver.recv(buf, &buflen)) {
    Serial.print("Message Received: ");
    Serial.println((char*)buf);
    if (buf[0] == 'E'){
      Serial.println("Transmitting packet");
      buf[strlen((char*)buf)] = '1';
      const char *msg = (char*)buf;
      rf_driver.send((uint8_t *)msg, strlen(msg)+1);
      rf_driver.waitPacketSent();
      delay(1000);
    }
  }
}
