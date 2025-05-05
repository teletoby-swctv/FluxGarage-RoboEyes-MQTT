/************************************************************************************************
# FluxGarage RoboEyes MQTT Projekt 

## Project Description
This project controls animated robot eyes on an OLED display (SH1106) via MQTT commands.
It supports animations, random blinking, sleep mode, idle mode adjustments, mood expressions, gaze directions, and special animations.

The MQTT server data (IP address and port) is conveniently configured via a **WiFiManager** in the captive portal.
---
## Hardware

| Komponente               | Description                        |
|--------------------------|------------------------------------|
| ESP8266 (z.B. NodeMCU)   | Mikrocontroller mit WLAN           |
| OLED Display 1,3" SH1106 | 128x64 Pixel, I2C Interface        |

---

## MQTT Steuerung

|          Topic          | Payload | Description |
|:------------------------|:--------|:--------------|
| `roboeyes/animation`    | 0–15 | Play specific animation |
| `roboeyes/random`       | 1/0 | Random animations ON/OFF |
| `roboeyes/random_blink` | 1/0 | Random blinking ON/OFF |
| `roboeyes/autoblinker`  | `ON,2,2` oder `OFF,0,0` | IdleMode steuern (Intervall, Variation) |
| `roboeyes/blinker`      | `1,1` or `1,0` or `0,1`| Blink (left, right) |
| `roboeyes/open`         | 1/0 | open eyes |
| `roboeyes/close`        | 1/0 | close eyes |
| `roboeyes/sleep`        | 1/0 | Sleep-mode ON/OFF |
| `roboeyes/IdleMode`     | `ON,2,2` oder `OFF,0,0` | IdleMode steuern (Intervall, Variation) |
| `roboeyes/mood`         | 0=DEFAULT, 1=TIRED, 2=ANGRY, 3=HAPPY | Stimmung setzen |
| `roboeyes/position`     | N, NE, E, SE, S, SW, W, NW, DEFAULT | Blickrichtung setzen |
| `roboeyes/curiosity`    | 1/0 | Curiosity aktivieren/deaktivieren |
| `roboeyes/hflicker`     | ON,Amplitude oder OFF,0 | Horizontal Flickern steuern |
| `roboeyes/vflicker`     | ON,Amplitude oder OFF,0 | Vertikal Flickern steuern |
| `roboeyes/confused`     | 1 | Confused-Animation starten |
| `roboeyes/laugh`        | 1 | Laugh-Animation starten |
| `roboeyes/width`        | byte | leftEye, byte rightEye |
| `roboeyes/height`       | byte | leftEye, byte rightEye |
| `roboeyes/borderradius` | byte | leftEye, byte rightEye |
| `roboeyes/spacebetween` | int |space -> can also be negative |

www.fluxgarage.com - https://github.com/FluxGarage/RoboEyes/tree/main
https://github.com/teletoby-swctv/FluxGarage-RoboEyes-MQTT
//***********************************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>

#define EEPROM_SIZE 128
#define EEPROM_MQTT_SERVER_ADDR 0
#define EEPROM_MQTT_PORT_ADDR 64

#define I2C_ADDRESS 0x3c
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#include <FluxGarage_RoboEyes.h>
roboEyes roboEyes;

WiFiClient espClient;
PubSubClient client(espClient);

WiFiManagerParameter mqttServerParam("server", "MQTT Server", "192.168.1.220", 40);
WiFiManagerParameter mqttPortParam("port", "MQTT Port", "1883", 6);

String mqttServer;
int mqttPort;

bool shouldSaveConfig = false;
bool randomMode = true;
bool randomBlinkMode = false;
bool specialIdleMode = false;
bool sleepMode = false;

unsigned long lastReconnectAttempt = 0;
unsigned long lastRandomSwitch = 0;
unsigned long lastRandomBlink = 0;
unsigned long lastActivity = 0;
const unsigned long idleTimeout = 300000; // 5 Minuten

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Wire.begin();
  display.begin(I2C_ADDRESS, true);
  display.setContrast(255);
  display.clearDisplay();
  display.display();

  EEPROM.begin(EEPROM_SIZE);

  char serverBuffer[64];
  for (int i = 0; i < 63; i++) {
    serverBuffer[i] = EEPROM.read(EEPROM_MQTT_SERVER_ADDR + i);
    if (serverBuffer[i] == '\0') break;
  }
  serverBuffer[63] = '\0';
  mqttServer = String(serverBuffer);
  EEPROM.get(EEPROM_MQTT_PORT_ADDR, mqttPort);

  if (mqttServer.length() == 0 || mqttPort <= 0 || mqttPort > 65535) {
    mqttServer = "192.168.1.220";
    mqttPort = 1883;
  }

  Serial.println("📦 Loaded MQTT server from EEPROM: " + mqttServer);
  Serial.println("📦 Loaded MQTT port from EEPROM: " + String(mqttPort));

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.close();

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&mqttServerParam);
  wifiManager.addParameter(&mqttPortParam);

  if (!wifiManager.autoConnect("RoboEyes_AP")) {
    Serial.println("⚠️ Connection failed. reboot...");
    delay(4000);
    ESP.restart();
  }


  if (shouldSaveConfig) {
    mqttServer = String(mqttServerParam.getValue());
    mqttPort = atoi(mqttPortParam.getValue());

    Serial.println("💾 Neue MQTT Konfiguration speichern...");
    for (int i = 0; i < mqttServer.length(); i++) {
      EEPROM.write(EEPROM_MQTT_SERVER_ADDR + i, mqttServer[i]);
    }
    EEPROM.write(EEPROM_MQTT_SERVER_ADDR + mqttServer.length(), '\0');
    EEPROM.put(EEPROM_MQTT_PORT_ADDR, mqttPort);
    EEPROM.commit();
  } else {
    Serial.println("🧠 Use saved MQTT configuration");
  }

  Serial.println("MQTT Server: " + mqttServer);
  Serial.println("MQTT Port: " + String(mqttPort));

  client.setServer(mqttServer.c_str(), mqttPort);
  client.setCallback(mqttCallback); // hier später aktivieren

  lastRandomSwitch = millis();
  lastRandomBlink = millis();
  lastActivity = millis();
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
    /*
    if (!specialIdleMode && (millis() - lastActivity > idleTimeout)) {
      roboEyes.setMood(TIRED);
      roboEyes.close();
      publishStatus("Special Idle: ON");
      specialIdleMode = true;
    }
    */
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
    client.subscribe("roboeyes/blink");
    client.subscribe("roboeyes/open");
    client.subscribe("roboeyes/close");
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
    publishStatus("Special idle: OFF");
  }

  if (String(topic) == "roboeyes/animation") {
    playSelectedAnimation(value);
    randomMode = false;
    publishStatus("Manual animation: " + String(value));
  }
  else if (String(topic) == "roboeyes/random") {
    randomMode = (value == 1);
    lastRandomSwitch = millis();
    publishStatus(String("Random mode: ") + (randomMode ? "ON" : "OFF"));
  }
  else if (String(topic) == "roboeyes/random_blink") {
    roboEyes.setAutoblinker(OFF, 0, 0);
    randomBlinkMode = (value == 1);
    lastRandomBlink = millis();
    publishStatus(String("Random blink mode: ") + (randomBlinkMode ? "ON" : "OFF"));
  }
  else if (String(topic) == "roboeyes/sleep") {
    sleepMode = (value == 1);
    if (sleepMode) {
      roboEyes.setMood(TIRED);
      roboEyes.setIdleMode(ON, 20, 15);
      roboEyes.setAutoblinker(OFF, 0, 0);
      roboEyes.close();
      randomMode = false;
      randomBlinkMode = false;
      publishStatus("Sleep mode: ON");
    } else {
      roboEyes.setMood(DEFAULT);
      roboEyes.setIdleMode(OFF, 0, 0);
      roboEyes.open();
      publishStatus("Sleep mode: OFF");
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

    publishStatus(String("IdleMode changed to: ") + mode + " (" + interval + "s, " + variation + "s)");
  }
  else if (String(topic) == "roboeyes/mood") {
    roboEyes.setMood(value);
    publishStatus("Mood changed to: " + String(value));
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
    publishStatus("Position changed to: " + pos);
  }
  else if (String(topic) == "roboeyes/curiosity") {
    roboEyes.setCuriosity(value == 1);
    publishStatus(String("Curiosity:") + (value == 1 ? "ON" : "OFF"));
  }
  else if (String(topic) == "roboeyes/hflicker") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    String onOff = payloadStr.substring(0, comma);
    int amp = payloadStr.substring(comma + 1).toInt();
    roboEyes.setHFlicker(onOff == "ON", amp);
    publishStatus(String("HFlicker:") + onOff + " with amplitude: " + String(amp));
  }
  else if (String(topic) == "roboeyes/vflicker") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    String onOff = payloadStr.substring(0, comma);
    int amp = payloadStr.substring(comma + 1).toInt();
    roboEyes.setVFlicker(onOff == "ON", amp);
    publishStatus(String("VFlicker: ") + onOff + " with amplitude: " + String(amp));
  }
  else if (String(topic) == "roboeyes/confused") {
    roboEyes.anim_confused();
    publishStatus("Confused animation: ON");
  }
  else if (String(topic) == "roboeyes/laugh") {
    roboEyes.anim_laugh();
    publishStatus("Laugh animation: ON");
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
    randomBlinkMode = false;
    roboEyes.setAutoblinker(active, interval, variation);

    publishStatus(String("AutoBlinker changed to: ") + mode + " (" + interval + "s, " + variation + "s)");
  }
  else if (String(topic) == "roboeyes/width") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.setWidth(left, right);
    publishStatus("Width changed to: L=" + String(left) + " R=" + String(right));
  }
  else if (String(topic) == "roboeyes/height") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.setHeight(left, right);
    publishStatus("Height changed to: L=" + String(left) + " R=" + String(right));
  }
  else if (String(topic) == "roboeyes/borderradius") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.setBorderradius(left, right);
    publishStatus("BorderRadius changed to: L=" + String(left) + " R=" + String(right));
  }
    else if (String(topic) == "roboeyes/blink") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.blink(left, right);
    publishStatus("Blinking with the: L=" + String(left) + " R=" + String(right));
  }
    else if (String(topic) == "roboeyes/close") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.close(left, right);
    publishStatus("Close eyes: L=" + String(left) + " R=" + String(right));
  }
    else if (String(topic) == "roboeyes/open") {
    String payloadStr = String((char*)payload);
    payloadStr.trim();
    int comma = payloadStr.indexOf(',');
    int left = payloadStr.substring(0, comma).toInt();
    int right = payloadStr.substring(comma + 1).toInt();
    roboEyes.open(left, right);
    publishStatus("Open eyes: L=" + String(left) + " R=" + String(right));
  }
  else if (String(topic) == "roboeyes/spacebetween") {
    int space = message.toInt();
    roboEyes.setSpacebetween(space);
    publishStatus("Spacebetween changed to: " + String(space));
  }
  else if (String(topic) == "roboeyes/cyclops") {
    roboEyes.setCyclops(value == 1);
    publishStatus(String("Cyclops: ") + (value == 1 ? "ON" : "OFF"));
  }
}

void publishStatus(const String &text) {
  client.publish("roboeyes/status", text.c_str());
}

void playSelectedAnimation(int id) {
  switch (id) {
    case 0: roboEyes.setMood(HAPPY); roboEyes.anim_laugh(); publishStatus("Animation: 0 | Mode: HAPPY" ); break;
    case 1: roboEyes.setMood(TIRED); roboEyes.blink(1, 0); publishStatus("Animation: 1 | Mode: TIRED"); break;
    case 2: roboEyes.setMood(ANGRY); roboEyes.blink(0, 1); publishStatus("Animation: 2 | Mode: ANGRY"); break;
    case 3: roboEyes.setMood(DEFAULT); roboEyes.anim_confused(); publishStatus("Animation: 3 | Mode: DEFAULT"); break;
    case 4: roboEyes.setMood(HAPPY); roboEyes.open(); publishStatus("Animation: 4 | Mode: HAPPY");break;
    case 5: roboEyes.setMood(TIRED); roboEyes.close(); delay(400); roboEyes.open(); publishStatus("Animation: 5 | Mode: TIRED");break;
    case 6: roboEyes.setMood(DEFAULT); roboEyes.blink(1, 1); publishStatus("Animation: 6 | Mode: DEFAULT");break;
    case 7: roboEyes.setMood(ANGRY); roboEyes.anim_confused(); publishStatus("Animation: 7 | Mode: ANGRY"); break;
    case 8: roboEyes.setMood(HAPPY); roboEyes.blink(1, 1); delay(300); roboEyes.anim_laugh(); publishStatus("Animation: 8 | Mode: HAPPY"); break;
    case 9: roboEyes.setMood(DEFAULT); roboEyes.blink(1, 1); publishStatus("Animation: 9 | Mode: DEFAULT"); break;
    case 10: roboEyes.setMood(DEFAULT); roboEyes.blink(1, 0); publishStatus("Animation: 10 | Mode: DEFAULT"); break;
    case 11: roboEyes.setMood(DEFAULT); roboEyes.blink(0, 1); publishStatus("Animation: 11 | Mode: DEFAULT"); break;
    case 12: roboEyes.setMood(ANGRY); roboEyes.anim_confused();  publishStatus("Animation: 12 | Mode: ANGRY"); break;
    case 13: roboEyes.setMood(TIRED); roboEyes.close(); publishStatus("Animation: 13 | Mode: TIRED"); break;
    case 14: roboEyes.setMood(DEFAULT); roboEyes.open(); publishStatus("Animation: 14 | Mode: DEFAULT"); break;
    case 15: roboEyes.setMood(DEFAULT); roboEyes.anim_confused(); publishStatus("Animation: 15 | Mode: DEFAULT"); break;
    case 16: roboEyes.setMood(TIRED); roboEyes.blink(1, 0); publishStatus("Animation: 1 | Mode: TIRED"); break;
  } 
}

void playRandomAnimation() {
  playSelectedAnimation(random(0, 16));
}

void playRandomBlink() {
  int r = random(0, 4);
  switch (r) {
    case 0: roboEyes.blink(0, 1); publishStatus("Random blink: right"); break;
    case 1: roboEyes.blink(1, 1); delay(200); roboEyes.blink(1, 1); publishStatus("Random blink: Double"); break;
    case 2: roboEyes.blink(1, 1); delay(150); roboEyes.blink(1, 0); delay(150); roboEyes.blink(1, 1); publishStatus("Random blink: Triple"); break;
    case 3: roboEyes.blink(1, 0); publishStatus("Random blink: left"); break;
  }
}

