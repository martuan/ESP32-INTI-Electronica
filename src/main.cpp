//#include <toggleLED.h>
#include <comunicacion.h>
#include <EEPROM.h>
#include <DHT.h>
#include <esp_task_wdt.h>

#define DHTPIN 33    // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);// Initialize DHT sensor.
#define WDT_TIMEOUT 60



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
  // put your main code here, to run repeatedly:
  //delay(10000);
  Serial.begin(115200);
  conexion.setup_bluetooth();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DHTPIN, INPUT);
  dht.begin();

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
  sensorTemp.topic = " prueba/user1/dataloggerINTI42";// "labo_inteligente/temperatura/"+ sensorTemp.id;  

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
    
}

void loop() {
  // put your main code here, to run repeatedly:

  bool resultadoPub = 0;
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

}