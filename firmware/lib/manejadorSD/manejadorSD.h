#ifndef manejadorSD_h
#define manejadorSD_h

#include <Arduino.h>
#include <hardware.h>
#include <SPI.h>
#include <SD.h>
#include "FS.h"

class manejadorSD   {
    public:
        manejadorSD(int CS1);
        boolean inicializarSD();
        void writeFile(fs::FS &fs, const char * path, const char * message);
        boolean appendFile(fs::FS &fs, const char * path, const char * message);
        void createDir(fs::FS &fs, const char * path);
        void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
        void listSubDir(fs::FS &fs, const char * dirname, uint8_t levels);
        void leerFile(fs::FS &fs, const char * path);

    private:
        int _CS1;
};

#endif