#include <Arduino.h>
#include "tiempoCumplido.h"
    // :: <- Acceso a ambito
   tiempoCumplido::tiempoCumplido(int centeSeg){    //Recibe centécimas de segundos

        _centeSeg = centeSeg;            
   }   
    
    boolean tiempoCumplido::calcularTiempo(boolean ejecutado){

        const int intervalo = 10;      // Cantidad de milisegundos
        static boolean tiempoCumplido;
        static unsigned long previousMillis = 0;
        static unsigned long currentMillis = 0;
        static int intervalosContados = 0;
            
        if(ejecutado){
            intervalosContados = 0;
        }

        currentMillis = millis();
        if (currentMillis - previousMillis >= intervalo) {
            previousMillis = currentMillis; 
            intervalosContados++; 
        }  
        if(intervalosContados >= _centeSeg){
            tiempoCumplido = true;
        }else{
            tiempoCumplido = false;
        }
        if(tiempoCumplido == true)  return(true);
        else  return(false);
    }