#include "web.h"
#include "page.h"
#include "config.h"
#include "net.h"
#include "relay.h"
#include "logger.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Update.h>

#ifndef FW_VERSION
#define FW_VERSION "0.0.0"
#endif

#define GITHUB_API "https://api.github.com/repos/pirndi/HuaweiSunBridge/releases/latest"

static AsyncWebServer   server(80);
static AsyncWebSocket   ws("/ws");
static uint32_t         s_rebootAt   = 0;
static String           s_fwLatest   = "?";
static uint32_t         s_lastFwCheck = 0;

static void scheduleReboot(uint32_t ms) { s_rebootAt = millis() + ms; }

// --------------------------------------------------------------------------
// WebSocket
// --------------------------------------------------------------------------

static void wsBroadcast(const String &msg) {
    ws.textAll(msg);
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        // Ringpuffer an neuen Client schicken
        JsonDocument doc;
        doc["t"] = "logbuf";
        JsonArray arr = doc["buf"].to<JsonArray>();
        String raw = logGetBuffer();
        JsonDocument tmp;
        deserializeJson(tmp, raw);
        for (auto v : tmp.as<JsonArray>()) arr.add(v);
        String out;
        serializeJson(doc, out);
        client->text(out);
    }
}

// --------------------------------------------------------------------------
// GitHub Update-Check (einmal beim Start + alle 6h)
// --------------------------------------------------------------------------

static void checkFwUpdate() {
    if (!gNet.ethUp) return;
    HTTPClient http;
    http.begin(GITHUB_API);
    http.addHeader("User-Agent", "HuaweiSunBridge");
    http.setTimeout(6000);
    int code = http.GET();
    if (code == 200) {
        JsonDocument doc;
        if (!deserializeJson(doc, http.getStream())) {
            const char *tag = doc["tag_name"];
            if (tag) s_fwLatest = String(tag);
        }
    }
    http.end();
    s_lastFwCheck = millis();
    logPrintf("[OTA ] Neueste Version: %s", s_fwLatest.c_str());
}

// --------------------------------------------------------------------------
// Hilfsfunktionen
// --------------------------------------------------------------------------

static String ipOrEmpty(const IPAddress &ip) {
    return ((uint32_t)ip == 0) ? String("") : ip.toString();
}

static bool parseIp(const char *s, IPAddress &out) {
    if (!s || !*s) { out = IPAddress((uint32_t)0); return true; }
    return out.fromString(s);
}

// --------------------------------------------------------------------------
// API-Handler
// --------------------------------------------------------------------------

static void handleStatus(AsyncWebServerRequest *req) {
    JsonDocument doc;
    doc["host"]     = configEffectiveHostname();
    doc["fw"]       = FW_VERSION;
    doc["fwLatest"] = s_fwLatest;
    doc["uptime"]   = (uint32_t)(millis() / 1000);

    JsonObject eth = doc["eth"].to<JsonObject>();
    eth["up"]    = gNet.ethUp;
    eth["ip"]    = ipOrEmpty(gNet.ethIp);
    eth["speed"] = gNet.ethLinkSpeed;

    JsonObject wifi = doc["wifi"].to<JsonObject>();
    wifi["up"]         = gNet.wifiUp;
    wifi["ssid"]       = gNet.wifiUp ? gNet.wifiSsid : gConfig.wrSsid;
    wifi["rssi"]       = gNet.wifiRssi;
    wifi["reconnects"] = gNet.wifiReconnects;

    doc["target"] = netTargetValid() ? netResolveTarget().toString() : String("");

    JsonObject r = doc["relay"].to<JsonObject>();
    r["listening"] = gRelay.listening;
    r["port"]      = gRelay.listenPort;
    r["active"]    = gRelay.activeSessions;
    r["max"]       = SB_MAX_SESSIONS;
    r["total"]     = gRelay.sessionsTotal;
    r["rejected"]  = gRelay.sessionsRejected;
    r["toInv"]     = gRelay.bytesToInverter;
    r["toCli"]     = gRelay.bytesToClient;
    r["lastAgo"]   = gRelay.lastActivityMs
                       ? (int32_t)((millis() - gRelay.lastActivityMs) / 1000)
                       : -1;
    r["err"]       = gRelay.lastError;

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

static void handleGetConfig(AsyncWebServerRequest *req) {
    JsonDocument doc;
    doc["ssid"]       = gConfig.wrSsid;
    doc["targetAuto"] = gConfig.targetAuto;
    doc["targetIp"]   = ipOrEmpty(gConfig.targetIp);
    doc["targetPort"] = gConfig.targetPort;
    doc["dhcp"]       = gConfig.ethDhcp;
    doc["ip"]         = ipOrEmpty(gConfig.ethIp);
    doc["subnet"]     = ipOrEmpty(gConfig.ethSubnet);
    doc["gw"]         = ipOrEmpty(gConfig.ethGateway);
    doc["dns"]        = ipOrEmpty(gConfig.ethDns);
    doc["listenPort"] = gConfig.listenPort;
    doc["hostname"]   = gConfig.hostname;

    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

static void handlePostConfig(AsyncWebServerRequest *req, JsonVariant &json) {
    JsonObject b = json.as<JsonObject>();
    String err;
    DeviceConfig next = gConfig;

    if (b["ssid"].is<const char *>())  next.wrSsid     = b["ssid"].as<String>();
    if (b["pass"].is<const char *>())  next.wrPassword  = b["pass"].as<String>();
    if (b["hostname"].is<const char*>()) next.hostname  = b["hostname"].as<String>();

    next.targetAuto = b["targetAuto"] | next.targetAuto;
    if (!parseIp(b["targetIp"] | "", next.targetIp)) err = "Ziel-IP ungültig";

    uint32_t tp = b["targetPort"] | (uint32_t)next.targetPort;
    if (tp == 0 || tp > 65535) err = "Ziel-Port ungültig"; else next.targetPort = tp;

    next.ethDhcp = b["dhcp"] | next.ethDhcp;
    if (!parseIp(b["ip"]     | "", next.ethIp))     err = "IP-Adresse ungültig";
    if (!parseIp(b["subnet"] | "", next.ethSubnet)) err = "Subnetzmaske ungültig";
    if (!parseIp(b["gw"]     | "", next.ethGateway)) err = "Gateway ungültig";
    if (!parseIp(b["dns"]    | "", next.ethDns))    err = "DNS ungültig";

    uint32_t lp = b["listenPort"] | (uint32_t)next.listenPort;
    if (lp == 0 || lp > 65535) err = "Server-Port ungültig"; else next.listenPort = lp;

    if (!next.ethDhcp && (uint32_t)next.ethIp == 0)
        err = "Bei fester Adresse wird eine IP benötigt";
    if (!next.targetAuto && (uint32_t)next.targetIp == 0)
        err = "Bei manuellem Ziel wird eine IP benötigt";

    JsonDocument res;
    if (err.length()) {
        res["ok"] = false; res["err"] = err;
        String out; serializeJson(res, out);
        req->send(400, "application/json", out);
        return;
    }

    gConfig = next;
    bool saved = configSave();
    res["ok"] = saved;
    if (!saved) res["err"] = "Speichern fehlgeschlagen";

    String out; serializeJson(res, out);
    req->send(saved ? 200 : 500, "application/json", out);
    if (saved) scheduleReboot(1200);
}

static void handleScanStart(AsyncWebServerRequest *req) {
    netScanStart();
    req->send(200, "application/json", "{\"running\":true}");
}

static void handleScanResult(AsyncWebServerRequest *req) {
    JsonDocument doc;
    int state = netScanState();
    doc["running"] = (state == -1);
    JsonArray nets = doc["nets"].to<JsonArray>();
    if (state > 0) {
        for (int i = 0; i < state; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) continue;
            JsonObject n = nets.add<JsonObject>();
            n["ssid"] = ssid;
            n["rssi"] = WiFi.RSSI(i);
            n["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        }
    }
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
}

static void handleUpdateEnd(AsyncWebServerRequest *req) {
    bool ok = !Update.hasError();
    req->send(ok ? 200 : 500, "text/plain", ok ? "OK" : Update.errorString());
    if (ok) scheduleReboot(1500);
}

static void handleUpdateUpload(AsyncWebServerRequest *req, const String &filename,
                               size_t index, uint8_t *data, size_t len, bool final) {
    if (index == 0) {
        logPrintf("[OTA ] Start: %s", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    }
    if (Update.write(data, len) != len) Update.printError(Serial);
    if (final) {
        if (Update.end(true))
            logPrintf("[OTA ] Fertig, %u Bytes", (unsigned)(index + len));
        else
            Update.printError(Serial);
    }
}

// --------------------------------------------------------------------------
// Öffentliche API
// --------------------------------------------------------------------------

void webBegin() {
    // Logger mit WebSocket-Broadcast verbinden
    logSetBroadcast(wsBroadcast);

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send_P(200, "text/html", PAGE_INDEX);
    });

    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/scan",   HTTP_POST, handleScanStart);
    server.on("/api/scan",   HTTP_GET,  handleScanResult);

    server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *req) {
        req->send(200, "application/json", "{\"ok\":true}");
        scheduleReboot(800);
    });

    server.on("/api/factory-reset", HTTP_POST, [](AsyncWebServerRequest *req) {
        configFactoryReset();
        req->send(200, "application/json", "{\"ok\":true}");
        scheduleReboot(1000);
    });

    auto *cfgHandler = new AsyncCallbackJsonWebHandler("/api/config", handlePostConfig);
    cfgHandler->setMethod(HTTP_POST);
    server.addHandler(cfgHandler);

    server.on("/update", HTTP_POST, handleUpdateEnd, handleUpdateUpload);

    server.onNotFound([](AsyncWebServerRequest *req) {
        req->send(404, "text/plain", "Nicht gefunden");
    });

    server.begin();
    logPrint("[WEB ] Oberfläche auf Port 80");

    // Erster Update-Check im Hintergrund (kurz nach dem Start)
    s_lastFwCheck = millis() - (6UL * 3600000UL - 15000UL);
}

void webLoop() {
    ws.cleanupClients();

    // Update-Check alle 6 Stunden, aber erst wenn Ethernet steht
    if (gNet.ethUp && (millis() - s_lastFwCheck > 6UL * 3600000UL)) {
        checkFwUpdate();
    }

    if (s_rebootAt && (int32_t)(millis() - s_rebootAt) >= 0) {
        logPrint("[SYS ] Neustart");
        delay(80);
        ESP.restart();
    }
}
