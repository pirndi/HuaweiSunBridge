# HuaweiSunBridge

Transparente TCP-Brücke zwischen dem eingebauten WLAN-Access-Point eines
Huawei SUN2000 und dem Heimnetz — für Setups, in denen Modbus über den SDongle
nicht verfügbar oder nicht stabil ist, oder in denen mehrere Wechselrichter
ausgelesen werden sollen, die per RS485 nicht zusammenhängen.

Jeder SUN2000-AP benutzt intern dieselbe Adresse. Eine Bridge pro
Wechselrichter löst das: jede bekommt eine eigene Adresse im Heimnetz und
reicht den Verkehr an „ihren" Wechselrichter weiter.

```
Home Assistant / evcc          Bridge (WT32-ETH01)         SUN2000
   192.168.x.y      ──LAN──▶   192.168.x.60   ──WLAN──▶   WR-AP-Gateway
                               Port 6607                   Port 6607
```

## Hardware

- **WT32-ETH01** (ESP32 + LAN8720 Ethernet-PHY)
- 5-V-Netzteil, mindestens 500 mA (das Board hat kein PoE)
- USB-TTL-Adapter zum Flashen (kein USB-Anschluss an Bord)

Flashen: IO0 während des Einschaltens auf GND, danach `pio run -t upload`.
Ab dann geht alles per OTA über die Weboberfläche.

## Inbetriebnahme

1. Board flashen und ans Netzwerk hängen.
2. Es spannt einen Einrichtungs-AP auf: SSID `sunbridge-<MAC>`,
   Passwort `sunbridge`. Alternativ ist es sofort über die per DHCP bezogene
   Ethernet-Adresse erreichbar, oder unter `sunbridge-<MAC>.local`.
3. Weboberfläche öffnen → **Netzwerke suchen** → WLAN des Wechselrichters
   wählen (`SUN2000-<Seriennummer>`), Passwort eintragen.
4. Speichern. Das Gerät startet neu und verbindet sich.
5. In Home Assistant (Integration `wlcrs/huawei_solar`) bzw. evcc als
   Modbus-Host die **Ethernet-Adresse der Bridge** eintragen, Port 6607.

Die Ziel-Adresse muss nirgends eingetragen werden: im WR-AP ist der
Wechselrichter selbst das Gateway, und genau das nimmt die Bridge als Ziel.
Manuell überschreiben geht trotzdem.

## Mehrere Wechselrichter

Eine Bridge pro Wechselrichter flashen, jeder einen eigenen Gerätenamen und
eine eigene Adresse geben (DHCP-Reservierung oder feste Adresse in der
Oberfläche). Kein Routing, kein NAT, keine statischen Routen im Router —
die Bridges wissen nichts voneinander.

## Weboberfläche

- Signalweg-Anzeige Heimnetz → Bridge → Wechselrichter, jeweils mit Zustand
- Statuszähler: aktive Sitzungen, übertragene Bytes je Richtung, Zeit seit den
  letzten Nutzdaten, WLAN-Pegel, letzter Fehler
- WLAN-Scanner statt Abtippen der SSID
- DHCP oder feste Adresse für die Ethernet-Seite
- Firmware-Update über den Browser

Nutzdaten werden bewusst **nicht** interpretiert. Die Bridge ist ein
Byte-Weiterleiter; die Auswertung macht Home Assistant.

## Bekannte Stolpersteine

- **`ETH.begin()`**: Arduino-Core 2.x und 3.x haben unterschiedliche
  Argumentreihenfolgen. `net.cpp` schreibt beide Varianten aus und wählt per
  `ESP_ARDUINO_VERSION_MAJOR`. Die Plattform ist in `platformio.ini` auf
  `espressif32@6.9.0` gepinnt (Core 2.x).
- **Pins des WT32-ETH01** sind fix: PHY-Adresse 1, Power GPIO16, MDC GPIO23,
  MDIO GPIO18, Takt extern über GPIO0. GPIO0 ist dadurch belegt.
- **Nur eine Modbus-Verbindung**: Der Wechselrichter selbst erlaubt meist nur
  einen Client gleichzeitig. Die Bridge nimmt zwar mehrere Sitzungen an, aber
  wenn Home Assistant *und* evcc parallel pollen, kann der Wechselrichter
  Fehler liefern. Empfehlung: nur Home Assistant lesen lassen, evcc über die
  HA-Sensoren versorgen.
- **Stromversorgung**: Das Board zieht Spitzen bis ~500 mA. Ein schwaches
  Netzteil äußert sich als sporadischer Reset oder als Ethernet, das nicht
  hochkommt.

## Stand

Grundgerüst, noch nicht auf Hardware verifiziert. Getestet werden muss der
Reihe nach: Ethernet-Link und Adresse, WLAN-Verbindung zum WR-AP,
Weiterleitung mit einem Modbus-Client, OTA.

## Lizenz

MIT
