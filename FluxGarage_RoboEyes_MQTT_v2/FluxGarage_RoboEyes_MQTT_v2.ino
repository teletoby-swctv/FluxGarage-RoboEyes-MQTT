
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#include <FluxGarage_RoboEyes.h>
roboEyes roboEyes;

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT Server/Port Variablen (am Anfang leer)
String mqttServer;
int mqttPort;

WiFiManagerParameter mqttServerParam("server", "MQTT Server", "192.168.188.21", 40);
WiFiManagerParameter mqttPortParam("port", "MQTT Port", "1883", 6);

bool randomMode = true;
bool randomBlinkMode = false;
bool specialIdleMode = false;
bool sleepMode = false;

unsigned long lastReconnectAttempt = 0;
unsigned long lastRandomSwitch = 0;
unsigned long lastRandomBlink = 0;
unsigned long lastActivity = 0;
const unsigned long idleTimeout = 300000; // 5 Minuten

void setup() {
  Serial.begin(115200);
  delay(250);
  Wire.begin();
  display.begin(I2C_ADDRESS, true);
  display.setContrast(255);
  display.clearDisplay();
  display.display();

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  roboEyes.setIdleMode(ON, 2, 3);
  roboEyes.close();

  WiFiManager wifiManager;
  wifiManager.addParameter(&mqttServerParam);
  wifiManager.addParameter(&mqttPortParam);
  wifiManager.autoConnect("RoboEyes_AP");

  mqttServer = String(mqttServerParam.getValue());
  mqttPort = atoi(mqttPortParam.getValue());

  client.setServer(mqttServer.c_str(), mqttPort);
  client.setCallback(mqttCallback);

  lastRandomSwitch = millis();
  lastRandomBlink = millis();
  lastActivity = millis();
  playRandomAnimation();
  publishStatus("Random Modus gestartet");
}

void loop() {
  roboEyes.update();

  if (!client.connected()) {
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      reconnect();
    }
  } else {
    client.loop();
  }

  if (!sleepMode) {
    if (randomMode && millis() - lastRandomSwitch > 6000) {
      playRandomAnimation();
      lastRandomSwitch = millis();
    }
    if (randomBlinkMode && millis() - lastRandomBlink > random(3000, 6000)) {
      playRandomBlink();
      lastRandomBlink = millis();
    }
    if (!specialIdleMode && (millis() - lastActivity > idleTimeout)) {
      roboEyes.setMood(TIRED);
      roboEyes.close();
      publishStatus("Special Idle aktiviert");
      specialIdleMode = true;
    }
  }
}

boolean reconnect() {
  if (client.connect("RoboEyesClient")) {
    client.subscribe("roboeyes/animation");
    client.subscribe("roboeyes/random");
    client.subscribe("roboeyes/random_blink");
    client.subscribe("roboeyes/sleep");
    lastReconnectAttempt = 0;
  }
  return client.connected();
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String message = String((char*)payload);
  int value = message.toInt();

  lastActivity = millis();
  if (specialIdleMode) {
    specialIdleMode = false;
    publishStatus("Special Idle beendet");
  }

  if (String(topic) == "roboeyes/animation") {
    playSelectedAnimation(value);
    randomMode = false;
    publishStatus("Manual Animation " + String(value) + " gestartet");
  }
  if (String(topic) == "roboeyes/random") {
    randomMode = (value == 1);
    lastRandomSwitch = millis();
    publishStatus(String("Random Modus ") + (randomMode ? "gestartet" : "gestoppt"));
  }
  if (String(topic) == "roboeyes/random_blink") {
    randomBlinkMode = (value == 1);
    lastRandomBlink = millis();
    publishStatus(String("Random Blink Modus ") + (randomBlinkMode ? "gestartet" : "gestoppt"));
  }
  if (String(topic) == "roboeyes/sleep") {
    sleepMode = (value == 1);
    if (sleepMode) {
      roboEyes.setMood(TIRED);
      roboEyes.close();
      randomMode = false;
      randomBlinkMode = false;
      publishStatus("Sleep Modus aktiviert");
    } else {
      roboEyes.setMood(HAPPY);
      roboEyes.open();
      publishStatus("Sleep Modus deaktiviert");
    }
  }
}

void publishStatus(const String &text) {
  client.publish("roboeyes/status", text.c_str());
}

void playSelectedAnimation(int id) {
  switch (id) {
    case 0: roboEyes.setMood(HAPPY); roboEyes.anim_laugh(); break;
    case 1: roboEyes.setMood(TIRED); roboEyes.blink(1, 0); break;
    case 2: roboEyes.setMood(ANGRY); roboEyes.blink(0, 1); break;
    case 3: roboEyes.setMood(DEFAULT); roboEyes.anim_confused(); break;
    case 4: roboEyes.setMood(HAPPY); roboEyes.open(); break;
    case 5: roboEyes.setMood(TIRED); roboEyes.close(); delay(400); roboEyes.open(); break;
    case 6: roboEyes.setMood(DEFAULT); roboEyes.blink(1, 1); break;
    case 7: roboEyes.setMood(ANGRY); roboEyes.anim_confused(); break;
    case 8: roboEyes.setMood(HAPPY); roboEyes.blink(1, 1); delay(300); roboEyes.anim_laugh(); break;
    case 9: roboEyes.setMood(DEFAULT); roboEyes.blink(1, 1); break;
    case 10: roboEyes.setMood(DEFAULT); roboEyes.blink(1, 0); break;
    case 11: roboEyes.setMood(DEFAULT); roboEyes.blink(0, 1); break;
    case 12: roboEyes.setMood(ANGRY); roboEyes.anim_confused(); break;
    case 13: roboEyes.setMood(TIRED); roboEyes.close(); break;
    case 14: roboEyes.setMood(DEFAULT); roboEyes.open(); break;
    case 15: roboEyes.setMood(DEFAULT); roboEyes.anim_confused(); break;
  }
}

void playRandomAnimation() {
  playSelectedAnimation(random(0, 16));
}

void playRandomBlink() {
  int r = random(0, 4);
  switch (r) {
    case 0: roboEyes.blink(1, 1); publishStatus("Random Blink: Single"); break;
    case 1: roboEyes.blink(1, 1); delay(200); roboEyes.blink(1, 1); publishStatus("Random Blink: Double"); break;
    case 2: roboEyes.blink(1, 1); delay(150); roboEyes.blink(1, 1); delay(150); roboEyes.blink(1, 1); publishStatus("Random Blink: Triple"); break;
    case 3: roboEyes.blink(1, 0); publishStatus("Random Blink: Slow"); break;
  }
}