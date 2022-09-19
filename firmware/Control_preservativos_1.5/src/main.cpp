#include <Arduino.h>

/*
 * HADWARE: 
 * Se definen las constantes en el archivo hardware.h
 * Pin de Chip Select para escribir el SD : Pin 5
 * Pulsador inicio ensayo: Pin 4
 * Pulsador calibracion: Pin 13
 * Salida para activar electroválvula: Pin 25
 * Sensor de temperatura one wire: Pin 32
 * I2C :Pines SDA:21 y CLK:22
 * Sensor de presión 1 (entrada analógica): Pin 34
 * Sensor de presión 2 (entrada analógica): Pin 35
 * SOFTWARE
 Grabación de memoria SD: Se genera un archivo cada vez que se presiona el pulsador se genera un archivo .csv para almacenar los datos cuyo nombre 
 se crea en formato ISO8601 hora local (yyyymmddThhmmss), tomando fecha y hora del RTC
 Los archivos se guardan en directorios cuyo nombre de directorio es la fecha del ensayo.
 Se genera un archivo donde se almacenan el último valor de cada ensayo (se crea una vez si no existe) con el nombre Maquina_<NUMERO_MAQUINA>.csv . 
 Este archivo se guarda en la raiz. Cada máquina de ensayos tendrá su número

 Comandos Puerto Serie:
 Se puede setear el reloj de tiempo real (RTC) desde el puerto serie (115200 baud, 8 bits, sin paridad, un stop bit. Enviar setRTC:año mes dia hora minutos segundos (todo junto sin espacios)
 Ej.:Comando por puero serie: setRTC:20210213163218  -> año:2021, mes: 02, día: 13, hora: 16, minutos: 32, segundos: 18
 
 Enviando "leerDir:" devuelve el directorio Raiz de la SD.
 Enviando "leerArchivo:<Nombre de Archivo>" devuelve el contenido del archivo (UTF-8). Si el archivo esta dentro de un subdirectorio especificar ruta. Ej.: "leerArchivo:/20220601/20220601T083911.csv" 
 Enviando "leerSubDir:<Nombre de directorio>" devuelve el contenido del directorio. Ej.: Enviando "leerSubDir:/20220601" devuelve el contenido dentro del directorio /20220601
*/

#include <Wire.h>
#include <RTClib.h>
RTC_DS3231 rtc;
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "FS.h"
#include <EEPROM.h>
#include "pu2clr_pcf8574.h"
PCF pcf;

#include <manejadorSD.h>
#include <tiempoCumplido.h>
#include "hardware.h"
#include "leerEEPROM.h"
#include "imprimirLCDI2C.h"
#include "calibrarSensor.h"
#include "leerCalibracionEEProm.h"
#include "teclado4x4.h"

tiempoCumplido tiempoCumplido1(100);    //Usado para saber cuando escribir lecturas en la SD
leerEEPROM leerEEPROM1(addressEEPROM_1kPa_1, addressEEPROM_2kPa_2);
manejadorSD tarjetaSD1(CS);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd1(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);  //PCF8574_ADDR_A21_A11_A01 -> dicección I2C del display físico  0X27
//LiquidCrystal_I2C lcd2(PCF8574_ADDR_A21_A11_A00, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);  //PCF8574_ADDR_A21_A11_A00 -> dicección I2C del display físico 0X26
imprimirLCDI2C imprimirLcd1(lcd1);
calibrarSensor calibrarSensores1(lcd1);
leerCalibracionEEProm leerCalibracion1;
tiempoCumplido tiempoCumplido2(4);    //Usado para llamar a la lectura del teclado

teclado4x4 teclado1(pcf);             //Teclado 4x4 conectado al I2C a travez delPCF8574

//Rutina que crea en archivo de registro de nuevo ensayo, lo nombra según formato ISO8601 y Escribe dentro de el el encabezado del ensayo
//void calibracionSensoresPresion(void);
void fallaEscrituraSD(void);
//void leerEEProm(void);

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pulsadorInicio, INPUT);
  pinMode(activarElectrovalvula, OUTPUT);
  pinMode(pulsadorCalibracion, INPUT);

  boolean calibracion;
  int flagCalibracion;
  boolean memoriaSDinicializada = false;
  boolean datosCalibracionOk;
  //********************************************
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  imprimirLcd1.inicializarLCD(20, 4);
  imprimirLcd1.imprimirLCDfijo("  INTI CAUCHOS",0, 0);
  memoriaSDinicializada = tarjetaSD1.inicializarSD();
    if(!memoriaSDinicializada){
      Serial.println("inicialización fallida!"); 
      imprimirLcd1.imprimirLCDfijo("SD NO encontrada",0, 0);
    while(!memoriaSDinicializada){
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        lcd1.backlight();
        delay(500);                       // wait for a half second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        lcd1.noBacklight();
        delay(500);                       // wait for a half second
        memoriaSDinicializada = tarjetaSD1.inicializarSD();
     }
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
  EEPROM.begin(EEPROM_SIZE);
   
  imprimirLcd1.imprimirLCDfijo("Pulse INICIO",0, 0);
  imprimirLcd1.imprimirLCDfijo("para comenzar",0, 1);
 
  //leerEEProm();
  flagCalibracion = EEPROM.readInt(flagIntentoRecalibrar);
  datosCalibracionOk = leerCalibracion1.verificarCalibracion();

  if(datosCalibracionOk == false){
    if(flagCalibracion == 0){
      imprimirLcd1.imprimirLCDfijo("Recomponiendo       ",0, 0);
      imprimirLcd1.imprimirLCDfijo("calibracion y       ",0, 1);
      imprimirLcd1.imprimirLCDfijo("reiniciando         ",0, 2);
      delay(2000);
      EEPROM.writeInt(flagIntentoRecalibrar, 1);       ////Se indica en "uno" que si entro en Recupero de calibración
      EEPROM.commit();
      ESP.restart();  
    }else{
      imprimirLcd1.imprimirLCDfijo("No se puede corregir",0, 0);
      imprimirLcd1.imprimirLCDfijo("calibracion         ",0, 1);
      imprimirLcd1.imprimirLCDfijo("     CALIBRAR       ",0, 2);
      while(true){
        calibracion = digitalRead(pulsadorCalibracion);
        if(!calibracion)   calibrarSensores1.calibrar();
        delay(300);
      }    
    }     
  }
  
  pcf.setup(addresspcf8574_KeyPad);        
//  pcf.write(0B11111111);  // Turns all pins/ports HIGH

}
//********Fin Setup***************************************************************************
/*
void leerEEProm(){
  int ultimaLecturaEEPROM = 10;
  int address = 0;
  int b, m;
  int cont = 0;
  int checkSum =0;
 
 while(cont < ultimaLecturaEEPROM){
    m = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
      Serial.print("dirección actual: ");
      Serial.println(address);
    address += sizeof(m); //update address value  
      Serial.print("tamaño m: ");
      Serial.println(m);
    b = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    address += sizeof(b); //update address value  
      Serial.print("tamaño b: ");
      Serial.println(b);
    cont ++;    
  }
    Serial.print("dirección checksum: ");
    Serial.println(address);
    checkSum = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    Serial.print("CheckSum: ");
    Serial.println(checkSum);

}
*/
//**************************************************************************************************
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
void fallaEscrituraSD(){
  imprimirLcd1.imprimirLCDfijo("                    ",0, 0);
  imprimirLcd1.imprimirLCDfijo(" Falla Escritura SD ",0, 1);
  imprimirLcd1.imprimirLCDfijo("                    ",0, 2);
  imprimirLcd1.imprimirLCDfijo("                    ",0, 3);
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
  boolean registrarDatos = LOW;     //Se pone en HIGH cuando la función tiempoCumplido() indica el momento de tomar medición, para no usar delay() bloqueante
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
  String lineaMedicionLCD = ""; 
  boolean datoAnexadoEnSD;
             
  nombreMaquina += String(NUMERO_MAQUINA);        //El nombre del archivo corresponde al número de máquina
  nombreMaquina += ".csv";

  valorAD_minimo_fuelle = EEPROM.readInt(addressEEPROM_1kPa_1); //Valor de presión mínimo (en entero del AD) en el fuelle (sensor de presión 2)   
  valorAD_maximo_fuelle = EEPROM.readInt(addressEEPROM_2kPa_1); //Valor de presión máximo (en entero del AD) en el fuelle (sensor de presión 2) 

  digitalWrite(activarElectrovalvula, HIGH);
  delay(200);
  while(segundos < timeOut){
    registrarDatos = tiempoCumplido1.calcularTiempo(false);
    lineaMedicion = "";
    if(registrarDatos){
        presion1 = obtenerPresion1();            //Se obtiene la presión en el preservativo
        presion1PartEntera = int(presion1);      //sprintf no formatea float, entonces separo partes entera y decimal
        presion1Partdecimal = int((presion1 - presion1PartEntera) * 100);
        sprintf(medicion, "%d.%02d", (int)presion1, presion1Partdecimal);
        lineaMedicion += String(segundos);
        lineaMedicion += ",  ";
        lineaMedicion += String(medicion);
        lineaMedicionLCD += lineaMedicion;
        lineaMedicion += "\r\n";
        Serial.print(lineaMedicion);         //Si se quita el envío al puerto serie, agregar delay(10) para que escriba bien en la sd el fin de línea
        datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());   
        segundos ++;
        imprimirLcd1.imprimirMedicionLCD(String(lineaMedicionLCD), 2);
        lineaMedicionLCD = "";
        registrarDatos = tiempoCumplido1.calcularTiempo(true);
        guardarLineaMedicionAnterior = true;
   }
   if(presion1 > 0.1)  { superoPresionMinima = true; }
   if(superoPresionMinima == true){
//     presion1 = obtenerPresion1();            //Se obtiene la presión en el preservativo
     if(presion1 < (presion1Anterior * 0.6)){       //Si la presión cae un 40% se asume que el preservativo reventó
        escribeUltimaMedicion += nombreArchivo;
        escribeUltimaMedicion += ", ";
        escribeUltimaMedicion += lineaMedicionAnterior;
        
        datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreMaquina.c_str(), escribeUltimaMedicion.c_str());
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
      lineaMedicionLCD += lineaMedicion;
      lineaMedicionLCD +=  " Time Out";
      imprimirLcd1.imprimirMedicionLCD(String(lineaMedicionLCD), 2);
      lineaMedicion += "Fin de ensayo por Time Out \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por Time Out \r\n"; 
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo
      Serial.println("Fin de ensayo por Time Out");
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
   }     
   
   //  delay(200);
   inicio = digitalRead(pulsadorInicio);
   if(inicio == LOW){
  //    lineaMedicionLCD +=  "Fin por Usuario";
      lineaMedicion += "Fin de ensayo por parada de usuario \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por parada de usuario \r\n"; 
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo
      Serial.println("Fin de ensayo por parada de usuario");
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
      segundos = timeOut + 1;         //Para que salga del While
      digitalWrite(activarElectrovalvula, LOW);
      imprimirLcd1.imprimirMedicionLCD("Fin por Usuario", 2);
      delay(1500);
   }

    valorAD_fuelle = analogRead(sensorPresion2);
    if(segundos > 2){                                                             //Para dar tiempo a que suba la presión en el fuelle. La misma electroválvula 
      if(valorAD_fuelle < valorAD_minimo_fuelle)   bajaPresionFuelle = HIGH;      //infla preservativo y el fuelle de sujeción
      if(valorAD_fuelle > valorAD_maximo_fuelle)   altaPresionFuelle = HIGH;      
    }

    if(bajaPresionFuelle == HIGH){
      lineaMedicion += "Fin de ensayo por baja presión en fuelle \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por baja presión en fuelle \r\n"; 
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo    
      Serial.println("Fin de ensayo por baja presión en fuelle");
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
      segundos = timeOut + 1;       //Para que salga del While
      imprimirLcd1.imprimirMedicionLCD("Baja Presion Fuelle",2);
      delay(300);
    }
    if(altaPresionFuelle == HIGH){
      lineaMedicion += "Fin de ensayo por alta presión en fuelle \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por alta presión en fuelle \r\n"; 
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());               //Escribe última medición antes de reventado en archivo de este ensayo  
      Serial.println("Fin de ensayo por alta presión en fuelle");
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());  //Escribe última medición antes de reventado en archivo Maquina_#
      segundos = timeOut + 1;       //Para que salga del While
      imprimirLcd1.imprimirMedicionLCD("Alta Presion Fuelle",2);
      //imprimirLCD("Alta Presion Fuelle",2);
      delay(300);
    }
    if(!datoAnexadoEnSD){
      segundos = timeOut + 1;       //Para que salga del While  
      fallaEscrituraSD();
    }
  }
  digitalWrite(activarElectrovalvula, LOW);
}
//********************************************************************************
String rutinaInicioEnsayo(){
    String nombreArchivo = "";
    String cabecera = "";
    String nombreDirectorio = "";
    String tempHumedadLCD = "";
    boolean datoAnexadoEnSD;
    
    float temp = dht.readTemperature(); //Leemos la temperatura en grados Celsius
    float humed = dht.readHumidity(); //Leemos la Humedad
    tempHumedadLCD += temp;
    tempHumedadLCD = tempHumedadLCD.substring(0,4);
    tempHumedadLCD += (char)223; //Agrega símbolo de grados
    tempHumedadLCD += "C    ";
    tempHumedadLCD += humed;
    tempHumedadLCD = tempHumedadLCD.substring(0,12);
    tempHumedadLCD += "%";
    DateTime now = rtc.now();
    
    char dateISO[9];
    sprintf(dateISO, "%02d%02d%02d", now.year(), now.month(),now.day()); 
    nombreDirectorio += "/";
    nombreDirectorio += String(dateISO);
    tarjetaSD1.createDir(SD,nombreDirectorio.c_str());

    char dateTimeISO[26];
    sprintf(dateTimeISO, "%02d%02d%02dT%02d%02d%02d", now.year(), now.month(),now.day(),  now.hour(), now.minute(), now.second()); 
    nombreArchivo += nombreDirectorio;
    nombreArchivo += "/";
    nombreArchivo += String(dateTimeISO);
    nombreArchivo += ".csv";
    tarjetaSD1.writeFile(SD, nombreArchivo.c_str(), "Máquina número:, 01 \r\n");
    datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), "Fecha  y  hora, Temperatura, Humedad \r\n");
    imprimirLcd1.imprimirLCDfijo(String(tempHumedadLCD),3, 0);
    imprimirLcd1.imprimirLCDfijo(String(dateTimeISO),2, 1);
        
    char dateTime[20];
    sprintf(dateTime, "%02d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(),  now.hour(), now.minute(), now.second()); 
    cabecera += String(dateTime);
    cabecera += "  ,";
    cabecera += String(temp);
    cabecera += "  ,";
    cabecera += String(humed);
    cabecera += "\r\n";
    imprimirPC(now, temp, humed);
    datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), cabecera.c_str());
    String str = nombreArchivo.c_str();  
    if(!datoAnexadoEnSD){
      fallaEscrituraSD();
    }
  return str; 
}

//******************************************************************************************
void leerSerie(){
  String comando = "";
  String dateTime = "";
  String nombreArchivo = "";
  String ano, mes, dia, hora, minutos, segundos;
  String directorio = "";
  int anoInt, mesInt, diaInt, horaInt, minutosInt, segundosInt, eepromInt;
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
      tarjetaSD1.listDir(SD, "/", 0);
  }  
 if(comando == "leerSubDir"){
      directorio += "/";
      directorio += Serial.readString(); 
      tarjetaSD1.listSubDir(SD, directorio.c_str(), 0);
  }  
    if(comando == "leerArchivo"){
      nombreArchivo = "/";
      nombreArchivo += Serial.readString();
      tarjetaSD1.leerFile(SD, nombreArchivo.c_str());
  }  
    if(comando == "eeprom"){    // Escribo en una dirección específica de la eeprom Ej: eeprom:1:23   -> Escribe 23 en la dirección 1
      int direepromInt;
      String direeprom = "";
      direeprom = Serial.readStringUntil(':');
      direepromInt = direeprom.toInt();
      nombreArchivo = Serial.readString();
      eepromInt = nombreArchivo.toInt();
      EEPROM.writeInt(direepromInt, eepromInt);       //Modifico la direccion 0 de la EEProm para testear como funciona el checksum     
      EEPROM.commit();

  }  
}
//**************************************************************************************  
char leerTeclado(){
  int i;
  char teclaLeida = 'z';

   teclaLeida = teclado1.obtenerTecla();
   if(teclaLeida != 'z'){
   // Serial.print("\nTecla seleccionada: ");
   // Serial.print(teclaLeida);
    delay(100); // Taking a little time to allow you to release the button
   }
   return(teclaLeida);
}

void loop() {
//  boolean tiempoCumplido;
  boolean calibracion;
//  String inicio = "";           //Cuando se inicia la placa Impacto envía al inicio de la comunicación "ini")
  String nombreArchivo = "";
  boolean estadoPulsadorInicio, consultarTeclado;
  char letraLeida;


  while (Serial.available() > 0) {    // Es para lectura del puerto serie
  leerSerie();
  }
  calibracion = digitalRead(pulsadorCalibracion);
 // if(!calibracion)  calibracionSensoresPresion();
   if(!calibracion)   calibrarSensores1.calibrar();
        
   estadoPulsadorInicio = digitalRead(pulsadorInicio);
   if(estadoPulsadorInicio == LOW){
   nombreArchivo = rutinaInicioEnsayo();
   rutinaEnsayo(nombreArchivo);
   }

    consultarTeclado = tiempoCumplido2.calcularTiempo(false);
  if(consultarTeclado){
    letraLeida = leerTeclado();
    consultarTeclado = tiempoCumplido2.calcularTiempo(true);
  }
 
/*
 for (uint8_t i = 4; i < 8; i++ ) {
    pcf.digitalWrite(i-4,LOW );
    delay(2);
    if ( pcf.digitalRead(i) == LOW) {
      Serial.print("\nThe port ");
      Serial.print(i);
      Serial.print(" is LOW (button pressed)");
      delay(100); // Taking a little time to allow you to release the button
      pcf.write(0B11111111);  // Turns all pins/ports HIGH
    }
  } */      
}