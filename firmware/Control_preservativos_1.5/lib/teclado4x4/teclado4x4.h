#ifndef teclado4x4_h
#define teclado4x4_h

#include <Arduino.h>
#include "pu2clr_pcf8574.h"

class teclado4x4
{
    public:
        teclado4x4(PCF pcf1);   //
        int obtenerTecla();
    private:
        PCF _pcf1;
};

#endif
