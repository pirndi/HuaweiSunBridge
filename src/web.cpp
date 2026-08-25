#include "../include/web.h"
#include "../include/page.h"
#include "../include/config.h"
#include "../include/net.h"
#include "../include/relay.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Update.h>

#ifndef FW_VERSION
#define FW_VERSION "0.0.0"
#endif

static AsyncWebServer server(80);
static uint32_t s_rebootAt = 0;      // 0 = kein Neustart geplant

static void scheduleReboot(uint32_t delayMs) { s_rebootAt = millis() + delayMs; }

static String ipOrEmpty(const IPAddress &ip) {
    return ((uint32_t)ip == 0) ? String("") : ip.toString();
}

static bool parseIp(const char *s, IPAddress &out) {
    if (!s || !*s) { out = IPAddress((uint32_t)0); return true; }  // leer ist erlaubt
    return out.fromString(s);
}

// ---------------------------------------------------------------------------

static void handleStatus(AsyncWebServerRequest *req) {
    JsonDocument doc;

    doc["host"]   = configEffectiveHostname();
    doc["fw"]     = FW_VERSION;
    doc["uptime"] = (uint32_t)(millis() / 1000);

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
    doc["ssid"]       = gConfig.wrSsid;          // Passwort wird nie ausgeliefert
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

    DeviceConfig next = gConfig;   // erst pruefen, dann uebernehmen

    if (b["ssid"].is<const char *>()) next.wrSsid = b["ssid"].as<String>();
    if (b["pass"].is<const char *>()) next.wrPassword = b["pass"].as<String>();

    next.targetAuto = b["targetAuto"] | next.targetAuto;
    if (!parseIp(b["targetIp"] | "", next.targetIp)) err = F("Ziel-IP ungueltig");

    uint32_t tp = b["targetPort"] | (uint32_t)next.targetPort;
    if (tp == 0 || tp > 65535) err = F("Ziel-Port ungueltig"); else next.targetPort = tp;

    next.ethDhcp = b["dhcp"] | next.ethDhcp;
    if (!parseIp(b["ip"] | "", next.ethIp))          err = F("IP-Adresse ungueltig");
    if (!parseIp(b["subnet"] | "", next.ethSubnet))  err = F("Subnetzmaske ungueltig");
    if (!parseIp(b["gw"] | "", next.ethGateway))     err = F("Gateway ungueltig");
    if (!parseIp(b["dns"] | "", next.ethDns))        err = F("DNS ungueltig");

    uint32_t lp = b["listenPort"] | (uint32_t)next.listenPort;
    if (lp == 0 || lp > 65535) err = F("Server-Port ungueltig"); else next.listenPort = lp;

    if (b["hostname"].is<const char *>()) next.hostname = b["hostname"].as<String>();

    if (!next.ethDhcp && (uint32_t)next.ethIp == 0)
        err = F("Bei fester Adresse wird eine IP benoetigt");
    if (!next.targetAuto && (uint32_t)next.targetIp == 0)
        err = F("Bei manuellem Ziel wird eine IP benoetigt");

    JsonDocument res;
    if (err.length()) {
        res["ok"] = false;
        res["err"] = err;
        String out; serializeJson(res, out);
        req->send(400, "application/json", out);
        return;
    }

    gConfig = next;
    bool saved = configSave();
    res["ok"] = saved;
    if (!saved) res["err"] = F("Speichern fehlgeschlagen");

    String out; serializeJson(res, out);
    req->send(saved ? 200 : 500, "application/json", out);

    if (saved) scheduleReboot(1200);   // sauberer Neustart mit neuer Netzkonfig
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

// ---------------------------------------------------------------------------
// OTA - bewusst ohne zusaetzliche Bibliothek, damit hier nichts an einer
// fremden Versionsabhaengigkeit haengt.
// ---------------------------------------------------------------------------

static void handleUpdateEnd(AsyncWebServerRequest *req) {
    bool ok = !Update.hasError();
    req->send(ok ? 200 : 500, "text/plain",
              ok ? "OK" : Update.errorString());
    if (ok) scheduleReboot(1500);
}

static void handleUpdateUpload(AsyncWebServerRequest *req, const String &filename,
                               size_t index, uint8_t *data, size_t len, bool final) {
    if (index == 0) {
        Serial.printf("[OTA ] Start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len) {
        Update.printError(Serial);
    }

    if (final) {
        if (Update.end(true)) {
            Serial.printf("[OTA ] Fertig, %u Bytes\n", (unsigned)(index + len));
        } else {
            Update.printError(Serial);
        }
    }
}

// ---------------------------------------------------------------------------

void webBegin() {
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
    Serial.println(F("[WEB ] Oberflaeche auf Port 80"));
}

void webLoop() {
    if (s_rebootAt && (int32_t)(millis() - s_rebootAt) >= 0) {
        Serial.println(F("[SYS ] Neustart"));
        delay(50);
        ESP.restart();
    }
}
