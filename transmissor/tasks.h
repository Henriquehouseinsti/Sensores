
//Coleta dados sensor e transmite
// ESP8266 433MHz transmitter test 1
#include <RH_ASK.h> 
#include <SPI.h> 
#include <stdio.h>
#include <string.h> // Para usar strcat()

#define pinSensorLuz 4 // Ler o nível de luz

#define leitorpin 22 // Definir um pino analógico adequado
#define saidapin 19 // Definir um pino analógico adequado

void onda125k(void *pvParameters);
