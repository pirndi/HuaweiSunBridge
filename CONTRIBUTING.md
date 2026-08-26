# Beitragen

Danke für dein Interesse! Jede Hilfe ist willkommen — egal ob Fehlerbericht,
Verbesserungsvorschlag oder Code.

## Fehler melden

Bitte öffne ein [GitHub Issue](https://github.com/pirndi/HuaweiSunBridge/issues) und
füge folgende Informationen bei:

- SUN2000-Modell und Firmware-Version (steht in der FusionSolar-App unter „Gerät")
- Welche Integration du verwendest (`wlcrs/huawei_solar`, evcc, …)
- Was du erwartet hast und was stattdessen passiert ist
- Serial-Monitor-Ausgabe wenn vorhanden (Log-Konsole in der Weboberfläche oder USB)

## Entwicklungsumgebung einrichten

**Voraussetzungen:**
- [PlatformIO](https://platformio.org/) (VS Code Extension oder CLI)
- Python ≥ 3.9

**Projekt klonen und bauen:**
```bash
git clone https://github.com/pirndi/HuaweiSunBridge.git
cd HuaweiSunBridge
pio run -e wt32-eth01
```

**Flashen (erstmalig per USB, danach per OTA):**
```bash
# IO0 auf GND, dann Strom anlegen (Bootloader-Modus)
pio run -t upload -e wt32-eth01

# OTA (wenn Bridge bereits läuft und IP bekannt):
curl -X POST http://<bridge-ip>/update \
  -F "firmware=@.pio/build/wt32-eth01/firmware.bin"
```

**Projektstruktur:**

| Datei | Inhalt |
|---|---|
| `src/main.cpp` | Einstiegspunkt, orchestriert alle Module |
| `src/config.cpp` | NVS-Persistenz (Einstellungen lesen/schreiben) |
| `src/net.cpp` | Ethernet + WiFi-STA + Setup-AP + WLAN-Scanner |
| `src/relay.cpp` | TCP-Relay-Logik (nicht-blockierend, mehrere Sitzungen) |
| `src/web.cpp` | AsyncWebServer: API-Endpoints, WebSocket, OTA, Update-Check |
| `include/page.h` | Komplette Weboberfläche als PROGMEM-HTML (Single-File) |
| `include/logger.h` | Log-Ringpuffer + WebSocket-Broadcast |

## Pull Request einreichen

1. Fork erstellen
2. Feature-Branch anlegen: `git checkout -b feat/mein-feature`
3. Änderungen committen (bitte auf Englisch, Präsens: `Add X`, `Fix Y`)
4. `pio run` muss ohne Fehler durchlaufen
5. PR gegen `main` öffnen — kurze Beschreibung was und warum

**Was besonders willkommen ist:**
- Testergebnisse mit anderen SUN2000-Modellen oder Firmware-Versionen
- Stabilitätsverbesserungen beim Reconnect-Verhalten
- Unterstützung für andere ESP32-Ethernet-Boards (Olimex ESP32-POE, LILYGO T-ETH)
- Übersetzungen der Weboberfläche

**Bitte vorab ein Issue öffnen** bei größeren Änderungen (neue Features, Refactoring),
damit wir uns abstimmen können bevor du viel Zeit investierst.

## Hinweise zur Hardware

Der WT32-ETH01 hat **keinen USB-Anschluss** — zum Erstflashen wird ein FTDI-Programmer
(3,3 V) benötigt. Die Pinbelegung für den Programmer:

| FTDI | WT32-ETH01 |
|---|---|
| TX | RX |
| RX | TX |
| GND | GND |
| 3V3 | 3V3 |
| GND | IO0 (nur beim Flashen, danach trennen) |

Die Ethernet-PHY-Pins (LAN8720) sind fest: PHY-Adresse 1, Power GPIO16,
MDC GPIO23, MDIO GPIO18, Takt extern über GPIO0.