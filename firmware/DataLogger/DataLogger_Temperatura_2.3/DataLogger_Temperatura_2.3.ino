/*
 * HADWARE: 
 * Pin de Chip Select para escribir el SD : Pin 5
 * Pulsador inicio ensayo: Pin 4
 * Salida para activar electroválvula: Pin 25
 * Sensor de temperatura one wire: Pin 32
 * I2C :Pines SDA:21 y CLK:22
 * Sensor de presión 1 (entrada analógica): Pin 34
 * Sensor de presión 2 (entrada analógica): Pin 35
 18/03/22 -> Se genera un archivo cada vez que se presiona el pulsador se genera un archivo .csv para almacenar los datos cuyo nombre 
 se crea en formato ISO8601 hora local (yyyymmddThhmmss), tomando fecha y hora del RTC
*/

#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;
#include <SPI.h>
#include <SD.h>
// File myFile;
#include "DHT.h"
#include "FS.h"

#define DHTPIN 32     // Pin donde está conectado el sensor de temperatura 
#define LED_BUILTIN 2
#define sensorPresion1 34
#define sensorPresion2 35

#define DHTTYPE DHT22        // Sensor DHT22
//#define intervalo 1000      // Cantidad de milisegundos
//#define segundosLimite 10   //Cantidad de segundos entre medición y registro de temperatura
DHT dht(DHTPIN, DHTTYPE);

#define CS 5       //Pin de Chip Select para escribir el SD
#define pulsadorInicio 4
#define activarElectrovalvula 25

int archivoAbierto = 0; // En "1" se pudo abrir, en "0", No se pudo abrir

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.println();
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("File appended");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
//    Serial.printf("Appending to file: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
//        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pulsadorInicio, INPUT);
  pinMode(activarElectrovalvula, OUTPUT);
  
  //********************************************
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  if (!SD.begin(CS)) {
     Serial.println("inicialización fallida!"); 
     while(!SD.begin(CS)){
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(500);                       // wait for a half second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        delay(500);                       // wait for a half second
     }
  //  return;
  } else {
    Serial.println("initialization done.");
  }
  
   uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
  // **************************************************
  if (! rtc.begin()) {
    //   Serial.println("Couldn't find RTC");
    while (1);
  }

  //rtc.adjust(DateTime(__DATE__, __TIME__)); // which will set the time according to our PC time.

  if (rtc.lostPower()) {
    //    Serial.println("RTC lost power, lets set the time!");
  }
  //****************************************************
    dht.begin();    //Inicializar sensor de temperatura
 // dht.setup(dhtPin, DHTesp::DHT11);
}

  void imprimirPC(DateTime now, float temp, float humed){
      Serial.println("Fecha y hora: ");
      Serial.print(now.year(), DEC);
      Serial.print('/');
      Serial.print(now.month(), DEC);
      Serial.print('/');
      Serial.print(now.day(), DEC);
      Serial.print("   ");
      Serial.print(now.hour(), DEC);
      Serial.print(':');
      Serial.print(now.minute(), DEC);
      Serial.print(':');
      Serial.print(now.second(), DEC);
      Serial.println();
      Serial.print("Temperatura: ");
      Serial.print(temp);
      Serial.print("        ");
      Serial.print("Humedad: ");
      Serial.print(humed);
      Serial.println();
  }

boolean  tiempoTranscurrido(boolean ejecutado){
  const int intervalo = 10;      // Cantidad de milisegundos
  const int intervalosLimite = 30;   //Cantidad de intervalos entre medición y registro de datos (Presión y caudal)
  static boolean tiempoCumplido;
  static unsigned long previousMillis = 0;
  static unsigned long currentMillis = 0;
  static int intervalosContados = 0;

  if(ejecutado){
    intervalosContados = 0;
  }

   currentMillis = millis();
  if (currentMillis - previousMillis >= intervalo) {
    previousMillis = currentMillis; 
    intervalosContados++; 
  }  
  if(intervalosContados >= intervalosLimite){
    tiempoCumplido = true;
//    intervalosContados = 0;
  }else{
    tiempoCumplido = false;
  }
  if(tiempoCumplido == true)  return(true);
  else  return(false);
}

float obtenerPresion1(void){
  int lecturaAD1;
  float presion1;
  lecturaAD1 = analogRead(sensorPresion1);
  presion1 = (lecturaAD1*10)/4096;

  return presion1;
}

void rutinaEnsayo(String nombreArchivo){
  const int timeOut = 30;
  int segundos = 0;
  char medicion[5];
  String lineaMedicion = "";
  float presion1 = 0;
  float presion1Anterior = 0;
  boolean registrarDatos = false;

  digitalWrite(activarElectrovalvula, HIGH);
  while(segundos < timeOut){
    registrarDatos = tiempoTranscurrido(false);
    if(registrarDatos){
       presion1 = obtenerPresion1();
       sprintf(medicion, "%d.%02d", (int)presion1, (int)(fabs(presion1)*100)/100);
       lineaMedicion += String(segundos);
       lineaMedicion += ",  ";
       lineaMedicion += String(medicion);
       lineaMedicion += "\r";
       appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());    
       segundos ++;
       Serial.println(lineaMedicion);
       lineaMedicion = "";
       registrarDatos = tiempoTranscurrido(true);
   }
   if(presion1 < (presion1Anterior * 0.7))  segundos = 270;
   presion1Anterior = presion1;
   if(segundos == timeOut){
      lineaMedicion = "Fin de ensayo por Time Out \r"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());    
      Serial.println("Fin de ensayo por Time Out");
   }     
  }
  digitalWrite(activarElectrovalvula, LOW);
}

String rutinaInicioEnsayo(){
    String nombreArchivo = "";
    String cabecera = "";
    
    float temp = dht.readTemperature(); //Leemos la temperatura en grados Celsius
    float humed = dht.readHumidity(); //Leemos la Humedad
    DateTime now = rtc.now();
    
    char dateTimeISO[17];
    sprintf(dateTimeISO, "%02d%02d%02dT%02d%02d%02d", now.year(), now.month(),now.day(),  now.hour(), now.minute(), now.second()); 
    nombreArchivo += "/";
    nombreArchivo += String(dateTimeISO);
    nombreArchivo += ".csv";
    writeFile(SD, nombreArchivo.c_str(), "Fecha  y  hora, Temperatura, Humedad \r\n");
    
    char dateTime[20];
    sprintf(dateTime, "%02d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(),  now.hour(), now.minute(), now.second()); 
    cabecera += String(dateTime);
    cabecera += "  ,";
    cabecera += String(temp);
    cabecera += "  ,";
    cabecera += String(humed);
    cabecera += "\r\n";
    imprimirPC(now, temp, humed);
    appendFile(SD, nombreArchivo.c_str(), cabecera.c_str());
    String str = nombreArchivo.c_str();  

  return str; 
}
  
void loop() {
  boolean tiempoCumplido;
  String inicio = "";           //Cuando se inicia la placa Impacto envía al inicio de la comunicación "ini")
  static unsigned long previousMillis = 0;
  static unsigned long currentMillis = 0;
  String nombreArchivo = "";
  boolean estadoPulsadorInicio;
 
  while (Serial.available() > 0) {    // Es para lectura del puerto serie
 
  }

 
   estadoPulsadorInicio = digitalRead(pulsadorInicio);
   if(estadoPulsadorInicio == LOW){
   nombreArchivo = rutinaInicioEnsayo();
   rutinaEnsayo(nombreArchivo);
    
 //   float temp = dht.readTemperature(); //Leemos la temperatura en grados Celsius
//    float humed = dht.readHumidity(); //Leemos la Humedad
 //   DateTime now = rtc.now();
 //   delay(100);
  }
      
 
}
