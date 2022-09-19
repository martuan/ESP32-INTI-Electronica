#include <Arduino.h>
#include "teclado4x4.h"
#include "pu2clr_pcf8574.h"

#define EXISTE_TECLA 1
#define VALIDA_TECLA 2
#define ESPERA_LIBERACION_TECLA 3
#define VALIDA_LIBERACION 4
#define cantFilas 4
#define cantColumnas 4

char tablaTeclado[cantFilas][cantColumnas] =    // Scan codes de las teclas
{
	{'1','2','3','A'},	
	{'4','5','6','B'},
	{'7','8','9','C'},
	{'*','0','#','D'}
};



    teclado4x4::teclado4x4(PCF pcf1){
//        _i2cAddress = i2cAddress;
        _pcf1 = pcf1;
//        _pcf.setup(_i2cAddress);        
 //       _pcf1.write(0B11111111);  // Turns all pins/ports HIGH

    }
//******************************************************************
char teclado4x4::obtenerTecla(void){
    char letraLeida = 'a'; 
/*
    uint8_t j = 0;
    for (uint8_t i = 4 ; i < 8; i++ ) {
        _pcf1.digitalWrite(i-4,LOW );
        delay(2);
         //  Serial.print("\nDentro del For, i: ");
         //   Serial.print(i);
        if ( _pcf1.digitalRead(i) == LOW) {
            Serial.print("\nThe port ");
            Serial.print(i);
         //   Serial.print(" is LOW (button pressed)"); 
            _pcf1.write(0B11111111);  // Turns all pins/ports HIGH
            delay(10); 
            j = i; 
            i = 8;
        }else{
            j = 0;
        }
       // return(i);
               _pcf1.digitalWrite(i-4,HIGH);

    }*/

    letraLeida = this->teclado4x4::maquinaEstadosTecla();
    return(letraLeida);
}
//*******************************************************************
char teclado4x4::maquinaEstadosTecla(void){
    static uint8_t estado = EXISTE_TECLA;
    const uint8_t filasPines[cantFilas] = {4, 5, 6, 7};
    const uint8_t columnasPines[cantFilas] = {0, 1, 2, 3};
    static int filasPosicion;
    static int columnasPosicion;
    boolean flagTeclaValidada = false;
    uint8_t checkPortEstado;
    uint8_t i;
    uint8_t j;

    switch (estado)
    {
    case EXISTE_TECLA:
		flagTeclaValidada = false;
//		Serial.println("EXISTE_TECLA");
//		for(uint8_t i = 0; i < cantFilas; i++){//Barre las filas
		for(i = 0; i < 4; i++){//Barre las filas

//			//_pcf1.digitalWrite(filasPines[i], LOW);//Pone en bajo la fila 'i'
			_pcf1.digitalWrite(i, LOW);//Pone en bajo la fila 'i'
			//Serial.println(filasPines[i]);
//			for(uint8_t j = 0; j < cantColumnas; j++){//Lee las columnas
			for(j = 4; j < 8; j++){//Lee las columnas

//				if(_pcf1.digitalRead(columnasPines[j]) == LOW){//si hay tecla presionada
                checkPortEstado = _pcf1.digitalRead(j);
				if(checkPortEstado == LOW){//si hay tecla presionada
 
					filasPosicion = i;//guarda la fila
					columnasPosicion = j;//guarda la columna
					estado = VALIDA_TECLA;
            _pcf1.write(0B11111111);  // Turns all pins/ports HIGH

					//Serial.println("paso");
					break;
				}
				/*
				else{//si no hay tecla presionada
					//estado = EXISTE_TECLA;// continúa buscando tecla
				}
				*/
			}
            _pcf1.digitalWrite(i, HIGH);//Pone en bajo la fila 'i'
		}	
	
	break;
    case VALIDA_TECLA:    
        flagTeclaValidada = false;   
//pp		_pcf1.digitalWrite(filasPines[filasPosicion], LOW);//Pone en alto la fila que encontró previamente
		_pcf1.digitalWrite(filasPosicion, LOW);//Pone en alto la fila que encontró previamente
//		Serial.println("VALIDA_TECLA");
		
//		if(_pcf1.digitalRead(columnasPines[columnasPosicion]) == LOW){//si continúa la tecla presionada
		if(_pcf1.digitalRead(columnasPosicion) == LOW){//si continúa la tecla presionada
			
			estado = ESPERA_LIBERACION_TECLA;
		}else{//si la tecla presionada no persistió, vuelve a buscar
			estado = EXISTE_TECLA;
		}
 
	break;
    case ESPERA_LIBERACION_TECLA:
        flagTeclaValidada = false;
        checkPortEstado = _pcf1.digitalRead(columnasPosicion);
        if(checkPortEstado == HIGH){//si la tecla se liberó, sigue esperando
            estado = VALIDA_LIBERACION;
//			Serial.println("Proximo estado = VALIDA_LIBERACION");
        }else{
            estado = ESPERA_LIBERACION_TECLA;
		}
        
	break;
    case VALIDA_LIBERACION:
//		Serial.println("VALIDA_LIBERACION");
//        if(digitalRead(columnasPines[columnasPosicion]) == HIGH){//si se liberó, vuelve a buscar tecla presionada
        checkPortEstado = _pcf1.digitalRead(columnasPosicion);   
        if(checkPortEstado == HIGH){//si se liberó, vuelve a buscar tecla presionada
//			Serial.println("Proximo estado = EXISTE_TECLA");
		estado = EXISTE_TECLA;
            flagTeclaValidada = true;//levanta el flag de tecla validada      
            _pcf1.digitalWrite(filasPosicion, HIGH);//Pone en bajo la fila 'i'
        }else{//si la tecla no se liberó, sigue esperando
            estado = VALIDA_LIBERACION;
        }

        break;

    default:
        break;
    }
    if(flagTeclaValidada){
         Serial.print("Tecla presionada: ");
        Serial.println(tablaTeclado[filasPosicion][columnasPosicion - 4]);
        return(tablaTeclado[filasPosicion][columnasPosicion - 4]);       
    }   
    else return('z');

}