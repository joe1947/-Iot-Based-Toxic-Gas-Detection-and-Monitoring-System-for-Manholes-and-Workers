#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define WATER_SENSOR_PIN A0  // Water level sensor connected to A0
#define ONE_WIRE_BUS 2       // DS18B20 connected to D2
#define SIM800_TX 7          // SIM800 TX to Arduino D7
#define SIM800_RX 8          // SIM800 RX to Arduino D8

SoftwareSerial sim800l(SIM800_TX, SIM800_RX);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const int tempThreshold = 40;  // Temperature alert threshold (°C)
const int waterThreshold = 200;  // Water level threshold (Adjust based on testing)
const char phoneNumber[] = "+918921125269";  // Alert recipient

bool tempAlertSent = false;
bool waterAlertSent = false;

void setup() {
    Serial.begin(9600);
    sim800l.begin(9600);  // Start GSM module
    sensors.begin();

    delay(1000);
    Serial.println("System Initialized!");
}

void loop() {
    // Read Temperature
    sensors.requestTemperatures();  
    float temperature = sensors.getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    // Read Water Level
    int waterLevel = analogRead(WATER_SENSOR_PIN);
    Serial.print("Water Level: ");
    Serial.println(waterLevel);

    // Check for high temperature alert
    if (temperature > tempThreshold && !tempAlertSent) {
        sendSMS("ALERT! Temperature exceeded 40°C!");
        tempAlertSent = true;  // Prevent duplicate alerts
    } else if (temperature <= tempThreshold) {
        tempAlertSent = false;  // Reset flag when temperature normalizes
    }

    // Check for water detection alert
    if (waterLevel > waterThreshold && !waterAlertSent) {
        sendSMS("ALERT! Water detected!");
        waterAlertSent = true;  // Prevent duplicate alerts
    } else if (waterLevel <= waterThreshold) {
        waterAlertSent = false;  // Reset flag when water level drops
    }

    delay(5000);  // Delay for 5 seconds before next reading
}

// Function to send SMS
void sendSMS(String message) {
    Serial.println("Sending SMS...");
    sim800l.print("AT+CMGF=1\r");  // Set SMS mode to text
    delay(1000);
    sim800l.print("AT+CMGS=\"");
    sim800l.print(phoneNumber);
    sim800l.println("\"");
    delay(1000);
    sim800l.print(message);
    delay(1000);
    sim800l.write(26);  // ASCII code for Ctrl+Z (End SMS)
    delay(5000);
    Serial.println("SMS Sent!");
}
