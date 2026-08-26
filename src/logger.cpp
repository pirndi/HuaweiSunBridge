#include "logger.h"
#include <ArduinoJson.h>

#define LOG_BUF_SIZE 50

static String s_buf[LOG_BUF_SIZE];
static uint8_t s_head = 0;
static uint8_t s_count = 0;

// Funktionspointer - wird von web.cpp gesetzt um Zirkularabhaengigkeit zu vermeiden
static void (*s_wsBroadcast)(const String &) = nullptr;

void logInit() {
    // wird von web.cpp aufgerufen nach WS-Init
}

void logSetBroadcast(void (*fn)(const String &)) {
    s_wsBroadcast = fn;
}

static void pushLine(const String &line) {
    s_buf[s_head] = line;
    s_head = (s_head + 1) % LOG_BUF_SIZE;
    if (s_count < LOG_BUF_SIZE) s_count++;

    Serial.println(line);

    if (s_wsBroadcast) {
        JsonDocument doc;
        doc["t"] = "log";
        doc["m"] = line;
        String out;
        serializeJson(doc, out);
        s_wsBroadcast(out);
    }
}

void logPrint(const char *msg) {
    pushLine(String(msg));
}

void logPrint(const __FlashStringHelper *msg) {
    pushLine(String(msg));
}

void logPrintf(const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    pushLine(String(buf));
}

String logGetBuffer() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    // Reihenfolge: aeltester zuerst
    for (uint8_t i = 0; i < s_count; i++) {
        uint8_t idx = (s_head - s_count + i + LOG_BUF_SIZE) % LOG_BUF_SIZE;
        arr.add(s_buf[idx]);
    }
    String out;
    serializeJson(doc, out);
    return out;
}
