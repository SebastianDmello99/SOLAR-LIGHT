#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <DHT.h>

// **WiFi Credentials**
const char* WIFI_SSID = "PRISM";
const char* WIFI_PASSWORD = "7028565647";

// **Telegram Bot Token**
const char* BOT_TOKEN = "7550753332:AAHhA_pfxN79VUvctCOIo1oBLwBj0y0GFXs";
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// **Pin Definitions**
#define BUTTON_PIN 18   // SOS Button
#define BLUE_LED 2      // Blue LED for WiFi status
#define DHTPIN 12       // DHT11 data pin
#define DHTTYPE DHT11   // DHT 11 type
#define LED_PIN 13      // GP2Y1010AU0F LED control pin
#define SENSOR_PIN 34   // Analog output from GP2Y1010AU0F
#define LDR_PIN 35      // LDR analog input
#define MAIN_RELAY 26   // Main Relay (R1) - Controls overall power
#define SWITCH_RELAY 15 // Switching Relay (R2) - Switches Yellow/White

// **Initialize DHT sensor**
DHT dht(DHTPIN, DHTTYPE);

bool buttonState = false;
bool lastButtonState = false;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BLUE_LED, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(MAIN_RELAY, OUTPUT);
    pinMode(SWITCH_RELAY, OUTPUT);

    digitalWrite(LED_PIN, HIGH);
    digitalWrite(MAIN_RELAY, LOW);
    digitalWrite(SWITCH_RELAY, LOW);
    digitalWrite(BLUE_LED, LOW); // Ensure LED is off initially

    dht.begin();

    // **Connect to WiFi**
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected to WiFi!");
        digitalWrite(BLUE_LED, HIGH); // Turn on blue LED when connected
    } else {
        Serial.println("\n❌ WiFi connection failed! Check credentials.");
    }
    client.setInsecure();
}

// **Function to Read Dust Density**
float readDustDensity() {
    digitalWrite(LED_PIN, LOW);
    delayMicroseconds(280);
    int rawADC = analogRead(SENSOR_PIN);
    delayMicroseconds(40);
    digitalWrite(LED_PIN, HIGH);
    delayMicroseconds(9680);
    float voltage = rawADC * (3.3 / 4095.0);
    return (voltage - 0.1) / 0.005;
}

void loop() {
    buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW && lastButtonState == HIGH) {
        Serial.println("🔴 Button Pressed! Sending Telegram alert...");
        bool sent = bot.sendMessage("8067307882", "🚨 SOS Alert! Button Pressed!", "");
        Serial.println(sent ? "✅ SOS Sent Successfully!" : "❌ Failed to send SOS!");
    }
    lastButtonState = buttonState;

    // **Read Sensors**
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    float dustDensity = readDustDensity();
    int ldrValue = analogRead(LDR_PIN);

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT11 sensor!");
        return;
    }

    Serial.print("Humidity: "); Serial.print(humidity);
    Serial.print("%  Temperature: "); Serial.print(temperature);
    Serial.print("°C  Dust Density: "); Serial.print(dustDensity);
    Serial.println(" µg/m³");

    if (ldrValue < 2000) {
        digitalWrite(MAIN_RELAY, HIGH);
        Serial.println("Low light detected. Lights Active.");

        if (humidity > 40 && temperature < 35 && dustDensity > 10) {
            digitalWrite(SWITCH_RELAY, HIGH);
            Serial.println("Fog detected! Yellow light ON.");
        } else {
            digitalWrite(SWITCH_RELAY, LOW);
            Serial.println("No fog. White light ON.");
        }
    } else {
        digitalWrite(MAIN_RELAY, LOW);
        Serial.println("Sufficient light detected. Lights OFF.");
    }
    delay(2000);
}
