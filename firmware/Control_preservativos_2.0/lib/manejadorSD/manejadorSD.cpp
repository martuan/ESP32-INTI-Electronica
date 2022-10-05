#include <Arduino.h>
#include <SD.h>
#include "manejadorSD.h"

String directorio1; //, directorio2, directorio3, directorio4;

    manejadorSD::manejadorSD(int CS1){
       _CS1 = CS1;
    }

    boolean manejadorSD::inicializarSD(){
      if (!SD.begin(_CS1)) {
        Serial.println("inicialización fallida!"); 
        return(false);
      } else {
          Serial.println("initialization done.");
          return(true);
      }
      
      uint8_t cardType = SD.cardType();
      if(cardType == CARD_NONE){
          Serial.println("No SD card attached");
          return(false);
      }
      String str = "/Maquina_";              //El nombre del archivo corresponde al número de máquina
      str += String(NUMERO_MAQUINA);
      File archivo = SD.open(str.c_str());   //Si no existe el archivo lo crea. Aqui se guardan el último dato de cada ensayo ya sea, tiempo y presión de reventado, 
      if(!archivo) {                              //fin por timeout, presión de fuelle fuera de rango o parada por usuario (pulsador inicio)
        // writeFile(SD, str.c_str(), "Nombre archivo, Segundos, Estado final\r");
    }    
    }
//***********************************************************************************
//Rutina que crea en archivo de registro de nuevo ensayo, lo nombra según formato ISO8601 y Escribe dentro de el el encabezado del ensayo
void manejadorSD::writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.println();
    Serial.printf("Writing file: %s\n", path);
 
  File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("File appended");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

boolean manejadorSD::appendFile(fs::FS &fs, const char * path, const char * message){
//    Serial.printf("Appending to file: %s\n", path);
  boolean datoEscritoOk;
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        datoEscritoOk = false;
    }
    if(file.print(message)){
//        Serial.println("Message appended");
      datoEscritoOk = true;
    } else {
        Serial.println("Append failed");
        datoEscritoOk = false;
    }
    file.close();
     return(datoEscritoOk);
}
//********************************************************************************
void manejadorSD::createDir(fs::FS &fs, const char * path){
  boolean directorioExistente ;

  directorioExistente = SD.exists(path);
  if(directorioExistente){
          Serial.println("Directorio existe");
    }else{
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
      Serial.println("Dir created");
    } else {
      Serial.println("mkdir failed");
    }
  }
}
//********************************************************************************
void manejadorSD::listDir(fs::FS &fs, const char * dirname, uint8_t levels, boolean listEnArchivo){   //
  boolean borrarlistadoDirectorio = true;
  unsigned char i = 0;

  Serial.println("");
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
 
      directorio1 = String(file.name());

      if(levels){
        listDir(fs, file.name(), levels -1, false);
      }
    }else {
      Serial.print("  Archivo: ");
      Serial.print(file.name());
      Serial.print("  Tamaño: ");
      Serial.println(file.size());
    }
 //     Serial.println("");
    file = root.openNextFile();
  }
      if(listEnArchivo){
        //if(borrarlistadoDirectorio)    
//        SD.remove("/listadoDirectorio.txt");
//        this->manejadorSD::listDirLCD(String(file.name()));
  //      this->manejadorSD::listDirArchivo();
        //borrarlistadoDirectorio = false;
      }
  }
//*******************************************************************************************************
void manejadorSD::listSubDir(fs::FS &fs, const char * dirname, uint8_t levels){      //Devuelve el contenido del directorio. Ej.: Enviando "leerSubDir:/20220601" devuelve el contenido dentro del directorio /20220601
  Serial.println("");
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
String rutaNombre = "";
String nombre = "";
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      rutaNombre = file.name();
      nombre = rutaNombre;    //.substring(9);
      Serial.println(nombre);
      if(levels){
        listDir(fs, file.name(), levels -1, false);
      }
    }
    file = root.openNextFile();
  }
}

//**************************************************************************************
void manejadorSD::leerFile(fs::FS &fs, const char * path){
  Serial.printf("Leyendo archivo: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("Datos del archivo: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}
//******************************************************************************************
/*String manejadorSD::nombreDirectorio1(void){
  return(directorio1);
}*/
//*******************************************************************************************************
int manejadorSD::listCantidadArchivos(fs::FS &fs, const char * dirname, uint8_t levels){      //Devuelve el contenido del directorio. Ej.: Enviando "leerSubDir:/20220601" devuelve el contenido dentro del directorio /20220601
  int cantArchivos = 0;
  Serial.printf("Listado de directorio: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return(0);
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return(0);
  }
String rutaNombre = "";
String nombre = "";
  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      rutaNombre = file.name();
      nombre = rutaNombre;    //.substring(9);
      Serial.println(nombre);
      //cantArchivos++;
      if(levels){
        listDir(fs, file.name(), levels -1, false);
      }
    }
    cantArchivos++;
    file = root.openNextFile();
  }
    Serial.printf("Cantidad archivos en SD: ");
    Serial.println(String(cantArchivos));

  return(cantArchivos);
}