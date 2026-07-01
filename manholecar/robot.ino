#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// Firebase credentials
#define FIREBASE_HOST "      "
#define FIREBASE_AUTH "      "

// Wi-Fi credentials
#define WIFI_SSID "home"
#define WIFI_PASSWORD "123456789"

// Firebase objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// Thresholds and pins
#define TEMP_THRESHOLD 40.0
#define SPO2_THRESHOLD 70
#define ECG_PIN 35 // GPIO D35 for ECG sensor

// SpO2-related constants
#define BUFFER_SIZE 100

// MAX30102 Setup
MAX30105 particleSensor;
#define SDA_PIN 4
#define SCL_PIN 5

// Temperature Sensor Setup
#define ONE_WIRE_BUS 15
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// SIM800 Setup
#define SIM800_TX 17
#define SIM800_RX 16
HardwareSerial SIM800(1); // Use Serial1 for communication with SIM800

float irBuffer[BUFFER_SIZE];   // Buffer to store IR values
float redBuffer[BUFFER_SIZE];  // Buffer to store Red values
int bufferIndex = 0;           // Current index for the buffer

// Function to calculate SpO2
float calculateSpO2() {
  float irAC = 0, redAC = 0;
  float irDC = 0, redDC = 0;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    irDC += irBuffer[i];
    redDC += redBuffer[i];
  }
  irDC /= BUFFER_SIZE;
  redDC /= BUFFER_SIZE;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    irAC += pow((irBuffer[i] - irDC), 2);
    redAC += pow((redBuffer[i] - redDC), 2);
  }
  irAC = sqrt(irAC / BUFFER_SIZE);
  redAC = sqrt(redAC / BUFFER_SIZE);

  float ratio = (redAC / redDC) / (irAC / irDC);
  float spo2 = 110.0 - (25.0 * ratio);
  if (spo2 > 100) spo2 = 100;
  if (spo2 < 0) spo2 = 0;
  return spo2;
}

// Function to send SMS alert
void sendAlertSMS(String message) {
  Serial.print("Sending SMS: ");
  Serial.println(message);
  SIM800.println("AT");
  delay(1000);
  if (SIM800.available()) {
    Serial.print("Modem response: ");
    while (SIM800.available()) {
      Serial.write(SIM800.read());
    }
  }
  
  SIM800.println("AT+CMGF=1"); // Set SMS to text mode
  delay(1000);
  SIM800.println("AT+CMGS=\"+918921125269\""); // Replace with your phone number
  delay(1000);
  SIM800.print(message);
  delay(1000);
  SIM800.write(26); // End the SMS with Ctrl+Z
  delay(1000); // Wait for the message to send

  if (SIM800.available()) {
    Serial.print("SMS send response: ");
    while (SIM800.available()) {
      Serial.write(SIM800.read());
    }
  }
  Serial.println("SMS command sent.");
}

void setup() {
  Serial.begin(115200);

  // Wi-Fi setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");

  // Firebase setup
  config.database_url = FIREBASE_HOST;
  config.api_key = FIREBASE_AUTH;
  auth.user.email = "noahvalorantl7l7@gmail.com";
  auth.user.password = "manhole";
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase setup completed.");

  // Initialize I2C for MAX30102
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 sensor not found!");
    while (1);
  }
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeGreen(0);

  sensors.begin();
  SIM800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX); // Initialize SIM800

  for (int i = 0; i < BUFFER_SIZE; i++) {
    irBuffer[i] = 0;
    redBuffer[i] = 0;
  }
}

void loop() {
  // Read temperature
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // Read ECG
  int ecgValue = analogRead(ECG_PIN);

  // Read Red and IR values from MAX30102
  long redValue = particleSensor.getRed();
  long irValue = particleSensor.getIR();
  irBuffer[bufferIndex] = irValue;
  redBuffer[bufferIndex] = redValue;

  bufferIndex++;
  if (bufferIndex >= BUFFER_SIZE) {
    bufferIndex = 0;
  }

  float spo2Value = calculateSpO2();

  // Send data to Firebase
  Firebase.setFloat(firebaseData, "/temperature", temperature);
  Firebase.setFloat(firebaseData, "/spo2", spo2Value);
  Firebase.setInt(firebaseData, "/ecg", ecgValue);

  // Print values to Serial Monitor
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("SpO2: ");
  Serial.println(spo2Value);
  Serial.print("ECG: ");
  Serial.println(ecgValue);

  // Check thresholds and send SMS alerts
  if (temperature > TEMP_THRESHOLD) {
    sendAlertSMS("Alert: High temperature detected!");
  }

  if (spo2Value < SPO2_THRESHOLD) {
    sendAlertSMS("Alert: Low SpO2 detected!");
  }

  delay(1000);
}
