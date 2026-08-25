/*
 * HuaweiSunBridge
 *
 * Transparente TCP-Bruecke zwischen dem WLAN-Access-Point eines Huawei
 * SUN2000 und dem Heimnetz. Das Geraet haengt per Ethernet im Heimnetz und
 * per WLAN am Wechselrichter; alles, was an seinem eigenen Port ankommt,
 * wird unveraendert zum Wechselrichter durchgereicht.
 *
 * Damit bekommt jeder Wechselrichter eine eigene Adresse im Heimnetz,
 * obwohl alle SUN2000-APs intern dieselbe IP benutzen.
 *
 * Board: WT32-ETH01 (ESP32 + LAN8720)
 */

#include <Arduino.h>
#include <ESPmDNS.h>

#include "../include/config.h"
#include "../include/net.h"
#include "../include/relay.h"
#include "../include/web.h"

#ifndef FW_VERSION
#define FW_VERSION "0.0.0"
#endif

void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println(F("=== HuaweiSunBridge " FW_VERSION " ==="));

    configLoad();
    Serial.printf("[CFG ] Geraet: %s\n", configEffectiveHostname().c_str());
    Serial.printf("[CFG ] WR-AP: %s\n",
                  gConfig.isProvisioned() ? gConfig.wrSsid.c_str() : "(nicht eingerichtet)");

    netBegin();

    if (MDNS.begin(configEffectiveHostname().c_str())) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[SYS ] Erreichbar als %s.local\n",
                      configEffectiveHostname().c_str());
    }

    relayBegin();
    webBegin();

    Serial.println(F("[SYS ] Bereit"));
}

void loop() {
    netLoop();
    relayLoop();
    webLoop();
}
