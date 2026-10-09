#include <WiFi.h>           // WiFi library for ESP32
#include <LiquidCrystal.h> // LCD library
#include <DHT.h>           // DHT11 sensor library

// WiFi name and password
const char* ssid = "";
const char* password = "";

// ThingSpeak settings
String apiKey = "FGHVRWYYULI17PC5";
const char* server = "api.thingspeak.com";

// LCD pins: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(23, 22, 21, 19, 18, 5);

// DHT11 setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// MQ-3 sensor pin
#define sensor_pin 34

// Output pins
#define G_led 25
#define R_led 26
#define buzzer 27

// WiFi client object
WiFiClient client;

// Variables
float adcValue = 0;
float voltage = 0;
float mgL = 0;

void setup() {

  Serial.begin(115200);

  pinMode(sensor_pin, INPUT);

  pinMode(G_led, OUTPUT);
  pinMode(R_led, OUTPUT);
  pinMode(buzzer, OUTPUT);

  // Start LCD
  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("Alcohol Detect");

  lcd.setCursor(0, 1);
  lcd.print("ESP32 System");

  delay(2000);

  lcd.clear();

  // Start DHT11 sensor
  dht.begin();

  // Connect to WiFi
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  lcd.clear();

  lcd.print("WiFi Connected");

  Serial.println("WiFi connected");

  delay(2000);

  lcd.clear();
}
void loop() {

  adcValue = 0;

  // Read MQ-3 sensor 10 times and average the result
  for (int i = 0; i < 10; i++) {

    adcValue += analogRead(sensor_pin);

    delay(10);
  }

  adcValue = adcValue / 10.0;

  // Convert analog value to voltage
  voltage = adcValue * (3.3 / 4095.0);

  // Calculate alcohol value
  mgL = voltage * 0.67;

  // Read temperature
  float temp = dht.readTemperature();

  // If sensor fails
  if (isnan(temp)) {

    temp = 0;
  }

  // Overheat protection
  if (temp > 30) {

    lcd.setCursor(0, 0);
    lcd.print("OVERHEAT MODE");

    lcd.setCursor(0, 1);
    lcd.print("Alcohol Block");

    digitalWrite(buzzer, HIGH);
    digitalWrite(G_led, LOW);
    digitalWrite(R_led, HIGH);

    delay(2000);

    return;
  }

  // Show alcohol level
  lcd.setCursor(0, 0);

  lcd.print("BAC:");
  lcd.print(mgL, 2);
  lcd.print("mg/L");

  // Show status and temperature
  lcd.setCursor(0, 1);

  if (mgL > 0.8) {

    lcd.print("Drunk ");

  } else {

    lcd.print("Normal");
  }

  lcd.print(" T:");
  lcd.print(temp, 1);
  lcd.print("C");

  // Control LEDs and buzzer
  if (mgL > 0.8) {

    digitalWrite(buzzer, HIGH);
    digitalWrite(G_led, LOW);
    digitalWrite(R_led, HIGH);

  } else {

    digitalWrite(buzzer, LOW);
    digitalWrite(G_led, HIGH);
    digitalWrite(R_led, LOW);
  }

  // Send data to ThingSpeak
  if (client.connect(server, 80)) {

    String url = "/update?api_key=" + apiKey;

    url += "&field1=" + String(mgL);

    url += "&field2=" + String(temp);

    client.print(
      String("GET ") + url + " HTTP/1.1\r\n" +
      "Host: api.thingspeak.com\r\n" +
      "Connection: close\r\n\r\n"
    );

    Serial.println("Data sent to ThingSpeak");
  }

  // Close connection
  client.stop();

  // Wait 15 seconds before next update
  delay(15000);
