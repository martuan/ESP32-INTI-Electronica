#define INESTABLE(x) (x | MASKINESTABLE) 

#define CANT_REINTENTOS 10
#define CANT_REINTENTOS_MED 5

// Estados posibles --------------------------------------------------------------
/*#define INICIAL			0
#define WR_CONTROL_MODE	1
#define VERIF_WCM    	INESTABLE(2)
#define WR_SETPOINT	 	3
#define VERIF_SP	   	INESTABLE(4)
#define REINT_WCM		INESTABLE(5)
#define REINT_SP		INESTABLE(6)
#define WAIT_MED		7
#define CAUDAL			8
#define REINT_CAUDAL	INESTABLE(9)
#define CAUDAL_REC		INESTABLE(10)
#define MENSAJE_ERROR_LCD 11*/

// Funciones para el parser ------------------------------------------------------
void ParserComm(void);
void IniParserComm(void);
void NadaComm(void);
void SendWriteControlMode(void);
void VerifDatos(void);
void ErrorGeneral(void);
void ConfSetPoint(void);
void SendSetPoint(void);
void WriteSetPointOK(void);
//void ReinteConfigCallbacknto(void);
void SendMedCaudal(void);
void AWaitMed(void);
void VerifCaudal(void);
void GuardaMed(void);
void ReintentoMed(void);
void Reintento(void);
void MensajeErrorLCD(void);

struct estadoParserComm    //estructura que representa cada estado del parser
{
	unsigned char entrada;
	unsigned char proxEstado;
	void (* Accion)(void);
};

struct estadoParserComm inicial[] =  
{
	SI,WR_CONTROL_MODE,SendWriteControlMode,
	DEFAULT,INICIAL,NadaComm, 
};

struct estadoParserComm wrControlMode[] =  
{
	DATOSREC,VERIF_WCM,VerifDatos,
	TIMEOUT,REINT_WCM,Reintento,
//	TIMEOUT,REINT_WCM,ReintentoMed,
	DEFAULT,WR_CONTROL_MODE,NadaComm,  
};

struct estadoParserComm VerifWCM[] =   //Inestable.
{
	NO,WR_CONTROL_MODE,SendWriteControlMode,
	SI,WR_SETPOINT,ConfSetPoint,
//	ERROR_GENERAL,INICIAL,ErrorGeneral,
	ERROR_GENERAL,MENSAJE_ERROR_LCD,ErrorGeneral,
	DEFAULT,VERIF_WCM,NadaComm,
};

struct estadoParserComm Reint_WCM[] =   //Inestable.
{
	SI,WR_CONTROL_MODE,SendWriteControlMode,
//	ERROR_GENERAL,INICIAL,ErrorGeneral,
	ERROR_GENERAL,MENSAJE_ERROR_LCD,ErrorGeneral,
	DEFAULT,REINT_WCM,NadaComm,
};

struct estadoParserComm wrSetPoint[] =  
{
	DATOSREC,VERIF_SP,VerifDatos,
	TIMEOUT,REINT_SP,Reintento,
	DEFAULT,WR_SETPOINT,NadaComm,  
};

struct estadoParserComm VerifSP[] =   //Inestable.
{
	NO,WR_SETPOINT,SendSetPoint,
//	SI,INICIAL,WriteSetPointOK,
	SI,WAIT_MED,AWaitMed,
//	ERROR_GENERAL,INICIAL,ErrorGeneral,
	ERROR_GENERAL,MENSAJE_ERROR_LCD,ErrorGeneral,
	DEFAULT,VERIF_SP,NadaComm,
};

struct estadoParserComm Reint_SP[] =   //Inestable.
{
	SI,WR_SETPOINT,SendSetPoint,
//	ERROR_GENERAL,INICIAL,ErrorGeneral,
	ERROR_GENERAL,MENSAJE_ERROR_LCD,ErrorGeneral,
	DEFAULT,REINT_SP,NadaComm,
};

//Estados para medición de caudal.
struct estadoParserComm waitMedComm[] =
{
	MEDIR,CAUDAL,SendMedCaudal,
	DEFAULT,WAIT_MED,NadaComm,
};
struct estadoParserComm caudalComm[] =
{
	DATOSREC,CAUDAL_REC,VerifCaudal,
	TIMEOUTCAUD,REINT_CAUDAL,ReintentoMed,
	DEFAULT,CAUDAL,NadaComm,
};
struct estadoParserComm reintCaudalComm[] =	//Inestable
{
	SI,CAUDAL,SendMedCaudal,
//	ERROR_GENERAL,INICIAL,ErrorGeneral,
	ERROR_GENERAL,MENSAJE_ERROR_LCD,ErrorGeneral,
	DEFAULT,REINT_CAUDAL,NadaComm,
};
struct estadoParserComm caudalRecComm[] =	//Inestable
{
	SI,WAIT_MED,GuardaMed,
	NO,CAUDAL,SendMedCaudal,
//	ERROR_GENERAL,INICIAL,ErrorGeneral,
	ERROR_GENERAL,MENSAJE_ERROR_LCD,ErrorGeneral,
	DEFAULT,CAUDAL_REC,NadaComm,
};
struct estadoParserComm mensajeErrorLCD[] =	//Inestable
{
//	SI,WAIT_MED,GuardaMed,
//	NO,CAUDAL,SendMedCaudal,
	'A',INICIAL,MensajeErrorLCD,
	DEFAULT,MENSAJE_ERROR_LCD,NadaComm,
};

struct estadoParserComm * ptrEstadoParserComm;

struct estadoParserComm * dirEstadoParserComm[]=
{
	inicial,
	wrControlMode,
	VerifWCM,       //Inestable.
	wrSetPoint,
	VerifSP,       	//Inestable.
	Reint_WCM,		//Inestable.
	Reint_SP,		//Inestable.
	waitMedComm,
	caudalComm,
	reintCaudalComm,	//Inestable
	caudalRecComm,		//Inestable
	mensajeErrorLCD,
};
