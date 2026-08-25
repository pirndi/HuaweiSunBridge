#pragma once
#include <Arduino.h>
#include <IPAddress.h>

// --- WT32-ETH01: LAN8720 ---
// Diese Pins sind board-spezifisch und in mehreren Quellen bestaetigt
// (ESPHome-Boardliste, Wireless-Tag-Datenblatt).
#define SB_ETH_PHY_ADDR   1
#define SB_ETH_PHY_POWER  16
#define SB_ETH_PHY_MDC    23
#define SB_ETH_PHY_MDIO   18

// Passwort des Einrichtungs-AP, der bei leerer Konfiguration aufgeht.
#define SB_SETUP_AP_PASS  "sunbridge"

struct NetStatus {
    bool      ethUp = false;
    IPAddress ethIp;
    IPAddress ethGateway;
    IPAddress ethSubnet;
    uint8_t   ethLinkSpeed = 0;
    bool      ethFullDuplex = false;

    bool      wifiUp = false;
    String    wifiSsid;
    IPAddress wifiIp;
    IPAddress wifiGateway;   // == der Wechselrichter selbst
    int32_t   wifiRssi = 0;
    uint32_t  wifiReconnects = 0;

    bool      setupApActive = false;
    String    setupApSsid;
};

extern NetStatus gNet;

void netBegin();
void netLoop();

// Ziel-IP der Weiterleitung: entweder das WLAN-Gateway (= Wechselrichter)
// oder die manuell gesetzte Adresse. Ungueltig, solange kein WLAN steht.
IPAddress netResolveTarget();
bool      netTargetValid();

// --- WLAN-Scan (asynchron, damit die Weboberflaeche nicht blockiert) ---
void netScanStart();
// -1 = laeuft noch, -2 = noch nicht gestartet, sonst Anzahl Netze
int  netScanState();
