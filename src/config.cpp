#include "../include/config.h"
#include <Preferences.h>
#include <WiFi.h>

DeviceConfig gConfig;

static Preferences prefs;
static const char *NS = "sunbridge";

static IPAddress u32ToIp(uint32_t v) { return IPAddress(v); }
static uint32_t ipToU32(const IPAddress &ip) { return (uint32_t)ip; }

void configLoad() {
    prefs.begin(NS, true);  // read-only

    gConfig.wrSsid     = prefs.getString("ssid", "");
    gConfig.wrPassword = prefs.getString("pass", "");

    gConfig.targetAuto = prefs.getBool("tauto", true);
    gConfig.targetIp   = u32ToIp(prefs.getUInt("tip", 0));
    gConfig.targetPort = prefs.getUShort("tport", 6607);

    gConfig.ethDhcp    = prefs.getBool("dhcp", true);
    gConfig.ethIp      = u32ToIp(prefs.getUInt("eip", 0));
    gConfig.ethGateway = u32ToIp(prefs.getUInt("egw", 0));
    gConfig.ethSubnet  = u32ToIp(prefs.getUInt("esn", (uint32_t)IPAddress(255, 255, 255, 0)));
    gConfig.ethDns     = u32ToIp(prefs.getUInt("edns", 0));

    gConfig.listenPort = prefs.getUShort("lport", 6607);
    gConfig.hostname   = prefs.getString("host", "");

    prefs.end();
}

bool configSave() {
    if (!prefs.begin(NS, false)) return false;

    prefs.putString("ssid", gConfig.wrSsid);
    prefs.putString("pass", gConfig.wrPassword);

    prefs.putBool("tauto", gConfig.targetAuto);
    prefs.putUInt("tip", ipToU32(gConfig.targetIp));
    prefs.putUShort("tport", gConfig.targetPort);

    prefs.putBool("dhcp", gConfig.ethDhcp);
    prefs.putUInt("eip", ipToU32(gConfig.ethIp));
    prefs.putUInt("egw", ipToU32(gConfig.ethGateway));
    prefs.putUInt("esn", ipToU32(gConfig.ethSubnet));
    prefs.putUInt("edns", ipToU32(gConfig.ethDns));

    prefs.putUShort("lport", gConfig.listenPort);
    prefs.putString("host", gConfig.hostname);

    prefs.end();
    return true;
}

void configFactoryReset() {
    prefs.begin(NS, false);
    prefs.clear();
    prefs.end();
}

String configEffectiveHostname() {
    if (gConfig.hostname.length() > 0) return gConfig.hostname;

    // getEfuseMac() ist in Arduino-Core 2.x und 3.x identisch verfuegbar
    uint64_t mac = ESP.getEfuseMac();
    char buf[32];
    snprintf(buf, sizeof(buf), "sunbridge-%02x%02x%02x",
             (uint8_t)(mac >> 24), (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    return String(buf);
}
