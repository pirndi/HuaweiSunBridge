#pragma once
#include <Arduino.h>
#include <IPAddress.h>

// Persistente Geraetekonfiguration.
// Nichts davon ist im Code fest verdrahtet - alles kommt aus dem NVS und
// wird ueber die Weboberflaeche gesetzt.
struct DeviceConfig {
    // --- WLAN-Verbindung zum WR-AP des Wechselrichters ---
    String wrSsid;              // leer = noch nicht eingerichtet
    String wrPassword;

    // --- Ziel der Weiterleitung (im WR-AP-Netz) ---
    // targetAuto: Ziel-IP = Gateway der WLAN-Verbindung. Der SUN2000 ist im
    // eigenen AP immer selbst das Gateway, damit entfaellt jede feste IP.
    bool      targetAuto = true;
    IPAddress targetIp   = IPAddress((uint32_t)0);
    uint16_t  targetPort = 6607;

    // --- Ethernet-Seite (Heimnetz) ---
    bool      ethDhcp = true;
    IPAddress ethIp      = IPAddress((uint32_t)0);
    IPAddress ethGateway = IPAddress((uint32_t)0);
    IPAddress ethSubnet  = IPAddress(255, 255, 255, 0);
    IPAddress ethDns     = IPAddress((uint32_t)0);

    // --- Server-Seite ---
    uint16_t listenPort = 6607;

    // Freier Name, damit sich mehrere Bridges unterscheiden lassen
    // (leer = automatisch "sunbridge-<MAC-Suffix>")
    String hostname;

    bool isProvisioned() const { return wrSsid.length() > 0; }
};

// Globale Instanz (in config.cpp definiert)
extern DeviceConfig gConfig;

void configLoad();
bool configSave();
void configFactoryReset();

// Hostname inkl. Fallback auf "sunbridge-<MAC-Suffix>"
String configEffectiveHostname();
