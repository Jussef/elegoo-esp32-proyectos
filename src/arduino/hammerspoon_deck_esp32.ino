/*
 * ═══════════════════════════════════════════════
 *  HAMMERSPOON DECK — ESP32 + OLED 0.96"
 *  Motor de menú (copiado/adaptado de retro_terminal_esp32.ino)
 *  reconectado a un servidor Hammerspoon por HTTP en vez de MQTT.
 *  Al seleccionar un item hace POST a http://HOST:PORT/cmd con
 *  {"type":"action","payload":"<id>"} y el header X-Auth-Token.
 * ═══════════════════════════════════════════════
 *  Librerías: Adafruit SSD1306, Adafruit GFX
 *  Cableado: BTN_MENU=4  JOY_X=34  JOY_Y=35  JOY_SW=32  OLED SDA=21 SCL=22
 * ═══════════════════════════════════════════════
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"   // SECRET_WIFI_SSID, SECRET_WIFI_PASS, SECRET_HAMMERSPOON_TOKEN

// ---------- CONFIG ----------
const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASS = SECRET_WIFI_PASS;

const char* HAMMERSPOON_HOST  = "192.168.0.107";
const int   HAMMERSPOON_PORT  = 8484;
const char* HAMMERSPOON_TOKEN = SECRET_HAMMERSPOON_TOKEN;

// ---------- PINES ----------
#define PIN_BTN_MENU  4
#define PIN_JOY_Y     35
#define PIN_JOY_SW    32

// ---------- OLED ----------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- MENÚ ----------
struct Menu {
  const char** items;
  const char** ids;
  int len;
  int index;
  int scroll;
};

const char* deckLabels[] = { "VOLUME UP", "VOLUME DOWN", "MUTE", "LOCK", "SLEEP" };
const char* deckIds[]    = { "volume_up", "volume_down", "mute_toggle", "lock", "sleep" };
Menu deckMenu = { deckLabels, deckIds, sizeof(deckLabels) / sizeof(deckLabels[0]), 0, 0 };
const int MENU_VISIBLE = 4;

// ---------- INPUT ----------
unsigned long lastBtn = 0, lastJoy = 0;
const unsigned long BTN_DEBOUNCE = 250;
const unsigned long JOY_REPEAT  = 180;

// ---------- RESULTADO DEL ÚLTIMO COMANDO ----------
char resultMsg[32] = "";
bool resultOk = false;
unsigned long resultAt = 0;

// ═══════════════ SETUP ═══════════════
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN_MENU, INPUT_PULLUP);
  pinMode(PIN_JOY_SW,  INPUT_PULLUP);

  Wire.begin(21, 22);
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED no encontrado :(");
    while (true) delay(100);
  }

  connectWiFi();
  drawMenu();
}

// ═══════════════ LOOP ═══════════════
void loop() {
  handleInput();
  drawMenu();
  delay(30);
}

// ═══════════════ INPUT ═══════════════
void handleInput() {
  int y = analogRead(PIN_JOY_Y);
  if (millis() - lastJoy > JOY_REPEAT) {
    int dir = 0;
    if (y < 1000)      dir = -1;
    else if (y > 3000) dir = +1;
    if (dir != 0) {
      moveMenu(dir);
      lastJoy = millis();
    }
  }

  bool sw = (digitalRead(PIN_JOY_SW) == LOW);
  if (sw && millis() - lastBtn > BTN_DEBOUNCE) {
    lastBtn = millis();
    fireAction(deckMenu.index);
  }
}

void moveMenu(int dir) {
  deckMenu.index = constrain(deckMenu.index + dir, 0, deckMenu.len - 1);
  if (deckMenu.index <  deckMenu.scroll)                 deckMenu.scroll = deckMenu.index;
  if (deckMenu.index >= deckMenu.scroll + MENU_VISIBLE)  deckMenu.scroll = deckMenu.index - MENU_VISIBLE + 1;
}

// ═══════════════ HAMMERSPOON (HTTP) ═══════════════
void fireAction(int idx) {
  if (idx < 0 || idx >= deckMenu.len) return;

  strncpy(resultMsg, "...", sizeof(resultMsg) - 1);
  resultOk = true;
  resultAt = millis();
  drawMenu();

  if (WiFi.status() != WL_CONNECTED) {
    strncpy(resultMsg, "SIN WIFI", sizeof(resultMsg) - 1);
    resultOk = false;
    resultAt = millis();
    return;
  }

  HTTPClient http;
  char url[96];
  snprintf(url, sizeof(url), "http://%s:%d/cmd", HAMMERSPOON_HOST, HAMMERSPOON_PORT);
  http.begin(url);
  http.setTimeout(3000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Auth-Token", HAMMERSPOON_TOKEN);

  char body[64];
  snprintf(body, sizeof(body), "{\"type\":\"action\",\"payload\":\"%s\"}", deckMenu.ids[idx]);

  int code = http.POST(body);
  resultOk = (code == 200);
  strncpy(resultMsg, resultOk ? "OK" : ("ERR " + String(code)).c_str(), sizeof(resultMsg) - 1);
  resultMsg[sizeof(resultMsg) - 1] = '\0';
  Serial.printf("[HAMMERSPOON] %s -> %d\n", deckMenu.ids[idx], code);

  http.end();
  resultAt = millis();
}

// ═══════════════ PANTALLA ═══════════════
void drawMenu() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("[ HAMMERSPOON ]"));

  display.setCursor(92, 3);
  display.print("[");
  display.print(deckMenu.index + 1);
  display.print("/");
  display.print(deckMenu.len);
  display.print("]");

  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < MENU_VISIBLE; i++) {
    int idx = deckMenu.scroll + i;
    if (idx >= deckMenu.len) break;
    int yPos = 20 + i * 11;

    if (idx == deckMenu.index) {
      display.fillRect(0, yPos - 1, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(4, yPos);
      display.print(F(">"));
      display.setCursor(15, yPos);
      display.print(deckMenu.items[idx]);
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.setCursor(15, yPos);
      display.print(deckMenu.items[idx]);
    }
  }

  if (deckMenu.scroll > 0)                          display.fillTriangle(122, 20, 126, 20, 124, 17, SSD1306_WHITE);
  if (deckMenu.scroll + MENU_VISIBLE < deckMenu.len) display.fillTriangle(122, 60, 126, 60, 124, 63, SSD1306_WHITE);

  if (resultMsg[0] && millis() - resultAt < 2000) {
    display.fillRect(0, 53, 128, 11, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(3, 55);
    display.print(resultOk ? F(">> ") : F("!! "));
    display.print(resultMsg);
    display.setTextColor(SSD1306_WHITE);
  }

  display.display();
}

// ═══════════════ WIFI ═══════════════
void connectWiFi() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.print(F("WIFI: CONNECTING"));
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int dots = 0;
  while (WiFi.status() != WL_CONNECTED && dots < 40) {
    delay(250);
    display.print(".");
    display.display();
    dots++;
  }
}
