#define ServerVersion "1.0"
String webpage = "";
String filename = "";
String fechaEnsayoConsultada = "";
bool   SD_present = false;
//strcpy(src,  "");
//strcpy(dest, "/");
char src[100] = "";
char dest[100] = "/";
String pathDirectory = {};
//String pathDirectory = "/";
String pathLocation = {};
int cuenta = 0;

#ifdef ESP8266
#define SD_CS_pin           D8         // The pin on Wemos D1 Mini for SD Card Chip-Select
#else
#define SD_CS_pin           5        // Use pin 5 on MH-T Live ESP32 version of Wemos D1 Mini for SD Card Chip-Select
#endif                               // Use pin 13 on Wemos ESP32 Pro


