#define BAUDRATE 9600

#define RX2 16 //Nro de GPIO //PARA EL ESP32 DEVKIT DE 30 PINES!!!! OJO!!!!
#define TX2 17 //Nro de GPIO //PARA EL ESP32 DEVKIT DE 30 PINES!!!! OJO!!!!
#define PINDEBUG 5

#define MASKINESTABLE	0x80	// Para orear a los estados de parser donde sea
								// necesaria la llamada al parser, sin la entrada 
								// desde teclado. Para unsigned char.

//Definición de estados del Parser de comunicación
#define SI 0xff
#define NO 0xfe
#define DATOSREC 0xfd
#define ERROR_GENERAL 0xfc
#define TIMEOUT 0xfb
#define MEDIR  0xfa
#define TIMEOUTCAUD 0xf9
//Definiciones de los errores de comunicación
#define SIN_ERROR 0
#define ERROR_Reint_WCM 1
#define ERROR_VerifWCM 2
#define ERROR_Reint_SP 3
#define ERROR_VerifSP 4
#define ERROR_reintCaudal 5
#define ERROR_caudalRec 6