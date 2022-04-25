/*
 ESP32-INTI-Electrónica
  Versión 1.0
  Fecha   25/04/2022
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
  *  Permite interactuar desde un bot de Telegram
  *  Permite enviar y recibir mensajes desde un browser con WebSerial
*/
/*
2/01/22 https://www.teachmemicro.com/nodemcu-iot-weather-device/





 * HADWARE: 
 * Pin de Chip Select para escribir el SD : Pin 5
 * Pulsador inicio ensayo: Pin 4
 * Pulsador calibracion: Pin 13
 * Salida para activar electroválvula: Pin 25
 * Sensor de temperatura one wire: Pin 32
 * I2C :Pines SDA:21 y CLK:22
 * Sensor de presión 1 (entrada analógica): Pin 34
 * Sensor de presión 2 (entrada analógica): Pin 35
 18/03/22 -> Se genera un archivo cada vez que se presiona el pulsador se genera un archivo .csv para almacenar los datos cuyo nombre 
 se crea en formato ISO8601 hora local (yyyymmddThhmmss), tomando fecha y hora del RTC
*/



//#include <toggleLED.h>
#include <comunicacion.h>
#include <libNestor.h>
#include <EEPROM.h>
//#include <DHT.h>
#include <esp_task_wdt.h>


#define DHTPIN 33    // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

#define WDT_TIMEOUT 60

//SensorDHT sensDHT;


DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;







Network conexion;
Sensores sensorTemp;  // Create an object
Parametros parametros;
//Sensores sensor1;

hw_timer_t * timer3;
int tiempo1 = 60;//60 segundos
char flagProceso1 = 0;
char flagProceso2 = 0;
char flagProceso3 = 0;
int contadorProceso1 = 0;
int contadorProceso2 = 0;
int contadorProceso3 = 0;
String macAdd = "";

//timer de usos generales 1000ms
void IRAM_ATTR onTimer3() {

    contadorProceso1++;
    contadorProceso2++;
	  contadorProceso3++;

  if(contadorProceso1 == tiempo1/60){//proceso 1
      contadorProceso1 = 0;//resetea el contador
      flagProceso1 = 1;
  }
	if(contadorProceso2 == tiempo1){//proceso 2
        contadorProceso2 = 0;//resetea el contador
        flagProceso2 = 1;
    }
    if(contadorProceso3 == tiempo1 * 2){//proceso 3 (cada 2 min)
        contadorProceso3 = 0;//resetea el contador
        flagProceso3 = 1;//para chequear conexión con red wifi y broker
    }

    
}





//void toggleLED(void);

void setup() {
    
 // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  conexion.setup_bluetooth();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DHTPIN, INPUT); 
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(pulsadorInicio, INPUT);
  pinMode(activarElectrovalvula, OUTPUT);
  pinMode(pulsadorCalibracion, INPUT);

  boolean calibracion;
  

  macAdd = WiFi.macAddress();
  Serial.println( "MAC address: " + macAdd );
  conexion.serialBTprintln("MAC address: " + macAdd);

  //Configuración de timer
  timer3 = timerBegin(2, 80, true);
  timerAttachInterrupt(timer3, &onTimer3, true);
  timerAlarmWrite(timer3, 1000000, true);//valor en microsegundos [1 s]
  timerAlarmEnable(timer3);

  //Configuración de Watchdog
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch
	
	//sensDHT.inicializar();
  

  // Access attributes and set values
  sensorTemp.value = 25.0;
  //sensorTemp.SSID = "infiniem";
  //sensorTemp.SSID = "milton";    
  sensorTemp.SSID = "wifi01-ei";//   Wwifi01-ei Ax32MnF1975-ReB
  //sensorTemp.Password = "12345678";
  //sensorTemp.Password = "paternal";
  sensorTemp.Password = "Ax32MnF1975-ReB";
  sensorTemp.descriptor = "Mide temperatura, humedad y presion";
  sensorTemp.url_broker = "rbpi-electronica01-wifi.inti.gob.ar";//"broker.hivemq.com";
  sensorTemp.id = "sensorTemp_0001";
  sensorTemp.topic = "prueba/user1/dataloggerINTI42";// "labo_inteligente/temperatura/"+ sensorTemp.id;  

  conexion.ssid = sensorTemp.SSID;
  conexion.password = sensorTemp.Password;

  //parametros.ssid = "wifi01-ei";
  //parametros.password = "Ax32MnF1975-ReB";
  
 
  // Print attribute values
  Serial.println(sensorTemp.value);
  Serial.println(sensorTemp.SSID);
  Serial.println(sensorTemp.Password);

  conexion.setup_wifi(sensorTemp.SSID, sensorTemp.Password);
  conexion.setup_mqtt(sensorTemp.url_broker, sensorTemp.topic);

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

  //rtc.adjust(DateTime(__DATE__, __TIME__)); // which will set the time according to our PC time.

  if (rtc.lostPower()) {
    //    Serial.println("RTC lost power, lets set the time!");
  }

	dht.begin();    //Inicializar sensor de temperatura

	EEPROM.begin(EEPROM_SIZE);
	leerEEPROM();
	
	String str = "/Maquina_";              //El nombre del archivo corresponde al número de máquina
	str += String(NUMERO_MAQUINA);
	File archivo = SD.open(str.c_str());   //Si no existe el archivo lo crea. Aqui se guardan el último dato de cada ensayo ya sea, tiempo y presión de reventado, 
	if(!archivo) {                              //fin por timeout, presión de fuelle fuera de rango o parada por usuario (pulsador inicio)
		writeFile(SD, str.c_str(), "Nombre archivo, Segundos, Estado final\r");
	}    

   
}

void loop() {
  
  bool resultadoPub = 0;
  boolean tiempoCumplido;
  boolean calibracion;
  String inicio = "";           //Cuando se inicia la placa Impacto envía al inicio de la comunicación "ini")
  static unsigned long previousMillis = 0;
  static unsigned long currentMillis = 0;
  String nombreArchivo = "";
  boolean estadoPulsadorInicio;
  //int cantFallasMQTT = 0;

  conexion.loop();
  
  //alimentar watchDog
  esp_task_wdt_reset();

  conexion.cambioDeParametros();

  
  delay(3000);
  sensorTemp.value = dht.readTemperature();
  Serial.println(dht.readTemperature());

  resultadoPub = conexion.publicarData(sensorTemp.value);
  if(resultadoPub == 0){//si falla la publicación
	conexion.flagConexionOK = 0;
  }
  
  Serial.print("Resultado de la publicación: ");
  Serial.println(resultadoPub);
  Serial.print("Cantidad de fallas de publicación MQTT: ");
  Serial.println(conexion.cantDeFallasMQTT);
  

	if(conexion.flagConexionOK == 0){
		
		Serial.println("Intentando recuperar la conexión");
		//si alguna conexión se perdió, la reestablece
		conexion.comprobarConexion(conexion.ssid,conexion.password,sensorTemp.url_broker,sensorTemp.topic);
		
		if(conexion.flagConexionOK){//si la recuperó
			Serial.print("Se ha recuperado la conexión. flagConexionOK = ");
			Serial.println(conexion.flagConexionOK);
			conexion.publicarData(sensorTemp.value);// y publica
			Serial.println("publica ");
  			conexion.serialBTprintln("publica");
			//contadorReconexionExitosa++;
		}else{//si no la recuperó
		
			Serial.print("[PROBLEMAS] No se ha recuperado la conexión. flagConexionOK = ");
			Serial.println(conexion.flagConexionOK);
			//contadorReconexionFallida++;
		}


	}

//********************NESTOR************************************


 
  while (Serial.available() > 0) {    // Es para lectura del puerto serie
 
  }
  calibracion = digitalRead(pulsadorCalibracion);
  if(!calibracion)  calibracionSensoresPresion();
        
   estadoPulsadorInicio = digitalRead(pulsadorInicio);
   if(estadoPulsadorInicio == LOW){
	nombreArchivo = rutinaInicioEnsayo();
	rutinaEnsayo(nombreArchivo);
   }


}


