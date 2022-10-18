String userEnabled1 = "admin";
//String passEnabled1 = "Inti1957";
String passEnabled1 = "4321";

String paquete_html = {};
String header_html = 
"<!DOCTYPE html><html><body>\
	<head>\
	<title>ESP32 Datalogger Webserver - INTI</title>\
	<meta name='viewport' content='user-scalable=yes,initial-scale=1.0,width=device-width'>\
	<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>\
	<link rel='stylesheet' href='https://cdn.jsdelivr.net/npm/bootstrap@4.0.0/dist/css/bootstrap.min.css' integrity='sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm' crossorigin='anonymous'>\
	<style>\
	body{max-width:65%;margin:0 auto;font-family:arial;font-size:105%;text-align:center;color:blue;background-color:#F7F2Fd;}\
	ul{list-style-type:none;margin:0.1em;padding:0;border-radius:0.375em;overflow:hidden;background-color:#dcade6;font-size:1em;}\
	li{float:left;border-radius:0.375em;border-right:0.06em solid #bbb;}last-child {border-right:none;font-size:85%}\
	li a{display: block;border-radius:0.375em;padding:0.44em 0.44em;text-decoration:none;font-size:85%}\
	li a:hover{background-color:#EAE3EA;border-radius:0.375em;font-size:85%}\
	section {font-size:0.88em;}\
	h1{color:white;border-radius:0.5em;font-size:1em;padding:0.2em 0.2em;background:#558ED5;}\
	h2{color:orange;font-size:1.0em;}\
	h3{font-size:0.8em;}\
	table{font-family:arial,sans-serif;font-size:0.9em;border-collapse:collapse;width:85%;}\
	th,td {border:0.06em solid #dddddd;text-align:center;padding:0.3em;border-bottom:0.06em solid #dddddd;}\
	tr:nth-child(odd) {background-color:#eeeeee;}\
	.rcorners_n {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:75%;}\
	.rcorners_m {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:50%;color:white;font-size:75%;}\
	.rcorners_w {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:70%;color:white;font-size:75%;}\
	.column{float:left;width:50%;height:45%;}\
	.row:after{content:'';display:table;clear:both;}\
	*{box-sizing:border-box;}\
	footer{background-color:#eedfff; text-align:center;padding:0.3em 0.3em;border-radius:0.375em;font-size:60%;}\
	button{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:20%;color:white;font-size:130%;}\
	.buttons {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:80%;}\
	.buttonsm{border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:9%; color:white;font-size:70%;}\
	.buttonm {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:15%;color:white;font-size:70%;}\
	.buttonw {border-radius:0.5em;background:#558ED5;padding:0.3em 0.3em;width:40%;color:white;font-size:70%;}\
	.btn {background-color: DodgerBlue;border: none;color: white;padding: 12px 30px;cursor: pointer;font-size: 20px;}\
	.btn:hover {background-color: RoyalBlue;}\
	a{font-size:75%;}\
	p{font-size:75%;}\
	</style></head><body>";

String ending_html = "</body></html>";

const char* PARAM_USUARIO = "input1";
const char* PARAM_PASS = "input2";

int logueado = 0;
int cuentaDIR = 0;

//IPAddress inputIP = 0.0.0.0;
IPAddress inputIP;

int cuentaReconexion = 0;

#define ServerVersion "1.0"
String webpage = "";
String filename = "";
String fechaEnsayoConsultada = "";
bool   SD_present = false;
//strcpy(src,  "");
//strcpy(dest, "/");
char src[100] = "";
char dest[100] = "/";
//String pathDirectory = {};
String pathDirectory = "/";
String pathLocation = {};
int cuenta = 0;
bool flagPrintDirectory = 0;
String usernameLogin = {};
String passwordLogin = {};

String usernameLoginEnabled1 = "martin";
String passwordLoginEnabled1 = "luna";
String usernameLoginEnabled2 = "martin";
String passwordLoginEnabled2 = "cioffi";
bool flagUsuarioHabilitado = 0;
bool flagWebserverLibre = 1;
bool flagUsuarioLogueado = 0;
bool flagClienteNuevo = 0;



String clienteEntrante = {};
String clienteActual = "0.0.0.0";



#ifdef ESP8266
#define SD_CS_pin           D8         // The pin on Wemos D1 Mini for SD Card Chip-Select
#else
#define SD_CS_pin           5        // Use pin 5 on MH-T Live ESP32 version of Wemos D1 Mini for SD Card Chip-Select
#endif                               // Use pin 13 on Wemos ESP32 Pro


