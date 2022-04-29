#include "displayOLED.h"

/*
// Conexiones SPI para el display:
#define OLED_MOSI     13
#define OLED_CLK      12
#define OLED_DC       26
#define OLED_CS       15
#define OLED_RST      27

#define LINEHEIGHT 22  // para saltos de linea

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64,OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

*/
void displayOLED::testDisplay(void){

  // text display tests
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Bienvenido Datalogger");
  display.println("");
  display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
  display.println("INTI Electronica");
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.println("Edificio42");
  display.display();
  delay(3000);
  display.clearDisplay();

}

void displayOLED::mostrarTemperaturaPorDisplay(float temp){

	display.clearDisplay();
	display.setTextSize(1);
  	display.setTextColor(SH110X_WHITE);
	display.setCursor(10, 10);
	display.println("Temperatura");
	display.setTextSize(2);
	display.setCursor(15, 20);
  	display.println(temp);
  	display.display();


}

void displayOLED::printDisplay(String linea, int textSize, int xCursor, int yCursor, int demora){

	display.clearDisplay();
	display.setTextSize(textSize);
  	display.setTextColor(SH110X_WHITE);
	display.setCursor(xCursor, yCursor);
	display.println(linea);
	//display.setTextSize(2);
	//display.setCursor(15, 20);
  	//display.println(temp);
  	display.display();





}