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
#define electrovalvulaPresrevativo 25    //Activa electroválvula que permite paro de aire para inflado de preservativo
#define electrovalvulaFuelle 26    //Activa electroválvula que infla el fuelle para sujetar el preservativo
#define EEPROM_SIZE 420 //Se usaran 20 enteros => 4by * 20    //Utilizado para guardar las constantes de calibración de los sensores de presión
                                                              //Sensor 1 , se guardan pendientes y ordenadas al origen (Se linealiza la curva en 10 segmentos)
                                                              //Sensor 2 , se guardan los valores digitales del AD correspondientes a 1kPa y 2kPa. Aqui solo interesa
                                                              //que el valor se mantenga en este rango durante el ensayo. 
                                                              //por seguridad se duplica la tabla
//Se duplica la información de calibración para poder recuperarla si el checksum falla
#define inicioTabla2 90   //Se duplica la tabla de valores de calibración por si se corrompe la primera, a partir del valor 90                                                              
#define addressEEPROM_1kPa_1  180  //Guarda el valor del AD para 1kPa en el sensor 2 (Fuelle)
#define addressEEPROM_2kPa_1  184  //Guarda el valor del AD para 2kPa en el sensor 2 (Fuelle)
#define addressEEPROM_checksum_1_fuelle 188
#define addressEEPROM_1kPa_2  192  //Guarda el valor del AD para 1kPa en el sensor 2 (Fuelle)
#define addressEEPROM_2kPa_2  196  //Guarda el valor del AD para 2kPa en el sensor 2 (Fuelle)
#define addressEEPROM_checksum_2_fuelle 200
#define flagIntentoRecalibrar 204  // En "1" si ya intento recomponer calibración, en "0" si no   
#define addressEEPROM_ultimaOT 211  //Este utiliza 7 caracteres
//#define direccionLCD    0x27    //Igual a PCF8574_ADDR_A21_A11_A01
#define NUMERO_MAQUINA 1
#define addresspcf8574_KeyPad   0x20    // The PCF8574 is configured to 0x20 I2C address. Check A0, A1 and A2 pins of the device.

#endif