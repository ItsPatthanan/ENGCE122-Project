#include <DHT.h>
#define DHTPIN 22
#define DHTTYPE DHT11 

DHT dht(DHTPIN, DHTTYPE);
float humidity; 
float temperature; 

void setupDHT() {
  Serial.begin(115200);
  dht.begin();
}

void loopDHT() {

  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    humidity = -1;
    temperature = -1;
    return;
  }
 

}
