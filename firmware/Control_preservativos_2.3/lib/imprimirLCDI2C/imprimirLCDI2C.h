#ifndef imprimirLCDI2C_h
#define imprimirLCDI2C_h

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <hardware.h>

class imprimirLCDI2C   {
    public:
        imprimirLCDI2C(LiquidCrystal_I2C objetoLCD);
        void inicializarLCD(int numColum, int numFilas);
        void imprimirMedicionLCD(String lineaMedicionLCD, int fila);
        void mensajeInicialLCDcalibracion(int numeroSensor);
        void mensajeLCDcalibracion(int muestraPorPunto, float puntoCalibracion);
        void imprimirLCDfijo(String lineaMedicionLCD, int columna, int fila);
        void limpiarLCD(void);
  
    private:
        LiquidCrystal_I2C _objetoLCD;
};

#endif