# FluxGarage RoboEyes MQTT Projekt (Ultra Complete Version)

## Projektbeschreibung

Dieses Projekt steuert animierte Roboteraugen über MQTT vollständig:

- Zufällige Animationen (roboeyes/random)
- Sleep-Mode aktivieren/deaktivieren (roboeyes/sleep)
- Idle-Mode steuern (roboeyes/IdleMode)
- Stimmung ändern (roboeyes/mood)
- Blickrichtung setzen (roboeyes/position)
- Curiosity-Effekt aktivieren/deaktivieren (roboeyes/curiosity)
- Horizontal/Vertikal Flicker an/aus (roboeyes/hflicker, roboeyes/vflicker)
- Autoblinker konfigurieren (roboeyes/autoblinker)
- Augenbreite/Höhe/Rundung einstellen (roboeyes/width, roboeyes/height, roboeyes/borderradius)
- Augenabstand ändern (roboeyes/spacebetween)
- Cyclops-Modus aktivieren (roboeyes/cyclops)
- Spezialanimationen abspielen (roboeyes/confused, roboeyes/laugh)
- Statusmeldungen empfangen (roboeyes/status)

---

## Hardware

| Komponente               | Beschreibung |
|--------------------------|---------------|
| ESP8266 (z.B. NodeMCU)   | Mikrocontroller mit WLAN |
| OLED Display 1,3" SH1106 | 128x64 Pixel, I2C |
| Jumper Kabel             | Verbindungskabel |
| USB-Kabel                | Programmierung und Stromversorgung |

---

## Pinbelegung

| ESP8266 Pin | OLED Display Pin  |
|-------------|-------------------|
| 3V3         | VCC               |
| GND         | GND               |
| D2 (GPIO4)  | SDA               |
| D1 (GPIO5)  | SCL               |

*(Standard I2C-Anschlussbelegung)*

---

## MQTT Topics

|            Topic        | Payload | Beschreibung |
|:------------------------|:--------|:-------------|
| `roboeyes/animation`    | 0–15 | Bestimmte Animation abspielen |
| `roboeyes/random`       | 1/0 | Zufällige Animationen ein/aus |
| `roboeyes/random_blink` | 1/0 | Zufälliges Blinzeln ein/aus |
| `roboeyes/sleep`        | 1/0 | Sleep-Modus aktivieren/deaktivieren |
| `roboeyes/IdleMode`     | ON,2,2 oder OFF,0,0 | Idle Mode aktivieren/deaktivieren mit Intervall |
| `roboeyes/mood`         | 0=DEFAULT, 1=TIRED, 2=ANGRY, 3=HAPPY | Stimmung setzen |
| `roboeyes/position`     | N, NE, E, SE, S, SW, W, NW, DEFAULT | Blickrichtung setzen |
| `roboeyes/curiosity`    | 1/0 | Curiosity (seitliche Deformation) aktivieren/deaktivieren |
| `roboeyes/hflicker`     | ON,2 oder OFF,0 | Horizontal Flicker aktivieren/deaktivieren mit Amplitude |
| `roboeyes/vflicker`     | ON,2 oder OFF,0 | Vertikal Flicker aktivieren/deaktivieren mit Amplitude |
| `roboeyes/confused`     | 1 | Verwirrte Animation abspielen |
| `roboeyes/laugh`        | 1 | Lachende Animation abspielen |
| `roboeyes/autoblinker`  | ON,3,2 oder OFF,0,0 | Autoblinker konfigurieren |
| `roboeyes/width`        | 36,40 | Augenbreite links,rechts setzen |
| `roboeyes/height`       | 30,34 | Augenhöhe links,rechts setzen |
| `roboeyes/borderradius` | 8,10 | Augenrundung links,rechts setzen |
| `roboeyes/spacebetween` | 10 | Abstand zwischen den Augen setzen |
| `roboeyes/cyclops`      | 1/0 | Cyclops-Modus ein/aus (Einzelauge) |
| `roboeyes/status`       | Text | Statusmeldungen vom Gerät |

---

## Installation

1. Lade die Arduino-Bibliotheken:
   - **Adafruit GFX**
   - **Adafruit SH110X**
   - **WiFiManager**
   - **PubSubClient**
   - **FluxGarage RoboEyes**

2. Verbinde dein ESP8266-Board per USB.

3. Flashe den Sketch.

4. Beim ersten Start öffnet sich der WiFiManager:
   - Verbinde dich mit "RoboEyes_AP"
   - Gib dein WLAN ein
   - Gib die IP und den Port deines MQTT-Servers ein

5. Danach ist dein RoboEyes-System MQTT-gesteuert online!

---

## Beispiel MQTT Befehle

- **Animation starten:**  
  `roboeyes/animation` → `3` → Verwirrte Augenanimation

- **Random Modus aktivieren:**  
  `roboeyes/random` → `1`

- **Cyclops Modus aktivieren:**  
  `roboeyes/cyclops` → `1`

- **Augenbreite ändern:**  
  `roboeyes/width` → `36,40`

- **AutoBlinker deaktivieren:**  
  `roboeyes/autoblinker` → `OFF,0,0`

---

**Viel Spaß beim Steuern der RoboEyes! 🚀👀**

---
> Erstellt von teletoby-swctv
