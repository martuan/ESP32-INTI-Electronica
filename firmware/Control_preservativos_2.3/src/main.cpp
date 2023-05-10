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
 Se puede setear el reloj de tiempo real (RTC) desde el puerto serie (115200 baud, 8 bits, sin paridad, un stop bit. Enviar setRTC:año mes dia hora minutos cantMediosSegundos (todo junto sin espacios)
 Ej.:Comando por puero serie: setRTC:20210213163218  -> año:2021, mes: 02, día: 13, hora: 16, minutos: 32, cantMediosSegundos: 18
 
 Enviando "leerDir:" devuelve el directorio Raiz de la SD.
 Enviando "leerArchivo:<Nombre de Archivo>" devuelve el contenido del archivo (UTF-8). Si el archivo esta dentro de un subdirectorio especificar ruta. Ej.: "leerArchivo:/20220601/20220601T083911.csv" 
 Enviando "leerSubDir:<Nombre de directorio>" devuelve el contenido del directorio. Ej.: Enviando "leerSubDir:/20220601" devuelve el contenido dentro del directorio /20220601
*/
//Definiciones para el Timer 0--------------------------------------------------------
#define TIMER_0 0
#define FREC_CLOCK_CPU 80000000  //En Hz
#define PRESCALER_T0  80 //Prescales para timer 0.
//timer speed (Hz) = Timer clock speed (Mhz) / prescaler
#define FREC_T0 2 //En Hz 
//#define CUENTA_T0 1000000 //en microcantMediosSegundos.
#define CUENTA_T0 (FREC_CLOCK_CPU / (PRESCALER_T0 * FREC_T0)) //en microcantMediosSegundos.
//Definiciones para el Timer 1--------------------------------------------------------
#define TIMER_1 1
#define PRESCALER_T1  80 //Prescales para timer 1.
#define CUENTA_T1 10000 //10mseg (en microcantMediosSegundos.)

#define timeOut 40    //Cantidad de segundos 

#include <Wire.h>
#include <RTClib.h>
RTC_DS3231 rtc;

#include <SPI.h>
//#include <SD.h>
#include <SdFat.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "FS.h"
#include <EEPROM.h>
#include "pu2clr_pcf8574.h"
PCF pcf;      //Maneja el I2C para el teclado 4x4

#include <manejadorSD.h>
#include <tiempoCumplido.h>
#include "hardware.h"
#include "leerEEPROM.h"
#include "imprimirLCDI2C.h"
#include "calibrarSensor.h"
#include "leerCalibracionEEProm.h"
#include "teclado4x4.h"
#include "defines.h"
#include "definesComm.h"
//#include "parser.cpp"

tiempoCumplido tiempoCumplido1(100);    //Usado para saber cuando escribir lecturas en la SD
leerEEPROM leerEEPROM1(addressEEPROM_1kPa_1, addressEEPROM_2kPa_2);
manejadorSD tarjetaSD1(CS);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd1(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);  //PCF8574_ADDR_A21_A11_A01 -> dicección I2C del display físico  0X27
//LiquidCrystal_I2C lcd2(PCF8574_ADDR_A21_A11_A00, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);  //PCF8574_ADDR_A21_A11_A00 -> dicección I2C del display físico 0X26
imprimirLCDI2C imprimirLcd1(lcd1);
//leerCalibracionEEProm leerCalibracion1;
leerCalibracionEEProm leerCalibracion1(lcd1);
tiempoCumplido tiempoCumplido2(1);    //Usado para llamar a la lectura del teclado

teclado4x4 teclado1(pcf);             //Teclado 4x4 conectado al I2C a travez delPCF8574
calibrarSensor calibrarSensores1(lcd1);
//leerPuertoSerie 

//*** Timer
hw_timer_t * timer0 = NULL;
boolean flagEntroTimer0 = false;  
void IRAM_ATTR onTimer0();
//int finEnsayoSinRuptura(int cantMediosSegundos, String lineaMedicion, String nombreDirectorioArchivo,String directorioArchivoOT,  boolean suspenderEnsayo){
hw_timer_t *timer1 = NULL; //Puntero para configurar el timer.
//boolean flagEntroTimer1 = false;  
void IRAM_ATTR onTimer1();

boolean flagResetVolumen = false; //Para que GuardaMed() en ParserComm.cpp ponga volumen acumulado en cero al inicio del ensayo
boolean flagResetRespArray = false;
float segundosAcumulados;
bool flagContabilizarVolumenes;
//**************************************/
//Rutina que crea en archivo de registro de nuevo ensayo, lo nombra según formato ISO8601 y Escribe dentro de el el encabezado del ensayo
//********* Maquna de estados -> menues en display
extern unsigned char inParser;			// dato de entrada al parser
extern void Parser(void);
extern void IniParser(void);
extern unsigned char estadoActual; 	// estado del parser
extern boolean flagEnsayoEnCurso;
extern boolean  suspenderEnsayo;
extern boolean flagCalibracionEnCurso;

//********* Variables para el controlador de caudal por comunicación serie "Comm"
extern unsigned char inParserComm;			// dato de entrada al parser
extern unsigned char estadoActualComm; 	// estado del parser
extern void ParserComm(void);
void IniParserComm(void);
extern unsigned char timeOutCounterComm; //Contador para el time out de comunicación.
extern unsigned char timeOutCaudCounter; //Contador para el time out de comandos de medición.
//extern static unsigned int cantVecesGuardaMed;	 //Sirve como contador de tiempo para calcular volumen acumulado en GuardarMed()

extern bool llamaParserComm;
extern unsigned char codigoErrorComm;		// 0 = Sin Error; 1 = Reint_WCM; 2 = VerifWCM; 3 = Reint_SP; 4 = VerifSP; 5 = reintCaudal; 6 = caudalRec 									
extern boolean errorSetUpCaudal;
extern boolean llamarMPPAL;
//extern float sumatoriaCaudalesMedidos;;
//extern unsigned int caudalValorDigitalDecimal;
extern bool flagInicializarpreviousMillis;
extern float volumenParcial;
//extern 	float segundosTranscurridos;
extern bool flagCalculoVolumenParcial;
void ConfigCallback();

void esperarCalentamientoSensor(void);
//***************************************************************
void fallaEscrituraSD(void);
//****************************************************************************************
void setUpWebServer(void);
//****************************************************************************************
void IRAM_ATTR onTimer0() {
  flagEntroTimer0 = true;

//  digitalWrite(LED_PORT, !digitalRead(LED_PORT)); //DEBUG
  if ( timeOutCounterComm != 0)
    if(--timeOutCounterComm == 0)
    {
  //    inParserComm = TIMEOUT;
      llamaParserComm = true;  
    } 
}
//***************************************************************************************
void IRAM_ATTR onTimer1()
{
 /* static unsigned int i=0;
    if(i==200){
      i=0;
      flagEntroTimer1 = true;
    }
    i++;*/
   //TimeOut para los comandos de medición.--------------
  if ( timeOutCaudCounter != 0){
         //    Serial.print("Entro al if Valor de timeOutCaudCounter: ");
         //        Serial.println(timeOutCaudCounter);
    if(--timeOutCaudCounter == 0)
    {
//      inParserComm = TIMEOUTCAUD;
      inParserComm = MEDIR;
      llamaParserComm = true; 
      //cantVecesGuardaMed++;	
          //  Serial.println("Entro en timeOutCaudCounter de onTimmer1");
    }
  }
}
//***************************************************************************************
void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pulsadorInicio, INPUT);
  pinMode(electrovalvulaPresrevativo, OUTPUT);
  pinMode(electrovalvulaFuelle, OUTPUT);
  pinMode(pulsadorCalibracion, INPUT);

  boolean calibracion;
 // int flagCalibracion;
  boolean memoriaSDinicializada = false;
  boolean datosCalibracionOk;
  char letraLeida = 'z';
  //********************************************
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  imprimirLcd1.inicializarLCD(20, 4);
 // imprimirLcd1.imprimirLCDfijo("  INTI CAUCHOS",0, 0);
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
  
  calibracion = leerCalibracion1.verificarCalibracion();
  
  pcf.setup(addresspcf8574_KeyPad);        
//  pcf.write(0B11111111);  // Turns all pins/ports HIGH

  //Configuración del Timer 0 ----------------------------------------------------
  timer0 = timerBegin(TIMER_0, PRESCALER_T0, true); // timer utilizado; Prescaler; cuenta ascendente= TRUE.
  timerAttachInterrupt(timer0, &onTimer0, true); // Asigna la rutina de atención de ingterrupción
                                                  // al timer0 . Activa por flanco.
  timerAlarmWrite(timer0, CUENTA_T0, true); //Carga la cuenta del timer, Autorrecarga.
  timerAlarmEnable(timer0); //Habilita la interrupción del timer.
  
//Configuración del Timer 1 ----------------------------------------------------
  timer1 = timerBegin(TIMER_1, PRESCALER_T1, true); // timer utilizado; Prescaler; cuenta ascendente= TRUE.
  timerAttachInterrupt(timer1, &onTimer1, true); // Asigna la rutina de atención de interrupción
                                                  // al timer1 . Activa por flanco.
  timerAlarmWrite(timer1, CUENTA_T1, true); //Carga la cuenta del timer, Autorrecarga.
  //timerAlarmEnable(timer1); //Habilita la interrupción del timer.
  timerAlarmDisable(timer1);
//**************************************************************************

//******** Serial Comm 2
  //Configuración del UART para configuración del Controlador-----------------------------------
  Serial2.begin(38400,SERIAL_8N1,RX2,TX2);
  //Serial2.onReceive(Serial2_callback);
  Serial2.setRxTimeout(3); //3 símbolos de Time OUT.
  Serial2.onReceive(ConfigCallback);
  //----------------------------------------------------------------------------------------
  esperarCalentamientoSensor();
//********************************************************************
  IniParser();
  
  if(calibracion == false){
      //Parser();
      inParser = '9';
      Parser();
      inParser = 'A';
      Parser();
      Serial.print("estado en setup: ");
      Serial.println(estadoActual);
  }
  llamaParserComm = true;
  IniParserComm();
  flagInicializarpreviousMillis = false;

  setUpWebServer();
}
//********Fin Setup***************************************************************************
void setUpWebServer(){

}
//*********************************************************************************************
void esperarCalentamientoSensor(){  //El sensor del controlador de caudal requiere requiere esperar al menos 30 seg. luego de energizado
int cantMediosSegundos = 2;                   //para que el sensor alcance estabilidad térmica y mida con un error inferior al 2%. Luego de 5 minutos el error garantizado es inferior al 1% 
  imprimirLcd1.limpiarLCD();
  imprimirLcd1.imprimirLCDfijo("Esperar inicio del",0, 1);
  imprimirLcd1.imprimirLCDfijo("Sensor",0, 2);
  while(cantMediosSegundos>0){
   imprimirLcd1.imprimirLCDfijo(String(cantMediosSegundos),8, 3);
   cantMediosSegundos--;
   delay(1000);
  }

}

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
      Serial.print("presion1 float: ");
      Serial.println(presion1);

  if(presion1 < 0)  presion1 = 0;

  return presion1;
}
//***********************************************************************************
void fallaEscrituraSD(){
  imprimirLcd1.limpiarLCD();
//  imprimirLcd1.imprimirLCDfijo("                    ",0, 0);
  imprimirLcd1.imprimirLCDfijo(" Falla Escritura SD ",0, 1);
//  imprimirLcd1.imprimirLCDfijo("                    ",0, 2);
//  imprimirLcd1.imprimirLCDfijo("                    ",0, 3);
}
//***********************************************************************************
void finEnsayoSinRuptura( String lineaMedicion, String nombreDirectorioArchivo,String directorioArchivoOT){
//int finEnsayoSinRuptura(int cantMediosSegundos, String lineaMedicion, String nombreDirectorioArchivo,String directorioArchivoOT){
 //boolean inicio = HIGH;            //Para verificar s se presionó el pulsador INICIO durante el ensayo y detener el mismo (parada de emergencia)
  static int valorAD_minimo_fuelle;
  static int valorAD_maximo_fuelle;
  int valorAD_fuelle = 0;
  boolean bajaPresionFuelle = LOW;
  boolean altaPresionFuelle = LOW;
  String lineaMedicionLCD = ""; 
  boolean datoAnexadoEnSD;
  String escribeUltimaMedicion = "";
  static String nombreDirectorioArchivoAnterior= "";

/*  Serial.println("Entro en finEnsayoSinRuptura");*/
   //   Serial.print("Valor suspenderEnsayo: ");
   //   Serial.println(suspenderEnsayo);

  escribeUltimaMedicion = nombreDirectorioArchivo;
  escribeUltimaMedicion += ", ";
  escribeUltimaMedicion += lineaMedicion;
  escribeUltimaMedicion += ", ";
 // if(segundosAcumulados < 2){                                             //Para que lea los valores una vez por ensayo
  if(nombreDirectorioArchivoAnterior != nombreDirectorioArchivo){
    valorAD_minimo_fuelle = EEPROM.readInt(addressEEPROM_1kPa_1); //Valor de presión mínimo (en entero del AD) en el fuelle (sensor de presión 2)   
    valorAD_maximo_fuelle = EEPROM.readInt(addressEEPROM_2kPa_1); //Valor de presión máximo (en entero del AD) en el fuelle (sensor de presión 2) 
      Serial.print("Valor Presión maximo fuelle inicial: ");
      Serial.println(valorAD_maximo_fuelle);
    nombreDirectorioArchivoAnterior = nombreDirectorioArchivo;   
  }

//   if(cantMediosSegundos == timeOut){
   if(segundosAcumulados >= timeOut){
//      lineaMedicionLCD += lineaMedicion;
//      lineaMedicionLCD +=  " Time Out";
//      imprimirLcd1.imprimirMedicionLCD(String(lineaMedicionLCD), 2);
 //     lineaMedicion += "Fin de ensayo por Time Out \r\n"; 
      escribeUltimaMedicion += " Time Out. \r\n"; 
              lineaMedicion += ", Time out \r\n";
//              lineaMedicion += "\r\n";
//      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo
      Serial.println("Fin de ensayo por Time Out");
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, directorioArchivoOT.c_str(), escribeUltimaMedicion.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
      imprimirLcd1.imprimirLCDfijo("Fin por Time out    ",0, 2);
   }     
   
   //  delay(200);
  // inicio = digitalRead(pulsadorInicio);
   if(suspenderEnsayo == true){

      SD.remove(nombreDirectorioArchivo.c_str());
      //tarjetaSD1.remove(nombreDirectorioArchivo.c_str());
      Serial.println("Fin de ensayo por parada de usuario");
      segundosAcumulados = timeOut + 1;
      //cantMediosSegundos = timeOut + 1;         //Para que salga del While
      digitalWrite(electrovalvulaPresrevativo, LOW);
//      imprimirLcd1.imprimirLCDfijo("Detenido por Usuario ",0, 1);
//      imprimirLcd1.imprimirMedicionLCD("Fin por Usuario", 2);
      delay(1500);
   }

    valorAD_fuelle = analogRead(sensorPresion2);
    if(segundosAcumulados > 2){                                                             
//    if(cantMediosSegundos > 2){                                                             
      if(valorAD_fuelle < valorAD_minimo_fuelle)   bajaPresionFuelle = HIGH;      //infla fuelle de sujeción
      if(valorAD_fuelle > valorAD_maximo_fuelle)   altaPresionFuelle = HIGH;      
    }

    if(bajaPresionFuelle == HIGH){
      lineaMedicion += ", Fin de ensayo por baja presión en fuelle \r\n"; 
      escribeUltimaMedicion += "Fin de ensayo por baja presión en fuelle \r\n"; 
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), lineaMedicion.c_str());                 //Escribe última medición antes de reventado en archivo de este ensayo    
      Serial.println("Fin de ensayo por baja presión en fuelle");
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, directorioArchivoOT.c_str(), escribeUltimaMedicion.c_str());    //Escribe última medición antes de reventado en archivo Maquina_#
      segundosAcumulados = timeOut + 1;
//      cantMediosSegundos = timeOut + 1;       //Para que salga del While
      imprimirLcd1.imprimirLCDfijo("Baja Presion Fuelle ",0, 2);
//      imprimirLcd1.imprimirMedicionLCD("Baja Presion Fuelle",2);
      delay(300);
    }
    if(altaPresionFuelle == HIGH){
      lineaMedicion += ", Fin de ensayo por alta presión en fuelle \r\n"; 
      escribeUltimaMedicion += "Fin de ensayo por alta presión en fuelle \r\n"; 
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), lineaMedicion.c_str());               //Escribe última medición antes de reventado en archivo de este ensayo  
      Serial.println("Fin de ensayo por alta presión en fuelle");
      Serial.print("Presión en fuelle: ");
      Serial.println(valorAD_fuelle);
      Serial.print("Valor Presión maximo fuelle: ");
      Serial.println(valorAD_maximo_fuelle);

      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, directorioArchivoOT.c_str(), escribeUltimaMedicion.c_str());  //Escribe última medición antes de reventado en archivo Maquina_#
      segundosAcumulados = timeOut + 1;
//      cantMediosSegundos = timeOut + 1;       //Para que salga del While
      imprimirLcd1.imprimirLCDfijo("Alta Presion Fuelle ",0, 2);
      //imprimirLcd1.imprimirMedicionLCD("Alta Presion Fuelle",2);
      //imprimirLCD("Alta Presion Fuelle",2);
      delay(300);
    }
  //  return cantMediosSegundos;
}
//***********************************************************************************
String obtenerVolumenAcumulado(void){
    //Se calcula en GuardarMed
  static float volumenAcumuladoFloat;
  
  if(flagResetVolumen == true){ 
    volumenAcumuladoFloat = 0;
      flagResetVolumen = false; 
  }
  if(flagCalculoVolumenParcial == true){  volumenAcumuladoFloat += volumenParcial; }

    String volumenAcumulado = "";
    volumenAcumulado = String(volumenAcumuladoFloat, 2);
//	Serial.print("Volumen acumulado: ");
//	Serial.println(volumenAcumulado);//DEBUG
//	Serial.print(" * cantMediosSegundos: ");
//	Serial.println(cantMediosSegundos);//DEBUG
  flagCalculoVolumenParcial = false;
    return(volumenAcumulado);
}
//***********************************************************************************
//boolean rutinaEnsayo(String nombreDirectorioArchivo,  boolean suspenderEnsayo){
boolean rutinaEnsayo(String nombreDirectorioArchivo){
//  static int cantMediosSegundos = 0;
  int segundos, decimal;
  static String lineaMedicionAnterior = "";  //Para no guardar en Maquina_1.csv el valor de presión durante loa caída. Interesa guardar la máxima antes del reventado
    
  char medicionPresion[5];
  static String lineaMedicion = "";
  String lineaMedicionNombreArchivo = ""; //Para escribir la última medición en Maquina_1 incluyendo el nombre del archivo
  String escribeUltimaMedicion = "";  //Sirve para registrar la máxima presión alcanzada antes de la ruptura.
  float presion1 = 0;               //Presión medida en preservativo
  static float presion1Anterior = 0;       //Presión medida en el ciclo anterior en preservativo
  boolean registrarDatos = LOW;     //Se pone en HIGH cuando la función tiempoCumplido() indica el momento de tomar medición, para no usar delay() bloqueante
  int presion1PartEntera = 0;       //Separo el valor real de presión para poder meterlo en un String
  int presion1Partdecimal = 0;      //Separo el valor real de presión para poder meterlo en un String
  //boolean inicio = HIGH;            //Para verificar s se presionó el pulsador INICIO durante el ensayo y detener el mismo (parada de emergencia)
  int valorAD_fuelle = 0;
  boolean bajaPresionFuelle = LOW;
 // boolean altaPresionFuelle = LOW;
  static  boolean guardarLineaMedicionAnterior = false;   //Sirve para registrar la máxima presión alcanzada antes de la ruptura.
  static boolean superoPresionMinima = false;            //Por debajo de 0.1 kPa tiene mucho error y no se considera la medicion.
  static int valorAD_minimo_fuelle;  
  String lineaMedicionLCD = ""; 
  static boolean datoAnexadoEnSD = true;
  boolean ensayoEnCurso = true;
  static boolean fuelleOK = false;
  
  String volumenAcumulado;
  static char esperaFuelle = 0;

  static String directorioArchivoOT;
//  String directorioOT = nombreDirectorioArchivo.substring(0,13);  
  String archivoOT = nombreDirectorioArchivo.substring(1,12);           
  directorioArchivoOT = nombreDirectorioArchivo.substring(0,13);
  directorioArchivoOT += archivoOT;
  directorioArchivoOT += ".csv";  //Se crea el archivo de registro de la OT con la última medición de cada ensayo. Por cada OT se revientan muchos preservativos
//  Serial.printf("Archivo OT: ");
//  Serial.println(directorioArchivoOT);
//  if(suspenderEnsayo == true) timeOut + 1;
  if(segundosAcumulados < 1){
//  if(cantMediosSegundos < 1){
  valorAD_minimo_fuelle = EEPROM.readInt(addressEEPROM_1kPa_1); //Valor de presión mínimo (en entero del AD) en el fuelle (sensor de presión 2)   
  }                                             //Para que lea los valores una vez por ensayo
  digitalWrite(electrovalvulaFuelle, HIGH);
  valorAD_fuelle = analogRead(sensorPresion2);                                                            
  if(valorAD_fuelle < valorAD_minimo_fuelle){
      if(flagEntroTimer0)  esperaFuelle++;
      if(esperaFuelle > 5){
      segundosAcumulados = timeOut + 1;  
//        cantMediosSegundos =  timeOut + 1; 
        imprimirLcd1.imprimirLCDfijo("Falla de Fuelle    ",0, 2);        
      } 
    }else
    {
    //bajaPresionFuelle = HIGH;      //infla preservativo y el fuelle de sujeción 
    digitalWrite(electrovalvulaPresrevativo, HIGH);
    fuelleOK = true;
    esperaFuelle = 0;
  }   

//  delay(200);
    lineaMedicion = "";
    if(flagEntroTimer0 && fuelleOK){
        presion1 = obtenerPresion1();            //Se obtiene la presión en el preservativo
        presion1PartEntera = int(presion1);      //sprintf no formatea float, entonces separo partes entera y decimal
        presion1Partdecimal = int((presion1 - presion1PartEntera) * 100);
        sprintf(medicionPresion, "%d.%02d", (int)presion1, presion1Partdecimal);
        volumenAcumulado = obtenerVolumenAcumulado();
        //Ahora se mide cada medio segundo
//        segundos = cantMediosSegundos/2;
//        decimal = cantMediosSegundos%2;
//        if(decimal == 1)  decimal = 5;
        /*lineaMedicion += String(segundos);
        lineaMedicion += ".";
        lineaMedicion += String(decimal);*/
        lineaMedicion += String(segundosAcumulados, 1);
        lineaMedicion += ",  ";
        lineaMedicion += String(medicionPresion);
        lineaMedicion += ",  ";
        lineaMedicion += volumenAcumulado;
        lineaMedicionLCD += lineaMedicion;
  
//        cantMediosSegundos ++;
        if(estadoActual == 4) imprimirLcd1.imprimirMedicionLCD(String(lineaMedicionLCD), 2);
        lineaMedicionLCD = "";
  //      registrarDatos = tiempoCumplido1.calcularTiempo(true);
        guardarLineaMedicionAnterior = true;
        flagEntroTimer0 =false;
  //      cantMediosSegundos ++;

        if(presion1 > 0.15)  { superoPresionMinima = true; }
        if(superoPresionMinima == true){
          escribeUltimaMedicion += nombreDirectorioArchivo;
          escribeUltimaMedicion += ", ";
          escribeUltimaMedicion += lineaMedicionAnterior;
 //         datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), lineaMedicion.c_str());   
          if(presion1 < (presion1Anterior * 0.6)){       //Si la presión cae un 40% se asume que el preservativo reventó
            Serial.println("Entro a condicional caida 60% ");
      
//              escribeUltimaMedicion = escribeUltimaMedicion.substring(0,48);
              escribeUltimaMedicion = escribeUltimaMedicion.substring(0,54);
              escribeUltimaMedicion += ", Reventado \r\n";
               
              datoAnexadoEnSD = tarjetaSD1.appendFile(SD, directorioArchivoOT.c_str(), escribeUltimaMedicion.c_str());
              superoPresionMinima = false; 
  //            cantMediosSegundos = timeOut + 1;       //No vuenve a entrar en  while(cantMediosSegundos < timeOut) ni entra en if(cantMediosSegundos == timeOut)
              segundosAcumulados = timeOut + 1;       //No vuenve a entrar en  while(cantMediosSegundos < timeOut) ni entra en if(cantMediosSegundos == timeOut)
          }else{
//            cantMediosSegundos = finEnsayoSinRuptura(cantMediosSegundos,lineaMedicion, nombreDirectorioArchivo, directorioArchivoOT, suspenderEnsayo);
            finEnsayoSinRuptura(lineaMedicion, nombreDirectorioArchivo, directorioArchivoOT);
            lineaMedicion += ", \r\n";
//            Serial.print(lineaMedicion);         //Si se quita el envío al puerto serie, agregar delay(10) para que escriba bien en la sd el fin de línea
//            if(cantMediosSegundos <= timeOut) datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), lineaMedicion.c_str());   
            if(segundosAcumulados <= timeOut){
              datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), lineaMedicion.c_str());   
//              datoAnexadoEnSD = tarjetaSD1.appendFile(SD, directorioArchivoOT.c_str(), escribeUltimaMedicion.c_str());    //Escribe última medición antes de reventado en archivo de OT#
            }
          }    
        }
    
        if(guardarLineaMedicionAnterior == true){   //Sirve para guardar la máxima presión en el preservativo antes del reventado
          presion1Anterior = presion1;
          lineaMedicionAnterior = " ";
          lineaMedicionAnterior += lineaMedicion;
          guardarLineaMedicionAnterior = false;
        }
//      cantMediosSegundos ++;
    }
    if(!datoAnexadoEnSD){
      segundosAcumulados = timeOut + 1;
     // cantMediosSegundos = timeOut + 1;       //Para que salga del While  
      Serial.println("Fallo escritura SD ");
    fallaEscrituraSD();
    }
  if(suspenderEnsayo == true) segundosAcumulados = timeOut + 1; //cantMediosSegundos = timeOut + 1;
  if(segundosAcumulados >= timeOut){
    digitalWrite(electrovalvulaPresrevativo, LOW);
    digitalWrite(electrovalvulaFuelle, LOW);
    flagEntroTimer0 =false;
  //  cantMediosSegundos = 0;
    lineaMedicionAnterior = "";  //Para no guardar en Maquina_1.csv el valor de presión durante loa caída. Interesa guardar la máxima antes del reventado
    ensayoEnCurso = false;
          inParser = 'B';       // Cambio de estado en Parser
//    imprimirLcd1.imprimirLCDfijo("Presione A          ",0, 3);
    presion1Anterior = 0;       //Para que no conserve el valor luego de un time out
  }
  return ensayoEnCurso;
}
//********************************************************************************
String rutinaInicioEnsayo(){
    String nombreDirectorioArchivo = "";
    String cabecera = "";
    String nombreDirectorio = "";
    String tempHumedadLCD = "";
    boolean datoAnexadoEnSD;
    int cantArchivos = 0;
    String otActual = "";     
   
    otActual = EEPROM.readString(addressEEPROM_ultimaOT);
//      Serial.print("otActual: ");
//      Serial.println(otActual);
    
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
  //  nombreDirectorio += String(dateISO);
    nombreDirectorio += otActual.substring(3,14);
    tarjetaSD1.createDir(SD,nombreDirectorio.c_str());
    char dateTimeISO[26];
    sprintf(dateTimeISO, "%02d%02d%02dT%02d%02d%02d", now.year(), now.month(),now.day(),  now.hour(), now.minute(), now.second()); 
    nombreDirectorioArchivo += nombreDirectorio;
    nombreDirectorioArchivo += "/";
    nombreDirectorioArchivo += String(dateTimeISO);
    nombreDirectorioArchivo += ".csv";
    tarjetaSD1.writeFile(SD, nombreDirectorioArchivo.c_str(), "Máquina número:, 01 \r\n");

    imprimirLcd1.imprimirLCDfijo("Leyendo microSD",0, 2);
    imprimirLcd1.imprimirLCDfijo("Espere...",0, 3);
    datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), "Fecha  y  hora, Temperatura, Humedad \r\n");
    cantArchivos = tarjetaSD1.listCantidadArchivos(SD, nombreDirectorio.c_str(), 0);    //Indica en LCD la cantidad de archivos

    imprimirLcd1.imprimirLCDfijo(String(cantArchivos),17, 0);
    imprimirLcd1.imprimirLCDfijo(String(dateTimeISO),0, 1);
        
    char dateTime[20];
    sprintf(dateTime, "%02d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(),  now.hour(), now.minute(), now.second()); 
    cabecera += String(dateTime);
    cabecera += "  ,";
    cabecera += String(temp);
    cabecera += "  ,";
    cabecera += String(humed);
    cabecera += "\r\n";
    imprimirPC(now, temp, humed);
    datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreDirectorioArchivo.c_str(), cabecera.c_str());
    String str = nombreDirectorioArchivo.c_str();  
    if(!datoAnexadoEnSD){
      fallaEscrituraSD();
    }
//    Serial.println("Termino Rutina Inicio Ensayo");
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
  if(comando == "setRTC"){              //Si recibe este String seguido por ":" por puerto serie seteará el RTC con la siguiente cadena año mes dia hora minutos cantMediosSegundos
    dateTime = Serial.readString();     // Ej.:Comando por puero serie: setRTC:20210213T163218  -> año:2021, mes: 02, día: 13, hora: 16, minutos: 32, cantMediosSegundos: 18
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
      tarjetaSD1.listDir(SD, "/", 0, false);
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
    delay(100); // Taking a little time to allow you to release the button
   }
   return(teclaLeida);
}
//****************************************************************************************
void funcionCalibrar(char letraLeida){
  static boolean enCalibracion = false;
  boolean calibracion;
      calibracion = digitalRead(pulsadorCalibracion);
      if(!calibracion){
        enCalibracion = true;
        Serial.println("En calibracion....");
      }  
      if(enCalibracion) calibrarSensores1.calibrar(letraLeida);  
}
//****************************************************************************************
void mensajeErrorLCD(void){
  imprimirLcd1.limpiarLCD();

    imprimirLcd1.imprimirLCDfijo("ERROR",7, 0);
    imprimirLcd1.imprimirLCDfijo("Presione O (C)",5, 3);

  if(codigoErrorComm == 1){
    imprimirLcd1.imprimirLCDfijo("Reint_WCM",5, 1);
  }
  if(codigoErrorComm == 2){
    imprimirLcd1.imprimirLCDfijo("VerifWCM",5, 1);
  }
  if(codigoErrorComm == 3){
    imprimirLcd1.imprimirLCDfijo("Reint_SP",5, 1);
  }
  if(codigoErrorComm == 4){
    imprimirLcd1.imprimirLCDfijo("Verif_SP",5, 1);
  }
  if(codigoErrorComm == 5){
    imprimirLcd1.imprimirLCDfijo("reintCaudal",5, 1);
  }
  if(codigoErrorComm == 6){
    imprimirLcd1.imprimirLCDfijo("CaudalRec",5, 1);
  }

}

boolean controlCaudal(char letraLeida, boolean ensayoEnCursoAux){
  boolean ensayoEnCurso;
  static int estadoActualAnterior;
  ensayoEnCurso = ensayoEnCursoAux;   //Si viene False conserva estado, si viene true cambia a false si hay error en medición de caudal.
  if (llamaParserComm)
  {
    if(estadoActualAnterior != estadoActual){
      Serial.print("Estado Eactual Comm: ");
      Serial.println(estadoActualComm);
      estadoActualAnterior = estadoActual;
    }
  //    Serial.print("Estado inParserComm: ");
  //    Serial.println(inParserComm);
    llamaParserComm = false;
    ParserComm(); //LLama al parser
  }  
  while( estadoActualComm & MASKINESTABLE ){//Si el estado actual es inestable, llama al parser.
    estadoActualComm = estadoActualComm & ~MASKINESTABLE; //Quita posible inestabilidad.
    ParserComm();
  }
  if(errorSetUpCaudal == true){
    mensajeErrorLCD();
    Serial.println("Entro en errorSetUpCaudal");
    errorSetUpCaudal = false;
    ensayoEnCurso = false;
  } 
 /* static int estadoActualCommAnterior = 0;
  if(estadoActualComm != estadoActualCommAnterior){       // 11 = MENSAJE_ERROR_LCD
    estadoActualCommAnterior = estadoActualComm;
    Serial.print("EstadoActualComm: ");
    Serial.println(estadoActualComm);
  }*/
//  if(estadoActualComm == MENSAJE_ERROR_LCD){       // 11 = MENSAJE_ERROR_LCD
  if(estadoActualComm == WR_CONTROL_MODE){       // 1 = WR_CONTROL_MODE
      if(letraLeida == 'O'){        //'O' es 'C' en teclado
        estadoActual =  M_PPAL;
        IniParserComm();
        llamaParserComm = true;
        Serial.println("Presiono 'O'");
        imprimirLcd1.limpiarLCD();
        imprimirLcd1.imprimirLCDfijo("REINTENTANDO",3, 2);
        delay(1500);
      }   
  }
  return(ensayoEnCurso);
}
//*****************************************************************************************
void loop() {

  boolean calibracion;
  boolean entroEnCalibracion;
  static String nombreDirectorioArchivo = "";
  boolean estadoPulsadorInicio, consultarTeclado, inicio;
  static boolean ensayoEnCurso = false;
  boolean ensayoEnCursoAux = false;
  static boolean  permiteTerminarPorTeclado = false;
  char letraLeida = 'z';
  static int i = 0;
  String stringEstadoActual = "";

  while (Serial.available() > 0) {    // Es para lectura del puerto serie
  leerSerie(); 
  }

  consultarTeclado = tiempoCumplido2.calcularTiempo(false);
  if(consultarTeclado){
    letraLeida = leerTeclado();
    consultarTeclado = tiempoCumplido2.calcularTiempo(true);
    if(letraLeida != 'z'){
      //inParser = Serial.read();
      inParser = letraLeida;
      if(estadoActualComm >= WAIT_MED){ 
        Parser(); //LLama al parser
      }

    }
  } 

  if(flagEnsayoEnCurso == true){
    timerAlarmEnable(timer1); //Habilita la interrupción del timer1.

    nombreDirectorioArchivo = rutinaInicioEnsayo();
    ensayoEnCurso = true;
  	Serial.println("Entro a flagEnsayo en curso");
    flagEnsayoEnCurso = false;
    flagResetRespArray = true;
    flagResetVolumen = true;
    flagInicializarpreviousMillis = true;
    segundosAcumulados = 0;
}
  ensayoEnCursoAux = ensayoEnCurso;
  ensayoEnCurso = controlCaudal(letraLeida, ensayoEnCursoAux);
  if(ensayoEnCurso){
    ensayoEnCurso = rutinaEnsayo(nombreDirectorioArchivo); 
//    permiteTerminarPorTeclado = true;
    if((!ensayoEnCurso) && (estadoActualComm != MENSAJE_ERROR_LCD)){   
      inParser = TERMINADO;
      Parser();
      timerAlarmDisable(timer1); //Habilita la interrupción del timer1.
    }
    flagContabilizarVolumenes = true;
  }else{
    flagContabilizarVolumenes = false;
  }

  if(flagCalibracionEnCurso){
      funcionCalibrar(letraLeida);    
  } 

  while( estadoActual & MASKINESTABLE ){
//    Serial.println("Es inestable ");
    estadoActual = estadoActual & ~MASKINESTABLE; //Quita posible inestabilidad.
//    Serial.println(estadoActual);
	  Parser();       //Si el estado actual es inestable, llama al parser.  
  }
  if(llamarMPPAL){    //Necesario para que aparezca el menu principal porque la máquina de estados del controlador de caudal
    IniParser();      //imprime pantalla de error hasta que logra el setUp del controlador y pasa al estado waitMed
    Parser();         //poniendo llamarMPPAL = true en la función AWaitMed()
    llamarMPPAL = false;
  }
}
