/*  
 *
 *	Programado por: Martín Cioffi
 *  Fecha: 05-07-2022
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
#ifdef ESP8266
  #include <ESP8266WiFi.h>       // Built-in
  #include <ESP8266WiFiMulti.h>  // Built-in
  #include <ESP8266WebServer.h>  // Built-in
  #include <ESP8266mDNS.h>
#else
	#include <WiFi.h>              // Built-in
	#include <WiFiMulti.h>         // Built-in
	#include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
	#include <ESPmDNS.h>
	#include "FS.h"
	#include <AsyncTCP.h>
	#include "SPIFFS.h"
#endif

#include "Network.h"
#include "Sys_Variables.h"
#include "CSS.h"
#include <SD.h> 
#include <SPI.h>

#ifdef ESP8266
  ESP8266WiFiMulti wifiMulti; 
  ESP8266WebServer server(80);
#else
  WiFiMulti wifiMulti;
  ESP32WebServer server(80);
#endif

void HomePage();
void File_Download();
void SD_file_download(String filename);
void handleFileUpload();
void SD_dir();
void printDirectory_v3(File dir, int numTabs);
void File_Stream();
void File_Delete();
void SD_file_delete(String filename);
void SendHTML_Header();
void SendHTML_Content();
void SendHTML_Stop();
void SelectInput(String heading1, String command, String arg_calling_name);
String file_size(int bytes);
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
void File_Upload();
void ReportCouldNotCreateFile(String target);
void ReportFileNotPresent(String target);
void ReportSDNotPresent();
void SD_file_stream(String filename);
void initSPIFFS(void);
void ConsultarPorFecha();

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup(void){
  Serial.begin(115200);

	initSPIFFS();
	
	File root;
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
        delay(5000);
      }
	/*
	//************************CONFIG MULTIWIFI***************************************

		if (!WiFi.config(local_IP, gateway, subnet, dns)) { //WiFi.config(ip, gateway, subnet, dns1, dns2);
			Serial.println("WiFi STATION Failed to configure Correctly"); 
		} 
		wifiMulti.addAP(ssid_1, password_1);  // add Wi-Fi networks you want to connect to, it connects strongest to weakest
		wifiMulti.addAP(ssid_2, password_2);  // Adjust the values in the Network tab
		wifiMulti.addAP(ssid_3, password_3);
		wifiMulti.addAP(ssid_4, password_4);  // You don't need 4 entries, this is for example!
		
		Serial.println("Connecting ...");
		while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
			delay(250); Serial.print('.');
		}
	*/
  Serial.println("\nConnected to "+WiFi.SSID()+" Use IP address: "+WiFi.localIP().toString()); // Report which SSID and IP is in use
  // The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
  if (!MDNS.begin(servername)) {          // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
    Serial.println(F("Error setting up MDNS responder!")); 
    ESP.restart(); 
  } 
  #ifdef ESP32
    // Note: SD_Card readers on the ESP32 will NOT work unless there is a pull-up on MISO, either do this or wire one on (1K to 4K7)
    Serial.println(MISO);
    pinMode(19,INPUT_PULLUP);
  #endif
  Serial.print(F("Initializing SD card...")); 
  if (!SD.begin(SD_CS_pin)) { // see if the card is present and can be initialised. Wemos SD-Card CS uses D8 
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

	server.on("/", HomePage);
	server.on("/download", File_Download);
	//server.on("/upload",   File_Upload);
	//server.on("/fupload",  HTTP_POST,[](){ server.send(200);}, handleFileUpload);
	//server.on("/stream",   File_Stream);
	server.on("/delete",   File_Delete);
	server.on("/dir",      SD_dir);
	server.on("/consultarPorFecha", ConsultarPorFecha);
  
  ///////////////////////////// End of Request commands
  	server.begin();
  	Serial.println("HTTP server started");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop(void){
  server.handleClient(); // Listen for client connections
  //server.addHandler();
}

// All supporting functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void HomePage(){

  SendHTML_Header();
  
  fechaEnsayoConsultada = "";
  webpage += F("<a href='/dir'><button>Listado de archivos</button></a>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop(); // Stop is needed because no content length was sent
  
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download(){ // This gets called twice, the first pass selects the input, the second pass then processes the command line arguments
  
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("download")){
		SD_file_download(server.arg(0));
		Serial.print("argumento = ");
		Serial.println(server.arg(0));
	} 
  }
  else SelectInput("Enter filename to download","download","download");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_download(String filename){

	Serial.print("filename = ");
	Serial.println(filename);
		
  if (SD_present) { 
    
	File download = SD.open("/"+filename);
	Serial.println();
	Serial.println(String(download.name()));
	Serial.println();
	Serial.print("filenamePath = ");
	Serial.println(download.path());
	Serial.println();
    
	if (download) {
		Serial.print("Existe el archivo = ");
		Serial.println(String(download.name()));
		Serial.println("Se descarga el archivo");
		Serial.println();
		
		server.sendHeader("Content-Type", "text/text");
		server.sendHeader("Content-Disposition", "attachment; filename="+filename);
		server.sendHeader("Connection", "close");
		server.streamFile(download, "application/octet-stream");
		download.close();
    }else ReportFileNotPresent("download"); 
  }else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Upload(){
  Serial.println("File upload stage-1");
  append_page_header();
  webpage += F("<h3>Select File to Upload</h3>"); 
  webpage += F("<FORM action='/fupload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='fupload' id = 'fupload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  Serial.println("File upload stage-2");
  server.send(200, "text/html",webpage);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
File UploadFile; 
void handleFileUpload(){ // upload a new file to the Filing system
  Serial.println("File upload stage-3");
  HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if(uploadfile.status == UPLOAD_FILE_START)
  {
    Serial.println("File upload stage-4");
    String filename = uploadfile.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SD.remove(filename);                         // Remove a previous version, otherwise data is appended the file again
    UploadFile = SD.open(filename, FILE_WRITE);  // Open the file for writing in SPIFFS (create it, if doesn't exist)
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    Serial.println("File upload stage-5");
    if(UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  } 
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if(UploadFile)          // If the file was successfully created
    {                                    
      UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header();
      webpage += F("<h3>File was successfully uploaded</h3>"); 
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename+"</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>"; 
      append_page_footer();
      server.send(200,"text/html",webpage);
    } 
    else
    {
      ReportCouldNotCreateFile("upload");
    }
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_dir(){
  if (SD_present) { 
    File root = SD.open("/");
    if (root) {
      root.rewindDirectory();
      SendHTML_Header();
      webpage += F("<h3 class='rcorners_m'>Contenido de la memoria SD</h3><br>");
	  webpage += F("<form action='/consultarPorFecha'><label for='Fecha'>Fecha: </label><input type='date' id='fechaEnsayo' name='fechaEnsayo'><input type='submit'></form><br>");
      webpage += F("<table align='center'>");
      webpage += F("<tr><th>Nombre</th><th>Fecha</th><th style='width:20%'>Archivo/Directorio</th><th>Tama&ntildeo</th><th>Eliminar</th><th>Descargar</th></tr>");

	  printDirectory_v3(root, 0);

	  webpage += F("</table>");
	  fechaEnsayoConsultada = "";
      SendHTML_Content();
      root.close();
    }
    else 
    {
      SendHTML_Header();
      webpage += F("<h3>No Files Found</h3>");
    }
    append_page_footer();
    SendHTML_Content();
    SendHTML_Stop();   // Stop is needed because no content length was sent
  } else ReportSDNotPresent();
  
  pathDirectory = "";//setea el directorio para una nueva búsqueda

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void printDirectory_v3(File dir, int numTabs){

	while(true){

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
		String fechaNormalizada = {};

		
		File entry =  dir.openNextFile();

		if (! entry) {
			// no hay mas archivos
			break;
		}
		for (uint8_t i = 0; i < numTabs; i++) {
		Serial.print('\t');
		}
		
		Serial.println(entry.name());
	
		
		if (webpage.length() > 1000) {
			SendHTML_Content();
		}
		
		if(entry.isDirectory() && fechaEnsayoConsultada.length() == 0){//Si es un directorio y no hay consulta por fechas
			
			Serial.print("entry.path() = ");
			Serial.println(entry.path());

			Serial.println(String(entry.isDirectory()?"Dir ":"File ")+String(entry.name()));
			webpage += "<tr><td>"+String(entry.path())+"</td><td></td><td>"+String(entry.isDirectory()?"Dir":"File")+"</td><td></td><td align='center'><input type='checkbox' id='vehicle4' name='vehicle1' value='Bike'><label for='vehicle1'> Borrar</label></td></tr>";
		
			printDirectory_v3(entry, numTabs + 1);
		}
		else//Si es un archivo
		{
		
			filename = String(entry.name());//convierte a String
			String rutaArchivo = String(entry.path());


			if(filename.length() == 19 && filename.charAt(8) == 'T'){//si el archivo es una fecha de ensayo

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

				fechaNormalizada = year + "-" + month + "-" + day;

			}else{
				fechaNormalizada = "";
			}
			
			//si fechaEnsayoConsultada coincide con fechaNormaliza, arma la tabla con las filas correspondientes
			//aplica filtro
			if(fechaEnsayoConsultada.length() > 0){//si se ha realizado consultaPorFecha
			
				Serial.print("-------------------------> ");
				Serial.print("SI se ha realizado consultaPorFecha");
				Serial.println(" <------------------------ ");
				
				if(fechaNormalizada.compareTo(fechaEnsayoConsultada) == 0){//si coinciden las fechas

					Serial.println("SI coinciden las fechas");
				
					webpage += "<tr><td>"+ rutaArchivo +"</td><td>"+fechaNormalizada+"</td>";
					Serial.print(String(entry.isDirectory()?"Dir ":"File ")+String(entry.name())+"\t");
					webpage += "<td>"+String(entry.isDirectory()?"Dir":"File")+"</td>";
					int bytes = entry.size();
					String fsize = "";
					
					if (bytes < 1024)                     fsize = String(bytes)+" B";
					else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
					else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
					else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";

					webpage += "<td>"+fsize+"</td><td><form action='/delete' method='post'><input type='text' hidden name='lname' value='"+String(rutaArchivo)+"'><input type='submit' name='delete' value='Borrar'></form></td>";
					Serial.print("Archivo a descargar = ");
					Serial.print(rutaArchivo);
					webpage += "<td><form action='/download' method='post'><input type='text' hidden name='lname' value='"+String(rutaArchivo)+"'><input type='submit' name='download' value='Descargar'></form></td>";
					Serial.println(String(fsize));
				}else{//si no coinciden las fechas

					Serial.println("NO coinciden las fechas");
				}
			
			
			}else{//si no se ha realizado consultaPorFecha, se muestra todo sin filtro
				
				
				Serial.print("-------------------------> ");
				Serial.print("NO se ha realizado consultaPorFecha");
				Serial.println(" <------------------------ ");

				webpage += "<tr><td>"+rutaArchivo+"</td><td>"+fechaNormalizada+"</td>";
				Serial.print(String(entry.isDirectory()?"Dir ":"File ")+String(entry.name())+"\t");
				webpage += "<td>"+String(entry.isDirectory()?"Dir":"File")+"</td>";
				int bytes = entry.size();
				String fsize = "";
				
				if (bytes < 1024)                     fsize = String(bytes)+" B";
				else if(bytes < (1024 * 1024))        fsize = String(bytes/1024.0,3)+" KB";
				else if(bytes < (1024 * 1024 * 1024)) fsize = String(bytes/1024.0/1024.0,3)+" MB";
				else                                  fsize = String(bytes/1024.0/1024.0/1024.0,3)+" GB";
				webpage += "<td>"+fsize+"</td><td><form action='/delete' method='post'><input type='text' hidden name='lname' value='"+String(rutaArchivo)+"'><input type='submit' name='delete' value='Borrar'></form></td>";
				Serial.print("Archivo a descargar = ");
				Serial.println(rutaArchivo);
				webpage += "<td><form action='/download' method='post'><input type='text' hidden name='lname' value='"+String(rutaArchivo)+"'><input type='submit' name='download' value='Descargar'></form></td>";
				Serial.println(String(fsize));

			}
		
			
		
		}
		
		entry.close();

	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Stream(){
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("stream")) SD_file_stream(server.arg(0));
  }
  else SelectInput("Enter a File to Stream","stream","stream");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_stream(String filename) { 
  if (SD_present) { 
    File dataFile = SD.open("/"+filename, FILE_READ); // Now read data from SD Card 
    Serial.print("Streaming file: "); Serial.println(filename);
    if (dataFile) { 
      if (dataFile.available()) { // If data is available and present 
        String dataType = "application/octet-stream"; 
        if (server.streamFile(dataFile, dataType) != dataFile.size()) {Serial.print(F("Sent less data than expected!")); } 
      }
      dataFile.close(); // close the file: 
    } else ReportFileNotPresent("Cstream");
  } else ReportSDNotPresent(); 
}   
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Delete(){
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("delete")) SD_file_delete(server.arg(0));
  }
  else SelectInput("Select a File to Delete","delete","delete");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(){
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate"); 
  server.sendHeader("Pragma", "no-cache"); 
  server.sendHeader("Expires", "-1"); 
  server.setContentLength(CONTENT_LENGTH_UNKNOWN); 
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves. 
  append_page_header();
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content(){
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop(){
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(String heading1, String command, String arg_calling_name){
  SendHTML_Header();
  webpage += F("<h3>"); webpage += heading1 + "</h3>"; 
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent(){
  SendHTML_Header();
  webpage += F("<h3>No SD Card present</h3>"); 
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target){
  SendHTML_Header();
  webpage += F("<h3>File does not exist</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target){
  SendHTML_Header();
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>"); 
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer();
  SendHTML_Content();
  SendHTML_Stop();
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

	fechaEnsayoConsultada = "";//borra la anterior consulta

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

}