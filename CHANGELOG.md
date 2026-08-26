# Changelog

Alle relevanten Änderungen werden hier dokumentiert.
Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/).

## [0.1.0] – 2025-08-26

### Hinzugefügt
- Transparenter TCP-Relay zwischen Heimnetz (Ethernet) und SUN2000-WR-AP (WLAN)
- Weboberfläche mit Tab-Navigation (Übersicht / Netzwerk)
- WLAN-Scanner — kein manuelles Abtippen der SSID nötig
- Signalweg-Anzeige mit Live-Status (Heimnetz → Bridge → Wechselrichter)
- Log-Konsole im Browser per WebSocket
- Firmware-Update-Badge im Header (zeigt neue GitHub-Release an)
- OTA-Update direkt über die Weboberfläche
- DHCP oder feste IP für die Ethernet-Seite konfigurierbar
- Automatische Ziel-IP-Ermittlung über WR-AP-Gateway (kein manuelles Eintragen)
- Einrichtungs-AP bei fehlender Konfiguration (`sunbridge-<MAC>`)
- mDNS-Erreichbarkeit (`sunbridge-<MAC>.local`)
- NVS-Persistenz für alle Einstellungen
- Unterstützung mehrerer Wechselrichter durch je eine Bridge pro Gerät
- Kompatibilität mit Arduino-Core 2.x und 3.x (`ETH.begin()`-Versionsweiche)
- GitHub Actions Workflow: automatischer Firmware-Build bei neuem Tag

### Getestet mit
- Huawei SUN2000-8KTL-M2 (Hybrid)
- `wlcrs/huawei_solar` HACS-Integration
- Home Assistant + evcc (über HA-Sensoren)

## [Unreleased]

_(Hier landen Änderungen, die noch kein Release haben)_