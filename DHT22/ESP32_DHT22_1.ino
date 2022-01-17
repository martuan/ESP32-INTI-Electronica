
#include <DHTesp.h>

DHTesp dht;
/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Pin number for DHT11 data pin */
int dhtPin = 32;

// ----------------------------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  // initialize serial communication
  delay(50);
  Serial.flush();
 // Initialize temperature sensor
  dht.setup(dhtPin, DHTesp::DHT22);
  // Init
}

void loop()
{

  delay(2000);
// Reading temperature and humidity takes about 250 milliseconds!
// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
TempAndHumidity lastValues = dht.getTempAndHumidity();

Serial.println("Temperature: " + String(lastValues.temperature,0));
Serial.println("Humidity: " + String(lastValues.humidity,0));
}
