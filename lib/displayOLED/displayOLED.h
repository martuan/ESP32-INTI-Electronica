#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_GFX.h>
#include <defines.h>


//class displayOLED : public Adafruit_SH1106G {
class displayOLED {

	public:
		//displayOLED():Adafruit_SH1106G(128, 64,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS){};//Constructor
		displayOLED(){};//Constructor
		~displayOLED(){};//Destructor
		Adafruit_SH1106G display = Adafruit_SH1106G(128, 64,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);//Constructor
		//Adafruit_SH1106G(128, 64,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);
		void testDisplay(void);
		void mostrarTemperaturaPorDisplay(float);
		void printDisplay(String, int, int, int, int);


};
