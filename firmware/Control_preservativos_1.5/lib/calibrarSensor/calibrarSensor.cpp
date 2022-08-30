#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"
#include "calibrarSensor.h"
#include "hardware.h"

//    calibrarSensor::calibrarSensor(LiquidCrystal_I2C objetoLCD){
    calibrarSensor::calibrarSensor(LiquidCrystal_I2C objetoLCD){
  //     imprimirLCDI2C = _imprimirLCD1;
      _objetoLCD = objetoLCD;    
    }

    //*******************************************************
    void calibrarSensor::calibrar(void){
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
      int checkSum1_sensor1 = 0;
      int checkSum2_sensor1 = 0, checkSum1_sensor2 = 0, checkSum2_sensor2 = 0;
      boolean inicio = HIGH;
      boolean calibracion;
      boolean cambioSensor = LOW; 
      
 //     LiquidCrystal_I2C lcd2(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);  //PCF8574_ADDR_A21_A11_A01 -> dicección I2C del display físico  0X27
      imprimirLCDI2C imprimirLcd2(_objetoLCD);
      imprimirLcd2.inicializarLCD(20, 4);
      //imprimirLCDI2C imprimirLCD1;

     
   //   imprimirLCDI2C.mensajeInicialLCDcalibracion(numeroSensor);
      imprimirLcd2.mensajeInicialLCDcalibracion(numeroSensor);
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
//        _imprimirLCDI2C.imprimirLCDfijo(String(numeroSensor),5, 1);    
        imprimirLcd2.imprimirLCDfijo(String(numeroSensor),5, 1);    
        Serial.println("Pulse INICIO para comenzar calibración");    
        cambioSensor = LOW;       
      }
      inicio = digitalRead(pulsadorInicio);
     }
     delay(1000);
     if(numeroSensor == 1){
       while(puntoCalibracion < 10){
        while(muestraPorPunto < 1){
          Serial.print(muestraPorPunto + 1);
          Serial.print("º. Desde Cero suba la presión hasta ");
          Serial.print(valorCalibracion[puntoCalibracion]);
          Serial.println("kPa  y presione INICIO");
//          _imprimirLCDI2C.mensajeLCDcalibracion((muestraPorPunto + 1), valorCalibracion[puntoCalibracion]);
          imprimirLcd2.mensajeLCDcalibracion((muestraPorPunto + 1), valorCalibracion[puntoCalibracion]);
          inicio = digitalRead(pulsadorInicio);
          while(inicio == HIGH){
            inicio = digitalRead(pulsadorInicio);
            }
          valorAcumulado = analogRead(sensorPresion1) + valorAcumulado;
          promedio = valorAcumulado / 1;
          muestraPorPunto ++;
          delay(600);
          }
        muestraPorPunto = 0;
        valorDigitalCalibrado[puntoCalibracion] = promedio;
        valorAcumulado = 0;
        puntoCalibracion ++;
       }
//       _imprimirLCDI2C.imprimirLCDfijo(" Sensor 1          ",0, 2); 
       imprimirLcd2.imprimirLCDfijo(" Sensor 1          ",0, 2); 
       
        m = int((valorDigitalCalibrado[1] - valorDigitalCalibrado[0])/0.25);
  //      m = int((valorDigitalCalibrado[1] - valorDigitalCalibrado[0])/(valorCalibracion[1] - valorCalibracion[0]));
        b = int(valorDigitalCalibrado[0] - m*valorCalibracion[0]);
        checkSum1_sensor1 = m + b;
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
  
        EEPROM.writeInt(addressEEPROM, m);       //Escribir en EEPROM m en posicion addressEEPROM
        EEPROM.commit();
        addressEEPROM += sizeof(m); //update address value
        EEPROM.writeInt(addressEEPROM, b);       //Escribir en EEPROM b en posicion addressEEPROM
        EEPROM.commit();
        addressEEPROM += sizeof(b); //update address value
        cont ++;
        checkSum1_sensor1 = m + b + checkSum1_sensor1;
 //       checkSum1_sensor1 = 30;
       }       
       
  // ** checksum *****
     /*   Serial.print("Valor m: ");
        Serial.println(m);
        Serial.print("Valor b: ");
        Serial.println(b);*/
//        EEPROM.writeInt(72, checkSum1_sensor1);       //Escribir la suma de todos los m y b en posicion addressEEPROM
        Serial.print("Valor checkSum: ");
        EEPROM.writeInt(addressEEPROM, checkSum1_sensor1);       //Escribir la suma de todos los m y b en posicion addressEEPROM
        EEPROM.commit();

     }
     if(numeroSensor == 2){
        Serial.println("Suba la presión a 1kPa  y presione INICIO");
//        _imprimirLCDI2C.mensajeLCDcalibracion(1, 1);
        imprimirLcd2.mensajeLCDcalibracion(1, 1);

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
//        _imprimirLCDI2C.mensajeLCDcalibracion(1, 2);
        imprimirLcd2.mensajeLCDcalibracion(1, 2);
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
//       _imprimirLcd1.imprimirLCDfijo(" Sensor 2          ",0, 2);
       imprimirLcd2.imprimirLCDfijo(" Sensor 2          ",0, 2);
     }
//     _imprimirLCDI2C.imprimirLCDfijo("                    ",0, 1);
//     _imprimirLCDI2C.imprimirLCDfijo(" Fin de Calibracion",0, 0);
//     _imprimirLCDI2C.imprimirLCDfijo("Reinicie el equipo  ",0, 3);
     imprimirLcd2.imprimirLCDfijo("                    ",0, 1);
     imprimirLcd2.imprimirLCDfijo(" Fin de Calibracion",0, 0);
     imprimirLcd2.imprimirLCDfijo("Reinicie el equipo  ",0, 3);

    }