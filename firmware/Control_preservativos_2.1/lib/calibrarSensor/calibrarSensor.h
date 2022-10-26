#ifndef calibrarSensor_h
#define calibrarSensor_h

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"
#include <hardware.h>
#include <EEPROM.h>
//#include "teclado4x4.h"
//#include "pu2clr_pcf8574.h"
//PCF pcf;      //Maneja el I2C para el teclado 4x4

class calibrarSensor   {
    public:
        calibrarSensor(LiquidCrystal_I2C objetoLCD);
        void calibrar(char letraLeida);
  
    private:
     LiquidCrystal_I2C _objetoLCD;
};

#endif