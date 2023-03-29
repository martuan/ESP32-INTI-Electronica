#include <Arduino.h>
#include "leerEEPROM.h"
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"
#include "leerCalibracionEEProm.h"
#include "hardware.h"

leerCalibracionEEProm::leerCalibracionEEProm(LiquidCrystal_I2C objeto1LCD){
     _objeto1LCD = objeto1LCD; 
}

//*************************************************************************
//boolean leerCalibracionEEProm::verificarCalibracion(void){  
boolean leerCalibracionEEProm::verificarCalibracion(void){  
  int ultimaLecturaEEPROM = 10;
  int address = 0;
  int b, m;
  int cont = 0;
  int checkSum =0;
  int checkSumEEProm1 =0;
  boolean checkSumOK_1, checkSumOK_2;         //CheckSum Ok. 
  boolean checkSumOK_fuelle;
  
 while(cont < ultimaLecturaEEPROM){
   m = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
       Serial.print("dirección actual: ");
      Serial.println(address);
    address += sizeof(m); //update address value  
      Serial.print("tamaño m: ");
      Serial.println(m);
    b = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    address += sizeof(b); //update address value  
      Serial.print("tamaño b: ");
      Serial.println(b);
    cont ++;    
    checkSum = m + b + checkSum;
  }
    Serial.print("dirección checksum: ");
    Serial.println(address);
    checkSumEEProm1 = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    Serial.print("CheckSum: ");
    Serial.println(checkSumEEProm1);

    if(checkSumEEProm1 == checkSum){    //Verifica el checksum de la calibración del sensor de presión en preservativo 
        checkSumOK_1 = true;
        Serial.println("CheckSum1: Valido ");
    } 
    else{
        checkSumOK_1 = false;         //checkSumOK_1 queda en false para forzar el restart y verifique si se corrigió
        Serial.println("CheckSum: Invalido");
        checkSumOK_2 = this->leerCalibracionEEProm::verificarCalibracionTalba2();
        if(checkSumOK_2 == true){
        Serial.println("CheckSumTabla2: Valido ");
        this->leerCalibracionEEProm::recomponerTabla1();
        }else{
          Serial.println("CheckSumTabla2: Invalido ");
        }
    }
    if(checkSumOK_1 == true){
      checkSumOK_fuelle = this->leerCalibracionEEProm::verificarCalibracionSensorFuelle();
      if(checkSumOK_fuelle == false){
        checkSumOK_1 = false;
        Serial.println("CheckSum2: Invalido");
         this->leerCalibracionEEProm::mensajeLCD(); 
      }    
    }
       
    return(checkSumOK_1);
}
//********************************************************************************************
boolean leerCalibracionEEProm::verificarCalibracionTalba2(void){
  int ultimaLecturaEEPROM = 10;
  int address = inicioTabla2;
  int b, m;
  int cont = 0;
  int checkSum =0;
  int checkSumEEProm1 =0;
  boolean checkSumOK_1;         //CheckSum Ok.
  
 while(cont < ultimaLecturaEEPROM){
    m = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración en la segunda tabla
      Serial.print("dirección actual: ");
      Serial.println(address);
    address += sizeof(m); //update address value  
      Serial.print("tamaño m: ");
      Serial.println(m);
    b = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    address += sizeof(b); //update address value  
      Serial.print("tamaño b: ");
      Serial.println(b);
    cont ++;    
    checkSum = m + b + checkSum;
  }
    Serial.print("dirección checksum: ");
    Serial.println(address);
    checkSumEEProm1 = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    Serial.print("CheckSum: ");
    Serial.println(checkSumEEProm1);

    if(checkSumEEProm1 == checkSum){
        checkSumOK_1 = true;
        Serial.println("CheckSum Tabla 2: Válido ");
    } 
        else{
            checkSumOK_1 = false;
            Serial.println("CheckSum Tabla 2: Inválido");
        }   
    return(checkSumOK_1);
}
//*********************************************************************************************
  void leerCalibracionEEProm::recomponerTabla1(void){
  int ultimaLecturaEEPROM = 10;
  int address = inicioTabla2;
  int addressTabla1; 
  int b, m;
  int cont = 0;
  int checkSum =0;
  int checkSumEEProm1 =0;
  boolean checkSumOK_1;         //CheckSum Ok.

 while(cont < ultimaLecturaEEPROM){
    m = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración en la segunda tabla
    addressTabla1 = address - inicioTabla2;
    EEPROM.writeInt(addressTabla1, m);       //Escribir en EEPROM m en posición 0
    EEPROM.commit();
      Serial.print("dirección actual correccion: ");
      Serial.println(addressTabla1);
    address += sizeof(m); //update address value  
//      Serial.print("tamaño m: ");
//      Serial.println(m);
    b = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    addressTabla1 = address - inicioTabla2;
    EEPROM.writeInt(addressTabla1, b);       //Escribir en EEPROM m en posición 0
    EEPROM.commit();
    address += sizeof(b); //update address value  
//      Serial.print("tamaño b: ");
//      Serial.println(b);
    cont ++;    
    checkSum = m + b + checkSum;
  }
    Serial.print("dirección checksum: ");
    Serial.println(address);
    checkSumEEProm1 = EEPROM.readInt(address); //Se lee la pendiente guardada durante la calibración
    addressTabla1 = address - inicioTabla2;
    EEPROM.writeInt(addressTabla1, checkSumEEProm1);       //Escribir en EEPROM m en posición 0
    EEPROM.commit();
    EEPROM.writeInt(flagIntentoRecalibrar, 0);       //Luego de recomponer tabla 1, resetea el flag que le indico que debia entrar
    EEPROM.commit();

}
//**************************************************************************************************
  boolean leerCalibracionEEProm::verificarCalibracionSensorFuelle(void){
    boolean checkSumOK_2;         //CheckSum Ok. Sensor Fuelle
    int presionFuelle_1kPa_1;
    int presionFuelle_2kPa_1;
    int sumaPresioneFuelle_1, checksum_Fuelle_1;
    int presionFuelle_1kPa_2;
    int presionFuelle_2kPa_2;
    int sumaPresioneFuelle_2, checksum_Fuelle_2;

    presionFuelle_1kPa_1 = EEPROM.readInt(addressEEPROM_1kPa_1); //
    presionFuelle_2kPa_1 = EEPROM.readInt(addressEEPROM_2kPa_1); //
    checksum_Fuelle_1 = EEPROM.readInt(addressEEPROM_checksum_1_fuelle);
    sumaPresioneFuelle_1 = presionFuelle_1kPa_1 + presionFuelle_2kPa_1;
    if(checksum_Fuelle_1 != sumaPresioneFuelle_1){
      presionFuelle_1kPa_2 = EEPROM.readInt(addressEEPROM_1kPa_2); //
      presionFuelle_2kPa_2 = EEPROM.readInt(addressEEPROM_2kPa_2); //
      checksum_Fuelle_2 = EEPROM.readInt(addressEEPROM_checksum_2_fuelle);
      sumaPresioneFuelle_2 = presionFuelle_1kPa_2 + presionFuelle_2kPa_2;
      if(checksum_Fuelle_2 == sumaPresioneFuelle_2){
      EEPROM.writeInt(addressEEPROM_1kPa_1, presionFuelle_1kPa_2);       //
      EEPROM.writeInt(addressEEPROM_2kPa_1, presionFuelle_2kPa_2);       //
      EEPROM.writeInt(addressEEPROM_checksum_1_fuelle, sumaPresioneFuelle_2);       //
      EEPROM.commit();

      }
    checkSumOK_2 = false;
    }else{
      checkSumOK_2 = true;
      Serial.println("CheckSum2: Valido ");
    }
    return(checkSumOK_2);
  }
//**************************************************************************************************
void leerCalibracionEEProm::mensajeLCD(void){
    boolean flagCalibracion;
    boolean calibracion;
    flagCalibracion = EEPROM.readInt(flagIntentoRecalibrar);
    imprimirLCDI2C imprimirLcd2(_objeto1LCD);
    imprimirLcd2.inicializarLCD(20, 4);

 //  if(datosCalibracionOk == false){
    if(flagCalibracion == 0){
      imprimirLcd2.imprimirLCDfijo("Recomponiendo       ",0, 0);
      imprimirLcd2.imprimirLCDfijo("calibracion y       ",0, 1);
      imprimirLcd2.imprimirLCDfijo("reiniciando         ",0, 2);
      delay(2000);
      EEPROM.writeInt(flagIntentoRecalibrar, 1);       ////Se indica en "uno" que si entro en Recupero de calibración
      EEPROM.commit();
      ESP.restart();  
    }else{
      imprimirLcd2.imprimirLCDfijo("No se puede corregir",0, 0);
      imprimirLcd2.imprimirLCDfijo("calibracion         ",0, 1);
      imprimirLcd2.imprimirLCDfijo("     CALIBRAR       ",0, 2);
     /* while(true){
        calibracion = digitalRead(pulsadorCalibracion);
        if(!calibracion)  // calibrarSensores1.calibrar();
        ;*/
      }   
      delay(3000);
    }     
