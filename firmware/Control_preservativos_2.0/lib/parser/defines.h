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
#define COD_OT_COMUN  'C' //Código utilizado para OT común.
#define COD_INTERLABORATORIO  'D' //Código utilizado para Interlaboratorio.
#define EDICION_OT  0   //Código de familia para cuando se está editando una OT.
#define OTRO 1   //Código de familia genérico
#define FLECHA_IZQ  '*' //Para retroceder cuando está editando.
#define FLECHA_DER  '#' //Para avanzar cuando está editando.

//***************************************************************************************************************

#endif