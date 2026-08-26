#pragma once
#include <Arduino.h>

// Initialisierung erfolgt in web.cpp (nach WebSocket-Setup)
void logInit();

// Ersatz fuer Serial.println() - schreibt auf Serial UND WebSocket-Clients
void logPrint(const char *msg);
void logPrint(const __FlashStringHelper *msg);
void logPrintf(const char *fmt, ...);

// Letzten n Eintraege als JSON-Array (fuer initiales Laden neuer WS-Clients)
String logGetBuffer();

// Wird von web.cpp gesetzt, damit logger.h nicht von ESPAsyncWebServer abhaengt
void logSetBroadcast(void (*fn)(const String &));
