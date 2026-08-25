#include "../include/relay.h"
#include "../include/net.h"
#include "../include/config.h"

#include <WiFi.h>

RelayStats gRelay;

struct Session {
    WiFiClient client;      // Heimnetz-Seite (Home Assistant, evcc, ...)
    WiFiClient upstream;    // WR-AP-Seite (Wechselrichter)
    uint32_t   lastActivity = 0;
    bool       inUse = false;
};

static WiFiServer *s_server = nullptr;
static Session     s_sessions[SB_MAX_SESSIONS];
static uint8_t     s_buf[SB_RELAY_BUF];

// ---------------------------------------------------------------------------

static void closeSession(Session &s, const char *reason) {
    if (!s.inUse) return;
    s.client.stop();
    s.upstream.stop();
    s.inUse = false;
    Serial.printf("[RLY ] Sitzung beendet (%s)\n", reason);
}

// Kopiert alles Verfuegbare von 'from' nach 'to'. Rueckgabe: Bytes.
// Bricht ab, sobald 'to' nicht mehr aufnahmefaehig ist - kein Blockieren.
static size_t pump(WiFiClient &from, WiFiClient &to, uint64_t &counter) {
    size_t moved = 0;
    while (from.connected() || from.available()) {
        int avail = from.available();
        if (avail <= 0) break;
        if (!to.connected()) break;

        int want = avail > (int)sizeof(s_buf) ? (int)sizeof(s_buf) : avail;
        int got  = from.read(s_buf, want);
        if (got <= 0) break;

        int written = to.write(s_buf, got);
        if (written > 0) {
            moved   += written;
            counter += written;
        }
        if (written < got) break;   // Gegenstelle nimmt gerade nichts mehr
    }
    return moved;
}

static bool openUpstream(Session &s) {
    if (!netTargetValid()) {
        gRelay.lastError = F("Kein WLAN zum Wechselrichter");
        gRelay.upstreamFailures++;
        return false;
    }

    IPAddress target = netResolveTarget();
    if (!s.upstream.connect(target, gConfig.targetPort, SB_CONNECT_TIMEOUT)) {
        gRelay.lastError = String(F("Wechselrichter ")) + target.toString() +
                           F(" nicht erreichbar");
        gRelay.upstreamFailures++;
        Serial.printf("[RLY ] %s\n", gRelay.lastError.c_str());
        return false;
    }

    s.upstream.setNoDelay(true);
    Serial.printf("[RLY ] Upstream offen: %s:%u\n",
                  target.toString().c_str(), gConfig.targetPort);
    return true;
}

// ---------------------------------------------------------------------------

void relayBegin() {
    if (s_server) {
        s_server->end();
        delete s_server;
        s_server = nullptr;
    }

    s_server = new WiFiServer(gConfig.listenPort);
    s_server->begin();
    s_server->setNoDelay(true);

    gRelay.listening  = true;
    gRelay.listenPort = gConfig.listenPort;
    gRelay.lastError  = "";

    Serial.printf("[RLY ] Lausche auf Port %u\n", gConfig.listenPort);
}

void relayRestart() {
    for (auto &s : s_sessions) closeSession(s, "Neustart");
    relayBegin();
}

void relayLoop() {
    if (!s_server) return;

    // --- Neue Verbindung annehmen ---
    if (s_server->hasClient()) {
        Session *slot = nullptr;
        for (auto &s : s_sessions) {
            if (!s.inUse) { slot = &s; break; }
        }

        WiFiClient incoming = s_server->available();

        if (!slot) {
            gRelay.sessionsRejected++;
            gRelay.lastError = F("Alle Sitzungsplaetze belegt");
            incoming.stop();
        } else {
            slot->client = incoming;
            slot->client.setNoDelay(true);
            slot->lastActivity = millis();

            if (openUpstream(*slot)) {
                slot->inUse = true;
                gRelay.sessionsTotal++;
                Serial.printf("[RLY ] Client %s verbunden\n",
                              slot->client.remoteIP().toString().c_str());
            } else {
                slot->client.stop();
            }
        }
    }

    // --- Datenfluss in beide Richtungen ---
    uint8_t active = 0;
    for (auto &s : s_sessions) {
        if (!s.inUse) continue;

        size_t up   = pump(s.client, s.upstream, gRelay.bytesToInverter);
        size_t down = pump(s.upstream, s.client, gRelay.bytesToClient);

        if (up || down) {
            s.lastActivity      = millis();
            gRelay.lastActivityMs = s.lastActivity;
        }

        // Eine Seite weg und nichts mehr im Puffer -> aufraeumen
        if ((!s.client.connected() && !s.client.available()) ||
            (!s.upstream.connected() && !s.upstream.available())) {
            closeSession(s, "Gegenstelle getrennt");
            continue;
        }

        if (millis() - s.lastActivity > SB_IDLE_TIMEOUT_MS) {
            closeSession(s, "Zeitueberschreitung");
            continue;
        }

        active++;
    }
    gRelay.activeSessions = active;
}
