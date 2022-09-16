#ifndef leerCalibracionEEProm_h
#define leerCalibracionEEProm_h

#include <Arduino.h>
#include <hardware.h>
#include "leerEEPROM.h"


class leerCalibracionEEProm   {
    public:
        leerCalibracionEEProm(void);
        boolean verificarCalibracion(void);     //Verifica la calibración del sensor de presión en el preservativo

    private:
        boolean verificarCalibracionTalba2(void);
        void recomponerTabla1(void);
        boolean verificarCalibracionSensorFuelle(void);
};

#endif