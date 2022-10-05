#include <Arduino.h>
#include "defines.h"
#include "parser.h"
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"
//#include <SD.h>
//#include "manejadorSD.h"
#include <EEPROM.h>

LiquidCrystal_I2C lcd3(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);  //PCF8574_ADDR_A21_A11_A01 -> dicección I2C del display físico  0X27
imprimirLCDI2C imprimirLcd3(lcd3);

//tarjetaSD2manejadorSD tarjetaSD2(CS);

static boolean crearDirectorio = false;
static String otActual;
//static unsigned char otActualChar[20];

//Mensajes del estado Menú Principal.
unsigned char msgMPpalF1[] = "1- OT ACTUAL        ";
unsigned char msgMPpalF2[] = "2- Otra OT          ";
//Mensajes del estado Menú de Selección.
unsigned char msgMSeleccion[] = "A Menu Seleccion";
//Mensajes del estado Menú de Edición.
unsigned char msgMEdicionF1[] = "Ingrese Nro OT      ";
unsigned char msgMEdicionF2[] = "y presione A        ";
unsigned char msgMEdicionF3[] = "* borrar - B Volver ";
unsigned char msgMEdicionF4[] = "OT:228-             ";			            
//Mensajes del estado Menú Inicio Ensayo.
unsigned char msgMIniciEnsF1[] = "Presione A para     ";
unsigned char msgMIniciEnsF2[] = "comenzar el ensayo  ";
//Mensajes del estado Menú Ensayo.
unsigned char msgMEnsayoF1[] = "OT                  ";
unsigned char msgMEnsayoF2[] = "Ensayo Nº           ";
unsigned char msgMEnsayoF3[] = "xxxxxx       xxxxxx ";
unsigned char msgMEnsayoF4[] = "B-Suspender         ";
//Mensajes del estado Menú Suspende Ensayo.
unsigned char msgMSuspEnsF1[] = "A-Suspende Ensayo   ";
unsigned char msgMSuspEnsF2[] = "B-Continua Ensayo   ";
//Mensajes del estado errorNroOT
unsigned char msgMerrorNroOTF1[] = "Complete Nro OT     ";
unsigned char msgMerrorNroOTF2[] = "A-para continuar    ";
unsigned char nroOT[CANTDIGITOS+1]; //Array que contiene el nro de OT actual en ASCII. Es +1 para el NULL

unsigned char lineaVacia[] = "                    ";
unsigned char msgAaceptar[] = " A - Aceptar        ";
unsigned char msgBvolver[] = "  B - Volver        ";
unsigned char msgINTICaucho[] = "   INTI - CAUCHO  ";

// variables para el parser ------------------------------------------------------
unsigned char inParser;			// dato de entrada al parser
unsigned char estadoActual; 	// estado del parser
unsigned char estadoAnterior;	// utilizado para los casos en que se 
								// necesita saber de donde viene.
//Variables para edición en el display.
unsigned char numPosIni;	//Posición inicial del cursor en el campo de edición.
unsigned char numPosActual; //Posición actual del cursor en el campo de edición.
unsigned char contadorCaract;
//String directorio1 = ""; 	

//**************************************************************************

//**************************************************************************
//
// Funciones para máquina de estados. (Parser)
//
//**************************************************************************
void Parser(void)
{
	//if(!inicializoLCD3){	}
  	unsigned char codigoFamilia;
	
  	codigoFamilia = OTRO;   // por ahora.
	estadoActual = estadoActual & ~MASKINESTABLE; //Quita posible inestabilidad.

	if( ((inParser >= '0') && (inParser <= '9')) || inParser == COD_OT_COMUN  
  		|| inParser == COD_INTERLABORATORIO )	
 	{
		codigoFamilia = EDICION_OT;
	}
  
  	ptrEstadoParser = dirEstadoParser[estadoActual];

	while ( (ptrEstadoParser->entrada != inParser) && 
			(ptrEstadoParser->entrada != DEFAULT)  &&
		 	(ptrEstadoParser->entrada != codigoFamilia) )
	{
		ptrEstadoParser++;//si no es la entrada buscada pasa a la otra.
	}
	estadoActual = ptrEstadoParser->proxEstado; //actualiza estado
	
	(*ptrEstadoParser->Accion) (); 			//realiza acción
}

void IniParser(void)
{

	estadoActual = M_PPAL;
	AMPpal();
	//inParser = CR;
	imprimirLcd3.inicializarLCD(20, 4);
	//inicializoLCD3 = true;
	imprimirLcd3.imprimirLCDfijo((const char *)msgINTICaucho,0, 0);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 1);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMPpalF1,0, 2);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMPpalF2,0, 3);

}
//
// Nada()
//
void Nada(void)
{
	//No hace nada.
}

void AMSeleccion(void)
{
	String directorio1; 	
	Serial.println();
	Serial.println((const char *)msgMSeleccion);

	directorio1 = EEPROM.readString(addressEEPROM_ultimaOT);
	otActual = directorio1;
	imprimirLcd3.imprimirLCDfijo(directorio1,3, 0);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 1);
	imprimirLcd3.imprimirLCDfijo((const char *) msgAaceptar,0, 2);
	imprimirLcd3.imprimirLCDfijo((const char *) msgBvolver,0, 3);
	Serial.print("directorio1: ");
	Serial.println(directorio1);

}
void AMInicioEns(void)
{
	Serial.println();
	Serial.println((const char *)msgMIniciEnsF1);
	Serial.println((const char *)msgMIniciEnsF2);
//	imprimirEnLCD((const char *)msgMIniciEnsF1,0, 0);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMIniciEnsF1,0, 0);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMIniciEnsF2,0, 1);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 2);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 3);
	crearDirectorio = true;
}
void AMPpal(void)
{
	Serial.println();
	Serial.println((const char *)msgMPpalF1);
	Serial.println((const char *)msgMPpalF2);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMPpalF2,0, 3);
	imprimirLcd3.imprimirLCDfijo((const char *)msgINTICaucho,0, 0);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 1);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMPpalF1,0, 2);
	crearDirectorio = false;
}

void AMEdicion(void)
{
	unsigned char i;
//	String directorio1;

	numPosIni = COLINICIAL; //Apunta a la columna a editar.
	numPosActual = 0;
	contadorCaract = 0;	

	for ( i = numPosIni; i < CANTCOLDISP-1; i++)//Limpia el mensaje.
	{
		msgMEdicionF4[i] = ' ';	
//		msgMEdicionF4[i] = numeroOT[i - numPosIni];
	}
	for ( i = 0; i < CANTDIGITOS; i++)//Limpia el mensaje.
	{
		nroOT[i] = ' ';	
	}

	nroOT[CANTDIGITOS] = 0x00;
	imprimirLcd3.imprimirLCDfijo((const char *)msgMEdicionF1,0, 0);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMEdicionF2,0, 1);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMEdicionF3,0, 2);
	imprimirLcd3.imprimirLCDfijo((const char *)msgMEdicionF4,0, 3);
}

void EditaOT (void)
{
	String cifraNroOT;
	String pruebaEEPROM = "";
//	if((inParser != COD_OT_COMUN) && (inParser != COD_INTERLABORATORIO) && (numPosActual < CANTDIGITOS))
	if((inParser == COD_OT_COMUN) || (inParser == COD_INTERLABORATORIO))
	{
		Serial.print("Num pos actual: ");
		Serial.println(numPosActual);
	if(numPosActual == (CANTDIGITOS -1 )){
			msgMEdicionF4[numPosActual+numPosIni] = inParser; //Agrega el caracter al mensaje.
			nroOT[numPosActual] = inParser; //Arma string para usar en el nombre del archivo.
			numPosActual = CANTDIGITOS;	//
		
			cifraNroOT = String((const char *)msgMEdicionF4);
	EEPROM.writeString(addressEEPROM_ultimaOT, cifraNroOT);
	EEPROM.commit();
	imprimirLcd3.imprimirLCDfijo(cifraNroOT,0, 3);
	pruebaEEPROM = EEPROM.readString(addressEEPROM_ultimaOT);
//		Serial.print("String escrito en EEPROM: ");
//		Serial.println(pruebaEEPROM);

		}
	}
	else // El caracter es COD_OT_COMUN o COD_INTERLABORATORIO
	{
		if(numPosActual < CANTDIGITOS-1) //Estos caracteres pueden estar SOLO al final del nro de OT.
		{
			msgMEdicionF4[numPosActual+numPosIni] = inParser; //Agrega el caracter al mensaje.
			nroOT[numPosActual] = inParser; //Arma string para usar en el nombre del archivo.
		//	Serial.print("Num pos actual: ");
		//	Serial.println(numPosActual);

			numPosActual++;	//Incrementa posición del cursor dentro del campo.
		}
	}
	contadorCaract++; //El nroOT[] debe tener ingrezado todos los caracterres (CANTDIGITOS).

	cifraNroOT = String((const char *)msgMEdicionF4);
	imprimirLcd3.imprimirLCDfijo(cifraNroOT,0, 3);

}

void RetrocedeCursor (void)
{
	String cifraNroOT;
	
	if( numPosActual > 0 ){
		numPosActual--;	//Decrementa posición del cursor dentro del campo.
		nroOT[numPosActual] = ' '; //Borra el último caracter escrito.
		msgMEdicionF4[numPosActual + numPosIni] = ' ';
	}
	cifraNroOT = String((const char *)msgMEdicionF4);
	//cifraNroOT += String((char *)nroOT);
	Serial.print("Cifra Nro OT Retroceder: ");
	Serial.println(cifraNroOT);
	imprimirLcd3.imprimirLCDfijo(cifraNroOT,0, 3);
}  
	
void AvanzaCursor (void)
{
	numPosActual++;	//Incrementa posición del cursor dentro del campo.
	if( numPosActual >= CANTDIGITOS )
		numPosActual = 0;	//Si se pasa de la cantidad de dígitos vuelve al inicio del campo.

	Serial.println((const char *)msgMEdicionF4);
}  

void AMEnsayo(void)
{
	String nombreDirectorio = "/";
	nombreDirectorio += String((const char *)msgMEdicionF4).substring(3);

	Serial.println();
	Serial.println((const char *)msgMEnsayoF1);
	Serial.println((const char *)msgMEnsayoF4);
//	imprimirLcd3.imprimirLCDfijo(String((const char *)msgMEdicionF4),0, 0);
	imprimirLcd3.imprimirLCDfijo(otActual,0, 0);
	imprimirLcd3.imprimirLCDfijo(String((const char *)msgMEnsayoF4),0, 1);

}

void AMSuspEns(void)
{
	Serial.println();
	Serial.println((const char *)msgMSuspEnsF1);
	Serial.println((const char *)msgMSuspEnsF2);
	imprimirLcd3.imprimirLCDfijo("A-Cancela Ensayo    ",0, 0);
	imprimirLcd3.imprimirLCDfijo("B-Continua Ensayo   ",0, 1);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 2);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 3);}

void AVerifNroOT (void)
{
	//Serial.println(contadorCaract);
	inParser = SI;
	if(contadorCaract < CANTDIGITOS)	
		inParser = NO;
			
}
 
void IndErrorNroOT (void)
{
	Serial.println();
	Serial.println((const char *)msgMerrorNroOTF1);
	Serial.println((const char *)msgMerrorNroOTF2);
	imprimirLcd3.imprimirLCDfijo("Complete Nro OT      ",0, 0);
	imprimirLcd3.imprimirLCDfijo("A para continuar    ",0, 1);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 2);
	imprimirLcd3.imprimirLCDfijo((const char *)lineaVacia,0, 3);
	}
