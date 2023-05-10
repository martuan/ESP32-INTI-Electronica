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
        void listDir(fs::FS &fs, const char * dirname, uint8_t levels, boolean listEnArchivo);
        void listSubDir(fs::FS &fs, const char * dirname, uint8_t levels);
        void leerFile(fs::FS &fs, const char * path);
        void deleteFile(fs::FS &fs, const char * path);
        int listCantidadArchivos(fs::FS &fs, const char * dirname, uint8_t levels);
       /* String nombreDirectorio1(void);
        String nombreDirectorio2(void);
        String nombreDirectorio3(void);
        String nombreDirectorio4(void);*/
    private:
        int _CS1;
        void leerArchivoDirectorios(fs::FS &fs, const char * path);
        void listDirArchivo(void);
//       void deleteFile(fs::FS &fs, const char * path);
};

#endif