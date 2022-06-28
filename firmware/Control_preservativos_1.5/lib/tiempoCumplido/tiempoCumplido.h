#ifndef tiempoCumplido_h
#define tiempocumplido_h

#include <Arduino.h>

class tiempoCumplido
{
    public:
        tiempoCumplido(int centeSeg);   //Recibe centécimas de segundos
        boolean calcularTiempo(boolean);
    private:
        int _centeSeg;
};

#endif
