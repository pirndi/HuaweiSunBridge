#include "../include/net.h"
#include "../include/config.h"

#include <ETH.h>
#include <WiFi.h>

NetStatus gNet;

static bool     s_ethStarted = false;
static uint32_t s_lastWifiAttempt = 0;
static uint32_t s_wifiBackoffMs = 5000;
static bool     s_scanRunning = false;
static bool     s_scanEverStarted = false;

// ---------------------------------------------------------------------------
// Ethernet
// ---------------------------------------------------------------------------

static void onNetEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            ETH.setHostname(configEffectiveHostname().c_str());
            break;

        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println(F("[ETH ] Link erkannt"));
            break;

        case ARDUINO_EVENT_ETH_GOT_IP:
            gNet.ethUp         = true;
            gNet.ethIp         = ETH.localIP();
            gNet.ethGateway    = ETH.gatewayIP();
            gNet.ethSubnet     = ETH.subnetMask();
            gNet.ethLinkSpeed  = ETH.linkSpeed();
            gNet.ethFullDuplex = ETH.fullDuplex();
            Serial.printf("[ETH ] IP %s  (%u Mbit/s, %s)\n",
                          gNet.ethIp.toString().c_str(),
                          gNet.ethLinkSpeed,
                          gNet.ethFullDuplex ? "Full Duplex" : "Half Duplex");
            break;

        case ARDUINO_EVENT_ETH_DISCONNECTED:
        case ARDUINO_EVENT_ETH_STOP:
            gNet.ethUp = false;
            gNet.ethIp = IPAddress((uint32_t)0);
            Serial.println(F("[ETH ] Link verloren"));
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            gNet.wifiUp      = true;
            gNet.wifiSsid    = WiFi.SSID();
            gNet.wifiIp      = WiFi.localIP();
            gNet.wifiGateway = WiFi.gatewayIP();
            s_wifiBackoffMs  = 5000;
            Serial.printf("[WLAN] Verbunden mit '%s', IP %s, Gateway %s\n",
                          gNet.wifiSsid.c_str(),
                          gNet.wifiIp.toString().c_str(),
                          gNet.wifiGateway.toString().c_str());
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            if (gNet.wifiUp) {
                gNet.wifiReconnects++;
                Serial.println(F("[WLAN] Verbindung zum WR-AP verloren"));
            }
            gNet.wifiUp = false;
            gNet.wifiIp = IPAddress((uint32_t)0);
            break;

        default:
            break;
    }
}

static bool ethStart() {
    // Arduino-Core 3.x hat die Argumentreihenfolge von ETH.begin() geaendert.
    // Beide Varianten sind hier explizit ausgeschrieben, damit der Aufruf
    // nicht von Standardwerten oder Reihenfolge-Annahmen abhaengt.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    return ETH.begin(ETH_PHY_LAN8720,
                     SB_ETH_PHY_ADDR,
                     SB_ETH_PHY_MDC,
                     SB_ETH_PHY_MDIO,
                     SB_ETH_PHY_POWER,
                     ETH_CLOCK_GPIO0_IN);
#else
    return ETH.begin(SB_ETH_PHY_ADDR,
                     SB_ETH_PHY_POWER,
                     SB_ETH_PHY_MDC,
                     SB_ETH_PHY_MDIO,
                     ETH_PHY_LAN8720,
                     ETH_CLOCK_GPIO0_IN);
#endif
}

// ---------------------------------------------------------------------------
// WLAN
// ---------------------------------------------------------------------------

static void startSetupAp() {
    String ssid = configEffectiveHostname();
    WiFi.softAP(ssid.c_str(), SB_SETUP_AP_PASS);
    gNet.setupApActive = true;
    gNet.setupApSsid   = ssid;
    Serial.printf("[WLAN] Einrichtungs-AP '%s' aktiv, Passwort '%s', IP %s\n",
                  ssid.c_str(), SB_SETUP_AP_PASS,
                  WiFi.softAPIP().toString().c_str());
}

static void connectWrAp() {
    if (gConfig.wrSsid.length() == 0) return;
    Serial.printf("[WLAN] Verbinde mit WR-AP '%s' ...\n", gConfig.wrSsid.c_str());
    WiFi.begin(gConfig.wrSsid.c_str(),
               gConfig.wrPassword.length() ? gConfig.wrPassword.c_str() : nullptr);
    s_lastWifiAttempt = millis();
}

// ---------------------------------------------------------------------------
// Oeffentliche API
// ---------------------------------------------------------------------------

void netBegin() {
    WiFi.onEvent(onNetEvent);

    // AP_STA: der Einrichtungs-AP bleibt erreichbar, auch wenn die
    // STA-Verbindung zum Wechselrichter gerade haengt.
    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);          // Sleep kostet hier nur Latenz
    WiFi.setAutoReconnect(true);

    s_ethStarted = ethStart();
    if (!s_ethStarted) {
        Serial.println(F("[ETH ] FEHLER: ETH.begin() fehlgeschlagen"));
    } else if (!gConfig.ethDhcp && (uint32_t)gConfig.ethIp != 0) {
        // Statische Adresse erst nach begin() setzen
        ETH.config(gConfig.ethIp, gConfig.ethGateway, gConfig.ethSubnet, gConfig.ethDns);
        Serial.printf("[ETH ] Statische Adresse %s\n", gConfig.ethIp.toString().c_str());
    }

    if (gConfig.isProvisioned()) {
        connectWrAp();
    } else {
        Serial.println(F("[CFG ] Keine WR-AP-Zugangsdaten hinterlegt"));
    }

    // Der Einrichtungs-AP laeuft immer mit: bei Fehlkonfiguration ist das
    // Geraet sonst ueber gar keinen Weg mehr erreichbar.
    startSetupAp();
}

void netLoop() {
    // Reconnect mit ansteigender Wartezeit (max. 60 s)
    if (gConfig.isProvisioned() && !gNet.wifiUp && !s_scanRunning) {
        if (millis() - s_lastWifiAttempt > s_wifiBackoffMs) {
            WiFi.disconnect();
            connectWrAp();
            s_wifiBackoffMs = min<uint32_t>(s_wifiBackoffMs * 2, 60000);
        }
    }

    if (gNet.wifiUp) gNet.wifiRssi = WiFi.RSSI();

    // Scan-Ende einsammeln
    if (s_scanRunning) {
        int n = WiFi.scanComplete();
        if (n >= 0 || n == WIFI_SCAN_FAILED) {
            s_scanRunning = false;
            // Nach dem Scan sofort wieder verbinden duerfen
            s_lastWifiAttempt = 0;
        }
    }
}

IPAddress netResolveTarget() {
    if (gConfig.targetAuto) return gNet.wifiGateway;
    return gConfig.targetIp;
}

bool netTargetValid() {
    IPAddress t = netResolveTarget();
    return gNet.wifiUp && (uint32_t)t != 0;
}

void netScanStart() {
    if (s_scanRunning) return;
    WiFi.scanDelete();
    WiFi.scanNetworks(true /* async */, true /* show hidden */);
    s_scanRunning     = true;
    s_scanEverStarted = true;
}

int netScanState() {
    if (!s_scanEverStarted) return -2;
    if (s_scanRunning) return -1;
    int n = WiFi.scanComplete();
    return (n < 0) ? 0 : n;
}
