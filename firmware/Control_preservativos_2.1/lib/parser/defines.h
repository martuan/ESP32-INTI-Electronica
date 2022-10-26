#ifndef defines_h
#define defines_h


//#define LED_PORT 2
#define MASKINESTABLE	0x80	// Para orear a los estados de parser donde sea
								// necesaria la llamada al parser, sin la entrada 
								// desde teclado. Para unsigned char.

//Defines para edición de Nro de OT
#define CANTCOLDISP 20
#define COLINICIAL  7   //Columna del display donde empieza el campo de edición.
#define FILADISPLAY 4   //Fila del display donde se edita el nro de OT.
#define CANTDIGITOS 7   //Cantidad de dígitos destinados al nro de OT.
#define COD_OT_COMUN  'O' //Código utilizado para Original.
#define COD_INTERLABORATORIO  'E' //Código utilizado para Envejecido.
#define EDICION_OT  0   //Código de familia para cuando se está editando una OT.
#define OTRO 1   //Código de familia genérico
#define FLECHA_IZQ  '*' //Para retroceder cuando está editando.
#define FLECHA_DER  '#' //Para avanzar cuando está editando.
#define TERMINADO 0xfd

//***************************************************************************************************************
#define M_PPAL 			0
#define M_EDICION 		1
#define M_SELECCION 	2
#define M_INICIOENS 	3
#define M_ENSAYO 		4
#define M_SUSPENS 		5
#define VERIF_NOMBRE 	INESTABLE(6)
#define ERROR_NRO_OT	7
#define TERMINO_ENSAYO 	8
#define M_CALIBRACION	9
#define CALIBRANDO		10		//Calibración en curso

#endif