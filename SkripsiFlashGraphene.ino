#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

const char* ssid = "DarkBlue";
const char* password = "hstn137201";

WebServer server(80);

#define GPIO_27 27
#define GPIO_26 26
#define GPIO_33 33
#define GPIO_25 25

Adafruit_INA219 ina219;

bool state_27 = true;
bool state_26 = true;
bool state_33 = true;
bool state_25 = true;

float busVoltage = 0.0;

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<style>body {font-family: Arial, sans-serif; text-align: center;}";
  html += "button {width: 90%; height: 100px; font-size: 24px; margin: 10px;}";
  html += ".on {background-color: green; color: white;}";
  html += ".off {background-color: red; color: white;}";
  html += ".warning {color: red; font-size: 36px; font-weight: bold; margin-top: 20px;}";
  html += "</style></head><body>";
  html += "<h1>Timothy Flash Graphene Control</h1>";
  html += "<button class=\"on\" onclick=\"location.href='/toggle27'\">Discharge #1</button>";
  html += "<button class=\"on\" onclick=\"location.href='/toggle26'\">Discharge #2</button>";
  html += "<button class=\"" + String(state_33 ? "on" : "off") + "\" onclick=\"location.href='/toggle33'\">Charge</button>";
  html += "<button class=\"" + String(state_25 ? "on" : "off") + "\" onclick=\"location.href='/toggle25'\">Bleed</button>";

  html += "<h2>Voltage: " + String(busVoltage, 2) + " V</h2>";

  if (busVoltage > 2.0) {
    html += "<div class=\"warning\">WARNING!<br>HIGH VOLTAGE!</div>";
  }

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void togglePin(int pin, bool &state) {
  state = !state;
  digitalWrite(pin, state ? HIGH : LOW);
  handleRoot();
}

void pulsePin(int pin, bool &state) {
  digitalWrite(pin, LOW);
  handleRoot();
  delay(500);
  digitalWrite(pin, HIGH);
  handleRoot();
}

void TaskWebServer(void* pvParameters) {
  while (true) {
    server.handleClient();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void TaskReadVoltage(void* pvParameters) {
  while (true) {
    busVoltage = ina219.getBusVoltage_V();
    Serial.print("Bus Voltage: ");
    Serial.print(busVoltage);
    Serial.println(" V");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(GPIO_27, OUTPUT);
  pinMode(GPIO_26, OUTPUT);
  pinMode(GPIO_33, OUTPUT);
  pinMode(GPIO_25, OUTPUT);

  digitalWrite(GPIO_27, HIGH);
  digitalWrite(GPIO_26, HIGH);
  digitalWrite(GPIO_33, HIGH);
  digitalWrite(GPIO_25, HIGH);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1);
  }
  Serial.println("INA219 sensor initialized.");

  server.on("/", handleRoot);
  server.on("/toggle27", []() {
    pulsePin(GPIO_27, state_27);
  });
  server.on("/toggle26", []() {
    pulsePin(GPIO_26, state_26);
  });
  server.on("/toggle33", []() {
    togglePin(GPIO_33, state_33);
  });
  server.on("/toggle25", []() {
    togglePin(GPIO_25, state_25);
  });

  server.begin();
  Serial.println("Starting web server....");
  Serial.println("Web server started. You can access it via http://" + WiFi.localIP().toString());

  xTaskCreatePinnedToCore(
    TaskWebServer,
    "TaskWebServer",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  xTaskCreatePinnedToCore(
    TaskReadVoltage,
    "TaskReadVoltage",
    4096,
    NULL,
    1,
    NULL,
    0
  );
}

void loop() {
  // Main loop does nothing since tasks handle everything
}