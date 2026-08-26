# HuaweiSunBridge

[![Ko-fi](https://img.shields.io/badge/Ko--fi-Unterst%C3%BCtzen-orange?logo=ko-fi&logoColor=white)](https://ko-fi.com/pirndi)
[![GitHub Release](https://img.shields.io/github/v/release/pirndi/HuaweiSunBridge)](https://github.com/pirndi/HuaweiSunBridge/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Transparente TCP-Brücke zwischen dem eingebauten WLAN-Access-Point eines Huawei SUN2000
und dem Heimnetz — für Setups, in denen der SDongle nicht verfügbar oder nicht stabil ist,
oder in denen mehrere Wechselrichter ausgelesen werden sollen, die per RS485 nicht
zusammenhängen.

**Getestet und produktiv im Einsatz** mit Huawei SUN2000 + `wlcrs/huawei_solar`
HACS-Integration + Home Assistant.

```
Home Assistant (wlcrs/huawei_solar)     Bridge (WT32-ETH01)      SUN2000
        192.168.x.y          ──LAN──▶   192.168.x.60  ──WLAN──▶  WR-AP
                                         Port 6607                Port 6607
```

## Warum diese Bridge?

Der SUN2000-WR-AP benutzt intern immer dieselbe IP-Adresse und verlangt beim
Verbinden eine Huawei-eigene Authentifizierung (Local O&M Login). Das führt
zu zwei Problemen:

**Problem 1 – Mehrere Wechselrichter:** Wer zwei oder mehr SUN2000 ohne
RS485-Daisy-Chain betreibt, kann nicht beide direkt ins Heimnetz einbinden —
sie haben alle dieselbe interne IP.

**Problem 2 – evcc direkt:** evcc implementiert den Huawei-Login nicht und
kann den WR-AP daher nicht direkt ansprechen. Die Lösung: evcc liest die
Daten über Home Assistant (HA-Integration als Datenquelle), HA pollt über
die Bridge.

**Die Bridge löst beides:** Jede Bridge bekommt eine eigene Adresse im
Heimnetz, leitet den Verkehr transparent durch, und übernimmt die
Verbindungsverwaltung zum jeweiligen Wechselrichter.

## Hardware

| Bauteil | Bezugsquelle | Hinweis |
|---|---|---|
| WT32-ETH01 (ESP32 + LAN8720) | [Amazon.de ↗](https://amzn.to/4wQ0k6O) | 1 Stück pro Wechselrichter |
| FTDI-Programmer (USB-TTL) | [Amazon.de ↗](https://amzn.to/4xnpoU0) | Nur zum Erstflashen nötig |
| 5V-Netzteil ≥ 500 mA | [Amazon.de ↗](https://amzn.to/4y48fyx) | Standard-Option |
| PoE-Splitter 5V (optional) | [Amazon.de ↗](https://amzn.to/4ce3R7z) | Wenn PoE-Switch vorhanden |

> *Diese Seite enthält Affiliate-Links zu Amazon. Als Amazon-Partner verdiene ich
> an qualifizierten Käufen. Der Preis für dich bleibt unverändert.*

## Inbetriebnahme

**Erstflashen (einmalig, USB erforderlich):**

1. IO0-Pin mit GND verbinden, dann Strom anlegen (Bootloader-Modus)
2. `pio run -t upload` — PlatformIO baut und flasht die Firmware
3. IO0-Brücke entfernen, neu starten
4. Ab jetzt: alle Updates per OTA über die Weboberfläche

**Einrichten:**

1. Board per Ethernet ans Heimnetz, Strom anlegen
2. Einrichtungs-AP erscheint: SSID `sunbridge-<MAC>`, Passwort `sunbridge`
   — oder direkt über die DHCP-Adresse per Browser erreichbar, bzw. `sunbridge-<MAC>.local`
3. Weboberfläche öffnen → **Netzwerke suchen** → WLAN des Wechselrichters wählen
   (`SUN2000-<Seriennummer>`), Passwort eintragen → **Speichern**
4. Gerät startet neu und verbindet sich automatisch

**In Home Assistant einrichten:**

HACS → `wlcrs/huawei_solar` → als Modbus-Host die **Ethernet-Adresse der Bridge**
eintragen, Port `6607`. Die Ziel-IP auf der WR-AP-Seite muss nicht konfiguriert
werden — die Bridge ermittelt sie automatisch aus dem Gateway des WR-WLANs.

## Mehrere Wechselrichter

Pro Wechselrichter eine Bridge flashen. Jede Bridge bekommt einen eigenen
Gerätenamen und eine eigene Adresse im Heimnetz (DHCP-Reservierung nach MAC
oder feste IP in der Weboberfläche). Kein Routing, kein NAT, keine statischen
Routen im Router — die Bridges wissen nichts voneinander.

In HA werden dann einfach mehrere Instanzen der `wlcrs/huawei_solar`-Integration
angelegt, jede mit der IP der zugehörigen Bridge.

## Weboberfläche

- **Signalweg-Anzeige** Heimnetz → Bridge → Wechselrichter mit Live-Status
- **WLAN-Scanner** — kein manuelles Abtippen der SSID
- **Statuszähler** — Sitzungen, übertragene Bytes, WLAN-Pegel, letzter Fehler
- **Log-Konsole** — Live-Ausgabe direkt im Browser
- **Firmware-Badge** im Header — zeigt an ob ein Update verfügbar ist
- **OTA-Update** — Firmware direkt über den Browser einspielen
- **Netzwerk-Tab** — DHCP oder feste IP, Gerätename, Ethernet-Einstellungen

## Kompatibilität

| Software | Status |
|---|---|
| `wlcrs/huawei_solar` (HACS) | ✅ Getestet, produktiv |
| evcc (direkt) | ❌ Huawei-Login nicht implementiert |
| evcc (über HA-Sensoren) | ✅ Funktioniert über `wlcrs/huawei_solar` |

## Bekannte Stolpersteine

**`ETH.begin()` — Arduino-Core-Versionen:** Core 2.x und 3.x haben unterschiedliche
Argumentreihenfolgen. `net.cpp` schreibt beide Varianten aus und wählt per
`ESP_ARDUINO_VERSION_MAJOR`. Die Plattform ist in `platformio.ini` auf
`espressif32@6.9.0` gepinnt (Core 2.x).

**Pins des WT32-ETH01 sind fix:** PHY-Adresse 1, Power GPIO16, MDC GPIO23,
MDIO GPIO18, Takt extern über GPIO0. GPIO0 ist dadurch als freier Pin belegt.

**Stromversorgung:** Das Board zieht Spitzen bis ~500 mA. Ein schwaches Netzteil
äußert sich als sporadischer Reset oder als Ethernet, das nicht hochkommt.

**Flash-Größe:** Bei ~85 % Flash-Auslastung ist noch ausreichend Platz für OTA
(Update-Partition braucht die Hälfte des Flash). Bei künftigen Erweiterungen im Blick
behalten.

## Unterstützung

Falls dir das Projekt Zeit gespart hat:

[![Ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/pirndi)

## Lizenz

MIT — siehe [LICENSE](LICENSE)