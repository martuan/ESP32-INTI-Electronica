#include <Arduino.h>
#include "teclado4x4.h"
#include "pu2clr_pcf8574.h"

    teclado4x4::teclado4x4(PCF pcf1){
//        _i2cAddress = i2cAddress;
        _pcf1 = pcf1;
//        _pcf.setup(_i2cAddress);        
 //       _pcf1.write(0B11111111);  // Turns all pins/ports HIGH

    }
//******************************************************************
int teclado4x4::obtenerTecla(void){
    uint8_t j = 0;
    for (uint8_t i = 4 ; i < 8; i++ ) {
        _pcf1.digitalWrite(i-4,LOW );
        delay(2);
//            Serial.print("\nDentro del For, i: ");
//            Serial.print(i);
        if ( _pcf1.digitalRead(i) == LOW) {
         //   Serial.print("\nThe port ");
         //   Serial.print(i);
         //   Serial.print(" is LOW (button pressed)"); 
            _pcf1.write(0B11111111);  // Turns all pins/ports HIGH
            delay(10); 
            j = i; 
            i = 8;
        }else{
            j = 0;
        }
       // return(i);
    }
    return(j);
}