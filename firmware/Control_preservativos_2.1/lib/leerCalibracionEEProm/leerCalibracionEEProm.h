#ifndef leerCalibracionEEProm_h
#define leerCalibracionEEProm_h

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"
#include <hardware.h>
#include <EEPROM.h>


class leerCalibracionEEProm   {
    public:
        leerCalibracionEEProm(LiquidCrystal_I2C objeto1LCD);
//        boolean verificarCalibracion(void);     //Verifica la calibración del sensor de presión en el preservativo
        void verificarCalibracion(void);     //Verifica la calibración del sensor de presión en el preservativo
      //  void mensajeLCD(void);

    private:
        boolean verificarCalibracionTalba2(void);
        void recomponerTabla1(void);
        boolean verificarCalibracionSensorFuelle(void);
        void mensajeLCD(void);
        void prueba(void);
        LiquidCrystal_I2C _objeto1LCD;
       // imprimirLCDI2C imprimirLcd2(LiquidCrystal_I2C _objeto1LCD);
};

#endif