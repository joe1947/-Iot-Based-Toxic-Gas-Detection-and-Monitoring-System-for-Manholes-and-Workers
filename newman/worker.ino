#define BLYNK_TEMPLATE_ID "TMPL33oUdPC3s"
#define BLYNK_TEMPLATE_NAME "manhole"
#define BLYNK_AUTH_TOKEN "A5jpHOPJ4HbK50JrKYKxrh9NUR9dRdcA"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define motor control pins
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4

// DS18B20 temperature sensor pin
#define ONE_WIRE_BUS D5

// MQ4 gas sensor pin
#define MQ4_PIN A0

// WiFi credentials
char ssid[] = "Joes phone";
char pass[] = "1234567i";

// Setup a OneWire instance to communicate with DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Function to stop all motors
void stopMotors() {
  Serial.println("Stopping Motors");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void setup() {
  // Set motor pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Stop motors at the start
  stopMotors();

  // Set up the MQ4 gas sensor pin
  pinMode(MQ4_PIN, INPUT);

  // Begin serial communication
  Serial.begin(9600);

  // Initialize DS18B20 sensor
  sensors.begin();

  // Initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

// Move forward
BLYNK_WRITE(V1) {
  int state = param.asInt();
  if (state == 1) {
    Serial.println("Moving Forward");
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    stopMotors();
  }
}

// Turn left
BLYNK_WRITE(V3) {
  int state = param.asInt();
  if (state == 1) {
    Serial.println("Turning Left");
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    stopMotors();
  }
}

// Turn right
BLYNK_WRITE(V4) {
  int state = param.asInt();
  if (state == 1) {
    Serial.println("Turning Right");
    digitalWrite(IN1, HIGH); // Left motor forward
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);  // Right motor stopped
    digitalWrite(IN4, LOW);
  } else {
    stopMotors();
  }
}

// Send temperature to Blynk (V2)
void sendTemperature() {
  sensors.requestTemperatures(); // Request temperature from DS18B20
  float temperature = sensors.getTempCByIndex(0); // Get temperature in Celsius
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Blynk.virtualWrite(V2, temperature);
}

// Send gas reading to Blynk (V5)
void sendGasLevel() {
  int gasLevel = analogRead(MQ4_PIN); // Read analog value from MQ4 sensor
  Serial.print("Gas Level: ");
  Serial.println(gasLevel);
  Blynk.virtualWrite(V5, gasLevel);
}

void loop() {
  Blynk.run();

  // Send sensor data periodically
  sendTemperature();
  sendGasLevel();

  delay(1000); // Delay to avoid spamming Blynk
}




