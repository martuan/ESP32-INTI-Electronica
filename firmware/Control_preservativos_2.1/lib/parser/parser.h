#ifndef parser_h
#define parser_h

#include <Arduino.h>
#include <hardware.h>
#include "defines.h"
#include <LiquidCrystal_I2C.h>
#include "imprimirLCDI2C.h"

#define MASKINESTABLE	0x80	// Para orear a los estados de parser donde sea
								// necesaria la llamada al parser, sin la entrada 
								// desde teclado. Para unsigned char.
#define INESTABLE(x) (x | MASKINESTABLE) 
#define SI 0xff
#define NO 0xfe
// Estados posibles --------------------------------------------------------------
// Ejemplo para el caso de estados inestables: #define ESTINEST 	INESTABLE(7)
/*#define M_PPAL 			0
#define M_EDICION 		1
#define M_SELECCION 	2
#define M_INICIOENS 	3
#define M_ENSAYO 		4
#define M_SUSPENS 		5
#define VERIF_NOMBRE 	INESTABLE(6)
#define ERROR_NRO_OT	7	*/

/*class parser1   {
    public:
        parser1(LiquidCrystal_I2C objeto1LCD);
       LiquidCrystal_I2C _objeto1LCD;

    private:

  };    */

	void Parser(void);
    void IniParser(void);
    void Nada(void);
    void AMSeleccion(void);
    void AMInicioEns(void);
    void AMPpal(void);
    void AMEdicion(void);
    void EditaOT (void);
    void RetrocedeCursor (void);
    void AvanzaCursor (void);
    void AMEnsayo(void);
    void AMSuspEns(void);
	void AMEnsTerminado(void);
	void IndErrorNroOT (void);
	void AVerifNroOT (void);
	void ensayoTerminado(void);
	void AMCalibracion(void);
	void calibracionEnCurso(void);
	
struct estadoParser    //estructura que representa cada estado del parser
{
	unsigned char entrada;
	unsigned char proxEstado;
	void (* Accion)(void);
};

struct estadoParser mPpal[] =  
{
	'1',M_SELECCION,AMSeleccion,
	'2',M_EDICION,AMEdicion,
	'9',M_CALIBRACION,AMCalibracion,
	DEFAULT,M_PPAL,Nada,
    
};
struct estadoParser mEdicion[] =  
{
	'A',VERIF_NOMBRE,AVerifNroOT,
	'B',M_PPAL,AMPpal,
	FLECHA_IZQ,M_EDICION,RetrocedeCursor,  
	FLECHA_DER,M_EDICION,Nada, 
	EDICION_OT,M_EDICION,EditaOT,
	DEFAULT,M_EDICION,Nada,
    
};
struct estadoParser verifNombre[] =  //INESTABLE
{
	SI,M_INICIOENS,AMInicioEns,
	NO,ERROR_NRO_OT,IndErrorNroOT,
	DEFAULT,VERIF_NOMBRE,Nada,
};
struct estadoParser errorNroOT[] =  
{
	'A',M_EDICION,AMEdicion,
	DEFAULT,ERROR_NRO_OT,Nada,
};
struct estadoParser mSeleccion[] =  
{
	'A',M_INICIOENS,AMInicioEns,
	'B',M_PPAL,AMPpal,
	DEFAULT,M_SELECCION,Nada,
};
	
struct estadoParser mInicioEns[] =  
{
	'A',M_ENSAYO,AMEnsayo,
	'B',M_PPAL,AMPpal,
	DEFAULT,M_INICIOENS,Nada,
};
	
struct estadoParser mEnsayo[] =  
{
	'B',M_SUSPENS,AMSuspEns,
	TERMINADO, TERMINO_ENSAYO, ensayoTerminado,
	DEFAULT,M_ENSAYO,Nada,
};	

struct estadoParser mSuspEns[] =  
{
	'A',M_PPAL,AMPpal,
	'B',M_ENSAYO,AMEnsayo,
	DEFAULT,M_SUSPENS,Nada,
};

struct estadoParser mEnsayoTerminado[] =  
{
	'A',M_PPAL,AMPpal,
	DEFAULT,TERMINO_ENSAYO,Nada,
};	

struct estadoParser mCalibracion[] =  
{
	'B',M_PPAL,AMPpal,
	'A',CALIBRANDO,calibracionEnCurso,
	DEFAULT,TERMINO_ENSAYO,Nada,
};	

struct estadoParser estadoCalibracionEnCurso[] =  
{
	DEFAULT,CALIBRANDO,Nada,
};	

struct estadoParser * ptrEstadoParser;

struct estadoParser * dirEstadoParser[]=
{
	mPpal,
	mEdicion,
	mSeleccion,
	mInicioEns,
	mEnsayo,
	mSuspEns,	
	verifNombre,	//Inestable.
	errorNroOT,
	mEnsayoTerminado,
	mCalibracion,
	estadoCalibracionEnCurso,
};

#endif