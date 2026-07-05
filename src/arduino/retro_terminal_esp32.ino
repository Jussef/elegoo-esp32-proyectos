/*
 * ═══════════════════════════════════════════════
 *  RETRO TERMINAL v0.3 — ESP32 + OLED 0.96"
 *  NUEVO en v0.3:
 *   📡 RFID SCANNER funcional (RC522)
 *      - animación de escaneo tipo radar
 *      - UID en hex + tipo de tarjeta
 *      - "ACCESS GRANTED" con flash
 * ═══════════════════════════════════════════════
 *  Librerías: Adafruit SSD1306, Adafruit GFX,
 *             MFRC522 (miguelbalboa)
 *  Cableado RC522: SS=5 SCK=18 MOSI=23 MISO=19 RST=17 (3.3V!)
 * ═══════════════════════════════════════════════
 */

#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- CONFIG ----------
const char* WIFI_SSID = "TU_SSID";
const char* WIFI_PASS = "TU_PASSWORD";

const long GMT_OFFSET_SEC = -6 * 3600;   // CDMX
const int  DST_OFFSET_SEC = 0;

// ---------- PINES ----------
#define PIN_BTN_MENU  4
#define PIN_JOY_X     34
#define PIN_JOY_Y     35
#define PIN_JOY_SW    32
#define PIN_RFID_SS   5
#define PIN_RFID_RST  17

// ---------- OLED ----------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- RFID ----------
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);

// ---------- ICONOS ----------
static const uint8_t PROGMEM ICON_SKULL[] = {
  0b00111100,
  0b01111110,
  0b11011011,
  0b11111111,
  0b11100111,
  0b01111110,
  0b00101010,
  0b00000000
};

// ---------- ESTADOS ----------
enum Screen { SCREEN_HOME, SCREEN_MENU, SCREEN_RFID };
Screen currentScreen = SCREEN_HOME;

// ---------- MENÚ ----------
const char* menuItems[] = {
  "RFID SCANNER",
  "IR REMOTE",
  "IR LEARN",
  "SYSTEM INFO",
  "SETTINGS",
  "< BACK"
};
const int MENU_LEN = sizeof(menuItems) / sizeof(menuItems[0]);
const int MENU_VISIBLE = 4;
int menuIndex = 0;
int menuScroll = 0;

// ---------- RFID SCANNER (estado) ----------
bool   cardPresent = false;
char   lastUID[24] = "";
char   lastType[20] = "";
unsigned long cardShownAt = 0;
int    scanAnimFrame = 0;
unsigned long lastScanAnim = 0;

// ---------- INPUT ----------
unsigned long lastBtn = 0, lastJoy = 0;
const unsigned long BTN_DEBOUNCE = 250;
const unsigned long JOY_REPEAT  = 180;

bool cursorOn = true;
unsigned long lastBlink = 0;

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

  SPI.begin();          // SCK=18, MISO=19, MOSI=23
  rfid.PCD_Init();
  Serial.print("RC522 version: 0x");
  Serial.println(rfid.PCD_ReadRegister(MFRC522::VersionReg), HEX);

  randomSeed(esp_random());
  bootAnimation();
  connectWiFi();
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "mx.pool.ntp.org", "pool.ntp.org");
}

// ═══════════════ LOOP ═══════════════
void loop() {
  handleInput();

  if (millis() - lastBlink > 500) {
    cursorOn = !cursorOn;
    lastBlink = millis();
  }

  switch (currentScreen) {
    case SCREEN_HOME: drawHome(); break;
    case SCREEN_MENU: drawMenu(); break;
    case SCREEN_RFID: rfidLoop(); break;
  }

  delay(30);
}

// ═══════════════ INPUT ═══════════════
void handleInput() {
  // Botón físico: desde cualquier pantalla regresa/alterna
  if (digitalRead(PIN_BTN_MENU) == LOW && millis() - lastBtn > BTN_DEBOUNCE) {
    lastBtn = millis();
    if (currentScreen == SCREEN_HOME) {
      currentScreen = SCREEN_MENU;
      menuIndex = 0;
      menuScroll = 0;
    } else if (currentScreen == SCREEN_RFID) {
      currentScreen = SCREEN_MENU;      // del scanner regresa al menú
      cardPresent = false;
    } else {
      currentScreen = SCREEN_HOME;
    }
  }

  if (currentScreen != SCREEN_MENU) return;

  int y = analogRead(PIN_JOY_Y);
  if (millis() - lastJoy > JOY_REPEAT) {
    if (y < 1000) { moveMenu(-1); lastJoy = millis(); }
    if (y > 3000) { moveMenu(+1); lastJoy = millis(); }
  }

  // Click del joystick: ahora SÍ ejecuta
  if (digitalRead(PIN_JOY_SW) == LOW && millis() - lastBtn > BTN_DEBOUNCE) {
    lastBtn = millis();
    selectMenuItem();
  }
}

void selectMenuItem() {
  switch (menuIndex) {
    case 0:   // RFID SCANNER
      currentScreen = SCREEN_RFID;
      cardPresent = false;
      lastUID[0] = '\0';
      break;
    case MENU_LEN - 1:   // < BACK
      currentScreen = SCREEN_HOME;
      break;
    default:             // el resto sigue en fase 1
      flashSelection();
      break;
  }
}

void moveMenu(int dir) {
  menuIndex = constrain(menuIndex + dir, 0, MENU_LEN - 1);
  if (menuIndex <  menuScroll)                menuScroll = menuIndex;
  if (menuIndex >= menuScroll + MENU_VISIBLE) menuScroll = menuIndex - MENU_VISIBLE + 1;
}

// ═══════════════ RFID SCANNER ═══════════════
void rfidLoop() {
  // ¿tarjeta nueva?
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // UID -> string hex "AA:BB:CC:DD"
    char* p = lastUID;
    for (byte i = 0; i < rfid.uid.size; i++) {
      p += sprintf(p, "%02X", rfid.uid.uidByte[i]);
      if (i < rfid.uid.size - 1) *p++ = ':';
    }
    *p = '\0';

    // tipo de tarjeta
    MFRC522::PICC_Type type = rfid.PICC_GetType(rfid.uid.sak);
    strncpy(lastType, (const char*)rfid.PICC_GetTypeName(type), sizeof(lastType) - 1);
    lastType[sizeof(lastType) - 1] = '\0';

    Serial.print("UID: ");
    Serial.println(lastUID);

    cardPresent = true;
    cardShownAt = millis();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    accessGrantedFlash();
  }

  // después de 6s sin tarjeta nueva, vuelve a modo escaneo
  if (cardPresent && millis() - cardShownAt > 6000) {
    cardPresent = false;
  }

  if (cardPresent) drawCardInfo();
  else             drawScanning();
}

// Pantalla "escaneando" con barrido tipo radar
void drawScanning() {
  display.clearDisplay();

  // header
  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("[ RFID SCANNER ]"));

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 22);
  display.print(F("SCANNING"));
  // puntitos animados
  for (int i = 0; i < (scanAnimFrame % 4); i++) display.print(".");

  // línea de barrido que sube y baja
  if (millis() - lastScanAnim > 60) {
    scanAnimFrame++;
    lastScanAnim = millis();
  }
  int sweep = 34 + (scanAnimFrame * 3) % 26;
  display.drawFastHLine(10, sweep, 108, SSD1306_WHITE);
  display.drawRect(8, 32, 112, 30, SSD1306_WHITE);

  display.setCursor(14, 42);
  display.print(F("ACERCA UNA TARJETA"));

  display.display();
}

// Pantalla con datos de la tarjeta leída
void drawCardInfo() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F(">> ACCESS GRANTED"));

  display.setTextColor(SSD1306_WHITE);
  display.drawBitmap(3, 20, ICON_SKULL, 8, 8, SSD1306_WHITE);
  display.setCursor(15, 20);
  display.print(F("TARGET ACQUIRED"));

  display.setCursor(3, 34);
  display.print(F("UID:"));
  display.setCursor(3, 44);
  display.print(lastUID);

  display.setCursor(3, 56);
  display.print(lastType);

  display.display();
}

void accessGrantedFlash() {
  for (int i = 0; i < 3; i++) {
    display.invertDisplay(true);  delay(50);
    display.invertDisplay(false); delay(50);
  }
}

// ═══════════════ PANTALLAS BASE ═══════════════
void drawHome() {
  display.clearDisplay();
  drawStatusBar();

  struct tm t;
  bool haveTime = getLocalTime(&t, 50);

  display.drawRect(0, 18, 128, 46, SSD1306_WHITE);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(6, 22);
  display.print(F(">JUSSEF.SYS ONLINE"));

  display.setCursor(6, 34);
  if (haveTime) {
    char fecha[24];
    strftime(fecha, sizeof(fecha), "%d/%m/%Y  %a", &t);
    display.print(F("DATE: "));
    display.print(fecha);
  } else {
    display.print(F("DATE: SYNCING..."));
  }

  display.setCursor(6, 46);
  display.print(F("IP: "));
  display.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "---");

  if (cursorOn) display.fillRect(6, 56, 6, 4, SSD1306_WHITE);

  display.display();
}

void drawMenu() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("== MAIN MENU =="));
  display.setCursor(92, 3);
  display.print("[");
  display.print(menuIndex + 1);
  display.print("/");
  display.print(MENU_LEN);
  display.print("]");

  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < MENU_VISIBLE; i++) {
    int idx = menuScroll + i;
    if (idx >= MENU_LEN) break;
    int yPos = 20 + i * 11;

    if (idx == menuIndex) {
      display.fillRect(0, yPos - 1, 128, 11, SSD1306_WHITE);
      display.drawBitmap(3, yPos, ICON_SKULL, 8, 8, SSD1306_BLACK);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(15, yPos);
      display.print(menuItems[idx]);
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.setCursor(15, yPos);
      display.print(menuItems[idx]);
    }
  }

  if (menuScroll > 0)                       display.fillTriangle(122, 20, 126, 20, 124, 17, SSD1306_WHITE);
  if (menuScroll + MENU_VISIBLE < MENU_LEN) display.fillTriangle(122, 60, 126, 60, 124, 63, SSD1306_WHITE);

  display.display();
}

void drawStatusBar() {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  struct tm t;
  display.setCursor(0, 4);
  if (getLocalTime(&t, 50)) {
    char hora[13];
    strftime(hora, sizeof(hora), "%I:%M:%S%p", &t);
    display.print(hora);
  } else {
    display.print(F("--:--:--"));
  }

  drawBiohazard(74, 7);
  drawWifiBars(104, 12);

  display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
}

void drawBiohazard(int cx, int cy) {
  display.drawCircle(cx,     cy - 3, 3, SSD1306_WHITE);
  display.drawCircle(cx - 3, cy + 2, 3, SSD1306_WHITE);
  display.drawCircle(cx + 3, cy + 2, 3, SSD1306_WHITE);
  display.fillCircle(cx, cy, 1, SSD1306_WHITE);
}

void drawWifiBars(int x, int yBase) {
  if (WiFi.status() != WL_CONNECTED) {
    display.setCursor(x, 4);
    display.print(F("X"));
    return;
  }
  long rssi = WiFi.RSSI();
  int bars = (rssi > -55) ? 4 : (rssi > -65) ? 3 : (rssi > -75) ? 2 : 1;
  for (int i = 0; i < 4; i++) {
    int h = 3 + i * 3;
    if (i < bars) display.fillRect(x + i * 5, yBase - h, 3, h, SSD1306_WHITE);
    else          display.drawRect(x + i * 5, yBase - h, 3, h, SSD1306_WHITE);
  }
}

// ═══════════════ BOOT ═══════════════
void bootAnimation() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  const char* title = "RETRO-TERM v0.3";
  display.setCursor(19, 4);
  for (int c = 0; title[c] != '\0'; c++) {
    display.print(title[c]);
    display.display();
    delay(25);
  }
  delay(300);

  matrixRain(3000);
}

void matrixRain(unsigned long durationMs) {
  const int COLS = 21;
  int heads[COLS];
  int speeds[COLS];
  int trails[COLS];

  for (int i = 0; i < COLS; i++) {
    heads[i]  = random(-48, 16);
    speeds[i] = random(3, 8);
    trails[i] = random(3, 7);
  }

  unsigned long start = millis();
  while (millis() - start < durationMs) {
    display.clearDisplay();

    display.setCursor(19, 4);
    display.print(F("RETRO-TERM v0.3"));
    display.drawFastHLine(0, 14, 128, SSD1306_WHITE);

    for (int i = 0; i < COLS; i++) {
      for (int t = 0; t < trails[i]; t++) {
        int y = heads[i] - t * 8;
        if (y >= 16 && y < 64) {
          display.setCursor(i * 6, y);
          char c = (random(0, 3) == 0) ? ('0' + random(0, 2))
                                       : (char)random(33, 126);
          display.write(c);
        }
      }
      heads[i] += speeds[i];
      if (heads[i] - trails[i] * 8 > 64) {
        heads[i]  = random(-24, 8);
        speeds[i] = random(3, 8);
        trails[i] = random(3, 7);
      }
    }

    display.display();
    delay(45);
  }
}

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

void flashSelection() {
  for (int i = 0; i < 2; i++) {
    display.invertDisplay(true);  delay(60);
    display.invertDisplay(false); delay(60);
  }
}
