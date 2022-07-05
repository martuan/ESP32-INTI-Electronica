#ifndef hardware_h
#define hardware_h

#define DHTPIN 32               // Pin donde está conectado el sensor de temperatura 
#define LED_BUILTIN 2           // Parpadea si no encuentra la tarjeta micro SD
#define sensorPresion1 34       // Sensa la presión dentro del preservativo
#define sensorPresion2 35       // Sensa la presión dentro del fuelle de sujeción 
#define pulsadorCalibracion 13  // Si bootea con el pulsador presionado entra en modo calibración de sensores de presión

#define DHTTYPE DHT22           // Sensor DHT22
#define CS 5                   //Pin de Chip Select para escribir el SD
#define pulsadorInicio 4       //Pulsador de inicio de ensayo. Parada durante el ensayo. Guardar valor durante calibración 
#define activarElectrovalvula 25    //Activa electroválvula que permite paro de aire para inflado de preservativo y fuelle
#define EEPROM_SIZE 420 //Se usaran 20 enteros => 4by * 20    //Utilizado para guardar las constantes de calibración de los sensores de presión
                                                              //Sensor 1 , se guardan pendientes y ordenadas al origen (Se linealiza la curva en 10 segmentos)
                                                              //Sensor 2 , se guardan los valores digitales del AD correspondientes a 1kPa y 2kPa. Aqui solo interesa
                                                              //que el valor se mantenga en este rango durante el ensayo. 
#define addressEEPROM_1kPa  80  //Guarda el valor del AD para 1kPa en el sensor 2 (Fuelle)
#define addressEEPROM_2kPa  84  //Guarda el valor del AD para 2kPa en el sensor 2 (Fuelle)
#define direccionLCD    0x27    //Igual a PCF8574_ADDR_A21_A11_A01
#define NUMERO_MAQUINA 1

#endif