#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <Wire.h>
#include <ModbusMaster.h>
#include <ESP32Servo.h>

#define CONNECTION_TIMEOUT 10

#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP  1800

#define MQTT_SERVER   "192.168.1.200"
#define MQTT_PORT     1883
#define MQTT_USERNAME "NvbN8h047qsGZDp6CKzl"
#define MQTT_PASSWORD "09200128"
#define MQTT_NAME     "ESP32"

RTC_DATA_ATTR int AngleC = 0;

WiFiClient client;
PubSubClient mqtt(client);

const char* ssid = "BTL_2.4G";
const char* password = "00009989";

extern float humidity;
extern float temperature;
extern float PZEMVoltage;
extern float PZEMCurrent;
extern float PZEMPower; 
extern float PZEMEnergy; 

void print_wakeup_reason(){
   esp_sleep_wakeup_cause_t wake_up_source;

   wake_up_source = esp_sleep_get_wakeup_cause();

   switch(wake_up_source){
      case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wake up caused by a timer"); break;
      default : Serial.printf("Wake up not caused by Deep Sleep: %d\n",wake_up_source); break;
   }
}


void setup() {
     Serial.begin(115200);
    delay(1000);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("\nConnecting");
    int timeout_counter = 0;

    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(200);
        timeout_counter++;
        if(timeout_counter >= CONNECTION_TIMEOUT*5){
        ESP.restart();
        }
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());

    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    setupDHT();
    setupPZEM();
    setupSERVO();
}
void loop() {
    loopDHT();
    loopPZEM();
    loopSERVO();
    delay(200);
     if (mqtt.connected() == false) {
        Serial.println("MQTT connection... ");
        delay(500);
        if (mqtt.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("connected");
        } else {
            Serial.println("failed");
            delay(5000);
        }
    } else {
      //debug
        Serial.print("humidity : ");Serial.print(humidity);Serial.println(" RH ");
        Serial.print("temperature : ");Serial.print(temperature);Serial.println(" C");

        Serial.print("PZEMVoltage : ");Serial.print(PZEMVoltage); Serial.println(" V");
        Serial.print("PZEMCurrent : ");Serial.print(PZEMCurrent);Serial.println(" A");
        Serial.print("PZEMPower : ");Serial.print(PZEMPower);Serial.println(" W");
        Serial.print("PZEMEnergy : ");Serial.print(PZEMEnergy);Serial.println(" Wh");

        mqtt.loop();
        String dataJS = "{\"Humidity\":" + String(humidity, 3) + ",\"Temperature\":" + String(temperature, 3) + ",\"Voltage\":" + String(PZEMVoltage, 3) + ",\"Current\":" + String(PZEMCurrent, 3) + ",\"Power\":" + String(PZEMPower, 3) + ",\"Energy\":" + String(PZEMEnergy, 3) + "}";
        char json[dataJS.length() + 1];
        dataJS.toCharArray(json, dataJS.length() + 1);
        mqtt.publish("v1/devices/me/telemetry", json);
        Serial.println("scuess");
        ++AngleC;
        Serial.println("AngleC: " + String(AngleC));
        delay(1000);
         print_wakeup_reason();

                //Timer Configuration
                esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
                Serial.println("ESP32 wake-up in " + String(TIME_TO_SLEEP) + " seconds");

                // Go in Deep Sleep mode
                Serial.println("Goes into Deep Sleep mode");
                Serial.println("----------------------");
                delay(100);
                esp_deep_sleep_start();
                Serial.println("This will never be displayed");
    }
}
