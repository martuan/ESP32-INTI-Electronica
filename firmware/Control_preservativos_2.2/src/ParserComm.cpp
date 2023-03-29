#include <Arduino.h>
#include "defines.h"
#include "definesComm.h"
#include "ParserComm.h"

#define TIME_OUT 2 //Valor de cuenta para el time out de comunicaciones.


// variables para el parser ------------------------------------------------------
unsigned char inParserComm;			// dato de entrada al parser
unsigned char estadoActualComm; 	// estado del parser
//unsigned char estadoAnterior;	// utilizado para los casos en que se 
								// necesita saber de donde viene.

unsigned char RespArray[20]; //Para almacenar la respuesta del controlador
bool llamaParserComm;
unsigned char reintentos;
unsigned char timeOutCounterComm; //Contador para el time out de comunicación.
unsigned char timeOutCaudCounter; 	//Contador para el time out de comunicación para los comandos
									//de configuración.
unsigned char codigoErrorComm;		// 0 = Sin Error; 1 = Reint_WCM; 2 = VerifWCM; 3 = Reint_SP; 4 = VerifSP; 5 = reintCaudal; 6 = caudalRec 									
boolean errorSetUpCaudal;
boolean llamarMPPAL;
//**************************************************************************
//
// Funciones para máquina de estados. (Parser)
//
//**************************************************************************

void IniParserComm(void)
{
   	estadoActualComm = INICIAL;
	inParserComm = SI;
	reintentos = 0;
	codigoErrorComm = 0;
	errorSetUpCaudal = false;
	llamarMPPAL = false;
}

void ParserComm(void)
{
  	//unsigned char codigoFamilia;

  	//codigoFamilia = OTRO;   // por ahora.
	estadoActualComm = estadoActualComm & ~MASKINESTABLE; //Quita posible inestabilidad.

	//if( ((inParser >= '0') && (inParser <= '9')) || inParser == COD_OT_COMUN  
  	//	|| inParser == COD_INTERLABORATORIO )	
 	//{
	//	codigoFamilia = EDICION_OT;
	//}
  
  	ptrEstadoParserComm = dirEstadoParserComm[estadoActualComm];

	while ( (ptrEstadoParserComm->entrada != inParserComm) && 
			(ptrEstadoParserComm->entrada != DEFAULT)  /*&&
		 	(ptrEstadoParser->entrada != codigoFamilia)*/ )
	{
		ptrEstadoParserComm++;//si no es la entrada buscada pasa a la otra.
	}
	estadoActualComm = ptrEstadoParserComm->proxEstado; //actualiza estado
	
	(*ptrEstadoParserComm->Accion) (); 			//realiza acción
}


//
// Nada()
//
void NadaComm(void)
{
	//No hace nada.
}

void SendWriteControlMode(void)
{
	//Serial.println("SendWriteControlMode()");//DEBUG
	Serial2.println(":058001010412"); // El println le agrega el \r\n al comando.
	timeOutCounterComm = TIME_OUT;
	errorSetUpCaudal = true;
}


/*----------------------------------------------------------------------------------
ConfigCallback()
Lee las respuestas del controlador y la almacena en RespArray[].
Actualiza InParserComm.
Levanta flag para que el main() llame al ParserComm.
----------------------------------------------------------------------------------*/

void ConfigCallback()
{
  unsigned char i=0;

  //Serial.println("ConfigCallback()");//DEBUG  
  while ( Serial2.available() )
  {
    RespArray[i] = Serial2.read();
    i++;
  }
  inParserComm = DATOSREC;
  llamaParserComm = true; 
}

void VerifDatos(void)
{
	//Serial.println("VerifDatos()");//DEBUG 
	if( (RespArray[0] == ':') && (RespArray[11] == 0x0D)  
		&& (RespArray[12] == 0x0A ) && (RespArray[7] == '0') 
		&& (RespArray[8] == '0')) //Trama y Status OK
	{
		inParserComm = SI;
		Serial.println("Comando ejecutado correctamente");//DEBUG 
	}
	else
	{
		inParserComm = NO;
		//Serial.println("NO");//DEBUG
		if(++reintentos == CANT_REINTENTOS)
		{
			Serial.println("Reintento");//DEBUG
			inParserComm = ERROR_GENERAL;
	//			llamaParserComm = true;

		} 
	}
//	if(estadoActualComm == WR_CONTROL_MODE)	codigoErrorComm = 2;
//	if(estadoActualComm == WR_SETPOINT)	codigoErrorComm = 4;
	if(estadoActualComm == VERIF_WCM)	codigoErrorComm = 2;
	if(estadoActualComm == VERIF_SP)	codigoErrorComm = 4;
		
}
void Reintento(void)
//void ReintentoMed(void)
{
	if(++reintentos == CANT_REINTENTOS)
	{
		inParserComm = ERROR_GENERAL;
	}
	else
	{
		inParserComm = SI;
	} 
//	if(estadoActualComm == WR_CONTROL_MODE)	codigoErrorComm = 1;
//	if(estadoActualComm == WR_SETPOINT)	codigoErrorComm = 3;
	if(estadoActualComm == REINT_WCM)	codigoErrorComm = 1;
	if(estadoActualComm == REINT_SP)	codigoErrorComm = 3;
}
void ErrorGeneral(void)
{
	Serial.println("Error General");//DEBUG
	//errorSetUpCaudal = true;
}

void SendSetPoint(void)
{
	//Serial.println("SendSetPoint()");//DEBUG
	Serial2.println(":06800101214E20"); // 25l/min. El println le agrega el \r\n al comando.
	timeOutCounterComm = TIME_OUT;
}

void ConfSetPoint(void)
{
	RespArray[0]=RespArray[11]=RespArray[12]=RespArray[7]=RespArray[8]=0;//Blanquea buffer de recepción.
	reintentos = 0;		//Reset de cantidad de reintentos.
	SendSetPoint();
}

void WriteSetPointOK(void)
{
	RespArray[0]=RespArray[11]=RespArray[12]=RespArray[7]=RespArray[8]=0;//Blanquea buffer de recepción.
	Serial.println("SetPoint configurado OK");//DEBUG
}

void AWaitMed(void)
{
	RespArray[0]=RespArray[7]=RespArray[8]=RespArray[15]=RespArray[16]=0;//Blanquea buffer de recepción.
	reintentos = 0;		//Reset de cantidad de reintentos.
	//Serial.println("AWaitMed");//DEBUG
	errorSetUpCaudal = false;
	llamarMPPAL = true;
		timeOutCaudCounter = TIMEOUTCAUD;//Activa TimeOut.
}

void SendMedCaudal(void)
{
	Serial2.println(":06800401210120"); // El println le agrega el \r\n al comando.
	timeOutCaudCounter = TIMEOUTCAUD;//Activa TimeOut.
	//digitalWrite(PINDEBUG,HIGH);//DEBUG
	Serial.println("SendMedCaudal");//DEBUG
}

void VerifCaudal(void)
{
	timeOutCaudCounter = 0;//Inhibe cuenta de TimeOut.
	if( (RespArray[0] == ':') && (RespArray[15] == 0x0D) && (RespArray[16] == 0x0A ) ) //Trama OK.
	{
		inParserComm = SI;
	//	Serial.println("Lectura Correcta");//DEBUG 
	}
	else
	{
		inParserComm = NO;
		if(++reintentos == CANT_REINTENTOS_MED)
		{
			Serial.println("Reintento");//DEBUG
			inParserComm = ERROR_GENERAL;
		} 
	}
	codigoErrorComm = 5;
	Serial.println("VerifCaudal");//DEBUG
//	Serial.print("Valor inParserComm: ");//DEBUG
//		Serial.println(inParserComm);//DEBUG

}
void GuardaMed(void)
{
//	unsigned long int caudalValorDigital = 0;
	unsigned int caudalValorDigital = 0;
	int mult =1;
	//char RespArrayDato[4]; //Para almacenar la respuesta del controlador
	//----------------------------------------------------
	//Aquí se debe guardar la lectura RespArray[11] a RespArray[14] en una variable....
	//-----------------------------------------------------

	for(int i = 14; i > 10; i--){
		//RespArrayDato[0] =	RespArray[i + 11];	
		caudalValorDigital += (RespArray[i] - '0') * mult;
		mult *= 10; // mult is used to get ones, tens, hundreds and thousands
	}
	//caudalValorDigital = atoi(RespArrayDato);
	reintentos = 0;		//Reset de cantidad de reintentos.
	inParserComm = MEDIR;
	timeOutCaudCounter = TIMEOUTCAUD;//Activa TimeOut.
	Serial.print("Caudal Valor digital: ");
	Serial.println(caudalValorDigital);//DEBUG
	
}
void ReintentoMed(void)
{
	if(++reintentos == CANT_REINTENTOS_MED)
	{
		inParserComm = ERROR_GENERAL;
	}
	else
	{
		inParserComm = SI;
	} 
	Serial.println("Reintento");//DEBUG
	codigoErrorComm = 6;
}

void MensajeErrorLCD(void)
{
   /*	estadoActualComm = INICIAL;
	inParserComm = SI;
	reintentos = 0;
	codigoErrorComm = 0;
	errorSetUpCaudal = false;*/
	IniParserComm();
	llamaParserComm = true;
	//errorSetUpCaudal = true;
}