/*
  ESP32-INTI-Electrónica
  Versión 1.0
  Fecha   13/12/2021
  Link    https://github.com/martuan/ESP32-INTI-Electronica/

  Sistema IoT para diversas aplicaciones en INTI con capacidad de 
  enviar datos a la nube y acceder a ellos con cualquier dispositivo 
  conectado a la red. 
  
  Características [Modificar]:
  * Manejo de display, buzzer, leds, sensor ultrasónico, sensor IR
  * Posee método de watchDog para resetear el microcontrolador si este 
    queda bloqueado
  * Descarta temperaturas fuera de un rango determinado
  * Establece conexión Wifi para enviar datos utilizando protocolo mqtt
  * Posee dos modos de funcionamiento: Modo local y Modo Red
    (seleccionables mediante switch externo)
  * Obtiene la fecha y hora (timeStamp) desde un servidor de reloj global
  * En caso de perder la conexión wifi/mqtt automáticamente se pone en 
    modo local. Al reestablecerse la conexión, vuelve al modo red
  * Permite cambiar ciertos parámetros por puerto serie o bluetooth
  * //Almacena una cantidad de lecturas en memoria interna EEPROM
  * Mide con 1 o 2 o 3 sensores IR y promedia las lecturas
  * Permite consultar las últimas lecturas
  * Permite escannear dispositivos i2C, grabar coeficiente de emisividad y 
    modificar dirección del dispositivo
  * Graba la temperatura de Offset en memoria EEPROM
  * Señaliza visualmente mediante tira de leds Neopixel
  * Señaliza acústicamente mediante buzzer variando la frecuencia del beep 
    de acuerdo a la distancia del objeto a medir
  * Envía mensaje de keepAlive cada cierto tiempo para indicar que el cliente está vivo 
  * Configurar SSID y pass, con un web service.
  * Establece el nombre del broker al enviarlo por puerto serie o bluetooth
  * Guía a través del display cómo conectarse a una red wifi (Modo Access Point)
  * Para github: se corrige el String recibido por Serie o Bluetooth
  *  eliminando caracteres no deseados como CR y LF.
  *  Se cambia la distancia de tolerancia a 1.0 cm
  *  Se cambia string del broker mqtt, ahora con ip
  *  Se cambia tiempo de reconexión a la red
  *  Se contabilizan las reconexiones exitosas y las fallidas
  *  Imprime por bluetooth las credenciales wifi almacenadas en EEPROM
*/

#include <Arduino.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MLX90614_modificado.h>
#include <SparkFunMLXm.h>
#include <ArduinoJson.h>
#include <NewPing.h>
#include <SPI.h>
#include <Adafruit_SH1106.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <esp_task_wdt.h>
#include <BluetoothSerial.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager  
//#include "funcionesHandtemp.h"


//#define HARDWARE ESP32

// define the number of bytes you want to access
#define EEPROM_SIZE 300

#define EEPROM_ADDRESS_OFFSET 0//double (8 bytes)
#define EEPROM_ADDRESS_NUMSENSOR 8//uint8_t (1 byte)
#define EEPROM_ADDRESS_FLAG_MODO_AP 16//uint8_t (1 byte)
#define EEPROM_ADDRESS_FLAG_OTA 18//uint8_t (1 byte)
#define EEPROM_ADDRESS_WIFI_SSID 20//String
#define EEPROM_ADDRESS_WIFI_PASS 60//String
#define EEPROM_ADDRESS_MQTT_SERVER 100//String


#define WDT_TIMEOUT 60//30//60//30 (dejar 30)
#define WDT_TIMEOUT_LONG 180//30

// Conexiones SPI para el display:
#define OLED_MOSI   13
#define OLED_CLK   14
#define OLED_DC    27
#define OLED_CS    15
#define OLED_RESET 26

#define LINEHEIGHT 22  // para saltos de linea

// Neopixel
#define PIN 33
#define CANTLEDS  16
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(CANTLEDS, PIN, NEO_GRB + NEO_KHZ800);


// Sensor de distancia ultrasónico
#define TRIGGER_PIN  17
#define ECHO_PIN     16
#define MAX_DISTANCE 200

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
Adafruit_SH1106 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

String version = "V:2.00";


//**************************************
//*********** MQTT CONFIG **************
//**************************************

String nombreSens = "sensor_4";

//Servidor en la nube
//const char *mqtt_server = "broker.hivemq.com";//"10.24.37.150";//"broker.hivemq.com";//"test.mosquitto.org";//"10.24.37.150";//"broker.hivemq.com";//"10.24.37.150";//"broker.hivemq.com";
char mqtt_server[100] = "10.24.37.150";//"broker.hivemq.com";
const int mqtt_port = 1883;
const char *mqtt_user = "";
const char *mqtt_pass = "";
char root_topic_subscribe[100] = "handtemp/sensor_1";//"undefi";
char root_topic_publish[100] = "handtemp/sensor_1";//"undefi";
char tempAmbiente_topic_subscribe[100] = "handtemp/sensor_1";//"undefi/handtemp/1/tempAmbiente";
char tempAmbiente_topic_publish[100] = "handtemp/sensor_1";//"undefi/handtemp/1/tempAmbiente";
char tempObjeto_topic_subscribe[100] = "handtemp/sensor_1";//"undefi/handtemp/1/tempObjeto";
char tempObjeto_topic_publish[100] = "handtemp/sensor_1";//"undefi/handtemp/1/tempObjeto";
char handtempKeepAlive_topic_publish[100] = "handtemp/keepAlive";


//**************************************
//*********** GLOBALES   ***************
//**************************************

WiFiClient espClient1;
PubSubClient client1(espClient1);

long count=0;
hw_timer_t * timer3;
int tiempo1 = 60;//60 segundos
char flagProceso1 = 0;
char flagProceso2 = 0;
char flagProceso3 = 0;
int contadorProceso1 = 0;
int contadorProceso2 = 0;
const int ledTempFiebre = 33;
const int switchDeModo = 34;
const int switchDeConfigWifi = 35;
double tempFiebre = 37.50;
double tempMin = 32.00;//25.00;
double tempMax = 42.00;//45.00;
float distanciaConfigurada = 3.00;
float distanciaTolerancia = 1.00;//0.20;//0.10;//0.20;
int flagCambioModoLocal = 0;
int cantFallos = 0;
int tiempoEntreLecturas = 10;
int cantLecturas = 20;
double tempAmbC = 0.0;
String codigoDeError = {};
int flagModoDebug = 0;
int tiempoMinutoSinReset = 0;
int tiempoHoraSinReset = 0;
int tiempoDiaSinReset = 0;
int cantSensoresIR = 1;
int flagLecturaEmisividad = 1;
double tempValidaMax = 50.00;
double tempValidaMin = 0.00;
int flagLecturasErroneas = 0;
float arrayUltimasLecturas[100] = {};
int idx2 = 0;
int flagUltimasLecturas = 0;
char outstr[10] = {};
float valor = 0;
int Red = 0;
int Green = 0;
int Blue = 0;
int neopixelFueApagado = 0;
char tiempoSinReset[50] = {};
char msgKeepAlive[150] = {};
int flagCorreccion = 1;//0;
int flagScan = 0;
int flagModoRed = 0;
int timeLED = 1000;
String strEEPROM = "";
String clientId = "";
int flagConexionOK = 0;//para cuando wifi y mqtt conectan correctamente
uint8_t numeroSensor = 0;
String macAdd = "";
String broker = "";
uint8_t flagModoAP = 0;
int lecturaSwitchMODO = 0;
String savedSSID = {};
String savedPASS = {};
String ssid = {};
String password = {};
char flagVieneDelReset = 0;
char flagModoAPforzado = 0;
char flagOTA = 0;
String ssidOTA = {};
String passwordOTA = {};
int contadorReconexionFallida = 0;
int contadorReconexionExitosa = 0;

// ***************** OFFSET ***********************
// Offset para el sensor de temperatura
double tempOffset = 9.5;//5.5;//2.0;//0.0;
// ***************** OFFSET ***********************



// **************** EEPROM *************************
double tempOffsetEEPROM = 0;
uint8_t numSensorEEPROM = 0;
String brokerEEPROM = "";
uint8_t modoAccessPointEEPROM = 0;
uint8_t flagOTAEEPROM = 0;
String wifiSSIDEEPROM = {};
String wifiPASSEEPROM = {};
// **************** EEPROM *************************

Adafruit_MLX90614 mlx1 = Adafruit_MLX90614(90);//address (0x00 to 0x07F)

float indiceEmisividad = 0.97;//ingresar el índice de emisividad

uint16_t emisividad = indiceEmisividad * 65535;
uint16_t emisividadLeida = 0;
int emisividadEEPROM = 0;
float emisividadPorSerie = 0.00;
double emisividadL = 0;
int flagEmisividad = 0;


//**** NTP CONFIG (hora mundial) ********
const char* ntpServer = "pool.ntp.org";//server del rtc global
const long  gmtOffset_sec = -10800;//3600;
const int   daylightOffset_sec = 0;//3600;

struct tm timeStamp;

char timeYear[10];
char timeMonth[10];
char timeDayName[10];
char timeDayNumber[10];
char timeHour24[10];
char timeHour12[10];
char timeMinute[10];
char timeSecond[10];
char fechaHora[50];
//**** NTP CONFIG (hora mundial) ********

//**************************************
//*********** BUZZER   *****************
//**************************************

// * Basado en: http://www.instructables.com/id/Arduino-Controlled-Flashing-Christmas-Fairy-Lights/step5/Code-for-Jingle-Bells/
// Relaciones entre nota, periodo y frecuencia.

#define  C1     1915
#define  D1     1700
#define  Eb1    1620 
#define  E1     1519
#define  F1     1432    
#define  G1     1275
#define  A1     1136
#define  B1     1014
#define  C2     956

// La nota R representa un silencio
#define  R     0

//int melody_beep[] = {C1, R, C1, R};
int melody_beep[] = {C1, R, R, R};
int melody_denied[] = {G1, Eb1, C1, R};
int melody_ok[] = {C1, E1, G1, C2};
int MAX_COUNT = sizeof(melody_ok) / 4; // Longitud de melodia
// tempo
long tempo = 10000;
// Silencio al final, tiene un loop controlado por rest_count
int rest_count = 1;
// Inicializacion
int tone_ = 0;
int beat = 20;
long duration  = beat * tempo;

// Asignamos gpio
const int buzzPin = 32;

//**************************************
//*********** DISPLAY  *****************
//**************************************

// fuentes de texto
auto mainFont = &FreeSans12pt7b;
auto font1 = &FreeSans9pt7b;
auto font2 = &FreeSans12pt7b;
auto font3 = &FreeSans18pt7b;

//**************************************
//*********** BLUETOOTH  *****************
//**************************************

BluetoothSerial SerialBT;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


//************************
//** F U N C I O N E S ***
//************************


// WIFI Y MQTT
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setup_wifi();
// SENSOR IR
double leerSensorIR(uint8_t);
//void cambioEmisividadPorSerie(void);
// BUZZER
void playTone(int melody[]);
void playTone2(int);
// DISPLAY
void handTempText(void);
void waitingForHand();
void waitingForHandCustom1(void);
void waitingForHandCustom2(void);
void waitingForHandCustom3(void);
void showTemperature(double temperature, String msg);
void showTemperatureOutOfRange(double temperature);
void showAllDataDebugMode(double tempC, double tempAmbC, float dist);
void cambioDeParametros(void);
//void grabarEmisividadEnSensorIR(void);
void publicarDataAmbiente(double, char[]);
void publicarDataObjeto(double, double, double, char[]);
void obtenerFechaHora(void);
int chequearCausaDeReset(void);
void imprimirUltimasLecturas(void);
double realizarCorreccion(double, double, double);
void cambiarDireccionI2C(int numSensor, uint16_t newDir);
void scannearDispositivosI2C(void);
void cambiarDireccionI2CNuevaVersion(byte, byte);
void evaluarDistancia(float);
void encenderNeopixel(char);
void apagarNeopixel(void);
void publicarKeepAlive(void);
void contabilizarTiempoSinReset(void);
void cambiarConfigMQTT(uint8_t);
void comprobarConexion(void);
//void intercambiarSensores(void);
void probarNeopixel(void);
void cargarDesdeEEPROM(void);
void setupModoRed(void);
void fallaConexion(void);
void displayText(char);
void switchCaseParametros(char, String);
void modoAccessPoint(void);
void imprimirModo(void);

//timer de usos generales 1000ms
void IRAM_ATTR onTimer3() {

    contadorProceso1++;
    contadorProceso2++;

    if(contadorProceso1 == tiempo1){//proceso 1
        contadorProceso1 = 0;//resetea el contador
        flagProceso1 = 1;
        flagProceso2 = 1;
    }
    if(contadorProceso2 == tiempo1 * 10){//proceso 2 (cada 10 min)
        contadorProceso2 = 0;//resetea el contador
        flagProceso3 = 1;//para chequear conexión con red wifi y broker
    }

    
}


//*************************************************************************
// SETUP
//*************************************************************************




void setup() {
    
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    Serial.begin(115200);
        
    int causaReset = 0;
    flagVieneDelReset = 1;
    SerialBT.begin("ESP32 Handtemp Bluetooth"); //Bluetooth device name

    causaReset = chequearCausaDeReset();

    if(causaReset == 6){//si volvió de un reset por task watchDog
      Serial.println("recuperación del sistema: watchDog causó el reboot");
    }

    macAdd = WiFi.macAddress();
    Serial.println( "MAC address: " + macAdd );

    

    timer3 = timerBegin(2, 80, true);
    timerAttachInterrupt(timer3, &onTimer3, true);
    timerAlarmWrite(timer3, 1000000, true);//valor en microsegundos [1 s]
    timerAlarmEnable(timer3);

    esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch

    mlx1.begin();

    pinMode(buzzPin, OUTPUT); // config pin de buzzer
    pinMode(switchDeModo, INPUT_PULLUP);
    pinMode(switchDeConfigWifi, INPUT_PULLUP);

    //digitalWrite(buzzPin, HIGH);
    cargarDesdeEEPROM();//levanta las variables guardadas previamente en EEPROM
    
    // inicio display
    display.begin(SH1106_SWITCHCAPVCC);
    display.clearDisplay();//borra cualquier dato del buffer del display
    display.setFont(mainFont);
    display.println(" ");//línea vacía
    display.display();


    // pantalla de bienvenida
    handTempText();

    pixels.begin(); // This initializes the NeoPixel library.
    probarNeopixel();

    //leer la distancia con sensor ultrasónico para ver si se solicita modoAP
    float uS = sonar.ping();
    float distancia = uS/57.0;    // The constant "US_ROUNDTRIP_CM" = 57
    //distancia = 3.00;
    Serial.println(distancia);
    if(distancia >= 0.01 && distancia <= 20.00){
      Serial.println(distancia);
      Serial.println("Se iniciará en modoAP forzado");
      flagModoAPforzado = 1;//se configura para elegir red desde un smartphone (modoAP)
    }
    
    if(flagOTA == 1){

      Serial.println("Booting - MODO OTA");
      WiFi.mode(WIFI_STA);

      //ssidOTA = "medtempOTA";
      //passwordOTA = "esp32OTA";
      //ssidOTA = "milton";
      //passwordOTA = "paternal";

      ssidOTA = ssid;//copia credenciales guardadas en EEPROM
      passwordOTA = password;//copia credenciales guardadas en EEPROM

      WiFi.begin(ssidOTA.c_str(), passwordOTA.c_str());
      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        //Serial.println("Connection Failed! Rebooting...");
        Serial.println("Conexión Fallida, esperando credenciales wifi...");
        cambioDeParametros();//chequea si le envían parámetros a trevés de bluetooth o serie
        ssidOTA = ssid;//copia credenciales guardadas en EEPROM
        passwordOTA = password;//copia credenciales guardadas en EEPROM
        WiFi.begin(ssidOTA.c_str(), passwordOTA.c_str());
        delay(5000);
        //ESP.restart();
      }
      Serial.println("Se deshabilita el modo RED mientras está activado OTA");
      // Port defaults to 3232
      // ArduinoOTA.setPort(3232);

      // Hostname defaults to esp3232-[MAC]
      // ArduinoOTA.setHostname("myesp32");

      // No authentication by default
      // ArduinoOTA.setPassword("admin");

      // Password can be set with it's md5 value as well
      // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
      // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

      ArduinoOTA
        .onStart([]() {
          String type;
          if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
          else // U_SPIFFS
            type = "filesystem";

          // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
          Serial.println("Start updating " + type);
        })
        .onEnd([]() {
          Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
          Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
          Serial.printf("Error[%u]: ", error);
          if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
          else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
          else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
          else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
          else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

        ArduinoOTA.begin();

        Serial.println("Ready");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }

        
    if(flagOTA == 0){
      

      if(analogRead(switchDeModo) == 0){//si está en MODO RED
  
        setupModoRed();//configura MQTT, revisa conectividad
   
      }  
    }

    

    imprimirModo();//informa por display Modo local o Modo Red


    
    
}


//*************************************************************************
// LOOP
//*************************************************************************


void loop() {

    
    float distancia = 0.00;
    unsigned int uS = 0;
    double tempF = 0;
    double tempC = 0;
    double tempC1 = 0;
    double tempC2 = 0;
    double tempC3 = 0;
    double tempAmbC1 = 0;
    double tempAmbC2 = 0;
    double tempAmbC3 = 0;
    double emisividadL1 = 0;
    double emisividadL2 = 0;
    double emisividadL3 = 0;
    int i = 0;
    int sensorValue = 0;

    if(flagOTA == 1){
      ArduinoOTA.handle();
    }
    
    //digitalWrite(buzzPin, HIGH);

    cambioDeParametros();

    client1.loop();

    //Keep Alive
    //Si está en modo Red y pasó cierto tiempo y la conexión está OK
    if(analogRead(switchDeModo) == 0 && flagProceso2 == 1 && flagConexionOK == 1){//Modo RED
    
      flagProceso2 = 0;
    
      if(client1.connected()){
        publicarKeepAlive();
      }else{
        Serial.println("MODO LOCAL...(temporal)");
        flagConexionOK = 0;//hubo un problema, lo avisa mediante el flag
      }
   
    }

    if(flagLecturaEmisividad == 1){//leer la emisividad de los sensores
      leerEmisividad();
      flagLecturaEmisividad = 0;
    }

    if(flagScan == 1){//si está activado el escaneo de dispositivos i2c
      flagScan = 0;
      scannearDispositivosI2C();
    }
    
    //apagar LEDs Neopixel
    if(neopixelFueApagado == 0){//si no fue apagado, apagar
      apagarNeopixel();
    }

    //alimentar watchDog
    esp_task_wdt_reset();

    

    
    //a modo informativo, va contando el tiempo que pasa durante funcionamiento normal
    if(flagProceso1){
      flagProceso1 = 0;
      contabilizarTiempoSinReset();
    }

    //cada cierto tiempo chequea la conexión MQTT
    //si el switch de modo esta en red, pero perdió la conexión
    lecturaSwitchMODO = analogRead(switchDeModo);
    //if(flagProceso3 && (analogRead(switchDeModo) == 0) && flagConexionOK == 0){
    if(flagProceso3 && lecturaSwitchMODO == 0 && flagConexionOK == 0){
      //prueba, ver valor del ADC
      Serial.print("lecturaSwitchMODO = ");
      Serial.println(lecturaSwitchMODO);

      
      flagProceso3 = 0;
      displayText('R');
      Serial.println("Intentando recuperar la conexión");
      comprobarConexion();//si alguna conexión se perdió, la reestablece
      if(flagConexionOK){//si la recuperó
        Serial.print("Se ha recuperado la conexión. flagConexionOK = ");
        Serial.println(flagConexionOK);
        displayText('1');
        contadorReconexionExitosa++;
      }else{//si no la recuperó
        
        Serial.print("[PROBLEMAS] No se ha recuperado la conexión. flagConexionOK = ");
        Serial.println(flagConexionOK);
        displayText('0');
        contadorReconexionFallida++;
      }
    }
   
   
    //leer la distancia con sensor ultrasónico
    uS = sonar.ping();
    distancia = uS/57.0;    // The constant "US_ROUNDTRIP_CM" = 57
    //distancia = 3.00;
    if(distancia >= 200 || distancia == 0.00){
      distancia = 200;
    }
  
    evaluarDistancia(distancia);
   
    //si distancia está OK (acepta distancia flotante con tolerancia)
    if(distancia >= distanciaConfigurada - distanciaTolerancia && distancia <= distanciaConfigurada + distanciaTolerancia){//si la superficie está a la distancia correcta para medir

      Serial.print("Distancia: ");
      Serial.print(distancia);
      Serial.println("cm");

      //realiza la lectura y muestra los valores por consola

      tempC1 = leerSensorIR(1);
      tempAmbC1 = mlx1.readAmbientTempC();
      emisividadL1 = mlx1.readEmissivity();

      tempC = tempC1;
      tempAmbC = tempAmbC1;
      
      if(flagCorreccion){//si está activada la corrección de temperatura
        tempC = realizarCorreccion(tempC, tempAmbC, tempOffset);
      }

      //almacenar valores en array para consultarlo cuando sea necesario
      arrayUltimasLecturas[idx2] = tempC;
    
      idx2++;
      if(idx2 == 100){
        idx2 = 0;
      }
      
      Serial.print("Temperatura *C: ");
      Serial.print(tempC);
      Serial.println("*C");
      Serial.print("Temperatura Ambiente *C: ");
      Serial.print(tempAmbC);
      Serial.println("*C");

        
      if(analogRead(switchDeModo) == 0 && flagConexionOK == 1){//Modo RED

        if(client1.connected()){//si está conectado
          publicarDataObjeto(tempC, tempF, emisividadL, fechaHora);
         }
      }

      //*********************** EVALUACIÓN DE TEMPERATURA *************
      
      //si la temperatura medida está dentro del rango permitido
      if(tempC >= tempMin && tempC <= tempMax){

        if(tempC >= tempFiebre){//Temperatura no OK
          
          encenderNeopixel('R');
          playTone(melody_denied);
          if(flagModoDebug == 0){
            showTemperature(tempC, "denegado");
          }else{
            showAllDataDebugMode(tempC, tempAmbC, distancia);
          }

        }else{//Temperatura OK
          
          encenderNeopixel('G');
          playTone(melody_ok);

          if(flagModoDebug == 0){
            showTemperature(tempC, "autorizado");
          }else{
            showAllDataDebugMode(tempC, tempAmbC, distancia);
          }

        }

      }else{//temperatura fuera de rango
        Serial.println("Temperatura fuera del rango permitido");
        showTemperatureOutOfRange(tempC);
      }

      Serial.print("Tiempo sin reset: ");
      Serial.println(tiempoSinReset);

    }else if(distancia > 20 || distancia == 0.00){//distancia no está OK

      waitingForHand();//display informa
   
    }
}



//*************************************************************************
// FUNCIONES
//*************************************************************************


//*****************************
//***    CONEXION WIFI      ***
//*****************************


void setup_wifi(){

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

  //reset settings - wipe credentials for testing
  //wm.resetSettings();

    WiFiManager wm;
   
    bool res;
    
    //Para no usar switch, puede detectarse un estado particular.
    //Por ejemplo, se activa la configuración Wifi cuando la distancia
    //leída es cercana a cero durante 10 segundos.
    //Podría lograrse, girando el equipo y conectándolo.  
    if(analogRead(switchDeConfigWifi) == 0 ){
      Serial.println("RESET WIFI SETTINGS");
        
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(6,30);
      display.setFont(font1);
      display.println("MODO AP");
      display.println("Reset WIFI");
      display.display();
      delay(2000);
      display.clearDisplay();
      display.setCursor(6,30);
      display.println("Restablecer");
      display.println("dip switch");
      display.display();
      delay(2000);
      display.clearDisplay();

      modoAccessPoint();//muestra por el display info para conectarse a una red wifi


      /*
      display.clearDisplay();
      display.setFont(mainFont);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      //display.setFont(mainFont);
      display.setCursor(6,30);
      display.println("Reset WIFI");
      delay(2000);
      display.clearDisplay();
      */
      wm.resetSettings();

      flagModoAP = 1;
      Serial.println("Se escribe en EEPROM flagModoAP = 1");
      //guarda en EEPROM el flag de modo AP
      EEPROM.write(EEPROM_ADDRESS_FLAG_MODO_AP, flagModoAP);
      EEPROM.commit();
      Serial.println("Se resetea la placa");
      delay(1000);
      ESP.restart();
    }
    



    //Poner un autoconnect corto con timeout para cuando debe recuperar conexión
    //y un autoconnect largo para cuando debe funcionar como Acces Point
    //if(flagModoAP == 1){//leído de la eeprom
    //if(flagVieneDelReset == 1 && flagModoAP == 1){//si viene de un reset por software o power on (conexión de alimentación)
    if(flagModoAPforzado == 1){//leído de la eeprom

        //Se detiene el wdt normal y se activa uno más largo para darle tiempo
        //al usuario a conectarse al wifi deseado
        flagVieneDelReset = 0;
        flagModoAPforzado = 0;
        esp_task_wdt_deinit();//deinit wdt normal
        esp_task_wdt_init(WDT_TIMEOUT_LONG, true); //init wdt largo
        
        modoAccessPoint();
        
        encenderNeopixel('B');//señaliza con los LEDs que está en proceso de conexión
        //esta función es bloqueante, ojo!!
        //wm.setConnectTimeout(150);//timeout para modo AP (150 segundos)
        wm.setConfigPortalTimeout(120);//timeout para modo AP (150 segundos)
        res = wm.autoConnect("MedTemp","12345678"); // password protected ap
        if(res){
          //savedSSID = wm.getWiFiSSID(true);
          //savedPASS = wm.getWiFiPass(true);
          ssid = wm.getWiFiSSID(true);
          password = wm.getWiFiPass(true);
          //Serial.println(savedSSID);
          //Serial.println(savedPASS);
          Serial.println(ssid);
          Serial.println(password);
          Serial.println("Se guardan en EEPROM las credenciales de conexión");
          //guarda config wifi en EEPROM
          
          EEPROM.writeString(EEPROM_ADDRESS_WIFI_SSID, ssid);
          EEPROM.commit();
          EEPROM.writeString(EEPROM_ADDRESS_WIFI_PASS, password);
          EEPROM.commit();
          
          //if you get here you have connected to the WiFi    
          Serial.println("Conectado a red WiFi!");
          flagModoAP = 0;
          //guarda en EEPROM el flag de modo AP
          //EEPROM.write(EEPROM_ADDRESS_FLAG_MODO_AP, flagModoAP);
          //EEPROM.commit();
        }else{
          Serial.println("No es posible conectar a WiFi");
          Serial.println("Se cambia a MODO LOCAL");
          flagModoAP = 0;
          //guarda en EEPROM el flag de modo AP
          //EEPROM.write(EEPROM_ADDRESS_FLAG_MODO_AP, flagModoAP);
          //EEPROM.commit();
        }
               
        apagarNeopixel();

        //Se detiene el wdt largo y se activa el normal
        esp_task_wdt_deinit();//deinit wdt largo
        esp_task_wdt_init(WDT_TIMEOUT, true); //init wdt normal

    }else{//si no está el flagModoAP = 1. Modo reconexión
      Serial.println("Modo reconexión");//si no viene de un reset

      int cuenta = 0;
      delay(10);
      // Nos conectamos a nuestra red Wifi
      Serial.println();
      Serial.print("Conectando a ssid: ");
      //Serial.println(savedSSID);
      Serial.println(ssid);

      //const char* ssidConverted = savedSSID.c_str();
      //const char* passwordConverted = savedPASS.c_str();
      const char* ssidConverted = ssid.c_str();
      const char* passwordConverted = password.c_str();
        
      WiFi.disconnect(1);
        
      WiFi.persistent(false);
      WiFi.begin(ssidConverted, passwordConverted);

      while ((WiFi.status() != WL_CONNECTED) && cuenta < 20) {
        delay(500);
        Serial.print(".");
        cuenta++;
      
      }
      if(WiFi.status() != WL_CONNECTED){//si no logró conectarse
      
        Serial.println("No es posible conectar a WiFi");
        Serial.println("Se cambia a MODO LOCAL");
        //flagModoAP = 1;//prepara para ponerse en modo Access Point
        //guarda en EEPROM el flag de modo AP
        //EEPROM.write(EEPROM_ADDRESS_FLAG_MODO_AP, flagModoAP);
        //EEPROM.commit();
        //flagConexionOK = 0;
        //flagCambioModoLocal = 1;
        //flagModoRed = 0;
        //fallaConexion();

      }else{//si logró conectarse

        //flagConexionOK = 1;
        //flagCambioModoLocal = 0;
        //flagModoRed = 1;
        Serial.println("");
        Serial.println("Conectado a red WiFi!");
        Serial.println("Dirección IP: ");
        Serial.println(WiFi.localIP());
        delay(5000);

      }








      //wm.setConnectTimeout(10);//timeout de reconexión (10 segundos)
      //wm.startConfigPortal("MedTemp", "12345678");
      //res = wm.autoConnect(savedSSID.c_str(),savedPASS.c_str()); // password protected ap
    }
    
        //modoAccessPoint();

    












}



//*****************************
//***    CONEXION MQTT      ***
//*****************************

void reconnect() {

  //alimentar watchDog
  //esp_task_wdt_reset();
  //flagCambioModoLocal = 0;
  int resultado = 0;

  while (!client1.connected() == 1 && flagConexionOK == 0) {
    Serial.println("Intentando conexión Mqtt1...");
    /*
    Serial.print("mqtt_server = ");
    Serial.println(mqtt_user);
    Serial.print("mqtt_pass = ");
    Serial.println(mqtt_pass);
    */
    // Creamos un cliente ID
    clientId = "IOTICOS_H_W_";
    clientId += String(random(0xffff), HEX);
    //String clientId = "UNDEFI_handtemp";
    // Intentamos conectar
    resultado = client1.connect(clientId.c_str(),mqtt_user,mqtt_pass);
    Serial.print("resultado: ");
    Serial.print(resultado);
    if(resultado == 1){
      Serial.println("Conectado!");
      flagConexionOK = 1;
    
    }else{
      
      cantFallos++;
      Serial.print(" cantFallos: ");
      Serial.print(cantFallos);
      Serial.print(" falló :( con error -> ");
      Serial.print(client1.state());
      Serial.println(" Intentamos de nuevo en 1 segundo");
      delay(1000);

      if(cantFallos == 2){
        cantFallos = 0;
        flagConexionOK = 0;
        break;
        
      }
    }
  }
}


//*****************************
//***       CALLBACK        ***
//*****************************

//puede cambiar parámetros a través del puerto serie o por bluetooth
//Se debe enviar un caracter de identificación del parámetro a cambiar y
//luego el valor.
//Por ejemplo: cambiar el tiempo entre lecturas de temperatura
//enviar T100  siendo T: tiempoEntreLecturas; 100: 100 ms
//los parámetros que se pueden modificar son:
//  distanciaConfigurada--> D;
//  distanciaTolerancia--> t;
//  tempFiebre--> F;
//  tempMin--> m;
//  tempMax--> M;
//  tempOffset--> O;
//  tiempoEntreLecturas--> T;
//  cantLecturas--> C;
//  emisividad--> E;
//  Wifi--> W;  [Ejemplo: Wmyssid mypassword](El espacio se usa como delimitador)
//  debug--> d  [1 para activarlo; 0 para desactivarlo]
//  cantSensoresIR-->S
//  consultarLecturas-->P
//  escannearDispositivosI2C-->s  [1 para activarlo; 0 para desactivarlo]
//  cambiarDireccionI2C-->A       [A90 91]
//  analizarLecturasCantidad-->U
//  intercambiarSensores-->I;
//  cambiarNumeroSensor-->Q;


void callback(char* topic, byte* payload, unsigned int length){
  
  String incoming = "";
  char charParamID = ' ';
  String valorParam = "";
  int inChar = 0;
  

  Serial.print("Mensaje recibido desde -> ");
  Serial.print(topic);
  Serial.println("");

  for (int i = 0; i < length; i++) {
    incoming += (char)payload[i];
  }

  incoming.trim();
  Serial.println("Mensaje -> " + incoming);

  //obtiene el identificador
  charParamID = incoming.charAt(0);
  
  Serial.println(charParamID);
  
  //obtiene el valor
  for(int i = 1; i < incoming.length(); i++){
    valorParam += incoming.charAt(i);
  }

  Serial.println(valorParam);

  //evalua el identificador y los parámetros enviados
  switchCaseParametros(charParamID, valorParam);

  //borra el contenido y lo prepara para recibir uno nuevo
  incoming = "";

}

//********** LECTURA DE SENSOR INFRARROJO ***************
//hace un promedio de 10 lecturas
double leerSensorIR(uint8_t numSensor){

    double lectura = 0;
    int i = 0;
    double lect = 0;
    float lectFloat = 0;
    float arrayLectSensor[cantLecturas] = {};
    int lecturaDescartada = 0;

    for(i = 0; i < cantLecturas; i++){

        if(numSensor == 1){
          lect = mlx1.readObjectTempC();
        }

        while(lect > tempValidaMax || lect < tempValidaMin){//si lee un valor erróneo, lo descarta
          lecturaDescartada++;//anota la cantidad de erróneos
          delay(tiempoEntreLecturas);
          Serial.println("Lectura errónea. Se vuelve a leer");
          //vuelve a leer
          if(numSensor == 1){
            lect = mlx1.readObjectTempC();
          }
          
          if(lecturaDescartada > 2){//si existen valores erróneos
            //Serial.println("Demasiados valores erróneos. Revise las conexiones");
            flagLecturasErroneas = 1;
            break;
          }
        }
        
        lectura += lect;
        
        lectFloat = (float)((lect * 100)/100);
        arrayLectSensor[i] = lectFloat;

        Serial.print(lectFloat);
        Serial.print(",");
        delay(tiempoEntreLecturas);
    }

    if(flagLecturasErroneas == 1){
      flagLecturasErroneas = 0;
      Serial.println("Demasiados valores erróneos. Revise las conexiones");
    }
    Serial.println();
    lectura = lectura/cantLecturas;
    
    return lectura;
}


// PLAY TONE  ==============================================
// Genera una señal PWM por una salida digital de determinada frecuencia
void playTone(int melody[]) {

  for (int i=0; i<MAX_COUNT; i++) {
    tone_ = melody[i];
    long elapsed_time = 0;
    if (tone_ > 0) { // hace sonar al tono hasta hasta que el tiempo llegue a la duracion
      while (elapsed_time < duration) {
        digitalWrite(buzzPin, HIGH);
        delayMicroseconds(tone_ / 2);
        digitalWrite(buzzPin, LOW);
        delayMicroseconds(tone_ / 2);
        // tiempo transcurrido desde que se reprodujo
        elapsed_time += (tone_);
      }
    }
    else { // loopeamos durante lo que dura el silencio
      for (int j = 0; j < rest_count; j++) {
        delayMicroseconds(duration); 
      } 
    }                              
  }  
}

// PLAY TONE  ==============================================
// Genera una señal PWM por una salida digital de determinada frecuencia
void playTone2(int tone) {
  
    long elapsed_time = 0;
    if (tone > 0) { // hace sonar al tono hasta hasta que el tiempo llegue a la duracion
      while (elapsed_time < 100000) {
        digitalWrite(buzzPin, HIGH);
        delayMicroseconds(tone / 2);
        digitalWrite(buzzPin, LOW);
        delayMicroseconds(tone / 2);
        // tiempo transcurrido desde que se reprodujo
        elapsed_time += tone;
      }
    }                            
}

// funciones del oled

void handTempText(void) {

 
  int timeDisplayBienvenida = 2000;
  
  display.clearDisplay();
  Serial.println("MedTemp");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(6,30);
  display.clearDisplay();
  display.println("MedTemp");
  display.drawFastHLine(8,10,112,WHITE);
  display.drawFastHLine(8,40,112,WHITE);
  display.setCursor(6,40 + LINEHEIGHT);
  display.setFont(mainFont);
  Serial.println(version);
  display.println(version); 
  display.display();
  delay(timeDisplayBienvenida);
  // limpio el bufer para la proxima pantalla
  display.clearDisplay();
  /*
  if(flagConexionOK == 1){//si está en modo red
    Serial.println("MODO RED");
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(6,30);
    display.setFont(font1);
    display.println("MODO RED");
    display.display();
    delay(2000);
    display.clearDisplay();
    display.setCursor(6,30);
    display.println(macAdd);
    display.display();
    delay(5000);
  }else{
    Serial.println("MODO LOCAL");
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(6,30);
    display.setFont(font1);
    display.println("MODO LOCAL");
    display.display();
  }
  
  delay(1000);
  display.setFont(mainFont);
  display.clearDisplay();
*/
}

void imprimirModo(void){

    if(flagConexionOK == 1){//si está en modo red
        Serial.println("MODO RED");
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(6,30);
        display.setFont(font1);
        display.println("MODO RED");
        display.display();
        delay(2000);
        display.clearDisplay();
        display.setCursor(6,30);
        display.println(macAdd);
        display.display();
        delay(5000);
    }else{
        Serial.println("MODO LOCAL");
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(6,30);
        display.setFont(font1);
        display.println("MODO LOCAL");
        display.display();
    }
    
    delay(1000);
    display.setFont(mainFont);
    display.clearDisplay();
}

void fallaConexion(void) {

  int timeDisplayBienvenida = 2000;
  
  display.clearDisplay();
  Serial.println("HandTemp");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(6,30);
  display.clearDisplay();
  Serial.println("Falla conexión");
  display.println("Falla conexión");
  display.display();
  delay(timeDisplayBienvenida);
  // limpio el bufer para la proxima pantalla
  display.clearDisplay();
    
  display.setFont(mainFont);
  
}


void modoAccessPoint(void) {

  int timeDisplayBienvenida = 2000;
  
  display.clearDisplay();
  Serial.println("MedTemp - Modo Access Point");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(font1);
  display.setCursor(6,30);
  display.clearDisplay();
  Serial.println("SSID:Medtemp");
  Serial.println("PASS:12345678");
  // limpio el bufer para la proxima pantalla
  display.clearDisplay();

  display.println("MODO AP:");//Imprime Modo AP
  display.display();
  
  delay(timeDisplayBienvenida);
  // limpio el bufer para la proxima pantalla
  display.clearDisplay();
  
  display.setCursor(6,30);
  display.println("SSID:");//Imprime SSID
  display.println("Medtemp");
  display.display();
  
  delay(timeDisplayBienvenida);
  // limpio el bufer para la proxima pantalla
  display.clearDisplay();

  display.setCursor(6,30);
  display.println("PASS:");//Imprime PASS
  display.println("12345678");
  display.display();
  
  delay(timeDisplayBienvenida);
  // limpio el bufer para la proxima pantalla
  display.clearDisplay();    
  display.setFont(mainFont);
  display.setCursor(6,30);
  
}
void waitingForHand(){

    display.clearDisplay();
    display.setCursor(10,26);
    display.println("Acerque");
    display.setCursor(10,26 + LINEHEIGHT);
    display.println("su mano");
    display.display();
    delay(200);
    display.clearDisplay();
}

void showTemperature(double temperature, String msg){

  int timeDisplay = 2000;

  display.clearDisplay();
  String tempText = static_cast<String>(round(temperature*100) / 100);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //display.setCursor(10,20);
  display.setCursor(3,20);
  //display.setFont(secondFont);
  display.setFont(font1);  // default font
  //display.setFont(secondFont);  // default font
  //display.println("Su temperatura es");
  display.println("Su temperatura");
  display.setFont(font3);
  //display.setTextSize(1);
  display.setCursor(26,52);
  display.println(tempText);
  display.display();
  delay(timeDisplay);
  display.clearDisplay();
  
  display.setFont(mainFont);
  display.setCursor(10,26);
  display.println("Acceso");
  display.setCursor(10,26 + LINEHEIGHT);
  display.println(msg);
  display.display();
  delay(timeDisplay);
  display.clearDisplay();
  
}

void showTemperatureOutOfRange(double temperature){

  int timeDisplay = 2000;

  
  display.clearDisplay();
  String tempText = static_cast<String>(round(temperature*100) / 100);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //display.setCursor(10,20);
  display.setCursor(3,15);
  //display.setFont(secondFont);
  display.setFont(font1);  // default font
  //display.setFont(secondFont);  // default font
  //display.println("Su temperatura es");
  display.println("Temperatura");
  display.println("fuera de rango");

  display.setFont(font1);
  //display.setTextSize(1);
  display.setCursor(35,57);
  display.println(tempText);
  display.display();
  delay(timeDisplay);
  display.setFont(mainFont);
  display.clearDisplay();
  
}

void showAllDataDebugMode(double tempC, double tempAmbC, float dist){

  int timeDisplay = 6000;

  display.clearDisplay();
  String tempCText = static_cast<String>(round(tempC*100) / 100);
  String tempAmbCText = static_cast<String>(round(tempAmbC*100) / 100);
  String distText = static_cast<String>(round(dist*100) / 100);
  String tempOffsetText = static_cast<String>(round(tempOffset*100) / 100);
  //String tempAmbOffsetText = static_cast<String>(round(tempAmbOffset*100) / 100);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //display.setCursor(10,20);
  display.setCursor(3,15);
  //display.setFont(secondFont);
  display.setFont(font1);  // default font
  //display.setFont(secondFont);  // default font
  //display.println("Su temperatura es");
  display.print("Tobj: ");
  display.println(tempCText);
  display.print("Tamb: ");
  display.println(tempAmbCText);
  display.print("Distancia: ");
  display.println(distText);

  //display.setTextSize(1);
  //display.setCursor(26,52);
  //display.println(tempText);
  display.display();
  delay(timeDisplay);
  display.clearDisplay();

  display.setCursor(3,15);

  display.print("tOffs: ");
  display.println(tempOffsetText);
  //display.print("tAmbOffs: ");
  //display.println(tempAmbOffsetText);

  display.display();
  delay(timeDisplay);
  
  display.setFont(mainFont);  // default font
  display.clearDisplay();
  
}

//puede cambiar parámetros a través del puerto serie o por bluetooth
//Se debe enviar un caracter de identificación del parámetro a cambiar y
//luego el valor.
//Por ejemplo: cambiar el tiempo entre lecturas de temperatura
//enviar T100  siendo T: tiempoEntreLecturas; 100: 100 ms
//los parámetros que se pueden modificar son:
//  distanciaConfigurada--> D;
//  distanciaTolerancia--> t;
//  tempFiebre--> F;
//  tempMin--> m;
//  tempMax--> M;
//  tempOffset--> O;
//  tiempoEntreLecturas--> T;
//  cantLecturas--> C;
//  emisividad--> E;
//  Wifi--> W;  [Ejemplo: Wmyssid mypassword](El espacio se usa como delimitador)
//  debug--> d  [1 para activarlo; 0 para desactivarlo]
//  cantSensoresIR-->S
//  consultarLecturas-->P
//  escannearDispositivosI2C-->s  [1 para activarlo; 0 para desactivarlo]
//  cambiarDireccionI2C-->A       [A90 91]
//  analizarLecturasCantidad-->U
//  intercambiarSensores-->I;
//  consultarContadorReconexion--X; 
void cambioDeParametros(void){

  char charParamID = ' ';
  String valorParam = "";
  int inChar = 0;
  String inString = "";
    
  
  //**** Chequeo por Serie o Bluetooth ***************
  while (Serial.available() > 0 || SerialBT.available() > 0) {

    if(Serial.available() > 0){
      inChar = Serial.read();
    }else if(SerialBT.available() > 0){
      inChar = SerialBT.read();
    }
    

    if(inChar != '\n'){
      Serial.print((char)inChar);

      inString += (char)inChar;//encola los caracteres recibidos

    }else{//si llegó el caracter de terminación
      
      Serial.print("Input string: ");
      Serial.println(inString);
      Serial.print("string length: ");
      Serial.println(inString.length());


      //obtiene el identificador
      charParamID = inString.charAt(0);
      
      Serial.println(charParamID);
      
      //obtiene el valor
      for(int i = 1; i < inString.length(); i++){
        valorParam += inString.charAt(i);
      }

      Serial.println(valorParam);

      //evalua el identificador y los parámetros enviados
      switchCaseParametros(charParamID, valorParam);
      
      //borra el contenido y lo prepara para recibir uno nuevo
      inString = "";
    
    }
  }

}

//realiza la lectura del timestamp desde el server ntp
void obtenerFechaHora(void){

  if(!getLocalTime(&timeStamp)){
      Serial.println("Failed to obtain time");
  }else{

      //guarda cada componente del timeStamp en variables
      strftime(timeYear,10, "%Y", &timeStamp);
      strftime(timeMonth,10, "%B", &timeStamp);
      strftime(timeDayName,10, "%A", &timeStamp);
      strftime(timeDayNumber,10, "%d", &timeStamp);
      strftime(timeHour24,10, "%H", &timeStamp);   
      strftime(timeHour12,10, "%I", &timeStamp);
      strftime(timeMinute,10, "%M", &timeStamp);
      strftime(timeSecond,10, "%S", &timeStamp);
      
      strcpy(fechaHora, "");//borra el contenido del string

      //arma un string con los componentes que interesan
      strcat(fechaHora, timeYear);
      strcat(fechaHora, " ");
      strcat(fechaHora, timeMonth);
      strcat(fechaHora, " ");
      strcat(fechaHora, timeDayNumber);
      strcat(fechaHora, " ");
      strcat(fechaHora, timeHour24);
      strcat(fechaHora, ":");
      strcat(fechaHora, timeMinute);
      strcat(fechaHora, ":");
      strcat(fechaHora, timeSecond);

  }
}



void publicarDataObjeto(double tempC, double tempF, double emisividadL, char fechaHora[]){
        
  //prepara el objeto JSON para publicar por MQTT
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
 
  JSONencoder["temp"] = round(tempC * 100) / 100;
  
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Enviando data de Temperatura del objeto por MQTT1...");
  Serial.println(JSONmessageBuffer);

  client1.publish(tempObjeto_topic_publish, JSONmessageBuffer);

}

void publicarDataAmbiente(double tempAmbienteC, char fechaHora[]){

        
  //prepara el objeto JSON para publicar por MQTT
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["device"] = "ESP32";
  JSONencoder["Temp Ambiente *C"] = round(tempAmbienteC * 100) / 100 ;
  JSONencoder["timeStamp"] = fechaHora;
  
  char JSONmessageBuffer[300];
  JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Serial.println("Enviando data de Temperatura del ambiente por MQTT...");
  Serial.println(JSONmessageBuffer);
  
  client1.publish(tempAmbiente_topic_publish, JSONmessageBuffer);
}

/*
    ESP_RST_UNKNOWN,    //!< Reset reason can not be determined
    ESP_RST_POWERON,    //!< Reset due to power-on event
    ESP_RST_EXT,        //!< Reset by external pin (not applicable for ESP32)
    ESP_RST_SW,         //!< Software reset via esp_restart
    ESP_RST_PANIC,      //!< Software reset due to exception/panic
    ESP_RST_INT_WDT,    //!< Reset (software or hardware) due to interrupt watchdog
    ESP_RST_TASK_WDT,   //!< Reset due to task watchdog
    ESP_RST_WDT,        //!< Reset due to other watchdogs
    ESP_RST_DEEPSLEEP,  //!< Reset after exiting deep sleep mode
    ESP_RST_BROWNOUT,   //!< Brownout reset (software or hardware)
    ESP_RST_SDIO,       /
*/
//chequea la causa del último reset e imprime por pantalla el motivo
int chequearCausaDeReset(void){

  int causaReset = 0;

  causaReset = esp_reset_reason();

  switch(causaReset){
    case 0:
      Serial.println("ESP_RST_UNKNOWN");
    break;
    case 1:
      Serial.println("ESP_RST_POWERON");
    break;
    case 2:
      Serial.println("ESP_RST_EXT");
    break;
    case 3:
      Serial.println("ESP_RST_SW");
    break;
    case 4:
      Serial.println("EESP_RST_PANIC");
    break;
    case 5:
      Serial.println("ESP_RST_INT_WDT");
    break;
    case 6:
      Serial.println("ESP_RST_TASK_WDT");     
    break;
    case 7:
      Serial.println("ESP_RST_WDT");
    break;
    case 8:
      Serial.println("ESP_RST_DEEPSLEEP");
    break;
    case 9:
      Serial.println("ESP_RST_BROWNOUT");
    break;
    case 10:
      Serial.println("ESP_RST_SDIO");
    break;
    default:
      Serial.println("Problema en obtener la causa del reset");
    break;

  }

  return causaReset;

}

void imprimirUltimasLecturas(void){

    Serial.print("últimas lecturas: ");

    //imprime por bluetooth y por serie
    int idxFinal = 0;

    //for(int j = 0; j < sizeof(arrayUltimasLecturas); j++){//recorre el array de lecturas
    for(int j = 0; j < 100; j++){//recorre el array de lecturas
      valor = arrayUltimasLecturas[j];//obtiene cada flotante
      dtostrf(valor,5, 2, outstr);// convierte de flotante a string

      Serial.print(outstr);

      for(int p = 0; p < strlen(outstr); p++){
        SerialBT.write(outstr[p]);
      }

      Serial.print(',');
      SerialBT.write(',');

      if(j == 99){
        Serial.println();
        SerialBT.println();
      }
    }
}

double realizarCorreccion(double tempC, double tempAmbC, double tempOffset){

 double tempCorregida = 0.0;


  double m = -0.852;//-0.1049121267;//pendiente
  double b = 32.231;//3.2272434367;//ordenada al origen

  double y = 0;//offset

  double x = tempAmbC;//la variable independiente es la temperatura ambiente

  y = m * x + b;


  tempCorregida = tempC + y + tempOffset;
  //tempCorregida = tempC + tempOffset;

  Serial.print("TempC: ");
  Serial.print(tempC);
  Serial.print("---> TempCorregida: ");
  Serial.print(tempCorregida);
  Serial.print("---> TempOffset: ");
  Serial.println(tempOffset);

  return tempCorregida;


}

void scannearDispositivosI2C(void){

  byte error, address;
  int nDevices;
 
  Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 0; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(5000);// wait 5 seconds for next scan


}

void cambiarDireccionI2CNuevaVersion(byte oldAddress, byte newAddress){

  IRTherm therm; // Create an IRTherm object to interact with throughout

  //Serial.println("Press a key to begin");
  //while (!Serial.available()) ;
  
  therm.begin(oldAddress); // Try using the old address first
  
  byte address;
  if (!therm.readID()) // Try reading the ID registers
  {
    // If the read fails, try re-initializing with the
    // new address. Maybe we've already set it.
    therm.begin(newAddress);
    
    if (therm.readID()) // Read from the ID registers
    { // If the read succeeded, print the ID:
      Serial.println("Communicated with new address.");
      Serial.println("ID: 0x" + 
                     String(therm.getIDH(), HEX) +
                     String(therm.getIDL(), HEX));
    }
    else
    {
      Serial.println("Failed to communicate with either address.");      
    }
  }
  else
  {
    // If the read suceeds, change the address to something
    // new.
    if (!therm.setAddress(newAddress))
    {
      Serial.println("Failed to set new address.");
    }
    else
    {
      Serial.println("Set the address to 0x" + String(newAddress, HEX));
      Serial.println("Cycle power to try it out.");

      Serial.println("Reconecte el sensor");
      Serial.println("10...");
      delay(1000);
      Serial.println("9...");
      delay(1000);
      Serial.println("8...");
      delay(1000);
      Serial.println("7...");
      delay(1000);
      Serial.println("6...");
      delay(1000);
      Serial.println("5...");
      delay(1000);
      Serial.println("4...");
      delay(1000);
      Serial.println("3...");
      delay(1000);
      Serial.println("2...");
      delay(1000);
      Serial.println("1...");
      delay(1000);
                
      therm.begin(newAddress);
      delay(500);
      Serial.println(therm.readAddress());
      delay(1000);

    }
  }
}

void waitingForHandCustom1(void){

    display.clearDisplay();
    display.setCursor(10,26);
    display.println("Demasiado");
    display.setCursor(10,26 + LINEHEIGHT);
    display.println("lejos");
    display.display();

}

void waitingForHandCustom2(void){

    display.clearDisplay();
    display.setCursor(10,26);
    display.println("Demasiado");
    display.setCursor(10,26 + LINEHEIGHT);
    display.println("cerca");
    display.display();

}

void waitingForHandCustom3(void){

    display.clearDisplay();
    display.setCursor(10,35);
    display.println("Midiendo");
    display.display();

}

void evaluarDistancia(float distancia){

  int timeDelayTone = 100;
  int tone = C2;

  if(distancia == 0.00){
      distancia = 20;
  }

  if(distancia < 20){//a partir de acá empieza a informar por display y por buzzer

    if(distancia >= distanciaConfigurada + distanciaTolerancia){
      //demasiado lejos
      waitingForHandCustom1();
    
    }else if(distancia <= distanciaConfigurada - distanciaTolerancia){
      //demasiado cerca
      waitingForHandCustom2();

    }else if(distancia >= distanciaConfigurada - distanciaTolerancia && distancia <= distanciaConfigurada + distanciaTolerancia){
      //dentro de la distancia correcta
      waitingForHandCustom3();

    }
    if(distancia >= distanciaConfigurada + 7){//3+7 = 10
           
      Serial.println(distancia);
      playTone2(tone);
      delay(timeDelayTone*1);
      playTone2(tone);
      
    }else if(distancia >= distanciaConfigurada + 6 && distancia <= distanciaConfigurada + 7){//entre 6 y 7
    
      Serial.println(distancia);
      playTone2(tone);
      delay(timeDelayTone/2);
      playTone2(tone);
      
    }else if(distancia >= distanciaConfigurada + 5 && distancia <= distanciaConfigurada + 6){//entre 5 y 6
    
      Serial.println(distancia);

      playTone2(tone);
      delay(timeDelayTone/3);
      playTone2(tone);

    }else if(distancia >= distanciaConfigurada + 4 && distancia <= distanciaConfigurada + 5){//entre 4 y 5
    
      Serial.println(distancia);

      playTone2(tone);
      delay(timeDelayTone/4);
      playTone2(tone);
     

    }else if(distancia >= distanciaConfigurada + 3 && distancia <= distanciaConfigurada + 4){//entre 3 y 4
    
      Serial.println(distancia);

      playTone2(tone);
      delay(timeDelayTone/5);
      playTone2(tone);
     

    }else if(distancia >= distanciaConfigurada + 2 && distancia <= distanciaConfigurada + 3){//entre 2 y 3
    
      Serial.println(distancia);

      playTone2(tone);
      delay(timeDelayTone/10);
      playTone2(tone);       

    }
    else if(distancia >= distanciaConfigurada + 1 && distancia <= distanciaConfigurada + 2){//entre 1 y 2
    
      Serial.println(distancia);

      playTone2(tone);
      delay(timeDelayTone/20);
      playTone2(tone);
      
    }else if(distancia >= distanciaConfigurada - distanciaTolerancia && distancia <= distanciaConfigurada + distanciaTolerancia){//dentro de la distancia correcta
    //[DISTANCIA CORRECTA]

      Serial.println(distancia);

    }else if(distancia <= distanciaConfigurada - distanciaTolerancia){//menor a la distancia correcta
    
      Serial.println(distancia);
      playTone2(tone/2);
      delay(timeDelayTone);
      playTone2(tone/2);
      
    }    

  }

}


void encenderNeopixel(char color){

    int i = 0;

    pixels.setPin(33);
    //pixels.setBrightness(200);
    //pixels.show();

    switch(color){
        case 'R'://RED
            //todos los LEDs en rojo
            for(i = 0; i < CANTLEDS; i++){
                pixels.setPixelColor(i, pixels.Color(255, 0, 0));
            }
            //pixels.show();
        break;
        case 'G'://GREEN
            //todos los LEDs en verde
            for(i = 0; i < CANTLEDS; i++){
                pixels.setPixelColor(i, pixels.Color(0, 255, 0));
            }
            //pixels.show();
        break;
        case 'Y'://YELLOW
            //todos los LEDs en amarillo
            for(i = 0; i < CANTLEDS; i++){
                pixels.setPixelColor(i, pixels.Color(255, 255, 0));
            }

           // pixels.show();
        break;
        case 'B'://YELLOW
            //todos los LEDs en amarillo
            for(i = 0; i < CANTLEDS; i++){
                pixels.setPixelColor(i, pixels.Color(255, 0, 255));
            }

           // pixels.show();
        break;
        default:
            Serial.println("Color incorrecto");
        break;
    }

    pixels.show();
    pixels.setPin(18);// cambia el pin del neopixel para canalizar los ruidos a otro pin
    //delay(1000);
    neopixelFueApagado = 0;//cambia el estado
    
}


void apagarNeopixel(void){

    int i = 0;

    //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    for(i = 0; i < CANTLEDS; i++){
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    pixels.show();
    pixels.setPin(33);

    for(i = 0; i < CANTLEDS; i++){
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    
    pixels.show();
    pixels.setPin(18);// cambia el pin del neopixel para canalizar los ruidos a otro pin
    //pixels.show();
    neopixelFueApagado = 1;//cambia el estado
     
}

void analizarLecturas(int cantidadDeLecturas){

  int idxInicio = 0;
  int idxFin = 0;
  int j = 0;
  float suma = 0;
  float promedio = 0;
  float maximo = 0;
  float minimo = 0;
  float valor = 0;

  idxFin = idx2;
  idxInicio = idx2 - cantidadDeLecturas;
  
  maximo = arrayUltimasLecturas[idxInicio];
  minimo = arrayUltimasLecturas[idxInicio];


  Serial.println("Análisis de últimas lecturas: ");
  Serial.print("idxInicio: ");
  Serial.print(idxInicio);
  Serial.print(" idxFin: ");
  Serial.println(idxFin);

  for(j = idxInicio; j < idxFin; j++){
    Serial.print(arrayUltimasLecturas[j]);
    valor = arrayUltimasLecturas[j];
    
    dtostrf(valor,5, 2, outstr);// convierte de flotante a string

    for(int p = 0; p < strlen(outstr); p++){
      SerialBT.write(outstr[p]);
    }

    //SerialBT.write(arrayUltimasLecturas[j]);
    Serial.print(",");
    SerialBT.write(',');
    suma += arrayUltimasLecturas[j];

    if(arrayUltimasLecturas[j] > maximo){
      maximo = arrayUltimasLecturas[j];//actualiza el máximo
    }
    if(arrayUltimasLecturas[j] < minimo){
      minimo = arrayUltimasLecturas[j];//actualiza el mínimo
    }

  }
  Serial.println();
  SerialBT.write('/n');


  promedio = suma/cantidadDeLecturas;

  Serial.print("Promedio: ");
  Serial.println(promedio);
  Serial.print("Mínimo: ");
  Serial.println(minimo);
  Serial.print("Máximo: ");
  Serial.println(maximo);

  SerialBT.write(' ');
  SerialBT.write('P');
  SerialBT.write(':');
  SerialBT.write(' ');

  valor = promedio;
  
  dtostrf(valor,5, 2, outstr);// convierte de flotante a string
    
  for(int p = 0; p < strlen(outstr); p++){
    SerialBT.write(outstr[p]);
  }

  //SerialBT.write(promedio);

  SerialBT.write(' ');
  SerialBT.write('m');
  SerialBT.write(':');
  SerialBT.write(' ');

  valor = minimo;

  dtostrf(valor,5, 2, outstr);// convierte de flotante a string
    
  for(int p = 0; p < strlen(outstr); p++){
    SerialBT.write(outstr[p]);
  }  
  
  //SerialBT.write(minimo);

  SerialBT.write(' ');
  SerialBT.write('M');
  SerialBT.write(':');
  SerialBT.write(' ');

  valor = maximo;

  dtostrf(valor,5, 2, outstr);// convierte de flotante a string
    
  for(int p = 0; p < strlen(outstr); p++){
    SerialBT.write(outstr[p]);
  }  
  
  //SerialBT.write(maximo);

}

/*
//void intercambiarSensores(int sensorA, int sensorB){
void intercambiarSensores(void){

  mlx1 = Adafruit_MLX90614(91);//escribe la dirección del sensor2
  mlx2 = Adafruit_MLX90614(90);//escribe la dirección del sensor1

  mlx1.begin();
  mlx2.begin();

}
*/
void publicarKeepAlive(void){
  
  strcpy(msgKeepAlive, "");//borra el contenido del string
  strcat(msgKeepAlive, "...Estoy vivo. ");
  strcat(msgKeepAlive, handtempKeepAlive_topic_publish);
  strcat(msgKeepAlive, " ");
  strcat(msgKeepAlive, tiempoSinReset);

  client1.publish(handtempKeepAlive_topic_publish, msgKeepAlive);
  Serial.println(handtempKeepAlive_topic_publish);
  Serial.println(msgKeepAlive);

}

void contabilizarTiempoSinReset(void){

  char str[10] = {};

  tiempoMinutoSinReset++;

  if(tiempoMinutoSinReset == 60){//cuenta las horas sin reset
    tiempoMinutoSinReset = 0;
    tiempoHoraSinReset++;

    if(tiempoHoraSinReset == 24){//cuenta los dias sin reset
      tiempoHoraSinReset = 0;
      tiempoDiaSinReset++;

      if(tiempoDiaSinReset == 65535){
        tiempoDiaSinReset = 0;
      }
    }
  }

  strcpy(tiempoSinReset, "");//borra el contenido del string

  //arma un string con los componentes que interesan
  sprintf(str, "%d", tiempoDiaSinReset);
  strcat(tiempoSinReset, str);
  strcat(tiempoSinReset, " Días ");
  sprintf(str, "%d", tiempoHoraSinReset);
  strcat(tiempoSinReset, str);
  strcat(tiempoSinReset, " Horas ");
  sprintf(str, "%d", tiempoMinutoSinReset);
  strcat(tiempoSinReset, str);
  strcat(tiempoSinReset, " Minutos ");

}

void cambiarConfigMQTT(uint8_t numSensor){
 
  char strNombreSensor[80] = "";
  char numMacAdd[80] = {}; 
  char strTopico[80] = "medtemp/";
  char str[10] = {};
  char str2[80] = {};
  char strBroker[80] = {};


  strcat(numMacAdd, macAdd.c_str());
  //sprintf(str, "%d", numSensor);
  strcat(strNombreSensor, numMacAdd);
  //strcat(strNombreSensor, str);
  Serial.print("strNombreSensor: ");
  Serial.println(strNombreSensor);

  strcat(strTopico, strNombreSensor);
  Serial.print("strTopico: ");
  Serial.println(strTopico);

  
  strcpy(root_topic_subscribe, strTopico);
  strcpy(root_topic_publish, strTopico);
  strcpy(tempAmbiente_topic_publish, strTopico);
  strcpy(tempObjeto_topic_publish, strTopico);
  strcpy(handtempKeepAlive_topic_publish, strTopico);
  strcat(strBroker, broker.c_str());
  strcpy(mqtt_server, strBroker);

}

//si perdió la conexión, intenta recuperarla
void comprobarConexion(void){

  setupModoRed();//configura MQTT, revisa conectividad
  /*
  if(!client1.connected()){
    reconnect();
  }
  */
  /*
  if(!client2.connected()){
    reconnect();
  }
  */

}


void switchCaseParametros(char charParamID, String valorParam){

  int inChar = 0;
  int index = 0;
  int valorParamLength = 0;
  int endIndex = 0;
  int modoDebug = 0;
  int consultarLecturas = 0;
  int correccionActivada = 0;
  uint8_t numSensor = 0;
  uint16_t direccion = 0;
  int scanActivado = 0;
  byte oldAddress = 0;
  byte newAddress = 0;
  int analizarLecturasCantidad = 0;
  int intercambioSensores = 0;
  int color = 0;
  String nombreSensor = "";
  
  //valorParam = 
  valorParam.replace(0x0A,'\0');//Se filtra el caracter LF
  valorParam.replace(0x0D,'\0');//Se filtra el caracter CR

  switch(charParamID){
    case 'D':
      distanciaConfigurada = valorParam.toFloat();
      Serial.print("DistanciaConfigurada: ");
      Serial.println(distanciaConfigurada);
    break;
    case 't':
      distanciaTolerancia = valorParam.toFloat();
      Serial.print("DistanciaTolerancia: ");
      Serial.println(distanciaTolerancia);
    break;
    case 'O':
      tempOffset = valorParam.toDouble();
      Serial.print("TempOffset: ");
      Serial.println(tempOffset);

      //guarda en EEPROM el OFFSET de temperatura
      //tempOffsetEEPROM = (uint8_t)(tempOffset * 10);
      tempOffsetEEPROM = tempOffset;
      EEPROM.writeDouble(0, tempOffsetEEPROM);
      EEPROM.commit();

    break;
    case 'T':
      tiempoEntreLecturas = valorParam.toInt();
      Serial.print("TiempoEntreLecturas: ");
      Serial.println(tiempoEntreLecturas);
    break;
    case 'm':
      tempMin = valorParam.toDouble();
      Serial.print("tempMin: ");
      Serial.println(tempMin);
    break;
    case 'M':
      tempMax = valorParam.toDouble();
      Serial.print("tempMax: ");
      Serial.println(tempMax);
    break;
    case 'F':
      tempFiebre = valorParam.toDouble();
      Serial.print("tempFiebre: ");
      Serial.println(tempFiebre);
    break;
    case 'E':
      emisividadPorSerie = valorParam.toFloat();
      emisividad = (uint16_t)(emisividadPorSerie * 65535);

      Serial.println(emisividadPorSerie);
      Serial.println(emisividad);

      mlx1.writeEmissivityReg(emisividad);//escribe la emisividad al sensor
      mlx1.begin();

      delay(500);
      
      //Verificación de emisividad
      Serial.println("Reconecte el sensor");
      Serial.println("10...");
      delay(1000);
      Serial.println("9...");
      delay(1000);
      Serial.println("8...");
      delay(1000);
      Serial.println("7...");
      delay(1000);
      Serial.println("6...");
      delay(1000);
      Serial.println("5...");
      delay(1000);
      Serial.println("4...");
      delay(1000);
      Serial.println("3...");
      delay(1000);
      Serial.println("2...");
      delay(1000);
      Serial.println("1...");
      delay(1000);
      Serial.println("Se leerá la emisividad");
      delay(3000);

      Serial.print("Emisividad sensor 1: ");
      Serial.println(mlx1.readEmissivity());

    break;
    case 'W':

      Serial.println("Wifi: ");
      valorParamLength = strlen(valorParam.c_str());
      endIndex = valorParamLength;

      index = valorParam.indexOf(' ');

      ssid = valorParam.substring(0, index);
      Serial.println(ssid);
      //password = valorParam.substring(index + 1, endIndex - 1);
      password = valorParam.substring(index + 1, endIndex);
      Serial.println(password);

      //guarda config wifi en EEPROM
      EEPROM.writeString(EEPROM_ADDRESS_WIFI_SSID, ssid);
      EEPROM.commit();
      EEPROM.writeString(EEPROM_ADDRESS_WIFI_PASS, password);
      EEPROM.commit();

      setup_wifi();

    break;
    case 'Q':

      numSensor = valorParam.toInt();
      Serial.println("MQTT (numSensor): ");
      Serial.println(numSensor);

      if(numSensor >= 0 && numSensor <= 255){

        cambiarConfigMQTT(numSensor);

        //guarda en EEPROM el número de sensor
        EEPROM.write(0 + sizeof(double), numSensor);
        EEPROM.commit();

      }else{

        Serial.println("Número incorrecto. Se acepta entre 0 y 255");

      }

    break;
    case 'C':
      cantLecturas = valorParam.toInt();
      Serial.print("cantLecturas: ");
      Serial.println(cantLecturas);
    break;
    case 'd':
      modoDebug = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
      Serial.print("modoDebug: ");
      Serial.println(modoDebug);
      if(modoDebug){
        flagModoDebug = 1;
      }else{
        flagModoDebug = 0;
      }
      
    break;
    case 'S':
      cantSensoresIR = valorParam.toInt();

      if(cantSensoresIR < 1 && cantSensoresIR > 3){//valida la cantidad declarada
        Serial.println("cantidad de sensoresIR incorrecta. Se configura a 1");
        cantSensoresIR = 1;
      }

      Serial.print("cantSensoresIR: ");
      Serial.println(cantSensoresIR);
      flagLecturaEmisividad = 1;//habilita para volver a leer la emisividad

    break;
    case 'P':
      //imprimir por pantalla todos los valores adquiridos
      consultarLecturas = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
      Serial.print("consultarLecturas: ");
      Serial.println(consultarLecturas);
      Serial.print("Índice: ");
      Serial.println(idx2);
      if(consultarLecturas == 1){
        flagUltimasLecturas = 1;
      }else{
        flagUltimasLecturas = 0;
      }

    break;
    case 'c':
      correccionActivada = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
      Serial.print("correccionActivada: ");
      Serial.println(correccionActivada);
      if(correccionActivada){
        flagCorreccion = 1;
      }else{
        flagCorreccion = 0;
      }

    break;
    case 'A':
      
      valorParamLength = strlen(valorParam.c_str());
      endIndex = valorParamLength;

      index = valorParam.indexOf(' ');

      oldAddress = (byte)valorParam.substring(0, index).toInt();
                
      Serial.println("OldAddress: ");
      Serial.println(oldAddress, HEX);
      newAddress = (byte)valorParam.substring(index + 1, endIndex - 1).toInt();
      Serial.println("NewAddress: ");
      Serial.println(newAddress, HEX);

      cambiarDireccionI2CNuevaVersion(oldAddress, newAddress);

    break;
    case 's':
      scanActivado = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
      Serial.print("scanActivado: ");
      Serial.println(scanActivado);
      if(scanActivado){
        flagScan = 1;
      }else{
        flagScan = 0;
      }
    break;
    case 'U':
      analizarLecturasCantidad = valorParam.toInt();
      Serial.print("AnalizarLecturasCantidad: ");
      Serial.println(analizarLecturasCantidad);
      if(analizarLecturasCantidad > 0 && analizarLecturasCantidad <= 10){

          analizarLecturas(analizarLecturasCantidad);

      }else{
        Serial.println("Valor incorrecto");
      }
    break;
    case 'I':
      intercambioSensores = valorParam.toInt();
      Serial.print("intercambioSensores: ");
      Serial.println(intercambioSensores);

      //intercambiarSensores();
    break;
    case 'R':
      color = valorParam.toInt();
      Serial.print("red: ");
      Serial.println(color);
      Red = color;
    break;
    case 'G':
      color = valorParam.toInt();
      Serial.print("green: ");
      Serial.println(color);
      Green = color;
    break;
    case 'B':
      color = valorParam.toInt();
      Serial.print("blue: ");
      Serial.println(color);
      Blue = color;
    break;
    case 'K':

      //valorParamLength = strlen(valorParam.c_str());
      //endIndex = valorParamLength;

      //index = valorParam.indexOf(' ');

      broker = valorParam;
      Serial.println("MQTT (broker): ");
      Serial.println(broker);
      Serial.println(strlen(valorParam.c_str()));

      //Serial.println(ssid);
      //password = valorParam.substring(index + 1, endIndex - 1);
      //Serial.println(password);

      //guarda config mqtt_server en EEPROM
      EEPROM.writeString(EEPROM_ADDRESS_MQTT_SERVER, broker);
      EEPROM.commit();
      //EEPROM.writeString(EEPROM_ADDRESS_WIFI_PASS, password);
      //EEPROM.commit();
      cambiarConfigMQTT(1);
    break;
    case 'o':

      //valorParamLength = strlen(valorParam.c_str());
      //endIndex = valorParamLength;



      //index = valorParam.indexOf(' ');
      flagOTA = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
      Serial.print("flagOTA: ");
      Serial.println(flagOTA);
      
      if(flagOTA == 1){
        EEPROM.write(EEPROM_ADDRESS_FLAG_OTA, flagOTA);
        EEPROM.commit();
      }else if(flagOTA == 0){
        EEPROM.write(EEPROM_ADDRESS_FLAG_OTA, flagOTA);
        EEPROM.commit();
      }
      Serial.println("Se reiniciará el sistema");
      delay(5000);
      ESP.restart();

    break;
    case 'r':
      Serial.print("Reset...");
      delay(3000);
      ESP.restart();
      
    break;

    case 'X':
      
      Serial.print("contadorReconexionExitosa: ");
      Serial.println(contadorReconexionExitosa);

      SerialBT.print("contadorReconexionExitosa: ");
      SerialBT.println(contadorReconexionExitosa);
      
      Serial.print("contadorReconexionFallida: ");
      Serial.println(contadorReconexionFallida);

      SerialBT.print("contadorReconexionFallida: ");
      SerialBT.println(contadorReconexionFallida);
    break;
    case 'Y':
      
      Serial.print("wifiSSIDEEPROM: ");
      Serial.println(wifiSSIDEEPROM);

      SerialBT.print("wifiSSIDEEPROM: ");
      SerialBT.println(wifiSSIDEEPROM);
      
      Serial.print("wifiPASSEEPROM: ");
      Serial.println(wifiPASSEEPROM);

      SerialBT.print("wifiPASSEEPROM: ");
      SerialBT.println(wifiPASSEEPROM);

      Serial.print("brokerEEPROM: ");
      Serial.println(brokerEEPROM);

      SerialBT.print("brokerEEPROM: ");
      SerialBT.println(brokerEEPROM);
    break;
    default:
      Serial.println("Parámetro incorrecto");
    break;

  }  
}

void setupModoRed(void){

  setup_wifi();

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //mqtt_server = "broker.hivemq.com";
  client1.setServer(mqtt_server, mqtt_port);//inicializa server en broker local
  //client1.setServer("broker.hivemq.com", mqtt_port);//inicializa server en broker local
  client1.setCallback(callback);

  //intenta conectar con MQTT
  if(!client1.connected()){
    reconnect();//si no está conectado, lo reconecta
  }
  if(client1.connected()){//si logró reconectarse o ya estaba conectado
    Serial.println("Conexión OK: Wifi y MQTT conectados");
    Serial.println("MODO RED");
    //flagCambioModoLocal = 0;//no hace falta cambiar a modo local
    flagConexionOK = 1;
    
  }else{
    Serial.println("Conexión Errónea: Wifi y MQTT no conectados");
    Serial.println("MODO LOCAL...(temporal)");
    //flagCambioModoLocal = 1;
    //flagModoRed = 0;
    flagConexionOK = 0;
  }

}

      
void cargarDesdeEEPROM(void){

  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  
  tempOffsetEEPROM = EEPROM.readDouble(EEPROM_ADDRESS_OFFSET);//lee el OFFSET de temperatura de la eeprom
  Serial.print("tempOffset EEPROM: ");
  Serial.println(tempOffsetEEPROM);
  tempOffset = tempOffsetEEPROM;//carga el valor leído de la eeprom
  
  //recupera el número de sensor guardado en EEPROM
  numSensorEEPROM = EEPROM.read(EEPROM_ADDRESS_NUMSENSOR);//lee el número de sensor de la eeprom
  Serial.print("numSensor EEPROM: ");
  Serial.println(numSensorEEPROM);
  numeroSensor = numSensorEEPROM;

  brokerEEPROM = EEPROM.readString(EEPROM_ADDRESS_MQTT_SERVER);//lee el broker de la eeprom
  Serial.print("broker EEPROM: ");
  Serial.println(brokerEEPROM);
  broker = brokerEEPROM;

  //recupera el flag de tipo de conexión (modo Access Point)
  modoAccessPointEEPROM = EEPROM.read(EEPROM_ADDRESS_FLAG_MODO_AP);//lee el flag de la eeprom
  Serial.print("flag AP EEPROM: ");
  Serial.println(modoAccessPointEEPROM);
  flagModoAP = modoAccessPointEEPROM;

  wifiSSIDEEPROM = EEPROM.readString(EEPROM_ADDRESS_WIFI_SSID);//lee el SSID de la eeprom
  Serial.print("SSID EEPROM: ");
  Serial.println(wifiSSIDEEPROM);
  ssid = wifiSSIDEEPROM;

  wifiPASSEEPROM = EEPROM.readString(EEPROM_ADDRESS_WIFI_PASS);//lee el PASS de la eeprom
  Serial.print("PASS EEPROM: ");
  Serial.println(wifiPASSEEPROM);
  password = wifiPASSEEPROM;

  flagOTAEEPROM = EEPROM.read(EEPROM_ADDRESS_FLAG_OTA);//lee el flag de la eeprom
  Serial.print("flag OTA EEPROM: ");
  Serial.println(flagOTAEEPROM);
  flagOTA = flagOTAEEPROM;

 
  cambiarConfigMQTT(numeroSensor);

}

void probarNeopixel(void){

  //prueba de LEDs Neopixel
  pixels.clear();
  delay(20);
  apagarNeopixel();
  delay(20);
  encenderNeopixel('R');
  delay(timeLED);
  apagarNeopixel();
  delay(timeLED);
  encenderNeopixel('G');
  //encenderNeopixel('B');
  delay(timeLED);
  apagarNeopixel();
  delay(timeLED);
  encenderNeopixel('Y');
  delay(timeLED);
  apagarNeopixel();
  delay(timeLED);

}

void displayText(char letra){

  int timeDisplayBienvenida = 1000;
  
  display.clearDisplay();
  //Serial.println("HandTemp");
  display.setFont(font1);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  //display.clearDisplay();
  if(letra == 'R'){
    display.setCursor(20,20);
    display.println("Recuperando");
    display.println("conexion");
  }else if(letra == '1'){
    display.setCursor(10,20);
    display.println("MODO RED");
    display.println("conexion OK");
  }else if(letra == '0'){
    display.setCursor(5,20);
    display.println("MODO LOCAL");
    display.println("conexion no OK");
  }
    
  display.display();
  delay(timeDisplayBienvenida);
  // limpio el bufer para la proxima pantalla
  display.clearDisplay();
  display.setFont(mainFont);  

}

void leerEmisividad(void){

  if(cantSensoresIR == 1){
    Serial.print("lectura de emisividad sensor 1: ");
    Serial.println(mlx1.readEmissivity());
  }
}
