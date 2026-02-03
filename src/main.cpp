#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <SD.h>

// --- Pin Definitions (Updated for Image Layout) ---
// SD Card (VSPI)
#define SD_CS_PIN       5
#define SD_SCK_PIN      18
#define SD_MISO_PIN     19
#define SD_MOSI_PIN     23

// LEDs
#define LED_MOTION_PIN  33 // Blue
#define LED_RUN_PIN     32 // Green
#define LED_FAULT_PIN   17 // Red
#define LED_GAS_PIN     16 // Red

// Sensors
#define DHT_PIN         4
#define PIR_PIN         27
#define MQ135_PIN       34 // Analog

// Actuators
#define RELAY_PIN       25
#define SERVO_PIN       26

// I2C OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objects
DHT dht(DHT_PIN, DHT22);
Servo myServo;

void setup() {
  Serial.begin(115200);
  Serial.println("--- Booting Industrial Sim (Image Config) ---");

  // Init LEDs
  pinMode(LED_MOTION_PIN, OUTPUT);
  pinMode(LED_RUN_PIN, OUTPUT);
  pinMode(LED_FAULT_PIN, OUTPUT);
  pinMode(LED_GAS_PIN, OUTPUT);
  
  // Init Actuators
  pinMode(RELAY_PIN, OUTPUT);
  myServo.attach(SERVO_PIN);
  
  // Init Sensors
  pinMode(DHT_PIN, INPUT_PULLUP);
  dht.begin();
  pinMode(PIR_PIN, INPUT);

  // Init SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed");
    digitalWrite(LED_FAULT_PIN, HIGH);
  } else {
    Serial.println("SD Card Initialized");
  }

  // Init I2C OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    digitalWrite(LED_FAULT_PIN, HIGH);
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(F("System Init..."));
    display.display();
  }

  Serial.println("--- Ready ---");
}

void loop() {
  // 1. Read Sensors
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int pirState = digitalRead(PIR_PIN);
  int gasLevel = analogRead(MQ135_PIN);

  // 2. Logic
  // Status: RUN
  digitalWrite(LED_RUN_PIN, HIGH);

  // Logic: Motion -> Blue LED + Servo
  if (pirState == HIGH) {
    digitalWrite(LED_MOTION_PIN, HIGH);
    myServo.write(90);
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("MOTION DETECTED!");
    display.display();
  } else {
    digitalWrite(LED_MOTION_PIN, LOW);
    myServo.write(0);
  }

  // Logic: Gas -> High Level -> Gas LED + Relay (Fan)
  if (gasLevel > 2000) { // Threshold
    digitalWrite(LED_GAS_PIN, HIGH);
    digitalWrite(RELAY_PIN, HIGH);
  } else {
    digitalWrite(LED_GAS_PIN, LOW);
    digitalWrite(RELAY_PIN, LOW);
  }

  // 3. Display Info (if no motion, or alternate)
  if (pirState == LOW) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.printf("Temp: %.1f C\n", temp);
    display.printf("Hum:  %.1f %%\n", hum);
    display.printf("Gas:  %d\n", gasLevel);
    display.display();
  }

  // 4. Debug
  Serial.printf("T:%.1f H:%.1f PIR:%d Gas:%d\n", temp, hum, pirState, gasLevel);

  delay(2000); // Wait for DHT
}
