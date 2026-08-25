#pragma once
#include <Arduino.h>

#define SB_MAX_SESSIONS     3
#define SB_RELAY_BUF        1460    // eine TCP-Nutzlast, kein Zerstueckeln
#define SB_IDLE_TIMEOUT_MS  60000
#define SB_CONNECT_TIMEOUT  4000

struct RelayStats {
    bool     listening = false;
    uint16_t listenPort = 0;

    uint8_t  activeSessions = 0;
    uint32_t sessionsTotal = 0;
    uint32_t sessionsRejected = 0;   // kein Slot frei
    uint32_t upstreamFailures = 0;   // Wechselrichter nicht erreichbar

    uint64_t bytesToInverter = 0;
    uint64_t bytesToClient = 0;

    uint32_t lastActivityMs = 0;     // millis() der letzten Nutzdaten
    String   lastError;
};

extern RelayStats gRelay;

void relayBegin();
void relayLoop();
// Server auf neuem Port neu binden (nach Konfigaenderung)
void relayRestart();
