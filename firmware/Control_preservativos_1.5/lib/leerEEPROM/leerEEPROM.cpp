#include <Arduino.h>
//#include <EEPROM.h>
#include "leerEEPROM.h"
//#include "hardware.h"
    // :: <- Acceso a ambito
   leerEEPROM::leerEEPROM(int direccionEEPROM_1kPa, int direccionEEPROM_2kPa){   
    _direccionEEPROM_1kPa = direccionEEPROM_1kPa;
    _direccionEEPROM_2kPa = direccionEEPROM_2kPa;
   }

//   boolean leerEEPROM::obtenerValores(){
   void leerEEPROM::obtenerValores(){
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
        readId = EEPROM.readInt(_direccionEEPROM_1kPa); 
        Serial.println(readId);
        readId = EEPROM.readInt(_direccionEEPROM_2kPa); 
        Serial.println(readId); 
        
     //  return(false);
   }