/*
 * HADWARE: 
 * Pin de Chip Select para escribir el SD : Pin 5
 * Pulsador inicio ensayo: Pin 4
 * Pulsador calibracion: Pin 13
 * Salida para activar electroválvula: Pin 25
 * Sensor de temperatura one wire: Pin 32
 * I2C :Pines SDA:21 y CLK:22
 * Sensor de presión 1 (entrada analógica): Pin 34
 * Sensor de presión 2 (entrada analógica): Pin 35
 * SOFTWARE
 18/03/22 -> Se genera un archivo cada vez que se presiona el pulsador se genera un archivo .csv para almacenar los datos cuyo nombre 
 se crea en formato ISO8601 hora local (yyyymmddThhmmss), tomando fecha y hora del RTC
 Se genera un archivo donde se almacenan el último valor de cada ensayo (se crea una vez si no existe) con el nombre Maquina_<NUMERO_MAQUINA>.csv . Cada máquina de ensayos tendrá su número

 Comandos Puerto Serie:
 Se puede setear el reloj de tiempo real (RTC) desde el puerto serie (115200 baud, 8 bits, sin paridad, un stop bit. Enviar setRTC:año mes dia hora minutos segundos (todo junto sin espacios)
 Ej.:Comando por puero serie: setRTC:20210213163218  -> año:2021, mes: 02, día: 13, hora: 16, minutos: 32, segundos: 18
 
 Enviando "leerDir:" devuelve el directorio Raiz de la SD.
 Enviando "leerArchivo:<Nombre de Archivo>" devuelve el contenido del archivo (UTF-8). Si el archivo esta dentro de un subdirectorio especificar ruta. Ej.: "leerArchivo:/20220601/20220601T083911.csv" 
 Enviando "leerSubDir:<Nombre de directorio>" devuelve el contenido del directorio. Ej.: Enviando "leerSubDir:/20220601" devuelve el contenido dentro del directorio /20220601
*/

#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;
#include <SPI.h>
#include <SD.h>
#include "DHT.h"
#include "FS.h"
#include <EEPROM.h>

#define DHTPIN 32               // Pin donde está conectado el sensor de temperatura 
#define LED_BUILTIN 2           // Parpadea si no encuentra la tarjeta micro SD
#define sensorPresion1 34       // Sensa la presión dentro del preservativo
#define sensorPresion2 35       // Sensa la presión dentro del fuelle de sujeción 
#define pulsadorCalibracion 13  // Si bootea con el pulsador presionado entra en modo calibración de sensores de presión

#define DHTTYPE DHT22           // Sensor DHT22
DHT dht(DHTPIN, DHTTYPE);

#define CS 5                   //Pin de Chip Select para escribir el SD
#define pulsadorInicio 4       //Pulsador de inicio de ensayo. Parada durante el ensayo. Guardar valor durante calibración 
#define activarElectrovalvula 25    //Activa electroválvula que permite paro de aire para inflado de preservativo y fuelle
#define EEPROM_SIZE 420 //Se usaran 20 enteros => 4by * 20    //Utilizado para guardar las constantes de calibración de los sensores de presión
                                                              //Sensor 1 , se guardan pendientes y ordenadas al origen (Se linealiza la curva en 10 segmentos)
                                                              //Sensor 2 , se guardan los valores digitales del AD correspondientes a 1kPa y 2kPa. Aqui solo interesa
                                                              //que el valor se mantenga en este rango durante el ensayo. 
#define addressEEPROM_1kPa  80  //Guarda el valor del AD para 1kPa en el sensor 2 (Fuelle)
#define addressEEPROM_2kPa  84  //Guarda el valor del AD para 2kPa en el sensor 2 (Fuelle)
#define NUMERO_MAQUINA 1


//Rutina que crea en archivo de registro de nuevo ensayo, lo nombra según formato ISO8601 y Escribe dentro de el el encabezado del ensayo
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

void calibracionSensoresPresion(void);
void  leerEEPROM(void);

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pulsadorInicio, INPUT);
  pinMode(activarElectrovalvula, OUTPUT);
  pinMode(pulsadorCalibracion, INPUT);

  boolean calibracion;
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
  // **********************************************************************
  if (! rtc.begin()) {
    //   Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    //    Serial.println("RTC lost power, lets set the time!");
  }
  //**********************************************************************
    dht.begin();    //Inicializar sensor de temperatura
 // dht.setup(dhtPin, DHTesp::DHT11);
  EEPROM.begin(EEPROM_SIZE);
  leerEEPROM();
  
  String str = "/Maquina_";              //El nombre del archivo corresponde al número de máquina
  str += String(NUMERO_MAQUINA);
  File archivo = SD.open(str.c_str());   //Si no existe el archivo lo crea. Aqui se guardan el último dato de cada ensayo ya sea, tiempo y presión de reventado, 
  if(!archivo) {                              //fin por timeout, presión de fuelle fuera de rango o parada por usuario (pulsador inicio)
  writeFile(SD, str.c_str(), "Nombre archivo, Segundos, Estado final\r");
  }    

}
//******************************************************************************************************************
void  leerEEPROM(){
  //Read data from eeprom
  int address = 0;
  int cont = 0;
  int readId;
  while(cont<10){
    readId = EEPROM.readInt(address); //EEPROM.get(address,readId);
    Serial.print("Read m = ");
    Serial.println(readId);
    address += sizeof(readId); //update address value  
    readId = EEPROM.readInt(address); //EEPROM.get(address,readId);
    Serial.print("Read b = ");
    Serial.println(readId);
    address += sizeof(readId); //update address value  
    cont ++;    
  }
    Serial.print("Sensor Nº 2: ");
    readId = EEPROM.readInt(addressEEPROM_1kPa); 
    Serial.println(readId);
    readId = EEPROM.readInt(addressEEPROM_2kPa); 
    Serial.println(readId);
}
//******* Calibración ***********************************************************************
void calibracionSensoresPresion(){
      int numeroSensor = 1;
      int puntoCalibracion = 0;
      int valorAcumulado = 0;
      int promedio;
      int valorDigitalCalibrado[10];
      const float valorCalibracion[10] = {0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.25, 2.50};
      int m = 0;
      int b = 0;
      int muestraPorPunto = 0;
      int addressEEPROM = 0;
      int cont = 0;
      int valorPresionFuelle;
      boolean inicio = HIGH;
      boolean calibracion;
      boolean cambioSensor = LOW; 
      Serial.println("Con pulsador CALIBRACION seleccione sensor: ");
      Serial.print("Sensor seleccionado: ");
      Serial.println(numeroSensor);
      Serial.println("Pulse INICIO para comenzar calibración");        
      delay(600); 
      while(inicio == HIGH){
      calibracion = digitalRead(pulsadorCalibracion);
      if(calibracion == LOW){
        if(numeroSensor == 1){
        numeroSensor = 2;
        }else{
        numeroSensor = 1;         
        }
        delay(600);  
        cambioSensor = HIGH;      
      }
      if(cambioSensor == HIGH){
        Serial.print("Sensor seleccionado: ");
        Serial.println(numeroSensor);
        Serial.println("Pulse INICIO para comenzar calibración");    
        cambioSensor = LOW;       
      }
      inicio = digitalRead(pulsadorInicio);
     }
     delay(1000);
     if(numeroSensor == 1){
       while(puntoCalibracion < 10){
        while(muestraPorPunto < 3){
          Serial.print(muestraPorPunto + 1);
          Serial.print("º. Desde Cero suba la presión hasta ");
          Serial.print(valorCalibracion[puntoCalibracion]);
          Serial.println("kPa  y presione INICIO");
          inicio = digitalRead(pulsadorInicio);
          while(inicio == HIGH){
            inicio = digitalRead(pulsadorInicio);
            }
          valorAcumulado = analogRead(sensorPresion1) + valorAcumulado;
          promedio = valorAcumulado / 3;
          muestraPorPunto ++;
          delay(600);
          }
        muestraPorPunto = 0;
        valorDigitalCalibrado[puntoCalibracion] = promedio;
        valorAcumulado = 0;
        puntoCalibracion ++;
       }
       
        m = int((valorDigitalCalibrado[1] - valorDigitalCalibrado[0])/0.25);
  //      m = int((valorDigitalCalibrado[1] - valorDigitalCalibrado[0])/(valorCalibracion[1] - valorCalibracion[0]));
        b = int(valorDigitalCalibrado[0] - m*valorCalibracion[0]);
  
        //Se guadan las constantes de calibración como enteros (4By c/u)
        EEPROM.writeInt(addressEEPROM, m);       //Escribir en EEPROM m en posición 0
        EEPROM.commit();
        addressEEPROM += sizeof(m); //update address value
        EEPROM.writeInt(addressEEPROM, b);       //Escribir en EEPROM b en posición 4
        EEPROM.commit();
        addressEEPROM += sizeof(b); //update address value
  
       while(cont<9){
  //      m = int((valorDigitalCalibrado[cont+1] - valorDigitalCalibrado[cont])/(valorCalibracion[cont+1] - valorCalibracion[cont]));
        m = int((valorDigitalCalibrado[cont+1] - valorDigitalCalibrado[cont])/0.25);
        b = int(valorDigitalCalibrado[cont] - m*valorCalibracion[cont]);
  
        EEPROM.writeInt(addressEEPROM, m);       //Escribir en EEPROM m y b en posicion addressEEPROM
        EEPROM.commit();
        addressEEPROM += sizeof(m); //update address value
        EEPROM.writeInt(addressEEPROM, b);       //Escribir en EEPROM b en posicion addressEEPROM
        EEPROM.commit();
        addressEEPROM += sizeof(b); //update address value
        cont ++;
       }       
     }
     if(numeroSensor == 2){
        Serial.println("Suba la presión a 1kPa  y presione INICIO");
        inicio = HIGH;          
        while(inicio == HIGH){
          inicio = digitalRead(pulsadorInicio); 
          }
        valorPresionFuelle = analogRead(sensorPresion2);
        EEPROM.writeInt(addressEEPROM_1kPa, valorPresionFuelle);       //Escribir en EEPROM b en posicion addressEEPROM
        EEPROM.commit();
        Serial.print("El valor para 1kPa es: ");         
        Serial.println(valorPresionFuelle);
        delay(800);
        Serial.println("Suba la presión a 2kPa  y presione INICIO");
        inicio = HIGH;          
        while(inicio == HIGH){
          inicio = digitalRead(pulsadorInicio);          
          }
        valorPresionFuelle = analogRead(sensorPresion2);
        EEPROM.writeInt(addressEEPROM_2kPa, valorPresionFuelle);       //Escribir en EEPROM b en posicion addressEEPROM
        EEPROM.commit();
        Serial.print("El valor para 2kPa es: ");         
        Serial.println(valorPresionFuelle);
        delay(800);                   
     }
     leerEEPROM();  
}
//******************** Fin calibración ************************************************************
  void imprimirPC(DateTime now, float temp, float humed){
      char dateTime[22];
      char tempHumed[46];
      sprintf(dateTime, "%02d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(),now.day(),  now.hour(), now.minute(), now.second()); 
      Serial.println("Fecha y hora: ");
      Serial.print(dateTime);
      Serial.println();
      Serial.print("Temperatura: ");
      Serial.print(temp);
      Serial.print("        ");
      Serial.print("Humedad: ");
      Serial.print(humed);
      Serial.println();
  }
//*******************************************************************************
boolean  tiempoTranscurrido(boolean ejecutado){
  const int intervalo = 10;      // Cantidad de milisegundos
  const int intervalosLimite = 60;   //Cantidad de intervalos entre medición y registro de datos (Presión y caudal). intervalo * intervalosLimite = 1 segundo
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
//***************************************************************************************
float obtenerPresion1(void){
  int lecturaAD1;
  float presion1;
  int b, m;
  int address = 0;
  int cont = 0;
  int ultimaLecturaEEPROM = 1;

  lecturaAD1 = analogRead(sensorPresion1);
   
  ultimaLecturaEEPROM = 9;  //En función de la lectura del AD, se determina que datos de calibración utiluzar
  if(lecturaAD1 < 1220) { ultimaLecturaEEPROM = 8; }
  if(lecturaAD1 < 1075) { ultimaLecturaEEPROM = 7; }
  if(lecturaAD1 < 800) { ultimaLecturaEEPROM = 5; }
  if(lecturaAD1 < 937) { ultimaLecturaEEPROM = 6; }
  if(lecturaAD1 < 680) { ultimaLecturaEEPROM = 4; }
  if(lecturaAD1 < 560) { ultimaLecturaEEPROM =3; }
  if(lecturaAD1 < 440) { ultimaLecturaEEPROM = 2; }
  if(lecturaAD1 < 320) { ultimaLecturaEEPROM = 1; }

  while(cont < ultimaLecturaEEPROM){
    m = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    address += sizeof(m); //update address value  
    b = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    address += sizeof(b); //update address value  
    cont ++;    
  }

  presion1 = (float(lecturaAD1) - float(b))/float(m);
 
  if(presion1 < 0)  presion1 = 0;

  return presion1;
}
//***********************************************************************************
void rutinaEnsayo(String nombreArchivo){
  const int timeOut = 30;
  int segundos = 0;
  char medicion[5];
  String lineaMedicion = "";
  String lineaMedicionAnterior = "";  //Para no guardar en Maquina_1.csv el valor de presión durante loa caída. Interesa guardar la máxima antes del reventado
  String lineaMedicionNombreArchivo = ""; //Para escribir la última medición en Maquina_1 incluyendo el nombre del archivo
  String escribeUltimaMedicion = "";  //Sirve para registrar la máxima presión alcanzada antes de la ruptura.
  float presion1 = 0;               //Presión medida en preservativo
  float presion1Anterior = 0;       //Presión medida en el ciclo anterior en preservativo
  boolean registrarDatos = LOW;     //Se pone en HIGH cuando la función tiempoTranscurrido() indica el momento de tomar medición, para no usar delay() bloqueante
  int presion1PartEntera = 0;       //Separo el valor real de presión para poder meterlo en un String
  int presion1Partdecimal = 0;      //Separo el valor real de presión para poder meterlo en un String
  int valorAD_minimo_fuelle, valorAD_maximo_fuelle;
  boolean inicio = HIGH;            //Para verificar s se presionó el pulsador INICIO durante el ensayo y detener el mismo (parada de emergencia)
  int valorAD_fuelle = 0;
  boolean bajaPresionFuelle = LOW;
  boolean altaPresionFuelle = LOW;
  boolean guardarLineaMedicionAnterior = false;   //Sirve para registrar la máxima presión alcanzada antes de la ruptura.
  boolean superoPresionMinima = false;            //Por debajo de 0.1 kPa tiene mucho error y no se considera la medicion.
  String nombreMaquina = "/Maquina_";   
             
  nombreMaquina += String(NUMERO_MAQUINA);        //El nombre del archivo corresponde al número de máquina
  nombreMaquina += ".csv";

  valorAD_minimo_fuelle = EEPROM.readInt(addressEEPROM_1kPa); //Valor de presión mínimo (en entero del AD) en el fuelle (sensor de presión 2)   
  valorAD_maximo_fuelle = EEPROM.readInt(addressEEPROM_2kPa); //Valor de presión máximo (en entero del AD) en el fuelle (sensor de presión 2) 

  digitalWrite(activarElectrovalvula, HIGH);
  delay(200);
  while(segundos < timeOut){
    registrarDatos = tiempoTranscurrido(false);
    if(registrarDatos){
       presion1 = obtenerPresion1();            //Se obtiene la presión en el preservativo
       presion1PartEntera = int(presion1);      //sprintf no formatea float, entonces separo partes entera y decimal
       presion1Partdecimal = int((presion1 - presion1PartEntera) * 100);
       sprintf(medicion, "%d.%02d", (int)presion1, presion1Partdecimal);
       lineaMedicion += String(segundos);
       lineaMedicion += ",  ";
       lineaMedicion += String(medicion);
       lineaMedicion += "\r\n";
       Serial.print(lineaMedicion);         //Si se quita el envío al puerto serie, agregar delay(10) para que escriba bien en la sd el fin de línea
       appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());   
       segundos ++;
//       Serial.println(lineaMedicion);
       registrarDatos = tiempoTranscurrido(true);
       guardarLineaMedicionAnterior = true;
   }
   if(presion1 > 0.1)  { superoPresionMinima = true; }
   if(superoPresionMinima == true){
       if(presion1 < (presion1Anterior * 0.6)){       //Si la presión cae un 40% se asume que el preservativo reventó
          escribeUltimaMedicion += nombreArchivo;
          escribeUltimaMedicion += ", ";
          escribeUltimaMedicion += lineaMedicionAnterior;
          
          appendFile(SD, nombreMaquina.c_str(), escribeUltimaMedicion.c_str());
          segundos = timeOut + 1;       //No vuenve a entrar en  while(segundos < timeOut) ni entra en if(segundos == timeOut)
         Serial.print("Linea Medición anterior: ");
         Serial.println(lineaMedicionAnterior);
       }    
   }
   
  if(guardarLineaMedicionAnterior == true){   //Sirve para guardar la máxima presión en el preservativo antes del reventado
     presion1Anterior = presion1;
     lineaMedicionAnterior = " ";
     lineaMedicionAnterior += lineaMedicion;
     guardarLineaMedicionAnterior = false;
  }
   lineaMedicion = "";
   lineaMedicion += String(segundos);
   lineaMedicion += ", ";

   lineaMedicionNombreArchivo = "";
   lineaMedicionNombreArchivo += nombreArchivo; 
   lineaMedicionNombreArchivo += ", ";
   lineaMedicionNombreArchivo += String(segundos);
   lineaMedicionNombreArchivo += ", ";

   if(segundos == timeOut){
      lineaMedicion += "Fin de ensayo por Time Out \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por Time Out \r\n"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo
      Serial.println("Fin de ensayo por Time Out");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
   }     
   
   //  delay(200);
   inicio = digitalRead(pulsadorInicio);
   if(inicio == LOW){
      lineaMedicion += "Fin de ensayo por parada de usuario \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por parada de usuario \r\n"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo
      Serial.println("Fin de ensayo por parada de usuario");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
      segundos = timeOut + 1;         //Para que salga del While
      digitalWrite(activarElectrovalvula, LOW);
      delay(1500);
   }

    valorAD_fuelle = analogRead(sensorPresion2);
    if(valorAD_fuelle < valorAD_minimo_fuelle)   bajaPresionFuelle = HIGH;
    if(valorAD_fuelle > valorAD_maximo_fuelle)   altaPresionFuelle = HIGH;

    if(bajaPresionFuelle == HIGH){
      lineaMedicion += "Fin de ensayo por baja presión en fuelle \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por baja presión en fuelle \r\n"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo    
      Serial.println("Fin de ensayo por baja presión en fuelle");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
      segundos = timeOut + 1;       //Para que salga del While
      delay(300);
    }
    if(altaPresionFuelle == HIGH){
      lineaMedicion += "Fin de ensayo por alta presión en fuelle \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por alta presión en fuelle \r\n"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());               //Escribe última medición antes de reventado en archivo de este ensayo  
      Serial.println("Fin de ensayo por alta presión en fuelle");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());  //Escribe última medición antes de reventado en archivo Maquina_#
      segundos = timeOut + 1;       //Para que salga del While
      delay(300);
    }
    lineaMedicion = "";
  }
  digitalWrite(activarElectrovalvula, LOW);
}
//********************************************************************************
void createDir(fs::FS &fs, const char * path){
  Serial.printf("Creating Dir: %s\n", path);
  if(fs.mkdir(path)){
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}
//********************************************************************************
String rutinaInicioEnsayo(){
    String nombreArchivo = "";
    String cabecera = "";
    String nombreDirectorio = "";
    boolean directorioExistente = false;
    
    float temp = dht.readTemperature(); //Leemos la temperatura en grados Celsius
    float humed = dht.readHumidity(); //Leemos la Humedad
    DateTime now = rtc.now();
    
    char dateISO[9];
    sprintf(dateISO, "%02d%02d%02d", now.year(), now.month(),now.day()); 
    nombreDirectorio += "/";
    nombreDirectorio += String(dateISO);
    directorioExistente = SD.exists(nombreDirectorio.c_str());
    if(directorioExistente){
      
    }else{
      createDir(SD,nombreDirectorio.c_str());
    }
//    char dateTimeISO[17];
    char dateTimeISO[26];
    sprintf(dateTimeISO, "%02d%02d%02dT%02d%02d%02d", now.year(), now.month(),now.day(),  now.hour(), now.minute(), now.second()); 
    nombreArchivo += nombreDirectorio;
    nombreArchivo += "/";
    nombreArchivo += String(dateTimeISO);
    nombreArchivo += ".csv";
    writeFile(SD, nombreArchivo.c_str(), "Máquina número:, 01 \r\n");
    appendFile(SD, nombreArchivo.c_str(), "Fecha  y  hora, Temperatura, Humedad \r\n");
    
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
//**************************************************************************************************
void listSubDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.println("");
  Serial.printf("Listado de directorio: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }
String rutaNombre = "";
String nombre = "";
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      rutaNombre = file.name();
      nombre = rutaNombre.substring(11);
      Serial.println(nombre);
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    }
    file = root.openNextFile();
  }
}
//**************************************************************************************************
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.println("");
  Serial.printf("Listado de directorio: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());     
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    }else {
      Serial.print("  Archivo: ");
      Serial.print(file.name());
      Serial.print("  Tamaño: ");
      Serial.println(file.size());
    }
 //     Serial.println("");
    file = root.openNextFile();
  }
}
//**************************************************************************************
void leerFile(fs::FS &fs, const char * path){
  Serial.printf("Leyendo archivo: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Datos del archivo: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}
//******************************************************************************************
void leerSerie(){
  String comando = "";
  String dateTime = "";
  String nombreArchivo = "";
  String ano, mes, dia, hora, minutos, segundos;
  String directorio = "";
  int anoInt, mesInt, diaInt, horaInt, minutosInt, segundosInt;
  char dateTimeChar[22];
        
  comando = Serial.readStringUntil(':');
  if(comando == "setRTC"){              //Si recibe este String seguido por ":" por puerto serie seteará el RTC con la siguiente cadena año mes dia hora minutos segundos
    dateTime = Serial.readString();     // Ej.:Comando por puero serie: setRTC:20210213163218  -> año:2021, mes: 02, día: 13, hora: 16, minutos: 32, segundos: 18
    ano = dateTime.substring(0,4);
    mes = dateTime.substring(4,6);
    dia = dateTime.substring(6,8);
    hora = dateTime.substring(8,10);
    minutos = dateTime.substring(10,12);
    segundos = dateTime.substring(12,14);

    anoInt = ano.toInt();
    mesInt = mes.toInt();
    diaInt = dia.toInt();
    horaInt = hora.toInt();
    minutosInt = minutos.toInt();
    segundosInt = segundos.toInt();
    rtc.adjust(DateTime(anoInt, mesInt, diaInt, horaInt , minutosInt, segundosInt));
    DateTime now = rtc.now();
    sprintf(dateTimeChar, "%02d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(),now.day(),  now.hour(), now.minute(), now.second()); 
    Serial.println("Fecha y hora modificadas: ");
    Serial.print(dateTimeChar);

  }
  if(comando == "leerDir"){
      listDir(SD, "/", 0);
  }  
 if(comando == "leerSubDir"){
      directorio += "/";
      directorio += Serial.readString(); 
      listSubDir(SD, directorio.c_str(), 0);
  }  
    if(comando == "leerArchivo"){
      nombreArchivo = "/";
      nombreArchivo += Serial.readString();
      leerFile(SD, nombreArchivo.c_str());
  }  
}
//**************************************************************************************  

void loop() {
//  boolean tiempoCumplido;
  boolean calibracion;
//  String inicio = "";           //Cuando se inicia la placa Impacto envía al inicio de la comunicación "ini")
  String nombreArchivo = "";
  boolean estadoPulsadorInicio;
 
  while (Serial.available() > 0) {    // Es para lectura del puerto serie
  leerSerie();
  }
  calibracion = digitalRead(pulsadorCalibracion);
  if(!calibracion)  calibracionSensoresPresion();
        
   estadoPulsadorInicio = digitalRead(pulsadorInicio);
   if(estadoPulsadorInicio == LOW){
   nombreArchivo = rutinaInicioEnsayo();
   rutinaEnsayo(nombreArchivo);
   }
      
 
}
