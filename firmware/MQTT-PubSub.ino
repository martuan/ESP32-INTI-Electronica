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

// #include <Arduino.h>
 #include <PubSubClient.h>
// #include <Wire.h>
// #include <SparkFunMLXm.h>

// #include <ArduinoJson.h>

// #include <SPI.h>
// #include <Adafruit_SH1106.h>
// #include <Adafruit_GFX.h>
// #include <Fonts/FreeSans9pt7b.h>
// #include <Fonts/FreeSans12pt7b.h>
// #include <Fonts/FreeSans18pt7b.h>
 #include <esp_task_wdt.h>
 #include <BluetoothSerial.h>
// #include <Adafruit_NeoPixel.h>
// #include <EEPROM.h>
// #include <ESPmDNS.h>
// //#include <WiFiUdp.h>
// //#include <ArduinoOTA.h>
// //#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager  
// //#include <webserial_webpage.h>
//#include <WebSerial.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
//#include <ArduinoJson.h>
//#include <PubSubClient.h>

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

//************CONFIGURACIÓN DE TELEGRAM*****************

// Initialize Telegram BOT
#define BOTtoken "5021419842:AAGWdRxVtpBNxiWtD3oEDN_CIkK1LnuqefE"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "1461403941"

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

const int ledPin = 2;
bool ledState = LOW;

//************CONFIGURACIÓN DE TELEGRAM*****************

#define LED_ONBOARD 2

// define the number of bytes you want to access
#define EEPROM_SIZE 300
#define EEPROM_ADDRESS_WIFI_SSID 20//String
#define EEPROM_ADDRESS_WIFI_PASS 60//String

String version = "V:1.00";
#define WDT_TIMEOUT 60


// Conexiones SPI para el display:
#define OLED_MOSI   13
#define OLED_CLK   14
#define OLED_DC    27
#define OLED_CS    15
#define OLED_RESET 26

#define LINEHEIGHT 22  // para saltos de linea

//Adafruit_SH1106 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
// Neopixel
#define PIN 33
#define CANTLEDS  16
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(CANTLEDS, PIN, NEO_GRB + NEO_KHZ800);

//**************************************
//*********** MQTT CONFIG **************
//**************************************

String nombreSens = "sensor_1";

//Servidor en la nube
//const char *mqtt_server = "broker.hivemq.com";//"10.24.37.150";//"broker.hivemq.com";//"test.mosquitto.org";//"10.24.37.150";//"broker.hivemq.com";//"10.24.37.150";//"broker.hivemq.com";
char mqtt_server[100] = "broker.hivemq.com";
const int mqtt_port = 1883;
const char *mqtt_user = "pentium";
const char *mqtt_pass = "7364";
char root_topic_subscribe[100] = "INTI/Electronica/esp32/sensor_1";//"undefi";
char root_topic_publish[100] = "INTI/Electronica/esp32/sensor_1";//"undefi";
char tempAmbiente_topic_subscribe[100] = "INTI/Electronica/esp32/sensor_1";//"undefi/handtemp/1/tempAmbiente";
char tempAmbiente_topic_publish[100] = "INTI/Electronica/esp32/sensor_1";//"undefi/handtemp/1/tempAmbiente";
char tempObjeto_topic_subscribe[100] = "INTI/Electronica/esp32/sensor_1";//"undefi/handtemp/1/tempObjeto";
char tempObjeto_topic_publish[100] = "INTI/Electronica/esp32/sensor_1";//"undefi/handtemp/1/tempObjeto";
char data_topic_publish[100] = "INTI/Electronica/esp32/data/sensor_1";//"undefi/handtemp/1/tempObjeto";
char handtempKeepAlive_topic_publish[100] = "INTI/Electronica/esp32/keepAlive/sensor_1";


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
int contadorProceso3 = 0;
int flagCambioModoLocal = 0;
int cantFallos = 0;

int flagModoDebug = 0;

float arrayUltimasLecturas[100] = {};
int idx2 = 0;
int flagUltimasLecturas = 0;
char outstr[10] = {};
float valor = 0;
int Red = 0;
int Green = 0;
int Blue = 0;
int neopixelFueApagado = 0;
char msgKeepAlive[150] = {};
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
//*********** DISPLAY  *****************
//**************************************

// fuentes de texto
// auto mainFont = &FreeSans12pt7b;
// auto font1 = &FreeSans9pt7b;
// auto font2 = &FreeSans12pt7b;
// auto font3 = &FreeSans18pt7b;

//AsyncWebServer server(80);

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

// // DISPLAY
// void showAllDataDebugMode(double tempC, double tempAmbC, float dist);
// void cambioDeParametros(void);
// //void grabarEmisividadEnSensorIR(void);
// void publicarDataAmbiente(double, char[]);
// void publicarDataObjeto(double, double, double, char[]);
// void publicarData(long);
// void obtenerFechaHora(void);

// void encenderNeopixel(char);
// void apagarNeopixel(void);
void publicarKeepAlive(void);
// void cambiarConfigMQTT(uint8_t);
void comprobarConexion(void);
// void probarNeopixel(void);
 void setupModoRed(void);
// void fallaConexion(void);
// void displayText(char);
// void switchCaseParametros(char, String);
// void modoAccessPoint(void);
// void imprimirModo(void);
// void handTempText(void);
void recvMsg(uint8_t *, size_t);
void setup_mqtt(void);
void handleNewMessages(int);

//timer de usos generales 1000ms
void IRAM_ATTR onTimer3() {

    contadorProceso1++;
    contadorProceso2++;
	  contadorProceso3++;

    if(contadorProceso1 == tiempo1/6){//proceso 1
        contadorProceso1 = 0;//resetea el contador
        flagProceso1 = 1;
    }
	if(contadorProceso2 == tiempo1){//proceso 2
        contadorProceso2 = 0;//resetea el contador
        flagProceso2 = 1;
    }
    if(contadorProceso3 == tiempo1 * 10){//proceso 3 (cada 10 min)
        contadorProceso3 = 0;//resetea el contador
        flagProceso3 = 1;//para chequear conexión con red wifi y broker
    }

    
}


//*************************************************************************
// SETUP
//*************************************************************************




void setup() {
    
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    
    Serial.begin(115200);
        


    SerialBT.begin("INTI-ESP32 Bluetooth"); //Bluetooth device name

    macAdd = WiFi.macAddress();
    Serial.println( "MAC address: " + macAdd );

    ssid = "wifi01-ei";
    password = "Ax32MnF1975-ReB";
    //ssid = "milton";
    //password = "paternal";

	  client1.subscribe(root_topic_subscribe);

	#ifdef ESP8266
    	configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
    	client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  	#endif

	randomSeed(analogRead(27));
    

    timer3 = timerBegin(2, 80, true);
    timerAttachInterrupt(timer3, &onTimer3, true);
    timerAlarmWrite(timer3, 1000000, true);//valor en microsegundos [1 s]
    timerAlarmEnable(timer3);

    esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
    esp_task_wdt_add(NULL); //add current thread to WDT watch

    pinMode(LED_ONBOARD, OUTPUT);
	pinMode(ledPin, OUTPUT);
  	digitalWrite(ledPin, ledState);

 
    //digitalWrite(buzzPin, HIGH);
    //cargarDesdeEEPROM();//levanta las variables guardadas previamente en EEPROM
    
    // inicio display
    // display.begin(SH1106_SWITCHCAPVCC);
    // display.clearDisplay();//borra cualquier dato del buffer del display
    // display.setFont(mainFont);
    // display.println(" ");//línea vacía
    // display.display();


    // pantalla de bienvenida
    //handTempText();

    //pixels.begin(); // This initializes the NeoPixel library.
    //probarNeopixel();
  
    setupModoRed();//configura MQTT, revisa conectividad
	//setup_wifi();

    //imprimirModo();//informa por display Modo local o Modo Red
/*
    WebSerial.begin(&server);
    WebSerial.msgCallback(recvMsg);
    server.begin();
*/
    
    
}


//*************************************************************************
// LOOP
//*************************************************************************


void loop() {

	//Chequea cada cierto tiempo mensajes enviados por Telegram
/*
	if(flagProceso1 == 1){
		flagProceso1 = 0;
		int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
		Serial.println("numNewMessages: " + numNewMessages);	
		while(numNewMessages) {
      		Serial.println("got response");
      		handleNewMessages(numNewMessages);
      		numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    	}
	}
*/	
/*
	if (millis() > lastTimeBotRan + botRequestDelay)  {
		int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
		Serial.print("mensajes: ");
		Serial.println(numNewMessages);
		while(numNewMessages) {
		Serial.println("got response");
		handleNewMessages(numNewMessages);
		numNewMessages = bot.getUpdates(bot.last_message_received + 1);
		}

		lastTimeBotRan = millis();
  	}
   */
	long numero = 0;

    //cambioDeParametros();

    client1.loop();
    

    if(flagProceso1 == 1){//publica cada cierto tiempo un número aleatorio en formato json
      flagProceso1 = 0;
      numero = random(1,50);
	  Serial.println(numero);
      //publicarData(numero);
	  
	  
	  	int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
		Serial.print("numNewMessages: ");
		Serial.println(numNewMessages);
		
		while(numNewMessages) {
			Serial.println("got response");
			handleNewMessages(numNewMessages);
			numNewMessages = bot.getUpdates(bot.last_message_received + 1);
		}
		
		//WebSerial.println("Hello!");
	
    }
/*
    //Keep Alive
    //Si pasó cierto tiempo y la conexión está OK
    if(flagProceso2 == 1 && flagConexionOK == 1){
    
      flagProceso2 = 0;
    
      if(client1.connected()){
        publicarKeepAlive();
		
      }else{
        Serial.println("Perdió la conexión");
        flagConexionOK = 0;//hubo un problema, lo avisa mediante el flag
      }
   
    }
*/
    //alimentar watchDog
    esp_task_wdt_reset();

  /*  

    

    //cada cierto tiempo chequea la conexión MQTT solo si la perdió

    if(flagProceso3 && flagConexionOK == 0){

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
   */
    
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

    //WiFiManager wm;
   
    //bool res;
   


   

      //Serial.println("Modo reconexión");//si no viene de un reset

      int cuenta = 0;
      //delay(10);
      // Nos conectamos a nuestra red Wifi
      Serial.println();
      Serial.print("Conectando a ssid: ");
      //Serial.println(savedSSID);
      Serial.println(ssid);

      //const char* ssidConverted = savedSSID.c_str();
      //const char* passwordConverted = savedPASS.c_str();
      const char* ssidConverted = ssid.c_str();
      const char* passwordConverted = password.c_str();
        
      //WiFi.disconnect(1);
        
      //WiFi.persistent(false);
      WiFi.begin(ssidConverted, passwordConverted);
	  


      while ((WiFi.status() != WL_CONNECTED) && cuenta < 20) {//límite de 20 intentos de 500 ms
        delay(500);
        Serial.print(".");
        cuenta++;
      
      }
      if(WiFi.status() != WL_CONNECTED){//si no logró conectarse
      
        Serial.println("No es posible conectar a WiFi");
        Serial.println("Se cambia a MODO LOCAL");

      }else{//si logró conectarse

        Serial.println("");
        Serial.println("Conectado a red WiFi!");
        Serial.println("Dirección IP: ");
        Serial.println(WiFi.localIP());
        delay(5000);
		#ifdef ESP32
			client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
		#endif	 

      }


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
	  if(client1.subscribe(root_topic_subscribe)){
        Serial.println("Suscripcion ok");
	  }else{
        Serial.println("fallo Suscripción");
      }
    
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

  if(charParamID == 'P'){
	  Serial.println("Llegó una P");
	  digitalWrite(LED_ONBOARD, HIGH);
  }else if(charParamID == 'A'){
	  Serial.println("Llegó una A");
	  digitalWrite(LED_ONBOARD, LOW);
  }
  
  Serial.println(charParamID);
  
  //obtiene el valor
  for(int i = 1; i < incoming.length(); i++){
    valorParam += incoming.charAt(i);
  }

  Serial.println(valorParam);



  //evalua el identificador y los parámetros enviados
//  switchCaseParametros(charParamID, valorParam);

  //borra el contenido y lo prepara para recibir uno nuevo
  incoming = "";

}

// void handTempText(void) {

 
//   int timeDisplayBienvenida = 2000;
  
//   display.clearDisplay();
//   Serial.println("INTI-ESP32");
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   display.setCursor(6,30);
//   display.clearDisplay();
//   display.println("INTI-ESP32");
//   display.drawFastHLine(8,10,112,WHITE);
//   display.drawFastHLine(8,40,112,WHITE);
//   display.setCursor(6,40 + LINEHEIGHT);
//   display.setFont(mainFont);
//   Serial.println(version);
//   display.println(version); 
//   display.display();
//   delay(timeDisplayBienvenida);
//   // limpio el bufer para la proxima pantalla
//   display.clearDisplay();
//   /*
//   if(flagConexionOK == 1){//si está en modo red
//     Serial.println("MODO RED");
//     display.setTextSize(1);
//     display.setTextColor(WHITE);
//     display.setCursor(6,30);
//     display.setFont(font1);
//     display.println("MODO RED");
//     display.display();
//     delay(2000);
//     display.clearDisplay();
//     display.setCursor(6,30);
//     display.println(macAdd);
//     display.display();
//     delay(5000);
//   }else{
//     Serial.println("MODO LOCAL");
//     display.setTextSize(1);
//     display.setTextColor(WHITE);
//     display.setCursor(6,30);
//     display.setFont(font1);
//     display.println("MODO LOCAL");
//     display.display();
//   }
  
//   delay(1000);
//   display.setFont(mainFont);
//   display.clearDisplay();
// */
// }

// void imprimirModo(void){

//     if(flagConexionOK == 1){//si está en modo red
//         Serial.println("MODO RED");
//         display.setTextSize(1);
//         display.setTextColor(WHITE);
//         display.setCursor(6,30);
//         display.setFont(font1);
//         display.println("MODO RED");
//         display.display();
//         delay(2000);
//         display.clearDisplay();
//         display.setCursor(6,30);
//         display.println(macAdd);
//         display.display();
//         delay(5000);
//     }else{
//         Serial.println("MODO LOCAL");
//         display.setTextSize(1);
//         display.setTextColor(WHITE);
//         display.setCursor(6,30);
//         display.setFont(font1);
//         display.println("MODO LOCAL");
//         display.display();
//     }
    
//     delay(1000);
//     display.setFont(mainFont);
//     display.clearDisplay();
// }

// void fallaConexion(void) {

//   int timeDisplayBienvenida = 2000;
  
//   display.clearDisplay();
//   Serial.println("INTI-ESP32");
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   display.setCursor(6,30);
//   display.clearDisplay();
//   Serial.println("Falla conexión");
//   display.println("Falla conexión");
//   display.display();
//   delay(timeDisplayBienvenida);
//   // limpio el bufer para la proxima pantalla
//   display.clearDisplay();
    
//   display.setFont(mainFont);
  
// }


// void modoAccessPoint(void) {

//   int timeDisplayBienvenida = 2000;
  
//   display.clearDisplay();
//   Serial.println("INTI-ESP32 - Modo Access Point");
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   display.setFont(font1);
//   display.setCursor(6,30);
//   display.clearDisplay();
//   Serial.println("SSID:INTI-ESP32");
//   Serial.println("PASS:12345678");
//   // limpio el bufer para la proxima pantalla
//   display.clearDisplay();

//   display.println("MODO AP:");//Imprime Modo AP
//   display.display();
  
//   delay(timeDisplayBienvenida);
//   // limpio el bufer para la proxima pantalla
//   display.clearDisplay();
  
//   display.setCursor(6,30);
//   display.println("SSID:");//Imprime SSID
//   display.println("INTI-ESP32");
//   display.display();
  
//   delay(timeDisplayBienvenida);
//   // limpio el bufer para la proxima pantalla
//   display.clearDisplay();

//   display.setCursor(6,30);
//   display.println("PASS:");//Imprime PASS
//   display.println("12345678");
//   display.display();
  
//   delay(timeDisplayBienvenida);
//   // limpio el bufer para la proxima pantalla
//   display.clearDisplay();    
//   display.setFont(mainFont);
//   display.setCursor(6,30);
  
// }


// void showTemperature(double temperature, String msg){

//   int timeDisplay = 2000;

//   display.clearDisplay();
//   String tempText = static_cast<String>(round(temperature*100) / 100);
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   //display.setCursor(10,20);
//   display.setCursor(3,20);
//   //display.setFont(secondFont);
//   display.setFont(font1);  // default font
//   //display.setFont(secondFont);  // default font
//   //display.println("Su temperatura es");
//   display.println("Su temperatura");
//   display.setFont(font3);
//   //display.setTextSize(1);
//   display.setCursor(26,52);
//   display.println(tempText);
//   display.display();
//   delay(timeDisplay);
//   display.clearDisplay();
  
//   display.setFont(mainFont);
//   display.setCursor(10,26);
//   display.println("Acceso");
//   display.setCursor(10,26 + LINEHEIGHT);
//   display.println(msg);
//   display.display();
//   delay(timeDisplay);
//   display.clearDisplay();
  
// }


// void showAllDataDebugMode(double tempC, double tempAmbC, float dist){

//   int timeDisplay = 6000;

//   display.clearDisplay();
//   String tempCText = static_cast<String>(round(tempC*100) / 100);
//   String tempAmbCText = static_cast<String>(round(tempAmbC*100) / 100);
//   String distText = static_cast<String>(round(dist*100) / 100);
//   String tempOffsetText = static_cast<String>(round(tempOffset*100) / 100);
//   //String tempAmbOffsetText = static_cast<String>(round(tempAmbOffset*100) / 100);
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   //display.setCursor(10,20);
//   display.setCursor(3,15);
//   //display.setFont(secondFont);
//   display.setFont(font1);  // default font
//   //display.setFont(secondFont);  // default font
//   //display.println("Su temperatura es");
//   display.print("Tobj: ");
//   display.println(tempCText);
//   display.print("Tamb: ");
//   display.println(tempAmbCText);
//   display.print("Distancia: ");
//   display.println(distText);

//   //display.setTextSize(1);
//   //display.setCursor(26,52);
//   //display.println(tempText);
//   display.display();
//   delay(timeDisplay);
//   display.clearDisplay();

//   display.setCursor(3,15);

//   display.print("tOffs: ");
//   display.println(tempOffsetText);
//   //display.print("tAmbOffs: ");
//   //display.println(tempAmbOffsetText);

//   display.display();
//   delay(timeDisplay);
  
//   display.setFont(mainFont);  // default font
//   display.clearDisplay();
  
// }

// //puede cambiar parámetros a través del puerto serie o por bluetooth
// //Se debe enviar un caracter de identificación del parámetro a cambiar y
// //luego el valor.
// //Por ejemplo: cambiar el tiempo entre lecturas de temperatura
// //enviar T100  siendo T: tiempoEntreLecturas; 100: 100 ms
// //los parámetros que se pueden modificar son:
// //  distanciaConfigurada--> D;
// //  distanciaTolerancia--> t;
// //  tempFiebre--> F;
// //  tempMin--> m;
// //  tempMax--> M;
// //  tempOffset--> O;
// //  tiempoEntreLecturas--> T;
// //  cantLecturas--> C;
// //  emisividad--> E;
// //  Wifi--> W;  [Ejemplo: Wmyssid mypassword](El espacio se usa como delimitador)
// //  debug--> d  [1 para activarlo; 0 para desactivarlo]
// //  cantSensoresIR-->S
// //  consultarLecturas-->P
// //  escannearDispositivosI2C-->s  [1 para activarlo; 0 para desactivarlo]
// //  cambiarDireccionI2C-->A       [A90 91]
// //  analizarLecturasCantidad-->U
// //  intercambiarSensores-->I;
// //  consultarContadorReconexion--X; 
// void cambioDeParametros(void){

//   char charParamID = ' ';
//   String valorParam = "";
//   int inChar = 0;
//   String inString = "";
    
  
//   //**** Chequeo por Serie o Bluetooth ***************
//   while (Serial.available() > 0 || SerialBT.available() > 0) {

//     if(Serial.available() > 0){
//       inChar = Serial.read();
//     }else if(SerialBT.available() > 0){
//       inChar = SerialBT.read();
//     }
    

//     if(inChar != '\n'){
//       Serial.print((char)inChar);

//       inString += (char)inChar;//encola los caracteres recibidos

//     }else{//si llegó el caracter de terminación
      
//       Serial.print("Input string: ");
//       Serial.println(inString);
//       Serial.print("string length: ");
//       Serial.println(inString.length());


//       //obtiene el identificador
//       charParamID = inString.charAt(0);
      
//       Serial.println(charParamID);
      
//       //obtiene el valor
//       for(int i = 1; i < inString.length(); i++){
//         valorParam += inString.charAt(i);
//       }

//       Serial.println(valorParam);

//       //evalua el identificador y los parámetros enviados
//       switchCaseParametros(charParamID, valorParam);
      
//       //borra el contenido y lo prepara para recibir uno nuevo
//       inString = "";
    
//     }
//   }

// }

// //realiza la lectura del timestamp desde el server ntp
// void obtenerFechaHora(void){

//   if(!getLocalTime(&timeStamp)){
//       Serial.println("Failed to obtain time");
//   }else{

//       //guarda cada componente del timeStamp en variables
//       strftime(timeYear,10, "%Y", &timeStamp);
//       strftime(timeMonth,10, "%B", &timeStamp);
//       strftime(timeDayName,10, "%A", &timeStamp);
//       strftime(timeDayNumber,10, "%d", &timeStamp);
//       strftime(timeHour24,10, "%H", &timeStamp);   
//       strftime(timeHour12,10, "%I", &timeStamp);
//       strftime(timeMinute,10, "%M", &timeStamp);
//       strftime(timeSecond,10, "%S", &timeStamp);
      
//       strcpy(fechaHora, "");//borra el contenido del string

//       //arma un string con los componentes que interesan
//       strcat(fechaHora, timeYear);
//       strcat(fechaHora, " ");
//       strcat(fechaHora, timeMonth);
//       strcat(fechaHora, " ");
//       strcat(fechaHora, timeDayNumber);
//       strcat(fechaHora, " ");
//       strcat(fechaHora, timeHour24);
//       strcat(fechaHora, ":");
//       strcat(fechaHora, timeMinute);
//       strcat(fechaHora, ":");
//       strcat(fechaHora, timeSecond);

//   }
// }



// void publicarDataObjeto(double tempC, double tempF, double emisividadL, char fechaHora[]){
        
// /*		
//   //prepara el objeto JSON para publicar por MQTT
//   //StaticJsonBuffer<300> JSONbuffer;
//   StaticJsonDocument<300> JSONbuffer;
//   JsonObject JSONencoder = JSONbuffer.createObject();
 
//   JSONencoder["temp"] = round(tempC * 100) / 100;
  
//   char JSONmessageBuffer[300];
//   JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
//   Serial.println("Enviando data de Temperatura del objeto por MQTT1...");
//   Serial.println(JSONmessageBuffer);

//   client1.publish(tempObjeto_topic_publish, JSONmessageBuffer);
// */
// }

// void publicarDataAmbiente(double tempAmbienteC, char fechaHora[]){
// /*
        
//   //prepara el objeto JSON para publicar por MQTT
//   //StaticJsonBuffer<300> JSONbuffer;
//   StaticJsonDocument<300> JSONbuffer;
//   JsonObject JSONencoder = JSONbuffer.createObject();

//   JSONencoder["device"] = "ESP32";
//   JSONencoder["Temp Ambiente *C"] = round(tempAmbienteC * 100) / 100 ;
//   JSONencoder["timeStamp"] = fechaHora;
  
//   char JSONmessageBuffer[300];
//   JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
//   Serial.println("Enviando data de Temperatura del ambiente por MQTT...");
//   Serial.println(JSONmessageBuffer);
  
//   client1.publish(tempAmbiente_topic_publish, JSONmessageBuffer);
//   */
// }

// void publicarData(long dato){


// /*
// //***************VER de convertir a JSON 6 ******************

// 	// ArduinoJson 6
// 	DynamicJsonDocument doc(1024);
// 	doc["key"] = "value";
// 	doc["raw"] = serialized("[1,2,3]");
// 	serializeJson(doc, Serial);
// */

//   /*      
//   //prepara el objeto JSON para publicar por MQTT
//   //StaticJsonBuffer<300> JSONbuffer;
//   StaticJsonDocument<300> JSONbuffer;
//   JsonObject JSONencoder = JSONbuffer.createObject();
 
//   JSONencoder["dato"] = round(dato * 100) / 100;
  
//   char JSONmessageBuffer[300];
//   JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
//   Serial.println("Enviando data por MQTT1...");
//   Serial.println(JSONmessageBuffer);
//   WebSerial.println(JSONmessageBuffer);

//   client1.publish(data_topic_publish, JSONmessageBuffer);
// */
// }
// /*
// void encenderNeopixel(char color){

//     int i = 0;

//     pixels.setPin(33);
//     //pixels.setBrightness(200);
//     //pixels.show();

//     switch(color){
//         case 'R'://RED
//             //todos los LEDs en rojo
//             for(i = 0; i < CANTLEDS; i++){
//                 pixels.setPixelColor(i, pixels.Color(255, 0, 0));
//             }
//             //pixels.show();
//         break;
//         case 'G'://GREEN
//             //todos los LEDs en verde
//             for(i = 0; i < CANTLEDS; i++){
//                 pixels.setPixelColor(i, pixels.Color(0, 255, 0));
//             }
//             //pixels.show();
//         break;
//         case 'Y'://YELLOW
//             //todos los LEDs en amarillo
//             for(i = 0; i < CANTLEDS; i++){
//                 pixels.setPixelColor(i, pixels.Color(255, 255, 0));
//             }

//            // pixels.show();
//         break;
//         case 'B'://YELLOW
//             //todos los LEDs en amarillo
//             for(i = 0; i < CANTLEDS; i++){
//                 pixels.setPixelColor(i, pixels.Color(255, 0, 255));
//             }

//            // pixels.show();
//         break;
//         default:
//             Serial.println("Color incorrecto");
//         break;
//     }

//     pixels.show();
//     pixels.setPin(18);// cambia el pin del neopixel para canalizar los ruidos a otro pin
//     //delay(1000);
//     neopixelFueApagado = 0;//cambia el estado
    
// }
// */

// /*
// void apagarNeopixel(void){

//     int i = 0;

//     //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
//     for(i = 0; i < CANTLEDS; i++){
//         pixels.setPixelColor(i, pixels.Color(0, 0, 0));
//     }
//     pixels.show();
//     pixels.setPin(33);

//     for(i = 0; i < CANTLEDS; i++){
//         pixels.setPixelColor(i, pixels.Color(0, 0, 0));
//     }
    
//     pixels.show();
//     pixels.setPin(18);// cambia el pin del neopixel para canalizar los ruidos a otro pin
//     //pixels.show();
//     neopixelFueApagado = 1;//cambia el estado
     
// }
// */

void publicarKeepAlive(void){
  
  strcpy(msgKeepAlive, "");//borra el contenido del string
  strcat(msgKeepAlive, "...Estoy vivo. ");
  strcat(msgKeepAlive, handtempKeepAlive_topic_publish);
  //strcat(msgKeepAlive, " ");
  //strcat(msgKeepAlive, tiempoSinReset);

  client1.publish(handtempKeepAlive_topic_publish, msgKeepAlive);
  Serial.println(handtempKeepAlive_topic_publish);
  Serial.println(msgKeepAlive);
  //WebSerial.println(msgKeepAlive);

}

// void cambiarConfigMQTT(uint8_t numSensor){
 
//   char strNombreSensor[80] = "";
//   char numMacAdd[80] = {}; 
//   char strTopico[80] = "INTI-ESP32/";
//   char str[10] = {};
//   char str2[80] = {};
//   char strBroker[80] = {};


//   strcat(numMacAdd, macAdd.c_str());
//   //sprintf(str, "%d", numSensor);
//   strcat(strNombreSensor, numMacAdd);
//   //strcat(strNombreSensor, str);
//   Serial.print("strNombreSensor: ");
//   Serial.println(strNombreSensor);

//   strcat(strTopico, strNombreSensor);
//   Serial.print("strTopico: ");
//   Serial.println(strTopico);

  
//   strcpy(root_topic_subscribe, strTopico);
//   strcpy(root_topic_publish, strTopico);
//   strcpy(tempAmbiente_topic_publish, strTopico);
//   strcpy(tempObjeto_topic_publish, strTopico);
//   strcpy(handtempKeepAlive_topic_publish, strTopico);
//   strcat(strBroker, broker.c_str());
//   strcpy(mqtt_server, strBroker);

// }

// //si perdió la conexión, intenta recuperarla
// void comprobarConexion(void){

//   setupModoRed();//configura MQTT, revisa conectividad
//   /*
//   if(!client1.connected()){
//     reconnect();
//   }
//   */
//   /*
//   if(!client2.connected()){
//     reconnect();
//   }
//   */

// }


// void switchCaseParametros(char charParamID, String valorParam){

//   int inChar = 0;
//   int index = 0;
//   int valorParamLength = 0;
//   int endIndex = 0;
//   int modoDebug = 0;
//   int consultarLecturas = 0;
//   int correccionActivada = 0;
//   uint8_t numSensor = 0;
//   uint16_t direccion = 0;
//   int scanActivado = 0;
//   byte oldAddress = 0;
//   byte newAddress = 0;
//   int analizarLecturasCantidad = 0;
//   int intercambioSensores = 0;
//   int color = 0;
//   String nombreSensor = "";
  
//   //valorParam = 
//   valorParam.replace(0x0A,'\0');//Se filtra el caracter LF
//   valorParam.replace(0x0D,'\0');//Se filtra el caracter CR

//   switch(charParamID){
//     case 'D':
// 	/*
//       distanciaConfigurada = valorParam.toFloat();
//       Serial.print("DistanciaConfigurada: ");
//       Serial.println(distanciaConfigurada);
// 	  */
//     break;
//     case 't':
// 	/*
//       distanciaTolerancia = valorParam.toFloat();
//       Serial.print("DistanciaTolerancia: ");
//       Serial.println(distanciaTolerancia);
// 	  */
//     break;
//     case 'O':
//       tempOffset = valorParam.toDouble();
//       Serial.print("TempOffset: ");
//       Serial.println(tempOffset);

//       //guarda en EEPROM el OFFSET de temperatura
//       //tempOffsetEEPROM = (uint8_t)(tempOffset * 10);
//       tempOffsetEEPROM = tempOffset;
//       EEPROM.writeDouble(0, tempOffsetEEPROM);
//       EEPROM.commit();

//     break;
//     case 'T':
//       /*
// 	  tiempoEntreLecturas = valorParam.toInt();
//       Serial.print("TiempoEntreLecturas: ");
//       Serial.println(tiempoEntreLecturas);
// 	  */
//     break;
//     case 'm':
// 	/*
//       tempMin = valorParam.toDouble();
//       Serial.print("tempMin: ");
//       Serial.println(tempMin);
// 	  */
//     break;
//     case 'M':
// 	/*
//       tempMax = valorParam.toDouble();
//       Serial.print("tempMax: ");
//       Serial.println(tempMax);
// 	  */
//     break;
//     case 'F':
// 	/*
//       tempFiebre = valorParam.toDouble();
//       Serial.print("tempFiebre: ");
//       Serial.println(tempFiebre);
// 	  */
//     break;
//     case 'E':
// 	/*
//       emisividadPorSerie = valorParam.toFloat();
//       emisividad = (uint16_t)(emisividadPorSerie * 65535);

//       Serial.println(emisividadPorSerie);
//       Serial.println(emisividad);

//       mlx1.writeEmissivityReg(emisividad);//escribe la emisividad al sensor
//       mlx1.begin();

//       delay(500);
      
//       //Verificación de emisividad
//       Serial.println("Reconecte el sensor");
//       Serial.println("10...");
//       delay(1000);
//       Serial.println("9...");
//       delay(1000);
//       Serial.println("8...");
//       delay(1000);
//       Serial.println("7...");
//       delay(1000);
//       Serial.println("6...");
//       delay(1000);
//       Serial.println("5...");
//       delay(1000);
//       Serial.println("4...");
//       delay(1000);
//       Serial.println("3...");
//       delay(1000);
//       Serial.println("2...");
//       delay(1000);
//       Serial.println("1...");
//       delay(1000);
//       Serial.println("Se leerá la emisividad");
//       delay(3000);

//       Serial.print("Emisividad sensor 1: ");
//       Serial.println(mlx1.readEmissivity());
// */
//     break;
//     case 'W':

//       Serial.println("Wifi: ");
//       valorParamLength = strlen(valorParam.c_str());
//       endIndex = valorParamLength;

//       index = valorParam.indexOf(' ');

//       ssid = valorParam.substring(0, index);
//       Serial.println(ssid);
//       //password = valorParam.substring(index + 1, endIndex - 1);
//       password = valorParam.substring(index + 1, endIndex);
//       Serial.println(password);

//       //guarda config wifi en EEPROM
//       EEPROM.writeString(EEPROM_ADDRESS_WIFI_SSID, ssid);
//       EEPROM.commit();
//       EEPROM.writeString(EEPROM_ADDRESS_WIFI_PASS, password);
//       EEPROM.commit();

//       setup_wifi();

//     break;
//     case 'Q':

//       numSensor = valorParam.toInt();
//       Serial.println("MQTT (numSensor): ");
//       Serial.println(numSensor);

//       if(numSensor >= 0 && numSensor <= 255){

//         cambiarConfigMQTT(numSensor);

//         //guarda en EEPROM el número de sensor
//         EEPROM.write(0 + sizeof(double), numSensor);
//         EEPROM.commit();

//       }else{

//         Serial.println("Número incorrecto. Se acepta entre 0 y 255");

//       }

//     break;
//     case 'C':
// 	/*
//       cantLecturas = valorParam.toInt();
//       Serial.print("cantLecturas: ");
//       Serial.println(cantLecturas);
// 	  */
//     break;
//     case 'd':
//       modoDebug = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
//       Serial.print("modoDebug: ");
//       Serial.println(modoDebug);
//       if(modoDebug){
//         flagModoDebug = 1;
//       }else{
//         flagModoDebug = 0;
//       }
      
//     break;
//     case 'S':
// 	/*
//       cantSensoresIR = valorParam.toInt();

//       if(cantSensoresIR < 1 && cantSensoresIR > 3){//valida la cantidad declarada
//         Serial.println("cantidad de sensoresIR incorrecta. Se configura a 1");
//         cantSensoresIR = 1;
//       }

//       Serial.print("cantSensoresIR: ");
//       Serial.println(cantSensoresIR);
//       flagLecturaEmisividad = 1;//habilita para volver a leer la emisividad
// */
//     break;
//     case 'P':
//       //imprimir por pantalla todos los valores adquiridos
//       consultarLecturas = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
//       Serial.print("consultarLecturas: ");
//       Serial.println(consultarLecturas);
//       Serial.print("Índice: ");
//       Serial.println(idx2);
//       if(consultarLecturas == 1){
//         flagUltimasLecturas = 1;
//       }else{
//         flagUltimasLecturas = 0;
//       }

//     break;
//     case 'c':
// 	/*
//       correccionActivada = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
//       Serial.print("correccionActivada: ");
//       Serial.println(correccionActivada);
//       if(correccionActivada){
//         flagCorreccion = 1;
//       }else{
//         flagCorreccion = 0;
//       }
// */
//     break;
//     case 'A':
      
//       valorParamLength = strlen(valorParam.c_str());
//       endIndex = valorParamLength;

//       index = valorParam.indexOf(' ');

//       oldAddress = (byte)valorParam.substring(0, index).toInt();
                
//       Serial.println("OldAddress: ");
//       Serial.println(oldAddress, HEX);
//       newAddress = (byte)valorParam.substring(index + 1, endIndex - 1).toInt();
//       Serial.println("NewAddress: ");
//       Serial.println(newAddress, HEX);

//       //cambiarDireccionI2CNuevaVersion(oldAddress, newAddress);

//     break;
//     case 's':
// 	/*
//       scanActivado = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
//       Serial.print("scanActivado: ");
//       Serial.println(scanActivado);
//       if(scanActivado){
//         flagScan = 1;
//       }else{
//         flagScan = 0;
//       }
// 	  */
//     break;
//     case 'U':
//       analizarLecturasCantidad = valorParam.toInt();
//       Serial.print("AnalizarLecturasCantidad: ");
//       Serial.println(analizarLecturasCantidad);
//       if(analizarLecturasCantidad > 0 && analizarLecturasCantidad <= 10){

//           //analizarLecturas(analizarLecturasCantidad);

//       }else{
//         Serial.println("Valor incorrecto");
//       }
//     break;
//     case 'I':
//       intercambioSensores = valorParam.toInt();
//       Serial.print("intercambioSensores: ");
//       Serial.println(intercambioSensores);

//       //intercambiarSensores();
//     break;
//     case 'R':
//       color = valorParam.toInt();
//       Serial.print("red: ");
//       Serial.println(color);
//       Red = color;
//     break;
//     case 'G':
//       color = valorParam.toInt();
//       Serial.print("green: ");
//       Serial.println(color);
//       Green = color;
//     break;
//     case 'B':
//       color = valorParam.toInt();
//       Serial.print("blue: ");
//       Serial.println(color);
//       Blue = color;
//     break;
//     case 'K':

//       //valorParamLength = strlen(valorParam.c_str());
//       //endIndex = valorParamLength;

//       //index = valorParam.indexOf(' ');

//       broker = valorParam;
//       Serial.println("MQTT (broker): ");
//       Serial.println(broker);
//       Serial.println(strlen(valorParam.c_str()));

//       //Serial.println(ssid);
//       //password = valorParam.substring(index + 1, endIndex - 1);
//       //Serial.println(password);

//       //guarda config mqtt_server en EEPROM
//       //EEPROM.writeString(EEPROM_ADDRESS_MQTT_SERVER, broker);
//       //EEPROM.commit();
//       //EEPROM.writeString(EEPROM_ADDRESS_WIFI_PASS, password);
//       //EEPROM.commit();
//       cambiarConfigMQTT(1);
//     break;
//     case 'o':

//       //valorParamLength = strlen(valorParam.c_str());
//       //endIndex = valorParamLength;



//       //index = valorParam.indexOf(' ');
//       flagOTA = valorParam.toInt();//1 para activarlo; 0 para desactivarlo
//       Serial.print("flagOTA: ");
//       Serial.println(flagOTA);
// /*      
//       if(flagOTA == 1){
//         EEPROM.write(EEPROM_ADDRESS_FLAG_OTA, flagOTA);
//         EEPROM.commit();
//       }else if(flagOTA == 0){
//         EEPROM.write(EEPROM_ADDRESS_FLAG_OTA, flagOTA);
//         EEPROM.commit();
//       }
//       Serial.println("Se reiniciará el sistema");
//       delay(5000);
//       ESP.restart();
// */
//     break;
//     case 'r':
//       Serial.print("Reset...");
//       delay(3000);
//       ESP.restart();
      
//     break;

//     case 'X':
      
//       Serial.print("contadorReconexionExitosa: ");
//       Serial.println(contadorReconexionExitosa);

//       SerialBT.print("contadorReconexionExitosa: ");
//       SerialBT.println(contadorReconexionExitosa);
      
//       Serial.print("contadorReconexionFallida: ");
//       Serial.println(contadorReconexionFallida);

//       SerialBT.print("contadorReconexionFallida: ");
//       SerialBT.println(contadorReconexionFallida);
//     break;
//     case 'Y':
      
//       Serial.print("wifiSSIDEEPROM: ");
//       Serial.println(wifiSSIDEEPROM);

//       SerialBT.print("wifiSSIDEEPROM: ");
//       SerialBT.println(wifiSSIDEEPROM);
      
//       Serial.print("wifiPASSEEPROM: ");
//       Serial.println(wifiPASSEEPROM);

//       SerialBT.print("wifiPASSEEPROM: ");
//       SerialBT.println(wifiPASSEEPROM);

//       Serial.print("brokerEEPROM: ");
//       Serial.println(brokerEEPROM);

//       SerialBT.print("brokerEEPROM: ");
//       SerialBT.println(brokerEEPROM);
//     break;
//     default:
//       Serial.println("Parámetro incorrecto");
//     break;

//   }  
// }

void setupModoRed(void){

  setup_wifi();
  setup_mqtt();

  //init and get the time
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  //mqtt_server = "broker.hivemq.com";
  

}
// /*
// void probarNeopixel(void){

//   //prueba de LEDs Neopixel
//   pixels.clear();
//   delay(20);
//   apagarNeopixel();
//   delay(20);
//   encenderNeopixel('R');
//   delay(timeLED);
//   apagarNeopixel();
//   delay(timeLED);
//   encenderNeopixel('G');
//   //encenderNeopixel('B');
//   delay(timeLED);
//   apagarNeopixel();
//   delay(timeLED);
//   encenderNeopixel('Y');
//   delay(timeLED);
//   apagarNeopixel();
//   delay(timeLED);

// }
// */
// void displayText(char letra){

//   int timeDisplayBienvenida = 1000;
  
//   display.clearDisplay();
//   //Serial.println("HandTemp");
//   display.setFont(font1);
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
  
//   //display.clearDisplay();
//   if(letra == 'R'){
//     display.setCursor(20,20);
//     display.println("Recuperando");
//     display.println("conexion");
//   }else if(letra == '1'){
//     display.setCursor(10,20);
//     display.println("MODO RED");
//     display.println("conexion OK");
//   }else if(letra == '0'){
//     display.setCursor(5,20);
//     display.println("MODO LOCAL");
//     display.println("conexion no OK");
//   }
    
//   display.display();
//   delay(timeDisplayBienvenida);
//   // limpio el bufer para la proxima pantalla
//   display.clearDisplay();
//   display.setFont(mainFont);  

// }
/*
void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
  if (d == "ON"){
    digitalWrite(LED_ONBOARD, HIGH);
  }
  if (d == "OFF"){
    digitalWrite(LED_ONBOARD, LOW);
  }
}
*/
void setup_mqtt(void){

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

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
	
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Use the following commands to control your outputs.\n\n";
      welcome += "/led_on to turn GPIO ON \n";
      welcome += "/led_off to turn GPIO OFF \n";
      welcome += "/state to request current GPIO state \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/led_on") {
      bot.sendMessage(chat_id, "LED state set to ON", "");
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/led_off") {
      bot.sendMessage(chat_id, "LED state set to OFF", "");
      ledState = LOW;
      digitalWrite(ledPin, ledState);
    }
    
    if (text == "/state") {
      if (digitalRead(ledPin)){
        bot.sendMessage(chat_id, "LED is ON", "");
      }
      else{
        bot.sendMessage(chat_id, "LED is OFF", "");
      }
    }
  }
  
}