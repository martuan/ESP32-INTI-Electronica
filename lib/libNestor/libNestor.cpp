#include "libNestor.h"


#define DHTPIN 33               // Pin donde está conectado el sensor de temperatura 
#define DHTTYPE DHT22           // Sensor DHT22

DHT dht2(DHTPIN, DHTTYPE);// Initialize DHT sensor.
RTC_DS3231 rtc2;

/*
//************ FUNCIONES **********************************


void SensorDHT::inicializar(void){

	DHT dht(DHTPIN, DHTTYPE);// Initialize DHT sensor.




}
*/
void  leerEEPROM(void){
  //Read data from eeprom
  int address = 0;
  int cont = 0;
  int readId;
  while(cont<10){
    readId = EEPROM.readInt(address); //EEPROM.get(address,readId);
    Serial.print("Read m = ");
    Serial.println(readId);
    address += sizeof(readId); //update address value  
    readId = EEPROM.readInt(address); //EEPROM.get(address,readId);
    Serial.print("Read b = ");
    Serial.println(readId);
    address += sizeof(readId); //update address value  
    cont ++;    
  }
    Serial.print("Sensor Nº 2: ");
    readId = EEPROM.readInt(addressEEPROM_1kPa); 
    Serial.println(readId);
    readId = EEPROM.readInt(addressEEPROM_2kPa); 
    Serial.println(readId);
}
//******* Calibración ***********************************************************************
void calibracionSensoresPresion(void){
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
     }
     leerEEPROM();  
}
//******************** Fin calibración ************************************************************
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
boolean  tiempoTranscurrido(boolean ejecutado){
  const int intervalo = 10;      // Cantidad de milisegundos
  const int intervalosLimite = 60;   //Cantidad de intervalos entre medición y registro de datos (Presión y caudal). intervalo * intervalosLimite = 1 segundo
  static boolean tiempoCumplido;
  static unsigned long previousMillis = 0;
  static unsigned long currentMillis = 0;
  static int intervalosContados = 0;

  if(ejecutado){
    intervalosContados = 0;
  }

   currentMillis = millis();
  if (currentMillis - previousMillis >= intervalo) {
    previousMillis = currentMillis; 
    intervalosContados++; 
  }  
  if(intervalosContados >= intervalosLimite){
    tiempoCumplido = true;
//    intervalosContados = 0;
  }else{
    tiempoCumplido = false;
  }
  if(tiempoCumplido == true)  return(true);
  else  return(false);
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
   
  ultimaLecturaEEPROM = 9;
  if(lecturaAD1 < 1220) { ultimaLecturaEEPROM = 8; }
  if(lecturaAD1 < 1075) { ultimaLecturaEEPROM = 7; }
  if(lecturaAD1 < 800) { ultimaLecturaEEPROM = 5; }
  if(lecturaAD1 < 937) { ultimaLecturaEEPROM = 6; }
  if(lecturaAD1 < 680) { ultimaLecturaEEPROM = 4; }
  if(lecturaAD1 < 560) { ultimaLecturaEEPROM =3; }
  if(lecturaAD1 < 440) { ultimaLecturaEEPROM = 2; }
  if(lecturaAD1 < 320) { ultimaLecturaEEPROM = 1; }

  while(cont < ultimaLecturaEEPROM){
    m = EEPROM.readInt(address); //
    address += sizeof(m); //update address value  
    b = EEPROM.readInt(address); //
    address += sizeof(b); //update address value  
    cont ++;    
  }
 /*   Serial.print("Read m = ");
    Serial.println(m);
    Serial.print("Read b = ");
    Serial.println(b);*/
  presion1 = (float(lecturaAD1) - float(b))/float(m);
 
  if(presion1 < 0)  presion1 = 0;

  return presion1;
}


//Rutina que crea en archivo de registro de nuevo ensayo, lo nombra según formato ISO8601 y Escribe dentro de el el encabezado del ensayo
void writeFile(fs::FS &fs, const char * path, const char * message){
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

void appendFile(fs::FS &fs, const char * path, const char * message){
//    Serial.printf("Appending to file: %s\n", path);
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
//        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}



//***********************************************************************************
void rutinaEnsayo(String nombreArchivo){
  const int timeOut = 30;
  int segundos = 0;
  char medicion[5];
  String lineaMedicion = "";
  String lineaMedicionAnterior = ""; 
  String ultimasMediciones = "/UltimasMediciones.csv";
  String escribeUltimaMedicion = "";
  float presion1 = 0;               //Presión medida en preservativo
  float presion1Anterior = 0;       //Presión medida en el ciclo anterior en preservativo
  boolean registrarDatos = LOW;
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
  nombreMaquina += String(NUMERO_MAQUINA);        //El nombre del archivo corresponde al número de máquina

  valorAD_minimo_fuelle = EEPROM.readInt(addressEEPROM_1kPa); //Valor de presión mínimo (en entero del AD) en el fuelle (sensor de presión 2)   
  valorAD_maximo_fuelle = EEPROM.readInt(addressEEPROM_2kPa); //Valor de presión máximo (en entero del AD) en el fuelle (sensor de presión 2) 

  digitalWrite(activarElectrovalvula, HIGH);
  while(segundos < timeOut){
    registrarDatos = tiempoTranscurrido(false);
    if(registrarDatos){
       presion1 = obtenerPresion1();            //Se obtiene la presión en el preservativo
       presion1PartEntera = int(presion1);      //sprintf no formatea float, entonces separo partes entera y decimal
       presion1Partdecimal = int((presion1 - presion1PartEntera) * 100);
       sprintf(medicion, "%d.%02d", (int)presion1, presion1Partdecimal);
       lineaMedicion += String(segundos);
       lineaMedicion += ",  ";
       lineaMedicion += String(medicion);
       lineaMedicion += "\r";
       appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());   
       segundos ++;
       Serial.println(lineaMedicion);
       registrarDatos = tiempoTranscurrido(true);
       guardarLineaMedicionAnterior = true;
   }
   if(presion1 > 0.1)  { superoPresionMinima = true; }
   if(superoPresionMinima == true){
       if(presion1 < (presion1Anterior * 0.6)){       //Si la presión cae un 40% se asume que el preservativo reventó
          escribeUltimaMedicion += nombreArchivo;
          escribeUltimaMedicion += ", ";
          escribeUltimaMedicion += lineaMedicionAnterior;
          
          appendFile(SD, nombreMaquina.c_str(), escribeUltimaMedicion.c_str());
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
    lineaMedicion += nombreArchivo; 
    lineaMedicion += ", ";
    lineaMedicion += String(segundos);
    lineaMedicion += ", ";
  
   if(segundos == timeOut){
      lineaMedicion += "Fin de ensayo por Time Out \r"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());    
      Serial.println("Fin de ensayo por Time Out");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicion.c_str());
   }     
   
   inicio = digitalRead(pulsadorInicio);
   if(inicio == LOW){
      lineaMedicion += "Fin de ensayo por parada de usuario \r"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());    
      Serial.println("Fin de ensayo por parada de usuario");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicion.c_str());
      segundos = timeOut + 1;
      digitalWrite(activarElectrovalvula, LOW);
      delay(1500);
   }

    valorAD_fuelle = analogRead(sensorPresion2);
    if(valorAD_fuelle < valorAD_minimo_fuelle)   bajaPresionFuelle = HIGH;
    if(valorAD_fuelle > valorAD_maximo_fuelle)   altaPresionFuelle = HIGH;

    if(bajaPresionFuelle == HIGH){
      lineaMedicion += "Fin de ensayo por baja presión en fuelle \r"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());    
      Serial.println("Fin de ensayo por baja presión en fuelle");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicion.c_str());
      segundos = timeOut + 1;
      delay(300);
    }
    if(altaPresionFuelle == HIGH){
      lineaMedicion += "Fin de ensayo por alta presión en fuelle \r"; 
      appendFile(SD, nombreArchivo.c_str(), lineaMedicion.c_str());    
      Serial.println("Fin de ensayo por alta presión en fuelle");
      appendFile(SD, nombreMaquina.c_str(), lineaMedicion.c_str());
      segundos = timeOut + 1;
      delay(300);
    }
    lineaMedicion = "";
  }
  digitalWrite(activarElectrovalvula, LOW);
}
//********************************************************************************
String rutinaInicioEnsayo(void){
    String nombreArchivo = "";
    String cabecera = "";
    
    float temp = dht2.readTemperature(); //Leemos la temperatura en grados Celsius
    float humed = dht2.readHumidity(); //Leemos la Humedad
    DateTime now = rtc2.now();
    
    char dateTimeISO[17];
    sprintf(dateTimeISO, "%02d%02d%02dT%02d%02d%02d", now.year(), now.month(),now.day(),  now.hour(), now.minute(), now.second()); 
    nombreArchivo += "/";
    nombreArchivo += String(dateTimeISO);
    nombreArchivo += ".csv";
    writeFile(SD, nombreArchivo.c_str(), "Máquina número:, 01 \r\n");
    appendFile(SD, nombreArchivo.c_str(), "Fecha  y  hora, Temperatura, Humedad \r\n");
    
    char dateTime[20];
    sprintf(dateTime, "%02d/%02d/%02d %02d:%02d:%02d", now.year(), now.month(), now.day(),  now.hour(), now.minute(), now.second()); 
    cabecera += String(dateTime);
    cabecera += "  ,";
    cabecera += String(temp);
    cabecera += "  ,";
    cabecera += String(humed);
    cabecera += "\r\n";
    imprimirPC(now, temp, humed);
    appendFile(SD, nombreArchivo.c_str(), cabecera.c_str());
    String str = nombreArchivo.c_str();  

  return str; 
}
