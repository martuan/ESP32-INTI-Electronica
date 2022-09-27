#ifndef calibrarSensor_h
#define calibrarSensor_h

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"
#include <hardware.h>
#include <EEPROM.h>

class calibrarSensor   {
    public:
        calibrarSensor(LiquidCrystal_I2C objetoLCD);
        void calibrar(void);
  
    private:
     LiquidCrystal_I2C _objetoLCD;
};

#endif