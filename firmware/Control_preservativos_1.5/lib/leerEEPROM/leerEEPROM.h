#ifndef leerEEPROM_h
#define leerEEPROM_h

#include <Arduino.h>
#include <EEPROM.h>

class  leerEEPROM
{
    public:
        leerEEPROM(int direccionEEPROM_1kPa, int direccionEEPROM_2kPa);   
        void obtenerValores();
    private:
    int _direccionEEPROM_1kPa;
    int _direccionEEPROM_2kPa;
        
};

#endif
