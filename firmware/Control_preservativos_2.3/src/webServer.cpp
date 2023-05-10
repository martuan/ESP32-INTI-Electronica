/*  
 *
 *	Programado por: Martín Cioffi, Martín Luna y Nestor Mariño
 *  Fecha: 07-11-2022
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
//*****************************************************************************************************

/*

#include <WiFi.h>              // Built-in
#include <WiFiMulti.h>         // Built-in
//#include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
//	#include "SPIFFS.h"
#include "esp_task_wdt.h"

#include <SdFat.h>

#include <SPI.h>

#include <manejadorSD.h>

#define ROOTDIR "/"
#define tiempotimeOut 300 //Tiempo para timeout del webserver (en segundos). Versión final sería de 300 segundos.
//definicion timer

#define BAUDRATE 115200

WiFiMulti wifiMulti;
//ESP32WebServer server(80);
AsyncWebServer server(80);

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
//void initSPIFFS(void);
void ConsultarPorFecha();
void agregaFilaEnTabla(String nombreDeArchivo, String fechaNormalizada, String rutaDeArchivo);
String obtenerFechaDeArchivo(void);
void toggleLED10veces(void);
void armarPaqueteHtml(String);
void reconnectWifi(void);
void IRAM_ATTR onTimer0();
void notFound(AsyncWebServerRequest *request);
void downloadAllFromDirectory(SdFile path, AsyncWebServerRequest *);

// ********** Funciones del controlador ***************

manejadorSD tarjetaSD1(CS);

SdFat sd;
SdFile root;
SdFile file;
SdFile entry;

WiFiClient client;

//Rutina que crea en archivo de registro de nuevo ensayo, lo nombra según formato ISO8601 y Escribe dentro de el el encabezado del ensayo

void fallaEscrituraSD(void);

ArRequestHandlerFunction handlerRoot(void);

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html>
<head>
	<meta charset='utf-8'>
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
	h2{color:blue;font-size:1.0em;}
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
<body><table><tr><td><h1>ESP32 Datalogger Webserver - INTI</h1></td></tr>

<tr><td>
<form action='/login' method='post' accept-charset='utf-8'>
<label for='user'><b>Usuario: </b></label><input type='text' name='user' id='user' value=''>
<label for='pass'><b>Password: </b></label><input type='password' name='pass' id='pass' value=''>
<input type='Submit' value='Aceptar'>
</form>
</td></tr></table>
</body></html>
)rawliteral";

const char menu_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html>
<head>
	<meta charset='utf-8'>
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
	h2{color:blue;font-size:1.0em;}
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
<body><table><tr><td colspan='2'><h1>Menu</h1></td></tr>
<tr>
<td>
<form action='/dir' method='post' accept-charset='utf-8'><input type='hidden' name='directorio' id='root' value='/'><input type='Submit' value='Listado de archivos'></form>
</td>
<td>
<form action='/logout' method='post' accept-charset='utf-8'><input type='hidden' name='logout' id='logout' value='/'><input type='Submit' value='Logout'></form>
</td>
</tr>
</table>

</body></html>
)rawliteral";

const char listSD_start_html[] PROGMEM = R"rawliteral(

<!DOCTYPE HTML><html>
<head>
	<meta charset='utf-8'>
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
	h2{color:blue;font-size:1.0em;}
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


<form action='/menu' method='post' accept-charset='utf-8'><input type='hidden' name='directorio' id='root' value='menu'><input type='Submit' value='Volver al menu'></form>
<br>
<form action='/logout' method='post' accept-charset='utf-8'><input type='hidden' name='logout' id='logout' value='/'><input type='Submit' value='Logout'></form>




)rawliteral";



const char listSD_end_html[] PROGMEM = R"rawliteral(

	</body></html>
)rawliteral";

const char html_root[] PROGMEM = R"rawliteral(
	
	<!DOCTYPE html><html>
	<head>
	<meta charset='utf-8'>
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
	h2{color:blue;font-size:1.0em;}
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







char estadoPin = 0;
bool flagCheckArgs = 0;
String ultimoDirExplorado = {};
int pinLED = 15;
int cuentaIntentos = 0;
int segundos;
int segundos_aux; //esta variable es temporaria, sólo para control t, no queda en versión final

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup(void){
  
  	Serial.begin(BAUDRATE);

	//initSPIFFS();
	pinMode(16, OUTPUT);
	pinMode(pinLED, OUTPUT);

	//de timer
	timeOutweb = false;
	segundos = 0;
	segundos_aux = 0;  //esta variable es solo para control temporario, no queda en versión final
	pinMode(LED_PORT,OUTPUT); //Configura puerto del LED. No hace falta en versión final, es sólo de control temporario

	timer0 = timerBegin(TIMER_0, PRESCALER_T0, true); // timer utilizado; Prescaler; cuenta ascendente= TRUE.
	timerAttachInterrupt(timer0, &onTimer0, true); // Asigna la rutina de atención de ingterrupción
													// al timer0 . Activa por flanco.
	timerAlarmWrite(timer0, CUENTA_T0, true); //Carga la cuenta del timer, Autorrecarga.
	timerAlarmEnable(timer0); //Habilita la interrupción del timer.
	//fin de timer

	
	//File root;
  	int cuenta = 0;

	// Nos conectamos a nuestra red Wifi
	Serial.println();
	Serial.print("Conectando a ssid: ");
	Serial.println(ssid_1);

	WiFi.mode(WIFI_STA);
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

	IPentrante = IPAddress(0,0,0,0);
  	IPlogueada = IPAddress(0,0,0,0);

	Serial.println("\nConnected to "+WiFi.SSID()+" Use IP address: "+WiFi.localIP().toString()); // Report which SSID and IP is in use
	// The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
	if (!MDNS.begin(servername)) {          // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
		Serial.println(F("Error setting up MDNS responder!")); 
		ESP.restart(); 
	} 
	#ifdef ESP32

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


	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		if (!logueado) { //si no hay usuario logueado envía página de login
			request->send_P(200, "text/html", index_html);
		}else{ //hay otro usuario logueado
		
			if ((request->client()->remoteIP()) == IPlogueada) { //ip entrante ip logueada son iguales
				if (timeOutweb) { //hubo timeout de web
					request->redirect("/logout");  //redirecciona a "/logout"
				}
				else {
					request->redirect("/menu");  //redirecciona a "/menu"
				}
			}
			else {
				request->send_P(200, "text/html","Servidor web ocupado...<br><a href=\"/\">Retornar a pantalla inicio</a>"); //servidor atendiendo otra IP
			}
		}
  });

    server.on("/login", HTTP_GET, [] (AsyncWebServerRequest *request) {  //manejo de petición a "/login" del tipo GET
    	request->redirect("/");  //redirecciona a "/"
    	segundos=0;
    	segundos_aux = 0;  //esta variable es temporaria, sólo para control t, no queda en versión final
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
		
			logueado = true;
			Serial.println(logueado);
			IPlogueada = request->client()->remoteIP();
			Serial.println(IPlogueada);
			segundos=0;
			segundos_aux = 0;  //esta variable es temporaria, sólo para control t, no queda en versión final

			request->redirect("/menu");

		}else{
			Serial.println(logueado);

			request->send(200, "text/html", "<p>HTTP POST enviado - credenciales no validas</p>" 
											"<br><a href=\"/\">Retornar a pantalla inicio</a>");
		}

	});
	
	server.on("/menu", HTTP_ANY, [] (AsyncWebServerRequest *request) {
		
		if (!logueado) { //nadie logueado....logueado=0
      		request->redirect("/"); //va a llamada al raíz "/"
			Serial.println("usuario no logueado, va al /");
		}else{ //hay usuario logueado
		
			if ((request->client()->remoteIP()) == IPlogueada) { //ip entrante y ya logueadan son iguales
				if (timeOutweb) { //hubo timeout de web
					request->redirect("/logout"); //o a "/timeout??"
					Serial.println("timeoutWeb");
				}else{
					Serial.print("Entrando al menu MENU....");    
					Serial.println(logueado);

					request->send(200, "text/html", menu_html);

				}
			}else{
				Serial.println("entra al else ServidorOcupado");
				request->send_P(200, "text/html","Servidor web ocupado...<br><a href=\"/\">Retornar a pantalla inicio</a>");
			}
		}
		segundos=0;
		segundos_aux = 0;  //esta variable es temporaria, sólo para control t, no queda en versión final
	
	});
	
	server.on("/dir", HTTP_POST, [] (AsyncWebServerRequest *request) {
		
		if (!logueado) { //nadie logueado....logueado=0
        	request->redirect("/"); //va a llamada al raíz "/"
			Serial.println("No logueado");
    	}else{ //hay usuario logueado
        
			if ((request->client()->remoteIP()) == IPlogueada) { //ip entrante y ya logueadan son iguales
				if (timeOutweb) { //hubo timeout de web
					request->redirect("/logout"); //a "/logout"
					Serial.println("venció el timeout, redirect a /logout");
				}else{
					Serial.print("Entrando al menu DIR....");    
					Serial.println(logueado);

					cuentaDIR += 1;
					Serial.println(cuentaDIR);

					String param1;
					String param2;

					Serial.println("pathDirectory antes de listar_SD_dir = ");
					Serial.println(pathDirectory);
					listar_SD_dir(param1, param2, request);
					Serial.println("pathDirectory después de listar_SD_dir = ");
					Serial.println(pathDirectory);
					
					cuentaIntentos++;

				}
			}else{
          		request->send_P(200, "text/html","Servidor web ocupado...<br><a href=\"/\">Retornar a pantalla inicio</a>");
			}
    	}
		segundos=0;
		segundos_aux = 0;  //esta variable es temporaria, sólo para control t, no queda en versión final

	});

	server.on("/logout", HTTP_ANY, [] (AsyncWebServerRequest *request) {
		
		cuentaDIR = 0;
		pathDirectory = ""; //resetea el directorio para una nueva navegación
		if ((request->client()->remoteIP()) == IPlogueada) { //ip entrante e ip logueada son iguales
			timeOutweb = false;
			logueado = false;
			Serial.println(logueado); //control temporario. no queda en versión final
				
			segundos = 0;
			segundos_aux = 0;  //esta variable es temporaria, sólo para control t, no queda en versión final
			digitalWrite(LED_PORT, LOW); //apaga el led (control temporario), no hace falta en versión definitiva
			
    	}
    	request->redirect("/");

	});

	server.on("/muestraIP", HTTP_ANY, [] (AsyncWebServerRequest *request) {

		Serial.print("Solicitud HTTP desde IP: ");    

		Serial.println(request->client()->remoteIP());
		Serial.println(logueado);
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

	});

	server.on("/downloadall", HTTP_ANY, [] (AsyncWebServerRequest *request) {

		Serial.print("Solicitud HTTP desde IP: ");    
		//Serial.println(inputIP);
		Serial.println(request->client()->remoteIP());
		Serial.println(logueado);



		if (request->hasArg("downloadall")){
			Serial.println("Se bajarán todos los archivos del directorio");

			root.open(pathDirectory.c_str());
			root.rewindDirectory();
			//downloadAllFromDirectory(root, request);
			
			AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "My file contents");
			
			response->addHeader("Accept-Encoding", "gzip");
			response->addHeader("Content-Disposition","attachment; filename=\"file.gzip\"");
			request->send(response);

			AsyncWebServerResponse *response2 = request->beginResponse(200, "text/plain", "My file contents");
			response2->addHeader("Content-Disposition","attachment; filename=\"file22222.txt\"");
			request->send(response2);
			

			root.close();

		} 

	});
	
	
	server.onNotFound(notFound);  //rutina de atención de páginas web solicitadas y no definidas
	server.begin();		//inicializa el webserver

	///////////////////////////// End of Request commands

  	Serial.println("HTTP server started");
	logueado = false;

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop(void){



	if(flagDescargarArchivo){
		Serial.println("flagDescargarArchivo = 1");
		
		SD_file_download(rutaDeArchivoDescarga, requestNuevo);
		flagDescargarArchivo = 0;
	}

	if (WiFi.status() != WL_CONNECTED) {//controla e informa si pierde conexión wifi
      Serial.println("WiFi desconectado..");
	  reconnectWifi();
    }

}

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

		download.close();
		download2.close();
    }else ReportFileNotPresent("download"); 
  }else ReportSDNotPresent();

}


void listar_SD_dir(String param1, String param2, AsyncWebServerRequest *request){


	Serial.print("cantidad de parametros = ");
	Serial.println(request->params());
	param1 = request->getParam(0)->value();

	Serial.println(param1);

	if (request->hasArg("MarcarTodo")){
		String marcarTodo = {};

		marcarTodo = request->getParam(0)->value();
		Serial.print("marcar todo = ");
		Serial.println(marcarTodo);
		
		if(marcarTodo == "si"){
			flagMarcarTodo = 1;
		}else if(marcarTodo == "no"){
			flagMarcarTodo = 0;
		}
		
	}
	

	Serial.println("***********************************");
	Serial.print("pathDirectory antes de verificar usuario logueado = ");
	Serial.println(pathDirectory);
	Serial.println("***********************************");

	if(logueado){
		Serial.println("usuario logueado");
		 if (SD_present) {
			 

    
			if(param1 == ROOTDIR){

				root.open(ROOTDIR);

			}else{
				
				String directorio = param1;
				Serial.print("Directorio tomado de param1 = ");
				Serial.println(directorio);
				

				if (request->hasArg("directorio")){//si se pide avanzar adelante (un directorio nuevo)
					
					Serial.println("Tiene argumento 'directorio'");
					pathDirectory = pathDirectory + directorio + "/";
					//pathDirectory = "/Ensayos/nivel1/nivel2/";
					Serial.print("pathDirectory = ");
					Serial.println(pathDirectory);
					
				}else if(request->hasArg("directorioAtras")){//si se pide avanzar atrás (el directorio padre)

					if(pathDirectory != ROOTDIR){
						
						Serial.println("Tiene argumento 'directorioAtras'");
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

				//webpage += F("<h3 class='rcorners_m'>Contenido de la memoria SD</h3><br>");
				webpage += F("<table><tr><td><h2 class='rcorners_m'>Contenido de la memoria SD</h2></td><td><form action='/logout' method='post' accept-charset='utf-8'><input type='hidden' name='logout' id='logout' value='/'><input type='Submit' value='Logout'></form></td></tr></table>");
				//webpage += F("<form action='/logout' method='post' accept-charset='utf-8'><input type='hidden' name='logout' id='logout' value='/'><input type='Submit' value='Logout'></form>");
				
				//webpage += F("<form action='/consultarPorFecha'><label for='Fecha'>Fecha: </label><input type='date' id='fechaEnsayo' name='fechaEnsayo'><input type='submit'></form><br>");

				webpage += "<h2 style='color:blue;' align = 'left'>Directorio actual = "+ pathDirectory + "</h2>";

				webpage += F("<table class='table table-striped'>");
				//webpage += F("<tr><th scope='col'>Nombre</th><th scope='col'>Fecha</th><th scope='col'>Archivo/Directorio</th><th scope='col'>Tama&ntildeo</th><th scope='col'>Descargar</th><td><form action='#' method='post' accept-charset='utf-8'><input type='hidden' name='MarcarTodo' value='si'><input type='Submit' value='Marcar Todo'></form></td><td><form action='#' method='post' accept-charset='utf-8'><input type='hidden' name='MarcarTodo' value='no'><input type='Submit' value='Desmarcar Todo'></form></td><td><form action='/downloadall' method='post' accept-charset='utf-8'><input type='hidden' name='downloadall' value='si'><input type='Submit' value='Descargar Todo'></form></td></tr>");
				webpage += F("<tr><th scope='col'>Nombre</th><th scope='col'>Fecha</th><th scope='col'>Archivo/Directorio</th><th scope='col'>Tama&ntildeo</th><th scope='col'>Descargar</th></tr>");
				webpage += "<tr><td><form action='/dir' method='post' accept-charset='utf-8'><input type='hidden' name='directorioAtras' id='directorioAtras' value='../'><input type='Submit' value='../'></form></td><td></td><td></td><td></td><td align='center'></td></tr>";

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
				Serial.println("No files found");
				
				String prueba = webpage;
				
				armarPaqueteHtml(prueba);

				request->send(200, "text/html", paquete_html.c_str());
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

		entry.getName(entryName, 100);//obtiene el nombre de la entrada, ya sea un directorio o un archivo
		
		if(entry.isDirectory()){//Si es un directorio
			
			rutaDeArchivo = String(entryName);
			
			//webpage += "<tr><td><form action='/dir' method='post' accept-charset='utf-8'><input type='hidden' name='directorio' id='directorio' value="+ String(rutaDeArchivo) +"><input type='hidden' name='cuenta' id='cuenta' value='"+ String(cuenta) +"'><input type='Submit' value='"+String(rutaDeArchivo)+"'></form></td><td></td><td>"+String(entry.isDirectory()?"Dir":"File")+"</td><td></td><td align='center'></td><td></td></tr>";
			webpage += "<tr><td><form action='/dir' method='post' accept-charset='utf-8'><input type='hidden' name='directorio' value='"+ String(rutaDeArchivo) +"'><input type='hidden' name='cuenta' id='cuenta' value='"+ String(cuenta) +"'><input type='Submit' value='"+String(rutaDeArchivo)+"'></form></td><td></td><td>"+String(entry.isDirectory()?"Dir":"File")+"</td><td></td><td></td></tr>";


			
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

void downloadAllFromDirectory(SdFile path, AsyncWebServerRequest *request){

	char entryName[100] = {};
	String nombreDeArchivo = {};
	String rutaDeArchivo = {};
	bool hayMasArchivos = 0;
	String dirNuevo = {};
	Serial.println("----------------Entra al downloadAllFromDirectory-----------------------");
	Serial.print("pathDirectory = ");
	Serial.println(pathDirectory);
	String fechaNormalizada = {};

	while(entry.openNext(&path, O_RDONLY)){

		entry.getName(entryName, 100);//obtiene el nombre de la entrada, ya sea un directorio o un archivo
		
		if(entry.isDirectory()){//Si es un directorio
			
			rutaDeArchivo = String(entryName);
						
		}
		else//Si es un archivo
		{
			
			nombreDeArchivo = String(entryName);
			//rutaDeArchivo = rutaDeArchivo + "/" + nombreDeArchivo;
			rutaDeArchivo = pathDirectory + nombreDeArchivo;
			rutaDeArchivoDescarga = pathDirectory + nombreDeArchivo;
			filename = nombreDeArchivo;
			Serial.println(filename);

			if(filename.endsWith(".csv")){
				Serial.println("termina con .csv");
			}
			
			flagDescargarArchivo = 1;

		}
		entry.close();
		
	}
	
}

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

void ConsultarPorFecha(void){

}

*/
//******************************************************************************************
/*void leerSerie(){
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
*/

/*
void agregaFilaEnTabla(String nombreDeArchivo, String fechaNormalizada, String rutaDeArchivo){

	webpage += "<tr><td>"+ String(nombreDeArchivo) +"</td><td>"+fechaNormalizada+"</td>";
	webpage += "<td>"+String(entry.isDirectory()?"Dir":"File")+"</td>";
	int bytes = entry.size();
	String fsize = "";
	
	if (bytes < 1024)                     fsize = String(bytes)+" B";
	else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
	else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
	else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";

	webpage += "<td>"+fsize+"</td></form></td>";
	Serial.print("		Archivo a descargar = ");
	Serial.print(rutaDeArchivo);

	webpage += "<td><form action='/download' method='post' accept-charset='utf-8'><input type='text' hidden name='lname' value='"+String(rutaDeArchivo)+"'><input type='submit' name='download' value='Descargar'></form></td>";
		
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

void notFound(AsyncWebServerRequest *request) { //rutina de atención para páginas solicitadas no definidas 
    //pathDirectory = ""; //resetea el directorio para una nueva navegación
	
	Serial.print("URL = ");
	Serial.println(request->url());
	if(request->url().endsWith(".ico")){
		Serial.println("se trata de favicon.ico");
	}else{
		Serial.println("URL no encontrada: no es favicon.ico");
		Serial.println("URL no encontrada, resetea pathDirectory y redirect a /");
		pathDirectory = ""; //resetea el directorio para una nueva navegación
		request->redirect("/");                     //Toda página solicitada no definida se redirecciona a la raíz ("/")
	}
	//Serial.println("URL no encontrada");
	//request->redirect("/");                     //Toda página solicitada no definida se redirecciona a la raíz ("/")
}

void IRAM_ATTR onTimer0() //rutina de atención del timer (se ejecuta cada 1 segundo)
{
  segundos += 1;
  if (segundos >= tiempotimeOut){
    segundos=0;
    segundos_aux = 0;  //esta variable es temporaria, sólo para control t, no queda en versión final
    if (logueado) {
		Serial.println("Resetea pathDirectory porque venció el timeout");
		pathDirectory = "";//resetea el directorio para una nueva navegación
		timeOutweb = true;  
		digitalWrite(LED_PORT, HIGH); //enciende el led (control temporario), no hace falta en versión definitiva 
    }
  }
}

ArRequestHandlerFunction handlerRoot(void){
	
	AsyncWebServerRequest *request;
		if (!logueado) { //si no hay usuario logueado envía página de login
			request->send_P(200, "text/html", index_html);
		}else{ //hay otro usuario logueado
		
			if ((request->client()->remoteIP()) == IPlogueada) { //ip entrante ip logueada son iguales
				if (timeOutweb) { //hubo timeout de web
				request->redirect("/logout");  //redirecciona a "/logout"
				}
				else {
				request->redirect("/menu");  //redirecciona a "/menu"
				}
			}
			else {
				request->send_P(200, "text/html","Servidor web ocupado...<br><a href=\"/\">Retornar a pantalla inicio</a>"); //servidor atendiendo otra IP
			}
		}
  }
  */
