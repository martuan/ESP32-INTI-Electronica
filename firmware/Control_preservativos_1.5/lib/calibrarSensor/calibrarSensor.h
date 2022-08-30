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
//        calibrarSensor(void);
        void calibrar(void);
       /* void inicializarLCD(int numColum, int numFilas);
        void imprimirMedicionLCD(String lineaMedicionLCD, int fila);
        void mensajeInicialLCDcalibracion(int numeroSensor);
        void mensajeLCDcalibracion(int muestraPorPunto, float puntoCalibracion);
        void imprimirLCDfijo(String lineaMedicionLCD, int columna, int fila);*/
  
    private:
     //   imprimirLCDI2C _imprimirLCDI2C;
     LiquidCrystal_I2C _objetoLCD;
};

#endif