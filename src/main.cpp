#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <DHT.h>

// -----------------------------
// BMP180
// -----------------------------
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

// -----------------------------
// AM2302 / DHT22
// -----------------------------
#define DHT_PIN 27
#define DHT_TYPE DHT22
#define LIGHT_PIN 34

DHT dht(DHT_PIN, DHT_TYPE);

// Remember whether BMP180 was detected
bool bmpWorking = false;

void setup() {

    Serial.begin(115200);

    // Start I2C
    // SDA = GPIO 21
    // SCL = GPIO 22
    Wire.begin(21, 22);

    // Start AM2302
    dht.begin();

    delay(2000);

    // Check BMP180
    if (bmp.begin()) {
        bmpWorking = true;
        Serial.println("BMP180 connected!");
    }
    else {
        bmpWorking = false;
        Serial.println("BMP180 not found!");
    }

    Serial.println("AM2302 started!");
    Serial.println("-------------------------");
}


void loop() {

    // ===================================
    // BMP180
    // ===================================

    if (bmpWorking) {

        sensors_event_t event;
        bmp.getEvent(&event);

        if (event.pressure) {

            Serial.print("Pressure: ");
            Serial.print(event.pressure);
            Serial.println(" hPa");

            float bmpTemperature;
            bmp.getTemperature(&bmpTemperature);

            Serial.print("BMP180 Temperature: ");
            Serial.print(bmpTemperature);
            Serial.println(" C");
        }
        else {
            Serial.println("BMP180 reading failed!");
        }
    }
    else {
        Serial.println("BMP180 unavailable");
    }


    // ===================================
    // AM2302 / DHT22
    // ===================================

    float humidity = dht.readHumidity();
    float dhtTemperature = dht.readTemperature();

    if (isnan(humidity) || isnan(dhtTemperature)) {

        Serial.println("AM2302 reading failed!");

    }
    else {

        Serial.print("AM2302 Temperature: ");
        Serial.print(dhtTemperature);
        Serial.println(" C");

        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");
    }


    Serial.println("-------------------------");
    int lightValue = analogRead(LIGHT_PIN);

Serial.print("Light raw value: ");
Serial.println(lightValue);

    // AM2302 needs time between measurements
    delay(2500);
}

