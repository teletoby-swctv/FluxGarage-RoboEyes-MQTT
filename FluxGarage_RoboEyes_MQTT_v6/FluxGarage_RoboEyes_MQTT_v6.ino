/************************************************************************************************
# FluxGarage RoboEyes MQTT Projekt

## Projektbeschreibung

Dieses Projekt steuert animierte Roboteraugen auf einem OLED-Display (SH1106) über MQTT-Kommandos.
Es unterstützt Animationen, zufälliges Blinzeln, Schlafmodus, Idle-Mode-Anpassungen, Stimmungsausdrücke, Blickrichtungen, sowie Spezialanimationen.

Die MQTT-Serverdaten (IP-Adresse und Port) werden bequem über einen **WiFiManager** im Captive Portal konfiguriert.

---

## Hardware

| Komponente             | Beschreibung                       |
|-------------------------|------------------------------------|
| ESP8266 (z.B. NodeMCU)   | Mikrocontroller mit WLAN           |
| OLED Display 1,3" SH1106 | 128x64 Pixel, I2C Interface        |

---

## MQTT Steuerung

|          Topic          | Payload | Beschreibung |
|:------------------------|:--------|:--------------|
| `roboeyes/animation`    | 0–15 | Bestimmte Animation abspielen |
| `roboeyes/random`       | 1/0 | Zufällige Animationen an/aus |
| `roboeyes/random_blink` | 1/0 | Zufälliges Blinzeln an/aus |
| `roboeyes/sleep`        | 1/0 | Sleep-Modus aktivieren/deaktivieren |
| `roboeyes/IdleMode`     | `ON,2,2` oder `OFF,0,0` | IdleMode steuern (Intervall, Variation) |
| `roboeyes/mood`         | 0=DEFAULT, 1=TIRED, 2=ANGRY, 3=HAPPY | Stimmung setzen |
| `roboeyes/position`     | N, NE, E, SE, S, SW, W, NW, DEFAULT | Blickrichtung setzen |
| `roboeyes/curiosity`    | 1/0 | Curiosity aktivieren/deaktivieren |
| `roboeyes/hflicker`     | ON,Amplitude oder OFF,0 | Horizontal Flickern steuern |
| `roboeyes/vflicker`     | ON,Amplitude oder OFF,0 | Vertikal Flickern steuern |
| `roboeyes/confused`     | 1 | Confused-Animation starten |
| `roboeyes/laugh`        | 1 | Laugh-Animation starten |

www.fluxgarage.com - https://github.com/FluxGarage/RoboEyes/tree/main
https://github.com/teletoby-swctv/FluxGarage-RoboEyes-MQTT
//***********************************************************************************************/


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

// WiFiManager Parameter für MQTT Server und Port
WiFiManagerParameter mqttServerParam("server", "MQTT Server", "192.168.188.21", 40);
WiFiManagerParameter mqttPortParam("port", "MQTT Port", "1883", 6);

String mqttServer;
int mqttPort;

// Steuerflags
bool randomMode = true;
bool randomBlinkMode = false;
bool specialIdleMode = false;
bool sleepMode = false;

// Timer
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
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setAutoblinker(ON, 3, 2);
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
    client.subscribe("roboeyes/IdleMode");
    client.subscribe("roboeyes/mood");
    client.subscribe("roboeyes/position");
    client.subscribe("roboeyes/curiosity");
    client.subscribe("roboeyes/hflicker");
    client.subscribe("roboeyes/vflicker");
    client.subscribe("roboeyes/confused");
    client.subscribe("roboeyes/laugh");
    client.subscribe("roboeyes/autoblinker");
    client.subscribe("roboeyes/width");
    client.subscribe("roboeyes/height");
    client.subscribe("roboeyes/borderradius");
    client.subscribe("roboeyes/spacebetween");
    client.subscribe("roboeyes/cyclops");
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
  else if (String(topic) == "roboeyes/random") {
    randomMode = (value == 1);
    lastRandomSwitch = millis();
    publishStatus(String("Random Modus ") + (randomMode ? "gestartet" : "gestoppt"));
  }
  else if (String(topic) == "roboeyes/random_blink") {
    randomBlinkMode = (value == 1);
    lastRandomBlink = millis();
    publishStatus(String("Random Blink Modus ") + (randomBlinkMode ? "gestartet" : "gestoppt"));
  }
  else if (String(topic) == "roboeyes/sleep") {
    sleepMode = (value == 1);
    if (sleepMode) {
      roboEyes.setMood(TIRED);
      roboEyes.setIdleMode(ON, 20, 15);
      roboEyes.close();
      randomMode = false;
      randomBlinkMode = false;
      publishStatus("Sleep Modus aktiviert");
    } else {
      roboEyes.setMood(HAPPY);
      roboEyes.setIdleMode(ON, 2, 2);
      roboEyes.open();
      publishStatus("Sleep Modus deaktiviert");
    }
  }
  else if (String(topic) == "roboeyes/IdleMode") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int firstComma = payloadStr.indexOf(',');
    int secondComma = payloadStr.indexOf(',', firstComma + 1);

    String mode = payloadStr.substring(0, firstComma);
    int interval = payloadStr.substring(firstComma + 1, secondComma).toInt();
    int variation = payloadStr.substring(secondComma + 1).toInt();

    bool active = (mode == "ON");

    roboEyes.setIdleMode(active, interval, variation);

    publishStatus(String("IdleMode geändert auf ") + mode + " (" + interval + "s, " + variation + "s)");
  }
  else if (String(topic) == "roboeyes/mood") {
    roboEyes.setMood(value);
    publishStatus("Mood geändert auf " + String(value));
  }
  else if (String(topic) == "roboeyes/position") {
    String pos = message;
    pos.trim();
    if (pos == "N") roboEyes.setPosition(N);
    else if (pos == "NE") roboEyes.setPosition(NE);
    else if (pos == "E") roboEyes.setPosition(E);
    else if (pos == "SE") roboEyes.setPosition(SE);
    else if (pos == "S") roboEyes.setPosition(S);
    else if (pos == "SW") roboEyes.setPosition(SW);
    else if (pos == "W") roboEyes.setPosition(W);
    else if (pos == "NW") roboEyes.setPosition(NW);
    else roboEyes.setPosition(DEFAULT);
    publishStatus("Position geändert auf " + pos);
  }
  else if (String(topic) == "roboeyes/curiosity") {
    roboEyes.setCuriosity(value == 1);
    publishStatus(String("Curiosity ") + (value == 1 ? "aktiviert" : "deaktiviert"));
  }
  else if (String(topic) == "roboeyes/hflicker") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    String onOff = payloadStr.substring(0, comma);
    int amp = payloadStr.substring(comma + 1).toInt();
    roboEyes.setHFlicker(onOff == "ON", amp);
    publishStatus(String("HFlicker ") + onOff + " mit Amplitude " + String(amp));
  }
  else if (String(topic) == "roboeyes/vflicker") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    String onOff = payloadStr.substring(0, comma);
    int amp = payloadStr.substring(comma + 1).toInt();
    roboEyes.setVFlicker(onOff == "ON", amp);
    publishStatus(String("VFlicker ") + onOff + " mit Amplitude " + String(amp));
  }
  else if (String(topic) == "roboeyes/confused") {
    roboEyes.anim_confused();
    publishStatus("Confused Animation gestartet");
  }
  else if (String(topic) == "roboeyes/laugh") {
    roboEyes.anim_laugh();
    publishStatus("Laugh Animation gestartet");
  }
  else if (String(topic) == "roboeyes/autoblinker") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int firstComma = payloadStr.indexOf(',');
    int secondComma = payloadStr.indexOf(',', firstComma + 1);

    String mode = payloadStr.substring(0, firstComma);
    int interval = payloadStr.substring(firstComma + 1, secondComma).toInt();
    int variation = payloadStr.substring(secondComma + 1).toInt();

    bool active = (mode == "ON");

    roboEyes.setAutoblinker(active, interval, variation);

    publishStatus(String("AutoBlinker geändert auf ") + mode + " (" + interval + "s, " + variation + "s)");
  }
  else if (String(topic) == "roboeyes/width") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.setWidth(left, right);
    publishStatus("Width geändert auf: L=" + String(left) + " R=" + String(right));
  }
  else if (String(topic) == "roboeyes/height") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.setHeight(left, right);
    publishStatus("Height geändert auf: L=" + String(left) + " R=" + String(right));
  }
  else if (String(topic) == "roboeyes/borderradius") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.setBorderradius(left, right);
    publishStatus("BorderRadius geändert auf: L=" + String(left) + " R=" + String(right));
  }
  else if (String(topic) == "roboeyes/spacebetween") {
    int space = message.toInt();
    roboEyes.setSpacebetween(space);
    publishStatus("Spacebetween geändert auf: " + String(space));
  }
  else if (String(topic) == "roboeyes/cyclops") {
    roboEyes.setCyclops(value == 1);
    publishStatus(String("Cyclops ") + (value == 1 ? "aktiviert" : "deaktiviert"));
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

