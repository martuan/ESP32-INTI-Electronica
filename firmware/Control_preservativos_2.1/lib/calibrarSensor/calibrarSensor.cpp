#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"
#include "calibrarSensor.h"
#include "hardware.h"
//#include "teclado4x4.h"
//#include "pu2clr_pcf8574.h"
//PCF pcf;      //Maneja el I2C para el teclado 4x4


//    calibrarSensor::calibrarSensor(LiquidCrystal_I2C objetoLCD){
    calibrarSensor::calibrarSensor(LiquidCrystal_I2C objetoLCD){
  //     imprimirLCDI2C = _imprimirLCD1;
      _objetoLCD = objetoLCD; 
     // _objetoTeclado1 = objetoTeclado1;  
            //teclado4x4 teclado1(pcf);             //Teclado 4x4 conectado al I2C a travez delPCF8574
 
    }

    //*******************************************************
    void calibrarSensor::calibrar(char letraLeida){
      static int numeroSensor = 0;
      static int numeroSensorSeleccionadoAnterior = 3;
      int puntoCalibracion = 0;
      static int valorAcumulado = 0;
      static int promedio;
      static int valorDigitalCalibrado[10];
      const float valorCalibracion[10] = {0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00, 2.25, 2.50};
      int m = 0;
      int b = 0;
      int muestraPorPunto = 0;
      int addressEEPROM = 0;
      int cont = 0;
      int valorPresionFuelle;
      int checkSum1_sensor1 = 0;
      int checkSum2_sensor1 = 0, checkSum1_sensor2 = 0, checkSum2_sensor2 = 0;
      int sumaValoresPresionesFuelle = 0;
      boolean inicio = HIGH;
      boolean calibracion;
      boolean cambioSensor = LOW; 
      static boolean primerIngreso = true;
      static boolean habilitaCalibracion = false;
      boolean grabarValoresSensor1 = false;
      static boolean primerIngresoCalibracionSensor1 = true;
      boolean calibracionFinalizada = false;
//     LiquidCrystal_I2C lcd2(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);  //PCF8574_ADDR_A21_A11_A01 -> dicección I2C del display físico  0X27
      static imprimirLCDI2C imprimirLcd2(_objetoLCD);
      static char letraLeidaAnterior = '3';
      static boolean _1kPaCalibrado = false;

        if(letraLeidaAnterior != letraLeida){
          letraLeidaAnterior = letraLeida;
          Serial.print("letra leida: ");
          Serial.println(letraLeida);
        }
        if(primerIngreso){
//          imprimirLCDI2C imprimirLcd2(_objetoLCD);
          imprimirLcd2.inicializarLCD(20, 4);

          Serial.println("Elija numero de sensor: ");
          Serial.print("Sensor seleccionado: ");
          Serial.println(numeroSensor);
          Serial.println("Pulse # para comenzar calibración"); 
          primerIngreso = false;
          imprimirLcd2.mensajeInicialLCDcalibracion(numeroSensor);
        }

        if(letraLeida == '1') numeroSensor = 1;
        if(letraLeida == '2') numeroSensor = 2;

        if(numeroSensorSeleccionadoAnterior != numeroSensor){
          Serial.print("Sensor seleccionado: ");
          Serial.println(numeroSensor);
          imprimirLcd2.imprimirLCDfijo(String(numeroSensor),10, 3); 
          numeroSensorSeleccionadoAnterior = numeroSensor;   
        }

        if(letraLeida == 'A'){
          imprimirLcd2.limpiarLCD();
          habilitaCalibracion = true;
        } 
  //      Serial.println("Pulse INICIO para comenzar calibración");    
  //      cambioSensor = LOW;       
  
//      inicio = digitalRead(pulsadorInicio);
 //    }
     delay(100);
     //*******************************************************************
     if((numeroSensor == 1) && habilitaCalibracion){
//       while(puntoCalibracion < 10){
//        while(muestraPorPunto < 1){
      if((letraLeida == '#') || primerIngresoCalibracionSensor1){
     // if(letraLeida == '#'){
        primerIngresoCalibracionSensor1 = false;
        if(puntoCalibracion < 10){
          if(muestraPorPunto < 1){
            Serial.print(muestraPorPunto + 1);
            Serial.print("º. Desde Cero suba la presión hasta ");
            Serial.print(valorCalibracion[puntoCalibracion]);
            Serial.println("kPa  y presione #");
  //          _imprimirLCDI2C.mensajeLCDcalibracion((muestraPorPunto + 1), valorCalibracion[puntoCalibracion]);
            imprimirLcd2.mensajeLCDcalibracion((muestraPorPunto + 1), valorCalibracion[puntoCalibracion]);
           /* inicio = digitalRead(pulsadorInicio);
            while(inicio == HIGH){
              inicio = digitalRead(pulsadorInicio);
              }*/
            valorAcumulado = analogRead(sensorPresion1) + valorAcumulado;
            promedio = valorAcumulado / 1;
            muestraPorPunto ++;
            delay(100);
            }
          muestraPorPunto = 0;
          valorDigitalCalibrado[puntoCalibracion] = promedio;
          valorAcumulado = 0;
          puntoCalibracion ++;
        }
          if(puntoCalibracion == 10)  grabarValoresSensor1 = true;
  //       _imprimirLCDI2C.imprimirLCDfijo(" Sensor 1          ",0, 2); 
          if(grabarValoresSensor1){
            imprimirLcd2.imprimirLCDfijo(" Sensor 1          ",0, 2);        
            m = int((valorDigitalCalibrado[1] - valorDigitalCalibrado[0])/0.25);
      //      m = int((valorDigitalCalibrado[1] - valorDigitalCalibrado[0])/(valorCalibracion[1] - valorCalibracion[0]));
            b = int(valorDigitalCalibrado[0] - m*valorCalibracion[0]);
            checkSum1_sensor1 = m + b;
            //Se guadan las constantes de calibración como enteros (4By c/u)
            EEPROM.writeInt(addressEEPROM, m);       //Escribir en EEPROM m en posición 0
            EEPROM.writeInt(addressEEPROM + inicioTabla2, m);       //Se duplica la tabla por seguridad       
            EEPROM.commit();
            addressEEPROM += sizeof(m); //update address value
            EEPROM.writeInt(addressEEPROM, b);       //Escribir en EEPROM b en posición 4
            EEPROM.writeInt(addressEEPROM + inicioTabla2, b);       //Se duplica la tabla por seguridad       
            EEPROM.commit();
            addressEEPROM += sizeof(b); //update address value
      
          while(cont<9){
      //      m = int((valorDigitalCalibrado[cont+1] - valorDigitalCalibrado[cont])/(valorCalibracion[cont+1] - valorCalibracion[cont]));
            m = int((valorDigitalCalibrado[cont+1] - valorDigitalCalibrado[cont])/0.25);
            b = int(valorDigitalCalibrado[cont] - m*valorCalibracion[cont]);
      
            EEPROM.writeInt(addressEEPROM, m);       //Escribir en EEPROM m en posicion addressEEPROM
            EEPROM.writeInt(addressEEPROM + inicioTabla2, m);       //Se duplica la tabla por seguridad       
            EEPROM.commit();
            addressEEPROM += sizeof(m); //update address value
            EEPROM.writeInt(addressEEPROM, b);       //Escribir en EEPROM b en posicion addressEEPROM
            EEPROM.writeInt(addressEEPROM + inicioTabla2, b);       //Se duplica la tabla por seguridad       
            EEPROM.commit();
            addressEEPROM += sizeof(b); //update address value
            cont ++;
            checkSum1_sensor1 = m + b + checkSum1_sensor1;
            }       
          
      // ** checksum *****
            Serial.print("Valor checkSum: ");
            EEPROM.writeInt(addressEEPROM, checkSum1_sensor1);       //Escribir la suma de todos los m y b en posicion addressEEPROM
            EEPROM.writeInt(addressEEPROM + inicioTabla2, checkSum1_sensor1);       //Se duplica la tabla por seguridad       
            EEPROM.commit();
            grabarValoresSensor1 = false;
            calibracionFinalizada = true;
          }

        }
     }

     if((numeroSensor == 2) && habilitaCalibracion){       //Como no interesa interpolar, solo se guardan los valoras digitales de las dos presiones sin calcular b ni m
        Serial.println("Suba la presión a 1kPa  y presione INICIO");
//        _imprimirLCDI2C.mensajeLCDcalibracion(1, 1);
        imprimirLcd2.mensajeLCDcalibracion(1, 1);

        inicio = HIGH;          
/*        while(inicio == HIGH){
          inicio = digitalRead(pulsadorInicio); 
          }*/
        if((letraLeida == '#') && (_1kPaCalibrado == false)) {
          valorPresionFuelle = analogRead(sensorPresion2);
          EEPROM.writeInt(addressEEPROM_1kPa_1, valorPresionFuelle);       //Escribir en EEPROM en posicion addressEEPROM
          EEPROM.writeInt(addressEEPROM_1kPa_2, valorPresionFuelle);       //(para recuperación por si falla el primero)
          EEPROM.commit();
          sumaValoresPresionesFuelle = valorPresionFuelle;
          Serial.print("El valor para 1kPa es: ");         
          Serial.println(valorPresionFuelle);
          delay(800);
          Serial.println("Suba la presión a 2kPa  y presione #");
  //        _imprimirLCDI2C.mensajeLCDcalibracion(1, 2);
          imprimirLcd2.mensajeLCDcalibracion(1, 2);  
          letraLeida = 'z';
          _1kPaCalibrado = true;
        }  

/*        inicio = HIGH;          
        while(inicio == HIGH){
          inicio = digitalRead(pulsadorInicio);          
          }*/
         if(letraLeida == '#'){
          valorPresionFuelle = analogRead(sensorPresion2);
          EEPROM.writeInt(addressEEPROM_2kPa_1, valorPresionFuelle);       //Escribir en EEPROM  en posicion addressEEPROM
          EEPROM.writeInt(addressEEPROM_2kPa_2, valorPresionFuelle);       //(para recuperación por si falla el primero)
          sumaValoresPresionesFuelle = valorPresionFuelle + sumaValoresPresionesFuelle;
          EEPROM.writeInt(addressEEPROM_checksum_1_fuelle, sumaValoresPresionesFuelle);       //Escribir checksum 1 del sensor fuelle
          EEPROM.writeInt(addressEEPROM_checksum_2_fuelle, sumaValoresPresionesFuelle);       //Escribir checksum 2 del sensor fuelle (para recuperación por si falla el primero)
          EEPROM.commit();
          
          Serial.print("El valor para 2kPa es: ");         
          Serial.println(valorPresionFuelle);
          delay(100);                   
  //       _imprimirLcd1.imprimirLCDfijo(" Sensor 2          ",0, 2);
        imprimirLcd2.imprimirLCDfijo(" Sensor 2          ",0, 2);
         }  
     }

if(calibracionFinalizada){
    imprimirLcd2.imprimirLCDfijo("                    ",0, 1);
    imprimirLcd2.imprimirLCDfijo(" Fin de Calibracion",0, 0);
    imprimirLcd2.imprimirLCDfijo("Reinicie el equipo  ",0, 3);
    EEPROM.writeInt(flagIntentoRecalibrar, 0);       //Se indica en "cero" que no entro en Recupero de calibración
    EEPROM.commit();
}

    }