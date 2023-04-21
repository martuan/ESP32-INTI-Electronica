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
//static unsigned int cantVecesGuardaMed;									
unsigned char codigoErrorComm;		// 0 = Sin Error; 1 = Reint_WCM; 2 = VerifWCM; 3 = Reint_SP; 4 = VerifSP; 5 = reintCaudal; 6 = caudalRec 									
boolean errorSetUpCaudal;
boolean llamarMPPAL;
//extern boolean flagEnsayoEnCurso;
extern boolean flagResetRespArray;
unsigned int volumenParcialDigital = 0;	//Guarda el volumen en el último período medidi (medio segundo)
//static unsigned int caudalValorDigitalDecimalAcumulado = 0;		//Utilizada en GuardaMed()
//float sumatoriaCaudalesMedidos;
	
unsigned long milisTranscurridos;
bool flagInicializarpreviousMillis;
static unsigned long previousMillis = 0;
float volumenParcial;
//float segundosTranscurridos;
float segundosEntreMedicion;
extern float segundosAcumulados;
extern bool flagContabilizarVolumenes;
bool flagCalculoVolumenParcial = true;

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
//	Serial.println("SendMedCaudal");//DEBUG
	if(flagResetRespArray == true){ 
		flagResetRespArray = false;
		for(int i = 11; i < 15; i++){
			RespArray[i] = '0';
		}		
		  	Serial.println("Entro a flagResetRespArray */*/*");
	}

	if(flagInicializarpreviousMillis){
		previousMillis = millis();			//Se inicializa previousMillis al inicio de cada ensayo. Durante el ensayo se actualiza en void VerifCaudal(void)
		flagInicializarpreviousMillis = false;
		  	Serial.println("Entro a flagInicializarpreviousMillis ");
	}	
}

void VerifCaudal(void)
{
	static unsigned long currentMillis;
	
	currentMillis = millis();

	timeOutCaudCounter = 0;//Inhibe cuenta de TimeOut.
	if( (RespArray[0] == ':') && (RespArray[15] == 0x0D) && (RespArray[16] == 0x0A ) ) //Trama OK.
	{
		inParserComm = SI;
		milisTranscurridos = currentMillis - previousMillis;
			/*Serial.print("currentMillis: ");
			Serial.print(currentMillis);//DEBUG
			Serial.print("     // previousMillis: ");
			Serial.println(previousMillis);//DEBUG	*/		
		previousMillis = currentMillis;

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
	//Serial.println("VerifCaudal");//DEBUG
}

unsigned int hexToDec(String hexString) {
  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return decValue;
}
void GuardaMed(void)
{
	//En el caudalímetro: Rango: 0 - 40L/min (0 - 32000d)
	char caudalValorDigitalChar[4];
	String caudalValorDigitalString = "";
	unsigned int caudalValorDigital;
	unsigned int volumenParcialDigitalInt;

	//Aquí se debe guardar la lectura RespArray[11] a RespArray[14] en una variable....

	for(int i = 11; i < 15; i++){
		caudalValorDigitalChar[i - 11] = RespArray[i] ;
	}
	caudalValorDigitalString = caudalValorDigitalChar;
	caudalValorDigital = hexToDec(caudalValorDigitalString); 
//	      cantVecesGuardaMed++;
	segundosEntreMedicion = float(milisTranscurridos)/1000;
	if(flagContabilizarVolumenes == true){
		volumenParcial =  (float(caudalValorDigital)/60)*segundosEntreMedicion/800;		// 32000/40 = 800;    Caudal máx: 40l/min = 32000 puntos(DAC)/min
	}
	segundosAcumulados += segundosEntreMedicion;
	reintentos = 0;		//Reset de cantidad de reintentos.
	inParserComm = MEDIR;
	timeOutCaudCounter = TIMEOUTCAUD;//Activa TimeOut.
//	Serial.print(" segundosEntreMedicion: ");
//	Serial.print(segundosEntreMedicion);//DEBUG
//	Serial.print("  * segundosAcumulados: ");
//	Serial.println(segundosAcumulados);//DEBUG
//	Serial.print(" volumenParcial: ");
//	Serial.println(volumenParcial, 3);//DEBUG
	flagCalculoVolumenParcial = true;
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