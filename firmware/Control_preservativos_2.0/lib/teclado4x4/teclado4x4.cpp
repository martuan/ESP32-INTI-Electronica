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
	{'7','8','9','O'},
	{'*','0','#','E'}
};
//***********Constructor****************************************
    teclado4x4::teclado4x4(PCF pcf1){
        _pcf1 = pcf1;
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
    if(letraLeida != 'z'){
 //   Serial.print("Tecla presionada: ");
 //   Serial.println(letraLeida);
    }

    return(letraLeida);
}
//*******************************************************************
char teclado4x4::maquinaEstadosTecla(void){
//    const uint8_t filasPines[cantFilas] = {4, 5, 6, 7};
//    const uint8_t columnasPines[cantFilas] = {0, 1, 2, 3};
    static uint8_t estado = EXISTE_TECLA;   //Entre estado y estado vuelve a la rutina principal, por lo que se necesita 
    static int filasPosicion;               //retener estas variables
    static int columnasPosicion;
    boolean flagTeclaValidada = false;
    uint8_t checkPortEstado;
    uint8_t i;
    uint8_t j;

    switch (estado)
    {
    case EXISTE_TECLA:
		flagTeclaValidada = false;
		for(i = 0; i < 4; i++){                 //Barre los pines 0 a 3 del multiplexor PCF8574, conectados a las filas del teclado
			_pcf1.digitalWrite(i, LOW);         //Pone en bajo el pin 'i' conectado a la fila del teclado 'i'. La tecla presionada se detecta por '0'
			for(j = 4; j < 8; j++){             //Lee los pines 4 a 7 del multiplexor, conectados a las columnas del teclado
                checkPortEstado = _pcf1.digitalRead(j);
				if(checkPortEstado == LOW){     //si hay tecla presionada
 
					filasPosicion = i;          //guarda la fila
					columnasPosicion = j;       //guarda la columna
					estado = VALIDA_TECLA;
                    _pcf1.write(0B11111111);    // pone en '1' todos los pines del multiplexor
					break;
				}
			}
            _pcf1.digitalWrite(i, HIGH);       //Vuelve a poner en '1' la fila que se barrió, independientemente si se detectó tecla o no.
		}	
	break;
    case VALIDA_TECLA:    
        flagTeclaValidada = false;   
		_pcf1.digitalWrite(filasPosicion, LOW);         //Pone en alto la fila que encontró previamente
		if(_pcf1.digitalRead(columnasPosicion) == LOW){ //si continúa la tecla presionada
			
			estado = ESPERA_LIBERACION_TECLA;
		}else{                                          //si la tecla presionada no persistió, vuelve a buscar
			estado = EXISTE_TECLA;
		}
	break;
    case ESPERA_LIBERACION_TECLA:
        flagTeclaValidada = false;
        checkPortEstado = _pcf1.digitalRead(columnasPosicion);
        if(checkPortEstado == HIGH){                    //si la tecla se liberó, sigue esperando
            estado = VALIDA_LIBERACION;
        }else{
            estado = ESPERA_LIBERACION_TECLA;
		}        
	break;
    case VALIDA_LIBERACION:
        checkPortEstado = _pcf1.digitalRead(columnasPosicion);   
        if(checkPortEstado == HIGH){                    //si se liberó, vuelve a buscar tecla presionada
		estado = EXISTE_TECLA;
            flagTeclaValidada = true;                   //levanta el flag de tecla validada      
            _pcf1.digitalWrite(filasPosicion, HIGH);    
        }else{                                          //si la tecla no se liberó, sigue esperando
            estado = VALIDA_LIBERACION;
        }

        break;
    default:
        break;
    }
    if(flagTeclaValidada){
//         Serial.print("Tecla presionada: ");
//        Serial.println(tablaTeclado[filasPosicion][columnasPosicion - 4]);
        return(tablaTeclado[filasPosicion][columnasPosicion - 4]);       
    }   
    else return('z');

}