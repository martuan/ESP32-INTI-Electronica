/*  
 *
 *	Programado por: Martín Cioffi y Nestor Mariño
 *  Fecha: 18-10-2022
 *
 *	Version 1
 *  
 *  ESP32/ESP8266 example of downloading and uploading a file from or to the device's Filing System
 *  
 This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.
 
 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY 
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
 *
*/

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



#ifdef ESP8266
  #include <ESP8266WiFi.h>       // Built-in
  #include <ESP8266WiFiMulti.h>  // Built-in
  #include <ESP8266WebServer.h>  // Built-in
  #include <ESP8266mDNS.h>
#else
	#include <WiFi.h>              // Built-in
	#include <WiFiMulti.h>         // Built-in
	//#include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
	#include <ESPAsyncWebServer.h>
	#include <ESPmDNS.h>
	#include <AsyncTCP.h>
	#include "SPIFFS.h"
#endif


#include <SdFat.h>
#include <Wire.h>
#include <RTClib.h>
RTC_DS3231 rtc;
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <EEPROM.h>
#include <manejadorSD.h>
#include <tiempoCumplido.h>
#include <hardware.h>
#include <leerEEPROM.h>

#define ROOTDIR "/"


#ifdef ESP8266
  ESP8266WiFiMulti wifiMulti; 
  ESP8266WebServer server(80);
#else
  WiFiMulti wifiMulti;
  //ESP32WebServer server(80);
  AsyncWebServer server(80);
#endif

#include "Network.h"
#include "Sys_Variables.h"
#include "CSS.h"

// ********** Funciones del webserver ***************
//void File_Download();
void SD_file_download(String filename, AsyncWebServerRequest *);
void listar_SD_dir(String, String, AsyncWebServerRequest *);
void printDirectory_v5(SdFile root);
void File_Delete();
void SD_file_delete(String filename);
void SelectInput(String heading1, String command, String arg_calling_name);
String file_size(int bytes);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void ReportCouldNotCreateFile(String target);
void ReportFileNotPresent(String target);
void ReportSDNotPresent();
void SD_file_stream(String filename);
void initSPIFFS(void);
void ConsultarPorFecha();
void agregaFilaEnTabla(String nombreDeArchivo, String fechaNormalizada, String rutaDeArchivo);
String obtenerFechaDeArchivo(void);
void toggleLED10veces(void);
void armarPaqueteHtml(String);
void reconnectWifi(void);

// ********** Funciones del controlador ***************



tiempoCumplido tiempoCumplido1(100);
leerEEPROM leerEEPROM1(addressEEPROM_1kPa, addressEEPROM_2kPa);
manejadorSD tarjetaSD1(CS);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

SdFat sd;
SdFile root;
SdFile file;
SdFile entry;

WiFiClient client;




//Rutina que crea en archivo de registro de nuevo ensayo, lo nombra según formato ISO8601 y Escribe dentro de el el encabezado del ensayo
void calibracionSensoresPresion(void);
//void leerEEPROM(void);
void imprimirLCDfijo(String, int, int);
void mensajeInicialLCDcalibracion(int);
void mensajeLCDcalibracion(int, float);
void fallaEscrituraSD(void);
void leerSerie(void);
String rutinaInicioEnsayo(void);
void rutinaEnsayo(String nombreArchivo);

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html>
<head>

	<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>
	<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>
	<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.0.0/dist/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>
		
	<style>
	body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}
	ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}
	li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}
	li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}
	li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}
	section {font-size:0.88em;}
	h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}
	h2{color:orange;font-size:1.0em;}
	h3{font-size:0.8em;}
	table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;} 
	th,td {border:0.06em solid #dddddd;text-align:center;padding:0.3em;border-bottom:0.06em solid #dddddd;} 
	tr:nth-child(odd) {background-color:#eeeeee;}
	.rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}
	.rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}
	.rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}
	.column{float:left;width:50%;height:45%;}
	.row:after{content:'';display:table;clear:both;}
	*{box-sizing:border-box;}
	footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}
	button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}
	.buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}
	.buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}
	.buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}
	.buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}
	.btn {background-color: DodgerBlue;border: none;color: white;padding: 12px 30px;cursor: pointer;font-size: 20px;}
	.btn:hover {background-color: RoyalBlue;}
	a{font-size:75%;}
	p{font-size:75%;}
	</style>
</head>


<title>INTI - Caucho</title>
<body><table><tr><td><h1>ESP32 Datalogger Webserver - INTI</h1></td>

<form action='/login' method='post'>
<label for='user'><b>Usuario: </b></label><input type='text' name='user' id='user' value=''>
<label for='pass'><b>Password: </b></label><input type='password' name='pass' id='pass' value=''>
<input type='Submit' value='Aceptar'>
</form>
</body></html>
)rawliteral";

const char menu_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html>
<head>

	<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>
	<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>
	<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.0.0/dist/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>
		
	<style>
	body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}
	ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}
	li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}
	li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}
	li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}
	section {font-size:0.88em;}
	h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}
	h2{color:orange;font-size:1.0em;}
	h3{font-size:0.8em;}
	table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;} 
	th,td {border:0.06em solid #dddddd;text-align:center;padding:0.3em;border-bottom:0.06em solid #dddddd;} 
	tr:nth-child(odd) {background-color:#eeeeee;}
	.rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}
	.rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}
	.rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}
	.column{float:left;width:50%;height:45%;}
	.row:after{content:'';display:table;clear:both;}
	*{box-sizing:border-box;}
	footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}
	button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}
	.buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}
	.buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}
	.buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}
	.buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}
	.btn {background-color: DodgerBlue;border: none;color: white;padding: 12px 30px;cursor: pointer;font-size: 20px;}
	.btn:hover {background-color: RoyalBlue;}
	a{font-size:75%;}
	p{font-size:75%;}
	</style>
</head>


<title>INTI - Caucho</title>
<body><table><tr><td><h1>Menu</h1></td></tr></table>

<form action='/dir' method='post'><input type='hidden' name='directorio' id='root' value='/'><input type='Submit' value='Listado de archivos'></form>
<br>
<form action='/logout' method='post'><input type='hidden' name='logout' id='logout' value='/'><input type='Submit' value='Logout'></form>


</body></html>
)rawliteral";

const char listSD_start_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html>
<head>

	<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>
	<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>
	<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.0.0/dist/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>
		
	<style>
	body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}
	ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}
	li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}
	li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}
	li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}
	section {font-size:0.88em;}
	h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}
	h2{color:orange;font-size:1.0em;}
	h3{font-size:0.8em;}
	table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;} 
	th,td {border:0.06em solid #dddddd;text-align:center;padding:0.3em;border-bottom:0.06em solid #dddddd;} 
	tr:nth-child(odd) {background-color:#eeeeee;}
	.rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}
	.rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}
	.rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}
	.column{float:left;width:50%;height:45%;}
	.row:after{content:'';display:table;clear:both;}
	*{box-sizing:border-box;}
	footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}
	button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}
	.buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}
	.buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}
	.buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}
	.buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}
	.btn {background-color: DodgerBlue;border: none;color: white;padding: 12px 30px;cursor: pointer;font-size: 20px;}
	.btn:hover {background-color: RoyalBlue;}
	a{font-size:75%;}
	p{font-size:75%;}
	</style>
</head>


<title>INTI - Caucho</title>
<body><table><tr><td><h1>DIR</h1></td></tr></table>


<form action='/menu' method='post'><input type='hidden' name='directorio' id='root' value='menu'><input type='Submit' value='Volver al menu'></form>
<br>
<form action='/logout' method='post'><input type='hidden' name='logout' id='logout' value='/'><input type='Submit' value='Logout'></form>




)rawliteral";



const char listSD_end_html[] PROGMEM = R"rawliteral(

	</body></html>
)rawliteral";

const char html_root[] PROGMEM = R"rawliteral(
	
	<!DOCTYPE html><html>
	<head>
	<title>ESP32 Datalogger Webserver - INTI</title> // NOTE: 1em = 16px
	<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>
	<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>
	<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.0.0/dist/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>
		
	<style>
	body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}
	ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}
	li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}
	li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}
	li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}
	section {font-size:0.88em;}
	h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}
	h2{color:orange;font-size:1.0em;}
	h3{font-size:0.8em;}
	table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;} 
	th,td {border:0.06em solid #dddddd;text-align:center;padding:0.3em;border-bottom:0.06em solid #dddddd;} 
	tr:nth-child(odd) {background-color:#eeeeee;}
	.rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}
	.rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}
	.rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}
	.column{float:left;width:50%;height:45%;}
	.row:after{content:'';display:table;clear:both;}
	*{box-sizing:border-box;}
	footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}
	button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}
	.buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}
	.buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}
	.buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}
	.buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}
	.btn {background-color: DodgerBlue;border: none;color: white;padding: 12px 30px;cursor: pointer;font-size: 20px;}
	.btn:hover {background-color: RoyalBlue;}


	a{font-size:75%;}
	p{font-size:75%;}
	</style></head><body><table><tr><td><h1>ESP32 Datalogger Webserver - INTI</h1></td></tr></table></body></html>
)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}





char estadoPin = 0;
bool flagCheckArgs = 0;
String ultimoDirExplorado = {};

int pinLED = 15;

int cuentaIntentos = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup(void){
  
  	Serial.begin(115200);

	//initSPIFFS();
	pinMode(16, OUTPUT);
	pinMode(pinLED, OUTPUT);
	
	//File root;
  	int cuenta = 0;

      // Nos conectamos a nuestra red Wifi
	Serial.println();
	Serial.print("Conectando a ssid: ");
	Serial.println(ssid_1);

	
	WiFi.begin(ssid_1, password_1);

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
		
	}

	Serial.println("\nConnected to "+WiFi.SSID()+" Use IP address: "+WiFi.localIP().toString()); // Report which SSID and IP is in use
	// The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
	if (!MDNS.begin(servername)) {          // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
		Serial.println(F("Error setting up MDNS responder!")); 
		ESP.restart(); 
	} 
	#ifdef ESP32
	// Note: SD_Card readers on the ESP32 will NOT work unless there is a pull-up on MISO, either do this or wire one on (1K to 4K7)
		//Serial.println(MISO);
		pinMode(19,INPUT_PULLUP);
	#endif

	SPI.begin(SCK, MISO, MOSI, SD_CS_pin);

	Serial.print(F("Initializing SD card...")); 

	SD.begin(SD_CS_pin);//prueba
	
	if (!sd.begin(SD_CS_pin, SD_SCK_MHZ(20))) { // see if the card is present and can be initialised. Wemos SD-Card CS uses D8 
		Serial.println(F("Card failed or not present, no SD Card data logging possible..."));
		SD_present = false; 
	} 
	else
	{
		Serial.println(F("Card initialised... file access enabled..."));
		SD_present = true; 
	}

	// Note: Using the ESP32 and SD_Card readers requires a 1K to 4K7 pull-up to 3v3 on the MISO line, otherwise they do-not function.
	//----------------------------------------------------------------------   
	///////////////////////////// Server Commands 


// Send web page with input fields to client

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
	  cuentaIntentos++;
	if (logueado == 0) { //ofrece loguin
		request->send_P(200, "text/html", index_html);
	}else{ //hay otro usuario logueado
		String htmlToSend = "Servidor web ocupado...Intento Nº"+String(cuentaIntentos)+"<br><a href=\"/\">Retornar a pantalla inicio</a>";
		//String htmlToSend = "Servidor web ocupado...Intento Nº<br><a href=\"/\">Retornar a pantalla inicio</a>";
		//request->send_P(200, "text/html","Servidor web ocupado...Intento Nº"+String(cuentaIntentos)+"<br><a href=\"/\">Retornar a pantalla inicio</a>");
		request->send_P(200, "text/html", htmlToSend.c_str());
	}
  });

  server.on("/login", HTTP_POST, [] (AsyncWebServerRequest *request) {
    String inputUsuario;
    String inputPass;

	Serial.println("cantidad de parametros = ");
	Serial.println(request->params());

    inputUsuario = request->getParam(0)->value();
    inputPass = request->getParam(1)->value();

    Serial.print("Usuario: ");    
    Serial.println(inputUsuario);
    Serial.print("Clave: ");    
    Serial.println(inputPass);
    
    if(inputUsuario == userEnabled1 && inputPass == passEnabled1){
      
      logueado = 1;
      Serial.println(logueado);

	  request->redirect("/menu");

    }else{
      Serial.println(logueado);

      request->send(200, "text/html", "<p>HTTP POST enviado - credenciales no validas</p>" 
                                        "<br><a href=\"/\">Retornar a pantalla inicio</a>");
    }

  });
  
  server.on("/menu", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    
    Serial.print("Entrando al menu MENU....");    
    Serial.println(logueado);

    request->send(200, "text/html", menu_html);
  });
  
  server.on("/dir", HTTP_POST, [] (AsyncWebServerRequest *request) {
    
    Serial.print("Entrando al menu DIR....");    
    Serial.println(logueado);
    cuentaDIR += 1;
    Serial.println(cuentaDIR);

	String param1;
    String param2;

	//request->send(200, "text/html", menu_html);
	//request->hasArg()

	listar_SD_dir(param1, param2, request);

//**************** contenido de listar_SD_dir ******************
//**************************************************************


	Serial.println("cantidad de parametros = ");
	Serial.println(request->params());
	param1 = request->getParam(0)->value();
    //param2 = request->getParam(0)->value();
	Serial.println(param1);

	cuentaIntentos++;
  });

  server.on("/logout", HTTP_ANY, [] (AsyncWebServerRequest *request) {
    String inputUsuario;
    String inputPass;

    logueado = 0;
    cuentaDIR = 0;
    Serial.println(logueado);
    
    request->redirect("/");

  });

  server.on("/muestraIP", HTTP_ANY, [] (AsyncWebServerRequest *request) {

    Serial.print("Solicitud HTTP desde IP: ");    
    //Serial.println(inputIP);
    Serial.println(request->client()->remoteIP());
    Serial.println(logueado);
    //logueado = 0;
    request->send(200, "text/html", "Solicitud HTTP desde IP: "
                        + (request->client()->remoteIP()).toString() +
                        "<br><a href=\"/\">Retornar a Menu Principal</a>"
                        "<br><a href=\"/logout\">Desconectarse de la Web</a>");

  });


 
  server.on("/download", HTTP_ANY, [] (AsyncWebServerRequest *request) {

    Serial.print("Solicitud HTTP desde IP: ");    
    //Serial.println(inputIP);
    Serial.println(request->client()->remoteIP());
    Serial.println(logueado);



	if (request->hasArg("download")){
		Serial.print("argumento = ");
		Serial.println(request->getParam(0)->value());
		SD_file_download(request->getParam(0)->value(), request);

	} 
	//request->send(SPIFFS, "/file.html", "text/html", true);

/*
    //logueado = 0;
    request->send(200, "text/html", "Solicitud HTTP desde IP: "
                        + (request->client()->remoteIP()).toString() +
                        "<br><a href=\"/\">Retornar a Menu Principal</a>"
                       "<br><a href=\"/logout\">Desconectarse de la Web</a>");
*/
  });
  
 
  server.onNotFound(notFound);
  server.begin();

  ///////////////////////////// End of Request commands

  	Serial.println("HTTP server started");


	// //******************** NESTOR ********************

	// pinMode(LED_BUILTIN, OUTPUT);
	// pinMode(pulsadorInicio, INPUT);
	// pinMode(activarElectrovalvula, OUTPUT);
	// pinMode(pulsadorCalibracion, INPUT);

	// boolean calibracion;
	// boolean memoriaSDinicializada = false;
	// //********************************************

	// // initializar the LCD
	// lcd.begin(20,4);
	// // Turn on the blacklight and print a message.
	// lcd.backlight();
	// lcd.print("INTI CAUCHOS");
	// memoriaSDinicializada = tarjetaSD1.inicializarSD();
	// //  if (!SD.begin(CS)) {
	// if(!memoriaSDinicializada){
	// 	Serial.println("inicialización fallida!"); 
	// 	lcd.setCursor(0, 0);
	// 	lcd.print("SD NO encontrada");
	// //while(!SD.begin(CS)){
	// while(!memoriaSDinicializada){
	// 	digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
	// 	lcd.backlight();
	// 	delay(500);                       // wait for a half second
	// 	digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
	// 	lcd.noBacklight();
	// 	delay(500);                       // wait for a half second
	// 	memoriaSDinicializada = tarjetaSD1.inicializarSD();
	// }
	
	// lcd.backlight();
	// lcd.clear();

	// } 
	// // **********************************************************************
	// if (! rtc.begin()) {
	// 	//   Serial.println("Couldn't find RTC");
	// 	while (1);
	// }

	// if (rtc.lostPower()) {
	// 	//    Serial.println("RTC lost power, lets set the time!");
	// }
	// //**********************************************************************
	// dht.begin();    //Inicializar sensor de temperatura
	// EEPROM.begin(EEPROM_SIZE);
	
	// imprimirLCDfijo("Pulse INICIO",0, 0);
	// imprimirLCDfijo("para comenzar",0, 1);



}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop(void){



	if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi desconectado..");
	  reconnectWifi();
    }

	
	/*
	boolean estadoPulsadorInicio;

	estadoPulsadorInicio = digitalRead(21);

	if(estadoPulsadorInicio == LOW && flagWebserverLibre){//chequea que no se esté accediendo a la memoria SD desde el webserver
	
		flagWebserverLibre = 0;//se ocupa el webserver
		Serial.println("NO se puede acceder a la memoria SD desde el webserver");
		toggleLED10veces();
		flagWebserverLibre = 1;//se libera el webserver
		Serial.println("SI se puede acceder a la memoria SD desde el webserver");
	
	}
*/

}

// All supporting functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
void File_Download(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments

//	flagWebserverLibre = 0;//se ocupa el webserver

	if (server.args() > 0 ) { // Arguments were received
		if (server.hasArg("download")){
			SD_file_download(server.arg(0));
			Serial.print("argumento = ");
			Serial.println(server.arg(0));
		} 
	}
	else SelectInput("Enter filename to download","download","download");

//  	flagWebserverLibre = 1;//se libera el webserver

}
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_download(String filename, AsyncWebServerRequest *request){

  if (SD_present) { 

	File download2 = SD.open("/"+filename);
	
	SdFile download;

	download.open(filename.c_str());

	if (download) {
		Serial.print("Existe el archivo = ");
		Serial.print(filename);
		Serial.print(" ");

		//Serial.println(String(download.name()));
		Serial.println("Se descarga el archivo");
		Serial.println();
		
		request->send(download2, filename.c_str(), "text/html", true);
		/*
		server.sendHeader("Content-Type", "text/text");
		server.sendHeader("Content-Disposition", "attachment; filename="+filename);
		server.sendHeader("Connection", "close");
		server.streamFile(download2, "application/octet-stream");
		*/
		download.close();
		download2.close();
    }else ReportFileNotPresent("download"); 
  }else ReportSDNotPresent();

}


void listar_SD_dir(String param1, String param2, AsyncWebServerRequest *request){


	Serial.println("cantidad de parametros = ");
	Serial.println(request->params());
	param1 = request->getParam(0)->value();

	Serial.println(param1);


	if(logueado){
		Serial.println("usuario logueado");
		 if (SD_present) {
			 

    
			if(param1 == ROOTDIR){

				root.open(ROOTDIR);

			}else{
				
				String directorio = param1;
				

				if (request->hasArg("directorio")){//si se pide avanzar adelante (un directorio nuevo)
					
					pathDirectory = pathDirectory + directorio + "/" ;
					Serial.print("pathDirectory = ");
					Serial.println(pathDirectory);
					
				}else if(request->hasArg("directorioAtras")){//si se pide avanzar atrás (el directorio padre)

					if(pathDirectory != ROOTDIR){
						
						
						pathDirectory = pathDirectory.substring(0, pathDirectory.lastIndexOf('/'));//recorta
						pathDirectory = pathDirectory.substring(0, pathDirectory.lastIndexOf('/')+1);//recorta
						Serial.print("pathDirectory (atras) = ");
						Serial.println(pathDirectory);

					}
					

				}

				root.open(pathDirectory.c_str());

			}


			if (root) {
				root.rewindDirectory();

				webpage += F("<h3 class='rcorners_m'>Contenido de la memoria SD</h3><br>");
				webpage += F("<form action='/consultarPorFecha'><label for='Fecha'>Fecha: </label><input type='date' id='fechaEnsayo' name='fechaEnsayo'><input type='submit'></form><br>");

				webpage += "<h3 align = 'left'>Directorio actual = "+ pathDirectory + "</h3>";

				webpage += F("<table class='table table-striped'>");
				webpage += F("<tr><th scope='col'>Nombre</th><th scope='col'>Fecha</th><th scope='col'>Archivo/Directorio</th><th scope='col'>Tama&ntildeo</th><th scope='col'>Eliminar</th><th scope='col'>Descargar</th></tr>");
				webpage += "<tr><td><form action='/dir' method='post'><input type='hidden' name='directorioAtras' id='directorioAtras' value='../'><input type='Submit' value='../'></form></td><td></td><td></td><td></td><td align='center'></td><td></td></tr>";

				printDirectory_v5(root);

				webpage += F("</table>");
				fechaEnsayoConsultada = "";
				
				String prueba = webpage;
				
				armarPaqueteHtml(prueba);

				request->send(200, "text/html", paquete_html.c_str());

				root.close();
			}
			else{
				webpage += F("<h3>No Files Found</h3>");
			}

		} else ReportSDNotPresent();

	}
}

void printDirectory_v5(SdFile path){

	char entryName[100] = {};
	String nombreDeArchivo = {};
	String rutaDeArchivo = {};
	bool hayMasArchivos = 0;
	String dirNuevo = {};
	Serial.println("----------------Entra al printDirectory_v5-----------------------");
	Serial.print("pathDirectory = ");
	Serial.println(pathDirectory);
	String fechaNormalizada = {};

	while(entry.openNext(&path, O_RDONLY)){

		/*
		if (webpage.length() > 1000) {
			SendHTML_Content();
		}
*/
		entry.getName(entryName, 100);//obtiene el nombre de la entrada, ya sea un directorio o un archivo
		
		if(entry.isDirectory()){//Si es un directorio
			
			rutaDeArchivo = String(entryName);
			
			webpage += "<tr><td><form action='/dir' method='post'><input type='hidden' name='directorio' id='directorio' value="+ String(rutaDeArchivo) +"><input type='hidden' name='cuenta' id='cuenta' value="+ String(cuenta) +"><input type='Submit' value="+String(rutaDeArchivo)+"></form></td><td></td><td>"+String(entry.isDirectory()?"Dir":"File")+"</td><td></td><td align='center'></td><td></td></tr>";


			
		}
		else//Si es un archivo
		{
			
			nombreDeArchivo = String(entryName);
			//rutaDeArchivo = rutaDeArchivo + "/" + nombreDeArchivo;
			rutaDeArchivo = pathDirectory + nombreDeArchivo;
			filename = nombreDeArchivo;
			Serial.println(filename);

			if(filename.endsWith(".csv")){
				Serial.println("termina con .csv");
			}

			
			if(filename.charAt(8) == 'T'){//si el archivo es una fecha de ensayo
				Serial.print("CharAt(8) = T		");
				fechaNormalizada = obtenerFechaDeArchivo();
				Serial.print("fechaNormalizada =");
				Serial.println(fechaNormalizada);

			}else{
				fechaNormalizada = "";
			}
					
			agregaFilaEnTabla(rutaDeArchivo, fechaNormalizada, rutaDeArchivo);
						
					
		}
		entry.close();
	}
}
/*
void File_Delete(){

	flagWebserverLibre = 0;//se ocupa el webserver

	if (server.args() > 0 ) { // Arguments were received
		if (server.hasArg("delete")) SD_file_delete(server.arg(0));
	}
	else SelectInput("Select a File to Delete","delete","delete");

	flagWebserverLibre = 1;//se libera el webserver
}
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
void SD_file_delete(String filename) { // Delete the file 
  if (SD_present) { 
    SendHTML_Header();
    File dataFile = SD.open("/"+filename, FILE_READ); // Now read data from SD Card 
    Serial.print("Deleting file: "); Serial.println(filename);
    if (dataFile)
    {
      if (SD.remove("/"+filename)) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>File '"+filename+"' has been erased</h3>"; 
        webpage += F("<a href='/dir'>[Back]</a><br><br>");
      }
      else
      { 
        webpage += F("<h3>File was not deleted - error</h3>");
        webpage += F("<a href='/dir'>[Back]</a><br><br>");
      }
    } else ReportFileNotPresent("delete");
    append_page_footer(); 
    SendHTML_Content();
    SendHTML_Stop();
  } else ReportSDNotPresent();
} 
*/


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void SelectInput(String heading1, String command, String arg_calling_name){
  //SendHTML_Header();
  webpage += F("<h3>"); webpage += heading1 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  //append_page_footer();
  //SendHTML_Content();
  //SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent(){
  //SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  //append_page_footer();
  //SendHTML_Content();
  //SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  //SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  //append_page_footer();
  //SendHTML_Content();
  //SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target){
  //SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  //append_page_footer();
  //SendHTML_Content();
  //SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String file_size(int bytes){
  String fsize = "";
  if (bytes < 1024)                 fsize = String(bytes)+" B";
  else if(bytes < (1024*1024))      fsize = String(bytes/1024.0,3)+" KB";
  else if(bytes < (1024*1024*1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
  else                              fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
  return fsize;
}


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
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
    } else {
      Serial.print("  Archivo: ");
      Serial.print(file.name());
      Serial.print("  Tamaño: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

// Initialize SPIFFS
void initSPIFFS() {
	if (!SPIFFS.begin()) {
		Serial.println("An error has occurred while mounting SPIFFS");
	}
		Serial.println("SPIFFS mounted successfully");
}

void ConsultarPorFecha(void){
/*
	fechaEnsayoConsultada = "";//borra la anterior consulta
	pathDirectory = "";//resetea la ruta

	SendHTML_Header();
	
	webpage += F("<h3>Consultar Por Fecha (en desarrollo)</h3>");

	if (server.args() > 0 ) { // Arguments were received
    	if (server.hasArg("fechaEnsayo")){


			fechaEnsayoConsultada = server.arg(0);
			webpage += F("<p>No se encontraron archivos</p>");
			webpage += "<p>fechaEnsayo = " + fechaEnsayoConsultada + "</p>";

			Serial.print("-----------------------> Fecha consultada = ");
			Serial.print(fechaEnsayoConsultada);
			Serial.println(" <----------------------");

		} 
  	}

	webpage += F("<a href='/'>[Back]</a><br><br>");
	//muestra listado de archivos (pero solo los que coincidan con la fecha consultada)
	webpage += F("<form action='/dir'><input type='submit' value='Mostrar tabla filtrada'></form><br>");
	append_page_footer();
	SendHTML_Content();
	SendHTML_Stop();
*/
}


// //******************************************NESTOR****************************

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
     
      mensajeInicialLCDcalibracion(numeroSensor);
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
        imprimirLCDfijo(String(numeroSensor),5, 1);    
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
          mensajeLCDcalibracion((muestraPorPunto + 1), valorCalibracion[puntoCalibracion]);
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
       imprimirLCDfijo(" Sensor 1          ",0, 2); 
       
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
        mensajeLCDcalibracion(1, 1);

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
        mensajeLCDcalibracion(1, 2);
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
       imprimirLCDfijo(" Sensor 2          ",0, 2);
     }
     imprimirLCDfijo("                    ",0, 1);
     imprimirLCDfijo(" Fin de Calibracion",0, 0);
     imprimirLCDfijo("Reinicie el equipo  ",0, 3);

     //leerEEPROM();  
     leerEEPROM1.obtenerValores();
}
//******************** Fin calibración ************************************************************
void imprimirLCD(String lineaMedicionLCD, int fila){    //Para facilitar la lectura durante el ensayo se actualiza la última medición en la última fila y la  
  static String lineaMedicionLCDanterior = "";          //medición anterior se corre hacia arriba
//    lcd.clear();
    lcd.setCursor(0, fila);
    lcd.print("                   ");
    lcd.setCursor(0, fila);
    lcd.print(lineaMedicionLCDanterior);
    lcd.setCursor(0, (fila + 1));
    lcd.print("                   ");
    lcd.setCursor(0, (fila + 1));
    lcd.print(lineaMedicionLCD);    
    lineaMedicionLCDanterior = lineaMedicionLCD;
  
}
//***************************************************************************************
void mensajeInicialLCDcalibracion(int numeroSensor){
    imprimirLCDfijo("Con pulsador",0, 0);
    imprimirLCDfijo("CALIBRACION",0, 1);
    imprimirLCDfijo("seleccione sensor",0, 2);
    delay(5000);
    lcd.clear();
    imprimirLCDfijo("Sensor seleccionado:",0, 0);
    imprimirLCDfijo(String(numeroSensor),0, 1);              
    imprimirLCDfijo("Presionar pulsador",0, 2);         
    imprimirLCDfijo("INICIO para calibrar",0, 3);         
}
//**************************************************************************************************

void mensajeLCDcalibracion(int muestraPorPunto, float puntoCalibracion){
      String linea_1 = String(muestraPorPunto);
      String linea_2 = "hasta: ";
      linea_1 += " Suba desde cero";
      linea_2 += String(puntoCalibracion);
      linea_2 += " kPa";
      imprimirLCDfijo(linea_1,0, 0);         
      imprimirLCDfijo(linea_2,0, 1);         
      imprimirLCDfijo("y presione",0, 2);         
      imprimirLCDfijo("INICIO",0, 3);         

}
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
//*******************************************************************************
void imprimirLCDfijo(String lineaMedicionLCD, int columna, int fila){

    lcd.setCursor(0, fila);
    lcd.print("                    ");
    lcd.setCursor(columna, fila);
    lcd.print(lineaMedicionLCD);    
  
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
  imprimirLCDfijo("                    ",0, 0);
  imprimirLCDfijo(" Falla Escritura SD ",0, 1);
  imprimirLCDfijo("                    ",0, 2);
  imprimirLCDfijo("                    ",0, 3);
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

  valorAD_minimo_fuelle = EEPROM.readInt(addressEEPROM_1kPa); //Valor de presión mínimo (en entero del AD) en el fuelle (sensor de presión 2)   
  valorAD_maximo_fuelle = EEPROM.readInt(addressEEPROM_2kPa); //Valor de presión máximo (en entero del AD) en el fuelle (sensor de presión 2) 

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
        imprimirLCD(String(lineaMedicionLCD), 2);
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
      imprimirLCD(String(lineaMedicionLCD), 2);
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
      imprimirLCD("Fin por Usuario", 2);
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
      imprimirLCD("Baja Presion Fuelle",2);
      delay(300);
    }
    if(altaPresionFuelle == HIGH){
      lineaMedicion += "Fin de ensayo por alta presión en fuelle \r\n"; 
      lineaMedicionNombreArchivo += "Fin de ensayo por alta presión en fuelle \r\n"; 
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());               //Escribe última medición antes de reventado en archivo de este ensayo  
      Serial.println("Fin de ensayo por alta presión en fuelle");
      datoAnexadoEnSD = tarjetaSD1.appendFile(SD, nombreMaquina.c_str(), lineaMedicionNombreArchivo.c_str());  //Escribe última medición antes de reventado en archivo Maquina_#
      segundos = timeOut + 1;       //Para que salga del While
      imprimirLCD("Alta Presion Fuelle",2);
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
    imprimirLCDfijo(String(tempHumedadLCD),3, 0);
    imprimirLCDfijo(String(dateTimeISO),2, 1);
        
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
}

void agregaFilaEnTabla(String nombreDeArchivo, String fechaNormalizada, String rutaDeArchivo){

	webpage += "<tr><td>"+ String(nombreDeArchivo) +"</td><td>"+fechaNormalizada+"</td>";
	webpage += "<td>"+String(entry.isDirectory()?"Dir":"File")+"</td>";
	int bytes = entry.size();
	String fsize = "";
	
	if (bytes < 1024)                     fsize = String(bytes)+" B";
	else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
	else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
	else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";

	webpage += "<td>"+fsize+"</td><td><form action='/delete' method='post'><input type='text' hidden name='lname' value='"+String(rutaDeArchivo)+"'><input type='submit' name='delete' value='Borrar'></form></td>";
	Serial.print("		Archivo a descargar = ");
	Serial.print(rutaDeArchivo);
	webpage += "<td><form action='/download' method='post'><input type='text' hidden name='lname' value='"+String(rutaDeArchivo)+"'><input type='submit' name='download' value='Descargar'></form></td>";
	Serial.print("\t");
	Serial.println(String(fsize));

}



String obtenerFechaDeArchivo(void){


	int indexBeginDate = 0;
	int indexEndDate = 0;

	int indexBeginYear = 0;
	int indexEndYear = 0;

	int indexBeginMonth = 0;
	int indexEndMonth = 0;

	int indexBeginDay = 0;
	int indexEndDay = 0;

	String date = {};
	String year = {};
	String month = {};
	String day = {};
	String fecha = {};


	//********FECHA******
	indexBeginDate = 0;
	indexEndDate =	filename.indexOf('T');

	date = filename.substring(indexBeginDate,indexEndDate);//obtiene la fecha del ensayo
	//Serial.println("Fecha: ");
	//Serial.println(date);

	//********AÑO******
	indexBeginYear = 0;
	indexEndYear =	3;

	year = filename.substring(indexBeginYear,indexEndYear + 1);//obtiene el año del ensayo
	//Serial.println("Año: ");
	//Serial.println(year);

	//********MES******
	indexBeginMonth = 4;
	indexEndMonth =	5;

	month = filename.substring(indexBeginMonth,indexEndMonth + 1);//obtiene el mes del ensayo
	//Serial.println("Mes: ");
	//Serial.println(month);

	//********DIA******
	indexBeginDay = 6;
	indexEndDay =	7;

	day = filename.substring(indexBeginDay,indexEndDay + 1);//obtiene el día del ensayo
	//Serial.println("Día: ");
	//Serial.println(day);

	fecha = year + "-" + month + "-" + day;
	
	return fecha;

}

void toggleLED10veces(void){

	

	for(int i = 0; i < 10; i++){
	
		Serial.println("pin Alto");
		digitalWrite(pinLED, HIGH);
		delay(1000);
		Serial.println("pin Bajo");
		digitalWrite(pinLED, LOW);
		delay(1000);
	
	}
	

}

void armarPaqueteHtml(String strRecibido){


	//paquete_html = "<html><body>hola "
	//paquete_html = "hola" +  strRecibido + "</body>\
	//</html>";
	//paquete_html = "ass\
	//<!DOCTYPE html><html><body>\
	//pepe" + strRecibido + "</body></html>";

/*
	paquete_html = header_html + 
	"<table><tr><td><h1>ESP32 Datalogger Webserver - INTI</h1></td><td>" + strRecibido + "</td></tr></table>" + 
	


	"<h3 class='rcorners_m'>Contenido de la memoria SD</h3><br>"
	"<form action='/consultarPorFecha'><label for='Fecha'>Fecha: </label><input type='date' id='fechaEnsayo' name='fechaEnsayo'><input type='submit'></form><br>"
	"<h3 align = 'left'>Directorio actual = "+ pathDirectory + "</h3>"
	"<table class='table table-striped'>"
	"<tr><th scope='col'>Nombre</th><th scope='col'>Fecha</th><th scope='col'>Archivo/Directorio</th><th scope='col'>Tama&ntildeo</th><th scope='col'>Eliminar</th><th scope='col'>Descargar</th></tr>"
	"<tr><td><form action='/dir' method='post'><input type='hidden' name='directorioAtras' id='directorioAtras' value='../'><input type='Submit' value='../'></form></td><td></td><td></td><td></td><td align='center'></td><td></td></tr></table>" +
	ending_html;
*/

	paquete_html = header_html + 

	strRecibido +
	
	ending_html;

	webpage = "";//limpia el string


}

void reconnectWifi(void){
	cuenta = 0;

	Serial.println("Intentando reconectar");

	WiFi.begin(ssid_1, password_1);

	while ((WiFi.status() != WL_CONNECTED) && cuenta < 20) {//límite de 20 intentos de 500 ms
		delay(500);
		Serial.print(".");
		cuenta++;
	
	}
	if(WiFi.status() != WL_CONNECTED){//si no logró conectarse
	
		Serial.println("No es posible conectar a WiFi");
		//Serial.println("Se cambia a MODO LOCAL");


	}else{//si logró conectarse


		Serial.println("");
		Serial.println("Reconexion exitosa Nº ");
		cuentaReconexion++;
		Serial.println(cuentaReconexion);
		Serial.println("Conectado a red WiFi!");
		Serial.println("Dirección IP: ");
		Serial.println(WiFi.localIP());
	
	}
}