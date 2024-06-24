#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10, 560, 201, 234);
EthernetServer server(83);

#define DHTPIN 3
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);

int flame_sensor_pin_1 = 8;
int combined_flame_pin = HIGH;

LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  Ethernet.begin(mac, ip);
  server.begin();
  dht.begin();
  pinMode(flame_sensor_pin_1, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  Serial.begin(9600);
  Serial.println("Server is listening");
}

void loop() {
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    String request = readRequest(client);

    if (request.indexOf("/SensorData") != -1) {
      handleSensorData(client);
    } else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-type:text/html");
      client.println();
      client.println("<h1>Not Found</h1>");
    }

    client.stop();
    Serial.println("Client disconnected");
  }

  int flame_state_1 = digitalRead(flame_sensor_pin_1);

  if (flame_state_1 == LOW) {
    updateLCD("FLAME DETECTED!", "Additional actions...");
  } else {
    float temperatureC = dht.readTemperature();
    float humidity = dht.readHumidity();
    updateLCD("Temp: " + String(temperatureC) + " C", "Humidity: " + String(humidity) + "%");
  }

  delay(1000);
}

String readRequest(EthernetClient& client) {
  String request = "";
  while (client.connected() && !client.available()) {
    delay(1);
  }

  while (client.available()) {
    char c = client.read();
    request += c;
    if (c == '\n') {
      break;
    }
  }

  return request;
}

void handleSensorData(EthernetClient& client) {
  float temperatureC = dht.readTemperature();
  float humidity = dht.readHumidity();
  int flameStatus = digitalRead(flame_sensor_pin_1);

  if (!isnan(temperatureC) && !isnan(humidity)) {
    updateLCD("Temp: " + String(temperatureC) + " C", "Humidity: " + String(humidity) + "%");

    String response = "{";
    response += "\"Temperature\": " + String(temperatureC) + ",";
    response += "\"Humidity\": " + String(humidity) + ",";
    response += "\"FireStatus\": ";
    response += (flameStatus == LOW) ? "\"ALERT! Fire detected\"" : "\"No fire detected\"";
    response += "}";

    sendResponse(client, response);
  } else {
    updateLCD("Error: Unable to read sensor data", "");
    sendResponse(client, "{\"error\": \"Unable to read sensor data\"}");
  }

  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.print("Fire Status: ");
  Serial.println(flameStatus == LOW ? "ALERT! Fire detected" : "No fire detected");
}

void sendResponse(EthernetClient& client, String response) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(response);
}

void updateLCD(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}
