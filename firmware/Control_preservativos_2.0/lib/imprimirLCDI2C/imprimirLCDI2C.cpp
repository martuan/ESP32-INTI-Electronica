#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"

    imprimirLCDI2C::imprimirLCDI2C(LiquidCrystal_I2C objetoLCD){
        _objetoLCD = objetoLCD;
        
    }
 
//*********************************************************************************************
void imprimirLCDI2C::inicializarLCD(int numColum, int numFilas){
    _objetoLCD.begin(numColum, numFilas);
}
//*********************************************************************************************
void imprimirLCDI2C::imprimirMedicionLCD(String lineaMedicionLCD, int fila){    //Para facilitar la lectura durante el ensayo se actualiza la última medición en la última fila y la  
  static String lineaMedicionLCDanterior = "";          //medición anterior se corre hacia arriba
    _objetoLCD.setCursor(0, fila);
    _objetoLCD.print("                   ");
    _objetoLCD.setCursor(0, fila);
    _objetoLCD.print(lineaMedicionLCDanterior);
    _objetoLCD.setCursor(0, (fila + 1));
    _objetoLCD.print("                   ");
    _objetoLCD.setCursor(0, (fila + 1));
    _objetoLCD.print(lineaMedicionLCD);    
    lineaMedicionLCDanterior = lineaMedicionLCD; 
}
//***************************************************************************************
void imprimirLCDI2C::imprimirLCDfijo(String lineaMedicionLCD, int columna, int fila){

 //   _objetoLCD.setCursor(0, fila);
 //   _objetoLCD.print("                    ");
    _objetoLCD.setCursor(columna, fila);
    _objetoLCD.print(lineaMedicionLCD);      
}
//***************************************************************************************
void imprimirLCDI2C::mensajeInicialLCDcalibracion(int numeroSensor){
    imprimirLCDfijo("Con pulsador",0, 0);
    imprimirLCDfijo("CALIBRACION",0, 1);
    imprimirLCDfijo("seleccione sensor",0, 2);
    delay(5000);
    _objetoLCD.clear();
    imprimirLCDfijo("Sensor seleccionado:",0, 0);
    imprimirLCDfijo(String(numeroSensor),0, 1);              
    imprimirLCDfijo("Presionar pulsador",0, 2);         
    imprimirLCDfijo("INICIO para calibrar",0, 3);         
}
//**************************************************************************************************

void imprimirLCDI2C::mensajeLCDcalibracion(int muestraPorPunto, float puntoCalibracion){
      String linea_1 = String(muestraPorPunto);
      String linea_2 = "hasta: ";
      linea_1 += " Suba desde cero";
      linea_2 += String(puntoCalibracion);
      linea_2 += " kPa";
      imprimirLCDfijo(linea_1,0, 0);         
      imprimirLCDfijo(linea_2,0, 1);         
      imprimirLCDfijo("y presione",0, 2);         
      imprimirLCDfijo("INICIO",0, 3);         

}
