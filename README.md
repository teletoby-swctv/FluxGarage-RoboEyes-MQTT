# FluxGarage RoboEyes MQTT Projekt

## Projektbeschreibung

Dieses Projekt steuert animierte Roboteraugen auf einem OLED-Display (SH1106) über MQTT-Kommandos.
Es unterstützt Animationen, zufälliges Blinzeln, einen Schlafmodus sowie automatische Idle-Animationen.

Die MQTT-Serverdaten (IP-Adresse und Port) werden bequem über einen **WiFiManager** im Captive Portal konfiguriert.

---

## Hardware

| Komponente             | Beschreibung                       |
|-------------------------|------------------------------------|
| ESP8266 (z.B. NodeMCU)   | Mikrocontroller mit WLAN           |
| OLED Display 1,3" SH1106 | 128x64 Pixel, I2C Interface        |
| Jumper Kabel             | Für die Verbindung                |
| USB-Kabel                | Für die Programmierung und Stromversorgung |

---

## Pinbelegung

| ESP8266 Pin  | OLED Display Pin |
|--------------|------------------|
| 3V3          | VCC               |
| GND          | GND               |
| D2 (GPIO4)   | SDA               |
| D1 (GPIO5)   | SCL               |

*(Standard I2C Pins auf NodeMCU/ESP8266)*

---

## Bibliotheken

Folgende Arduino-Bibliotheken müssen installiert sein:

- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110X)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes)

---

## Einrichtung

1. Alle Bibliotheken installieren (siehe oben).
2. Sketch `FluxGarage_RoboEyes_MQTT_FinalUltraSleepWiFi.ino` in der Arduino IDE öffnen.
3. Board: **ESP8266** auswählen (z.B. NodeMCU 1.0).
4. Sketch auf das Board hochladen.
5. Nach dem ersten Start öffnet der ESP ein WLAN namens **RoboEyes_AP**.
6. Mit dem Smartphone oder PC verbinden.
7. Im Web-Portal:
    - WLAN-Daten eintragen
    - MQTT-Server IP (z.B. 192.168.188.21) und Port (z.B. 1883) eintragen
8. Fertig! RoboEyes verbinden sich automatisch.

---

## MQTT Steuerung

| Topic                  | Payload  | Beschreibung |
|-------------------------|----------|--------------|
| `roboeyes/animation`    | 0–15    | Starte bestimmte Animation |
| `roboeyes/random`       | 1/0      | Zufällige Animationen an/aus |
| `roboeyes/random_blink` | 1/0      | Zufälliges Blinzeln an/aus |
| `roboeyes/sleep`        | 1/0      | Sleep-Modus aktivieren/deaktivieren |
| `roboeyes/status`       | Text     | Statusmeldungen (z.B. "Sleep Modus aktiviert") |

---

## Besondere Funktionen

- **Random Animation Mode:** Alle 6 Sekunden wird automatisch eine neue Animation abgespielt.
- **Random Blink Mode:** Augen blinzeln zufällig mit Single, Double oder Triple Blinks.
- **Sleep Mode:** Augen schließen sich per MQTT-Befehl.
- **Special Idle Mode:** Nach 5 Minuten Inaktivität wechseln die Augen in den "TIRED" Zustand.

---

## Beispiel MQTT Steuerung

Mit `MQTT.fx`, `Node-RED`, `ioBroker` oder einem anderen MQTT-Client kannst du einfach Topics senden:

- `roboeyes/animation` → Payload `3` ➞ verwirrt schauen
- `roboeyes/random` → Payload `1` ➞ zufällige Animationen starten
- `roboeyes/sleep` → Payload `1` ➞ Augen schließen (Schlafmodus)

---

**Viel Spaß beim Nachbauen und Anpassen!** 🎉

---

> Erstellt von [deinem Namen oder GitHub-Username] 🚀


