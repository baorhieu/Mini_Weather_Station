#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* temperature_topic = "/amc/ss2026/group3/am2302/temperature";
const char* humidity_topic = "/amc/ss2026/group3/am2302/humidity";
const char* pressure_topic = "/amc/ss2026/group3/bmp180/pressure";
const char* light_topic = "/amc/ss2026/group3/light/raw";
WiFiClient espClient;
PubSubClient client(espClient);


const char* ssid = "iPhone";
const char* password = "AYASHA07";

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
    Serial.println("Connecting to WiFi...");

WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
}

Serial.println();
Serial.println("WiFi connected!");

Serial.print("IP address: ");
Serial.println(WiFi.localIP());
client.setServer(mqtt_server, mqtt_port);

Serial.println("Connecting to MQTT...");

if (client.connect("ESP32_WeatherStation")) {
    Serial.println("MQTT connected!");
} else {
    Serial.print("MQTT connection failed, state: ");
    Serial.println(client.state());
}

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
 client.loop();
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
            char pressureMessage[16];
snprintf(pressureMessage, sizeof(pressureMessage), "%.2f", event.pressure);

client.publish(pressure_topic, pressureMessage);

Serial.print("Published pressure to MQTT: ");
Serial.println(pressureMessage);

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
        char tempMessage[16];
snprintf(tempMessage, sizeof(tempMessage), "%.2f", dhtTemperature);

client.publish(temperature_topic, tempMessage);

Serial.print("Published temperature to MQTT: ");
Serial.println(tempMessage);


        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.println(" %");
        char humidityMessage[16];
snprintf(humidityMessage, sizeof(humidityMessage), "%.2f", humidity);

client.publish(humidity_topic, humidityMessage);

Serial.print("Published humidity to MQTT: ");
Serial.println(humidityMessage);
    }


    Serial.println("-------------------------");
    int lightValue = analogRead(LIGHT_PIN);

Serial.print("Light raw value: ");
Serial.println(lightValue);
char lightMessage[16];
snprintf(lightMessage, sizeof(lightMessage), "%d", lightValue);

client.publish(light_topic, lightMessage);

Serial.print("Published light to MQTT: ");
Serial.println(lightMessage);

    // AM2302 needs time between measurements
    delay(2500);
}

