/**
 * WT32-ETH01 - TCP Relay Server
 * 
 * Board: Waveshare WT32-ETH01 (ESP32-WROVER + LAN8720 Ethernet PHY)
 * Framework: Arduino (espressif32)
 * 
 * Funktion:
 *   TCP-Server auf Port 6607 lauscht auf eingehende Verbindungen.
 *   Jede Verbindung wird bidirektional zum WR-AP (10.0.0.254:6607) durchgereicht.
 *   Timeout: 30 Sekunden Inaktivität schließt die Verbindung automatisch.
 */

#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

// --- Konfiguration ---
#define WIFI_SSID         "huawei SSID hier eintragen"
#define WIFI_PASSWORD     ""

#define ETH_POWER_PIN     16
#define ETH_CLK_MODE      ETH_CLOCK_GPIO0_IN

#define TCP_SERVER_PORT   6607
#define WR_AP_IP          {10, 0, 0, 254}
#define WR_AP_PORT        6607

#define RELAY_BUF_SIZE    512
#define IDLE_TIMEOUT_MS   30000
#define SELECT_TIMEOUT_S  1
#define MAX_CONNECT_ATTEMPTS 10


// --- Hilfsfunktionen ---

static void log_msg(const char *msg) {
  Serial.println(msg);
}

static void log_addr(const struct sockaddr_in *addr) {
  Serial.printf("  -> %d.%d.%d.%d:%d\n",
    addr->sin_addr.s_addr & 0xFF,
    (addr->sin_addr.s_addr >> 8) & 0xFF,
    (addr->sin_addr.s_addr >> 16) & 0xFF,
    (addr->sin_addr.s_addr >> 24) & 0xFF,
    ntohs(addr->sin_port));
}

// WR-AP TCP-Client Socket verbinden
static int connect_wr_ap() {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(WR_AP_PORT);
  addr.sin_addr.s_addr = htonl(
    (WR_AP_IP[0] << 24) | (WR_AP_IP[1] << 16) |
    (WR_AP_IP[2] << 8)  | WR_AP_IP[3]);

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0) {
    log_msg("[RELAY] Fehler: WR-AP Socket Erstellung fehlgeschlagen");
    return -1;
  }

  // Non-blocking setzen für sauberes Timeout-Handling
  int flags = fcntl(sock, F_GETFL, 0);
  fcntl(sock, F_SETFL, flags | O_NONBLOCK);

  log_msg("[RELAY] Verbinde zu WR-AP...");
  log_addr(&addr);

  // Verbindungsversuche mit retry
  for (int i = 0; i < MAX_CONNECT_ATTEMPTS; i++) {
    int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0) {
      log_msg("[RELAY] WR-AP verbunden!");
      return sock;
    }

    // EAGAIN/EINPROGRESS = noch im Gange, weiter warten
    int err = errno;
    if (err == EINPROGRESS || err == EAGAIN) {
      delay(500);
      continue;
    }

    // Blockierender Retry als Fallback
    close(sock);
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) break;

    ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0) {
      flags = fcntl(sock, F_GETFL, 0);
      fcntl(sock, F_SETFL, flags | O_NONBLOCK);
      log_msg("[RELAY] WR-AP verbunden!");
      return sock;
    }

    close(sock);
    delay(1000);
  }

  log_msg("[RELAY] FEHLER: Kann WR-AP nicht erreichen");
  return -1;
}

// Bidirektionales TCP-Relay zwischen Client und WR-AP
static void relay_loop(int client_sock, int wrap_sock) {
  log_msg("[RELAY] Relay aktiv - Datenfluss: CLIENT <-> WR-AP");

  fd_set readfds;
  struct timeval tv;
  uint32_t last_activity = millis();

  while (true) {
    FD_ZERO(&readfds);
    FD_SET(client_sock, &readfds);
    FD_SET(wrap_sock, &readfds);

    tv.tv_sec = SELECT_TIMEOUT_S;
    tv.tv_usec = 0;

    int maxfd = (client_sock > wrap_sock) ? client_sock : wrap_sock;
    int sel = select(maxfd + 1, &readfds, NULL, NULL, &tv);

    // Timeout oder Fehler -> Verbindung schließen
    if (sel <= 0) {
      uint32_t idle = millis() - last_activity;
      if (idle >= IDLE_TIMEOUT_MS) {
        log_msg("[RELAY] Timeout: Keine Aktivität, schließe Verbindung");
      }
      break;
    }

    // Client -> WR-AP
    if (FD_ISSET(client_sock, &readfds)) {
      uint8_t buf[RELAY_BUF_SIZE];
      int n = recv(client_sock, buf, sizeof(buf), 0);
      if (n > 0) {
        last_activity = millis();
        log_msg("[RELAY] Client->WR-AP: ");
        Serial.print(n);
        Serial.println(" Bytes");

        int sent = 0;
        while (sent < n) {
          int w = send(wrap_sock, buf + sent, n - sent, 0);
          if (w > 0) sent += w;
          else break;
        }
      } else {
        log_msg("[RELAY] Client getrennt");
        break;
      }
    }

    // WR-AP -> Client
    if (FD_ISSET(wrap_sock, &readfds)) {
      uint8_t buf[RELAY_BUF_SIZE];
      int n = recv(wrap_sock, buf, sizeof(buf), 0);
      if (n > 0) {
        last_activity = millis();
        log_msg("[RELAY] WR-AP->Client: ");
        Serial.print(n);
        Serial.println(" Bytes");

        int sent = 0;
        while (sent < n) {
          int w = send(client_sock, buf + sent, n - sent, 0);
          if (w > 0) sent += w;
          else break;
        }
      } else {
        log_msg("[RELAY] WR-AP getrennt");
        break;
      }
    }
  }

  close(client_sock);
  close(wrap_sock);
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  log_msg("========================================");
  log_msg("  WT32-ETH01 TCP Relay Server Start   ");
  log_msg("========================================");

  // --- WiFi STA (optional) ---
  Serial.println();
  Serial.print("Verbinde mit WiFi SSID: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 30) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("WiFi verbunden - IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi Verbindung fehlgeschlagen (wird ignoriert)");
  }

  // --- Ethernet Initialisierung ---
  log_msg("Initialisiere Ethernet (LAN8720)...");
  ETH.begin(1, ETH_POWER_PIN, 23, 18, ETH_PHY_LAN8720, ETH_CLK_MODE);
  delay(1000);

  Serial.print("Ethernet IP: ");
  Serial.println(ETH.localIP());

  // --- TCP-Server auf Port 6607 erstellen ---
  log_msg("[SERVER] Erstelle Socket...");
  int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock < 0) {
    log_msg("[SERVER] FEHLER: Socket Erstellung fehlgeschlagen!");
    while (true) delay(1000);
  }

  // SO_REUSEADDR für schnelles Neustart nach Reset
  int opt = 1;
  setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(TCP_SERVER_PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  log_msg("[SERVER] Binde Socket an Port ");
  Serial.println(TCP_SERVER_PORT);

  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    log_msg("[SERVER] FEHLER: Bind fehlgeschlagen!");
    while (true) delay(1000);
  }

  if (listen(server_sock, 1) < 0) {
    log_msg("[SERVER] FEHLER: Listen fehlgeschlagen!");
    while (true) delay(1000);
  }

  log_msg("========================================");
  log_msg(" TCP Relay Server bereit!");
  log_msg(" Lausche auf Port ");
  Serial.println(TCP_SERVER_PORT);
  log_msg(" Relay zu WR-AP: ");
  Serial.print(WR_AP_IP[0]); Serial.print(".");
  Serial.print(WR_AP_IP[1]); Serial.print(".");
  Serial.print(WR_AP_IP[2]); Serial.print(".");
  Serial.println(WR_AP_IP[3]);
  log_msg("========================================");


  // --- Hauptschleife ---
  while (true) {
    log_msg("[SERVER] Warte auf Client-Verbindung...");

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

    if (client_sock < 0) {
      log_msg("[SERVER] FEHLER: Accept fehlgeschlagen");
      delay(1000);
      continue;
    }

    log_msg("[SERVER] Client verbunden:");
    log_addr(&client_addr);

    // WR-AP Verbindung aufbauen
    int wrap_sock = connect_wr_ap();
    if (wrap_sock < 0) {
      log_msg("[SERVER] FEHLER: WR-AP Verbindung fehlgeschlagen, schließe Client");
      close(client_sock);
      continue;
    }

    // Relay starten (blockierend bis Ende der Verbindung)
    relay_loop(client_sock, wrap_sock);
  }
}


void loop() {
  // Alles läuft in setup() - Hauptschleife ist leer
  delay(1000);
}
