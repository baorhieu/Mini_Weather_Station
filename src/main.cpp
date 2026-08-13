#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>

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
#define DHT_PIN 19
#define DHT_TYPE DHT22
#define LIGHT_PIN 34

DHT dht(DHT_PIN, DHT_TYPE);

// =====================================================
// OUTPUT MODULE 
// =====================================================
#define BUZZER_PIN 23
#define LED_GREEN  2
#define LED_RED    4

// LCD 16x2 che do 4-bit
const uint8_t LCD_RS = 14;
const uint8_t LCD_E  = 13;
const uint8_t LCD_D4 = 26;
const uint8_t LCD_D5 = 25;
const uint8_t LCD_D6 = 33;
const uint8_t LCD_D7 = 32;

const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 2;

LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// WARNING THESHOLDS
const float T_ON     = 26.0;   
const float T_OFF    = 25.0;   
const float HUM_HIGH = 60.0;   

const uint32_t BEEP_MS     = 2000;    
const uint32_t COOLDOWN_MS = 60000;   

bool hotState = false;
bool beeping  = false;
uint32_t beepStart   = 0;
uint32_t lastBeepEnd = 0;
uint8_t  page = 0;

// DISPLAYED VALUES
float dispTemperature = NAN;
float dispHumidity    = NAN;
float dispPressure    = NAN;
int   dispLight       = 0;

// Remember whether BMP180 was detected
bool bmpWorking = false;

// -----------------------------
// SUPPORTED FUNCTIONS FOR LCD DISPLAY
// -----------------------------
void printLine(uint8_t row, const String &text) {
    String s = text;
    if (s.length() > LCD_COLS) s = s.substring(0, LCD_COLS);
    while (s.length() < LCD_COLS) s += ' ';
    lcd.setCursor(0, row);
    lcd.print(s);
}

void showTwoLines(const String &l0, const String &l1) {
    printLine(0, l0);
    printLine(1, l1);
}

// -----------------------------
// WARNING ALERTS - LED + BUZZER
// -----------------------------
void updateAlert() {
    if (isnan(dispTemperature)) return;

    if (!hotState && dispTemperature >= T_ON)  hotState = true;
    if ( hotState && dispTemperature <  T_OFF) hotState = false;

    digitalWrite(LED_RED,   hotState ? HIGH : LOW);
    digitalWrite(LED_GREEN, hotState ? LOW  : HIGH);

    // BUZZER ONLY WORKS WHEN HUMIDITY AND TEMPERATURE ARE HIGH
    uint32_t now = millis();
    bool shouldBeep = hotState && !isnan(dispHumidity) && dispHumidity >= HUM_HIGH;

    if (!shouldBeep) {
        if (beeping) { digitalWrite(BUZZER_PIN, LOW); beeping = false; }
        return;
    }

    if (beeping) {
        if (now - beepStart >= BEEP_MS) {
            digitalWrite(BUZZER_PIN, LOW);
            beeping = false;
            lastBeepEnd = now;
        }
    } else {
        if (lastBeepEnd == 0 || now - lastBeepEnd >= COOLDOWN_MS) {
            digitalWrite(BUZZER_PIN, HIGH);
            beeping = true;
            beepStart = now;
        }
    }
}

// -----------------------------
// DISPLAY LCD - PAGE SWITCHING
// -----------------------------
void updateDisplay() {
    char l0[20], l1[20];

    switch (page) {
        case 0:
            if (isnan(dispTemperature)) snprintf(l0, sizeof(l0), "Temp: --");
            else                        snprintf(l0, sizeof(l0), "Temp: %.1f C", dispTemperature);
            if (isnan(dispHumidity))    snprintf(l1, sizeof(l1), "Hum : --");
            else                        snprintf(l1, sizeof(l1), "Hum : %.0f %%", dispHumidity);
            break;

        case 1:
            if (isnan(dispPressure)) snprintf(l0, sizeof(l0), "Press: --");
            else                     snprintf(l0, sizeof(l0), "Press:%.0f hPa", dispPressure);
            snprintf(l1, sizeof(l1), "Light: %d", dispLight);
            break;

        default:
            snprintf(l0, sizeof(l0), hotState ? "Status: TOO HOT" : "Status: OK");
            snprintf(l1, sizeof(l1), client.connected() ? "MQTT: online" : "MQTT: offline");
            break;
    }

    showTwoLines(String(l0), String(l1));
    page = (page + 1) % 3;
}

void setup() {

    Serial.begin(115200);

    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_GREEN,  OUTPUT);
    pinMode(LED_RED,    OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_GREEN,  LOW);
    digitalWrite(LED_RED,    LOW);

    lcd.begin(LCD_COLS, LCD_ROWS);
    showTwoLines("Weather Station", "Connecting WiFi");

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

    showTwoLines("WiFi connected", WiFi.localIP().toString());
    delay(1500);

client.setServer(mqtt_server, mqtt_port);

Serial.println("Connecting to MQTT...");

if (client.connect("ESP32_WeatherStation")) {
    Serial.println("MQTT connected!");
    showTwoLines("MQTT connected", "starting...");
} else {
    Serial.print("MQTT connection failed, state: ");
    Serial.println(client.state());
    showTwoLines("MQTT failed", "state: " + String(client.state()));
}
    delay(1500);

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
        showTwoLines("BMP180 not found", "check wiring");
        delay(2000);
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

            dispPressure = event.pressure;          // FOR LCD

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

        dispTemperature = dhtTemperature;           // FOR LCD / LED / buzzer
        dispHumidity    = humidity;

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

    dispLight = lightValue;                          // FOR LCD

Serial.print("Light raw value: ");
Serial.println(lightValue);
char lightMessage[16];
snprintf(lightMessage, sizeof(lightMessage), "%d", lightValue);

client.publish(light_topic, lightMessage);

Serial.print("Published light to MQTT: ");
Serial.println(lightMessage);

    // ===================================
    // OUTPUT MODULE
    // ===================================
    updateAlert();
    updateDisplay();

    // AM2302 needs time between measurements
    delay(2500);
}
