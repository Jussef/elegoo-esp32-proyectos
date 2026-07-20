/*
 * ═══════════════════════════════════════════════
 *  RETRO TERMINAL v0.7 — ESP32 + OLED 0.96"  ·  DEV STREAM DECK
 *  NUEVO en v0.7:
 *   🎛️ MACROS — stream deck para dev/homelab:
 *      - el menú MACROS se autoconfigura por MQTT (topic
 *        macros/config retenido), sin reflashear.
 *      - al hacer click publica macro/fire; un "agente"
 *        Node en el homelab ejecuta la acción (salud del
 *        homelab, reiniciar contenedor Docker, ...) y
 *        devuelve el resultado en macro/result.
 *   (Se retiró el EMISOR IR. El RECEPTOR IR queda listo
 *    para la Fase 2: usar un control remoto como hotkeys.)
 *  v0.5:
 *   📡 MQTT — status (retained+LWT), telemetría (5s),
 *      escaneos RFID y base de tarjetas (retained);
 *      recibe cmd/# (ver MQTT/README.md para el contrato).
 * ═══════════════════════════════════════════════
 *  Librerías: Adafruit SSD1306, Adafruit GFX,
 *             MFRC522 (miguelbalboa),
 *             PubSubClient (knolleary), ArduinoJson (v7),
 *             IRremote (Armin Joachimsmeyer, v4.x)
 *  Cableado RC522: SS=5 SCK=18 MOSI=23 MISO=19 RST=17 (3.3V!)
 *  Cableado IR:  RECEPTOR Y->GPIO16  R->3.3V  G->GND  (sin emisor)
 * ═══════════════════════════════════════════════
 */

#include <WiFi.h>
#include <time.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <IRremote.hpp>          // v4.x — solo IrReceiver (emisor retirado; Fase 2)
#include "secrets.h"             // credenciales reales (NO se sube a GitHub; ver secrets.example.h)

// ---------- CONFIG ----------
const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASS = SECRET_WIFI_PASS;

const long GMT_OFFSET_SEC = -6 * 3600;   // CDMX
const int  DST_OFFSET_SEC = 0;

// ---------- MQTT ----------
const bool  MQTT_ENABLED = true;         // <-- pon en false si NO tienes broker (evita lag)
const char* MQTT_HOST = "192.168.0.169"; // IP del broker Mosquitto (homelab)
const int   MQTT_PORT = 1883;
const char* MQTT_USER = SECRET_MQTT_USER; // vacío = broker anónimo
const char* MQTT_PASS = SECRET_MQTT_PASS;
const char* DEVICE_ID = "term01";        // identifica esta terminal en los topics

// ---------- PINES ----------
#define PIN_BTN_MENU  4
#define PIN_JOY_X     34
#define PIN_JOY_Y     35
#define PIN_JOY_SW    32
#define PIN_RFID_SS   5
#define PIN_RFID_RST  17
#define PIN_IR_RECV   16         // pin Y del RECEPTOR IR (Fase 2: control como hotkeys)

// ---------- OLED ----------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ---------- RFID ----------
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);

// ---------- MQTT ----------
WiFiClient   netClient;
PubSubClient mqtt(netClient);
char topicBase[40];                 // "retroterm/term01"
unsigned long lastTelemetry = 0;
unsigned long lastMqttRetry = 0;
bool dbDirty = false;               // pide re-publicar la base de tarjetas

// ---------- CARD DATABASE (NVS) ----------
#define MAX_CARDS 20
struct Card {
  char uid[24];
  char name[12];
};
Card cards[MAX_CARDS];
int  cardCount = 0;
Preferences prefs;

// ---------- MACROS (stream deck) ----------
// La lista la envía el agente por MQTT (macros/config, retenido). No se guarda
// en NVS: al arrancar, el broker reentrega el retenido y se repuebla sola.
#define MAX_MACROS 16
struct Macro {
  char id[24];      // identificador que se publica en macro/fire
  char label[18];   // texto que se ve en el OLED
};
Macro macros[MAX_MACROS];
int   macroCount = 0;

// ---------- ICONOS RETROFUTURISTAS 8x8 ----------
static const uint8_t PROGMEM ICON_SKULL[] = {
  0b00111100, //     ####    
  0b01111110, //    ######   
  0b11011011, //  ## #  # ## 
  0b11111111, //   ########    
  0b11100111, //   ###  ###    
  0b01111110, //    ######    
  0b00101010, //   # # # #   
  0b00000000  //            
};

// Glitch / Error de Red / Cyberware dañado
static const uint8_t PROGMEM ICON_GLITCH[] = {
  0b11000011, // ##    ##
  0b01100110, //  ##  ## 
  0b00000000, //         
  0b11111111, // ########
  0b00000000, //         
  0b01100110, //  ##  ## 
  0b11000011, // ##    ##
  0b00000000  //         
};

// Disquete / diskette (Estilo 80s / Guardar partida)
static const uint8_t PROGMEM ICON_FLOPPY[] = {
  0b01111110, //  ###### 
  0b01100110, //  ##  ## 
  0b01111110, //  ###### 
  0b01111110, //  ###### 
  0b01000010, //  #    # 
  0b01011010, //  # ## # 
  0b01011010, //  # ## # 
  0b00000000  //         
};

// Ojo Cibernético / Ojo de corporación (Estilo Blade Runner)
static const uint8_t PROGMEM ICON_CYBER_EYE[] = {
  0b00111100, //    ####   
  0b01000010, //   #    #  
  0b10011001, //  #  ##  # 
  0b10111101, //  # ###### 
  0b10111101, //  # ###### 
  0b10011001, //  #  ##  # 
  0b01000010, //   #    #  
  0b00111100  //    ####   
};

// Monitor CRT / Terminal Hacker
static const uint8_t PROGMEM ICON_TERMINAL[] = {
  0b11111111, // ########
  0b10000001, // #      #
  0b10111001, // # ###  #
  0b10100001, // # #    #
  0b10000001, // #      #
  0b11111111, // ########
  0b00011000, //    ##   
  0b01111110  //  ###### 
};

// Pirámide de Corporación Neon / Triángulo Synthwave
static const uint8_t PROGMEM ICON_CORP_PYRAMID[] = {
  0b00011000, //    ##   
  0b00011000, //    ##   
  0b00111100, //   ####  
  0b00100100, //   #  #  
  0b01111110, //  ###### 
  0b01000010, //  #    # 
  0b11111111, // ########
  0b00000000  //         
};

// Rayo Láser / Energía Synthwave
static const uint8_t PROGMEM ICON_BOLT[] = {
  0b00011000, //    ##   
  0b00110000, //   ##    
  0b01100000, //  ##     
  0b01111100, //  #####  
  0b00011000, //    ##   
  0b00110000, //   ##    
  0b01100000, //  ##     
  0b01000000  //  #      
};

// Radiación / Contaminación Nuclear
static const uint8_t PROGMEM ICON_RADIATION[] = {
  0b01100110, //  ##  ## 
  0b11100111, // ###  ###
  0b11100111, // ###  ###
  0b00011000, //    ##   
  0b00011000, //    ##   
  0b00011000, //    ##   
  0b00111100, //   ####  
  0b01111110  //  ###### 
};


// ---------- ESTADOS ----------
enum Screen { SCREEN_HOME, SCREEN_MENU, SCREEN_RFID_MENU, SCREEN_RFID, SCREEN_REGISTER, SCREEN_CARDDB, SCREEN_MACROS };
Screen currentScreen = SCREEN_HOME;

// ---------- MENÚS ----------
// Modelo genérico: el mismo drawMenu/moveMenu sirve para cualquier menú.
struct Menu {
  const char** items;
  int len;
  int index;
  int scroll;
};

const char* mainItems[] = {
  "RFID SCANNER",     // -> submenú (scan + register + database)
  "MACROS",           // -> stream deck (lista recibida por MQTT)
  "SYSTEM INFO",
  "SETTINGS",
  "< BACK"
};
const char* rfidItems[] = {
  "SCAN CARD",
  "REGISTER CARD",
  "CARD DATABASE",
  "< BACK"
};
Menu mainMenu = { mainItems, sizeof(mainItems) / sizeof(mainItems[0]), 0, 0 };
Menu rfidMenu = { rfidItems, sizeof(rfidItems) / sizeof(rfidItems[0]), 0, 0 };
const int MENU_VISIBLE = 4;

// ---------- RFID SCANNER (estado) ----------
bool   cardPresent = false;
bool   lastGranted = false;
char   lastUID[24] = "";
char   lastType[20] = "";
char   lastName[12] = "";
unsigned long cardShownAt = 0;
int    scanAnimFrame = 0;
unsigned long lastScanAnim = 0;

// ---------- REGISTER / CARD DB (estado) ----------
int    regResult = 0;          // 0=esperando, 1=ok, 2=duplicada, 3=llena
unsigned long regShownAt = 0;
int    dbScroll = 0;
const int DB_VISIBLE = 4;
bool   confirmWipe = false;

// ---------- MACROS (estado) ----------
int    macroIndex = 0, macroScroll = 0;   // navegación de la lista
const int MACRO_VISIBLE = 4;
int    macroFiredIdx = -1;                 // índice disparado (para el flash)
unsigned long macroFiredAt = 0;
char   macroMsg[20] = "";                  // resultado que devuelve el agente
bool   macroOk = false;
unsigned long macroMsgAt = 0;

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

  loadCards();
  Serial.printf("Tarjetas registradas: %d\n", cardCount);

  // ---- IR receptor (listo para Fase 2: control remoto como hotkeys) ----
  IrReceiver.begin(PIN_IR_RECV, DISABLE_LED_FEEDBACK);

  randomSeed(esp_random());
  bootAnimation();
  connectWiFi();
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, "mx.pool.ntp.org", "pool.ntp.org");

  // ---- MQTT ----
  snprintf(topicBase, sizeof(topicBase), "retroterm/%s", DEVICE_ID);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(1600);        // la base de tarjetas (retained) puede ser grande
  mqtt.setSocketTimeout(2);        // no esperar 15s por el CONNACK -> evita el congelamiento
  netClient.setConnectionTimeout(1500);  // TCP connect máx 1.5s si el broker no responde
}

// ═══════════════ LOOP ═══════════════
void loop() {
  mqttLoop();
  handleInput();

  if (millis() - lastBlink > 500) {
    cursorOn = !cursorOn;
    lastBlink = millis();
  }

  switch (currentScreen) {
    case SCREEN_HOME:     drawHome();     break;
    case SCREEN_MENU:     drawMenu();     break;
    case SCREEN_RFID_MENU: drawMenu();    break;
    case SCREEN_RFID:     rfidLoop();     break;
    case SCREEN_REGISTER: registerLoop(); break;
    case SCREEN_CARDDB:   drawCardDB();   break;
    case SCREEN_MACROS:   drawMacros();   break;
  }

  delay(30);
}

// ═══════════════ NVS: BASE DE DATOS DE TARJETAS ═══════════════
void loadCards() {
  prefs.begin("rfiddb", true);            // solo lectura
  cardCount = prefs.getInt("count", 0);
  if (cardCount > MAX_CARDS) cardCount = MAX_CARDS;
  for (int i = 0; i < cardCount; i++) {
    char key[8];
    sprintf(key, "u%d", i);
    prefs.getString(key, "").toCharArray(cards[i].uid, sizeof(cards[i].uid));
    sprintf(key, "n%d", i);
    prefs.getString(key, "").toCharArray(cards[i].name, sizeof(cards[i].name));
  }
  prefs.end();
}

void saveCards() {
  prefs.begin("rfiddb", false);           // lectura/escritura
  prefs.putInt("count", cardCount);
  for (int i = 0; i < cardCount; i++) {
    char key[8];
    sprintf(key, "u%d", i);
    prefs.putString(key, cards[i].uid);
    sprintf(key, "n%d", i);
    prefs.putString(key, cards[i].name);
  }
  prefs.end();
}

int findCard(const char* uid) {
  for (int i = 0; i < cardCount; i++)
    if (strcmp(cards[i].uid, uid) == 0) return i;
  return -1;
}

// devuelve 1=ok, 2=ya existe, 3=llena. name=nullptr -> autonombra AGENT-NN
int addCardNamed(const char* uid, const char* name) {
  if (findCard(uid) >= 0) return 2;
  if (cardCount >= MAX_CARDS) return 3;
  strncpy(cards[cardCount].uid, uid, sizeof(cards[cardCount].uid) - 1);
  cards[cardCount].uid[sizeof(cards[cardCount].uid) - 1] = '\0';
  if (name && name[0]) {
    strncpy(cards[cardCount].name, name, sizeof(cards[cardCount].name) - 1);
    cards[cardCount].name[sizeof(cards[cardCount].name) - 1] = '\0';
  } else {
    snprintf(cards[cardCount].name, sizeof(cards[cardCount].name), "AGENT-%02d", cardCount + 1);
  }
  cardCount++;
  saveCards();
  dbDirty = true;
  return 1;
}

int addCard(const char* uid) {   // usado por el registro físico
  return addCardNamed(uid, nullptr);
}

bool deleteCardByUid(const char* uid) {
  int idx = findCard(uid);
  if (idx < 0) return false;
  for (int i = idx; i < cardCount - 1; i++) cards[i] = cards[i + 1];
  cardCount--;
  saveCards();
  dbDirty = true;
  return true;
}

bool renameCard(const char* uid, const char* name) {
  int idx = findCard(uid);
  if (idx < 0 || !name || !name[0]) return false;
  strncpy(cards[idx].name, name, sizeof(cards[idx].name) - 1);
  cards[idx].name[sizeof(cards[idx].name) - 1] = '\0';
  saveCards();
  dbDirty = true;
  return true;
}

void wipeCards() {
  prefs.begin("rfiddb", false);
  prefs.clear();
  prefs.end();
  cardCount = 0;
  dbDirty = true;
}

// ═══════════════ MACROS (stream deck) ═══════════════
// Dispara la macro idx: publica su id en macro/fire. El agente del homelab la
// ejecuta y responde por macro/result (lo pinta drawMacros).
void fireMacro(int idx) {
  if (idx < 0 || idx >= macroCount) return;
  if (!mqtt.connected()) {
    strncpy(macroMsg, "NO MQTT", sizeof(macroMsg) - 1);
    macroOk = false; macroMsgAt = millis();
    return;
  }
  char t[64]; mqttTopic(t, sizeof(t), "macro/fire");
  JsonDocument doc;
  doc["id"]  = macros[idx].id;
  doc["src"] = "menu";
  char buf[96]; serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(t, buf);

  macroFiredIdx = idx;
  macroFiredAt  = millis();
  strncpy(macroMsg, "...", sizeof(macroMsg) - 1);   // esperando respuesta del agente
  macroOk = true; macroMsgAt = millis();
  Serial.printf("MACRO FIRE: %s\n", macros[idx].id);
}

// ═══════════════ INPUT ═══════════════
void handleInput() {
  bool btn = (digitalRead(PIN_BTN_MENU) == LOW);
  bool sw  = (digitalRead(PIN_JOY_SW)  == LOW);

  // Botón físico: navegación / atrás
  if (btn && millis() - lastBtn > BTN_DEBOUNCE) {
    lastBtn = millis();
    switch (currentScreen) {
      case SCREEN_HOME:
        currentScreen = SCREEN_MENU;
        mainMenu.index = 0;
        mainMenu.scroll = 0;
        break;
      case SCREEN_MENU:
        currentScreen = SCREEN_HOME;
        break;
      case SCREEN_RFID_MENU:              // submenú RFID -> menú principal
      case SCREEN_MACROS:                 // stream deck   -> menú principal
        currentScreen = SCREEN_MENU;
        break;
      default:                            // scan/register/db -> submenú RFID
        currentScreen = SCREEN_RFID_MENU;
        cardPresent = false;
        confirmWipe = false;
        break;
    }
    return;
  }

  // Joystick vertical: navega cualquier menú, la CARD DB o la lista de MACROS
  if (currentScreen == SCREEN_MENU || currentScreen == SCREEN_RFID_MENU ||
      currentScreen == SCREEN_CARDDB || currentScreen == SCREEN_MACROS) {
    int y = analogRead(PIN_JOY_Y);
    if (millis() - lastJoy > JOY_REPEAT) {
      int dir = 0;
      if (y < 1000)      dir = -1;
      else if (y > 3000) dir = +1;
      if (dir != 0) {
        if      (currentScreen == SCREEN_CARDDB) moveDB(dir);
        else if (currentScreen == SCREEN_MACROS) moveMacros(dir);
        else                                     moveMenu(dir);
        lastJoy = millis();
      }
    }
  }

  // Click del joystick (SW): confirma / ejecuta
  if (sw && millis() - lastBtn > BTN_DEBOUNCE) {
    lastBtn = millis();
    if (currentScreen == SCREEN_MENU) {
      selectMainItem();
    } else if (currentScreen == SCREEN_RFID_MENU) {
      selectRfidItem();
    } else if (currentScreen == SCREEN_MACROS) {
      if (macroCount > 0) {               // click = dispara la macro resaltada
        fireMacro(macroIndex);
        flashSelection();
      }
    } else if (currentScreen == SCREEN_CARDDB) {
      if (!confirmWipe) {
        confirmWipe = true;               // primer click: pide confirmación
      } else {
        wipeCards();                      // segundo click: borra todo
        confirmWipe = false;
        dbScroll = 0;
        flashSelection();
      }
    }
  }
}

void selectMainItem() {
  if (mainMenu.index == 0) {                    // RFID SCANNER -> submenú
    currentScreen = SCREEN_RFID_MENU;
    rfidMenu.index = 0;
    rfidMenu.scroll = 0;
  } else if (mainMenu.index == 1) {             // MACROS -> stream deck
    currentScreen = SCREEN_MACROS;
    macroIndex = 0;
    macroScroll = 0;
    macroFiredIdx = -1;
    macroMsg[0] = '\0';
  } else if (mainMenu.index == mainMenu.len - 1) {  // < BACK
    currentScreen = SCREEN_HOME;
  } else {                                       // SYSTEM INFO / SETTINGS -> futuro
    flashSelection();
  }
}

void selectRfidItem() {
  switch (rfidMenu.index) {
    case 0:   // SCAN CARD
      currentScreen = SCREEN_RFID;
      cardPresent = false;
      lastUID[0] = '\0';
      break;
    case 1:   // REGISTER CARD
      currentScreen = SCREEN_REGISTER;
      regResult = 0;
      break;
    case 2:   // CARD DATABASE
      currentScreen = SCREEN_CARDDB;
      dbScroll = 0;
      confirmWipe = false;
      break;
    case 3:   // < BACK
      currentScreen = SCREEN_MENU;
      break;
  }
}

void moveMenu(int dir) {
  Menu* m = (currentScreen == SCREEN_RFID_MENU) ? &rfidMenu : &mainMenu;
  m->index = constrain(m->index + dir, 0, m->len - 1);
  if (m->index <  m->scroll)                m->scroll = m->index;
  if (m->index >= m->scroll + MENU_VISIBLE) m->scroll = m->index - MENU_VISIBLE + 1;
}

void moveDB(int dir) {
  if (cardCount == 0) return;
  int maxScroll = (cardCount > DB_VISIBLE) ? cardCount - DB_VISIBLE : 0;
  dbScroll = constrain(dbScroll + dir, 0, maxScroll);
  confirmWipe = false;
}

void moveMacros(int dir) {
  if (macroCount == 0) return;
  macroIndex = constrain(macroIndex + dir, 0, macroCount - 1);
  if (macroIndex <  macroScroll)                 macroScroll = macroIndex;
  if (macroIndex >= macroScroll + MACRO_VISIBLE) macroScroll = macroIndex - MACRO_VISIBLE + 1;
}

// ═══════════════ LECTURA COMÚN DE TARJETA ═══════════════
// Lee UID + tipo en lastUID/lastType. Devuelve true si leyó una tarjeta.
bool readCard() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;

  char* p = lastUID;
  for (byte i = 0; i < rfid.uid.size; i++) {
    p += sprintf(p, "%02X", rfid.uid.uidByte[i]);
    if (i < rfid.uid.size - 1) *p++ = ':';
  }
  *p = '\0';

  MFRC522::PICC_Type type = rfid.PICC_GetType(rfid.uid.sak);
  strncpy(lastType, (const char*)rfid.PICC_GetTypeName(type), sizeof(lastType) - 1);
  lastType[sizeof(lastType) - 1] = '\0';

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  return true;
}

// ═══════════════ RFID SCANNER (control de acceso) ═══════════════
void rfidLoop() {
  if (!cardPresent && readCard()) {
    int idx = findCard(lastUID);
    lastGranted = (idx >= 0);
    if (idx >= 0) {
      strncpy(lastName, cards[idx].name, sizeof(lastName) - 1);
      lastName[sizeof(lastName) - 1] = '\0';
    } else {
      lastName[0] = '\0';
    }

    Serial.printf("UID: %s -> %s\n", lastUID, lastGranted ? "GRANTED" : "DENIED");
    publishScan();

    cardPresent = true;
    cardShownAt = millis();

    if (lastGranted) accessGrantedFlash();
    else             accessDeniedFlash();
  }

  if (cardPresent && millis() - cardShownAt > 6000) cardPresent = false;

  if (cardPresent) {
    if (lastGranted) drawGranted();
    else             drawDenied();
  } else {
    drawScanning();
  }
}

// ═══════════════ REGISTER CARD ═══════════════
void registerLoop() {
  if (regResult == 0 && readCard()) {
    regResult  = addCard(lastUID);       // 1=ok, 2=dup, 3=llena
    regShownAt = millis();
    if (regResult == 1) accessGrantedFlash();
    else                accessDeniedFlash();
  }

  if (regResult != 0 && millis() - regShownAt > 2500) regResult = 0;

  if (regResult == 0) drawRegisterPrompt();
  else                drawRegisterResult();
}

// ═══════════════ PANTALLAS MACROS ═══════════════
void drawMacros() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("[ MACROS ]"));
  if (macroCount > 0) {
    display.setCursor(92, 3);
    display.print("[");
    display.print(macroIndex + 1);
    display.print("/");
    display.print(macroCount);
    display.print("]");
  }

  display.setTextColor(SSD1306_WHITE);

  if (macroCount == 0) {
    display.setCursor(10, 26);
    display.print(F("SIN MACROS"));
    display.setCursor(4, 42);
    display.print(mqtt.connected() ? F("ESPERANDO AGENTE..") : F("SIN CONEXION MQTT"));
    display.display();
    return;
  }

  // Si hay un resultado reciente del agente, se reserva la última fila para él
  bool showMsg = (macroMsg[0] && millis() - macroMsgAt < 3000);
  int rows = showMsg ? 3 : MACRO_VISIBLE;

  for (int i = 0; i < rows; i++) {
    int idx = macroScroll + i;
    if (idx >= macroCount) break;
    int yPos = 18 + i * 11;
    bool sel = (idx == macroIndex);
    if (sel) {
      display.fillRect(0, yPos - 1, 128, 11, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    }
    display.setCursor(3, yPos);
    display.print(macros[idx].label);
    if (sel) display.setTextColor(SSD1306_WHITE);
  }

  if (showMsg) {                           // franja de resultado (>> ok / !! error)
    display.fillRect(0, 53, 128, 11, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(3, 55);
    display.print(macroOk ? F(">> ") : F("!! "));
    display.print(macroMsg);
    display.setTextColor(SSD1306_WHITE);
  } else {
    if (macroScroll > 0)                          display.fillTriangle(122, 18, 126, 18, 124, 15, SSD1306_WHITE);
    if (macroScroll + MACRO_VISIBLE < macroCount) display.fillTriangle(122, 60, 126, 60, 124, 63, SSD1306_WHITE);
  }

  display.display();
}

// ═══════════════ PANTALLAS RFID ═══════════════
void drawScanning() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("[ RFID SCANNER ]"));

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 22);
  display.print(F("SCANNING"));
  for (int i = 0; i < (scanAnimFrame % 4); i++) display.print(".");

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

void drawGranted() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F(">> ACCESS GRANTED"));

  display.setTextColor(SSD1306_WHITE);
  display.drawBitmap(3, 20, ICON_SKULL, 8, 8, SSD1306_WHITE);
  display.setCursor(15, 20);
  display.print(F("WELCOME BACK"));

  display.setCursor(3, 34);
  display.print(F("USER: "));
  display.print(lastName);

  display.setCursor(3, 46);
  display.print(F("UID:"));
  display.setCursor(3, 56);
  display.print(lastUID);

  display.display();
}

void drawDenied() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("!! ACCESS DENIED"));

  display.setTextColor(SSD1306_WHITE);
  // gran X de advertencia (doble trazo para que se vea gruesa)
  display.drawLine(50, 20, 78, 50, SSD1306_WHITE);
  display.drawLine(51, 20, 79, 50, SSD1306_WHITE);
  display.drawLine(78, 20, 50, 50, SSD1306_WHITE);
  display.drawLine(79, 20, 51, 50, SSD1306_WHITE);

  display.setCursor(3, 56);
  display.print(F("UNK:"));
  display.print(lastUID);

  display.display();
}

void accessGrantedFlash() {
  for (int i = 0; i < 3; i++) {
    display.invertDisplay(true);  delay(50);
    display.invertDisplay(false); delay(50);
  }
}

void accessDeniedFlash() {
  for (int i = 0; i < 6; i++) {
    display.invertDisplay(true);  delay(35);
    display.invertDisplay(false); delay(35);
  }
}

// ═══════════════ PANTALLAS REGISTER / DB ═══════════════
void drawRegisterPrompt() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("[ REGISTER CARD ]"));

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(6, 24);
  display.print(F("ACERCA TARJETA"));
  display.setCursor(6, 36);
  display.print(F("A REGISTRAR..."));

  display.setCursor(6, 54);
  display.print(F("DB: "));
  display.print(cardCount);
  display.print(F("/"));
  display.print(MAX_CARDS);

  display.display();
}

void drawRegisterResult() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("[ REGISTER CARD ]"));

  display.setTextColor(SSD1306_WHITE);
  const char* msg;
  if      (regResult == 1) msg = ">> REGISTRADA OK";
  else if (regResult == 2) msg = "!! YA EXISTIA";
  else                     msg = "!! DB LLENA";

  display.setCursor(6, 24);
  display.print(msg);

  display.setCursor(6, 40);
  display.print(F("UID:"));
  display.setCursor(6, 50);
  display.print(lastUID);

  display.display();
}

void drawCardDB() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print(F("== CARD DB =="));
  display.setCursor(98, 3);
  display.print(F("["));
  display.print(cardCount);
  display.print(F("]"));

  display.setTextColor(SSD1306_WHITE);

  if (cardCount == 0) {
    display.setCursor(16, 30);
    display.print(F("SIN TARJETAS"));
    display.setCursor(10, 44);
    display.print(F("REGISTRA UNA :)"));
    display.display();
    return;
  }

  if (confirmWipe) {
    display.setCursor(10, 24);
    display.print(F("BORRAR TODO?"));
    display.setCursor(10, 38);
    display.print(F("SW  = CONFIRMAR"));
    display.setCursor(10, 50);
    display.print(F("BTN = CANCELAR"));
    display.display();
    return;
  }

  for (int i = 0; i < DB_VISIBLE; i++) {
    int idx = dbScroll + i;
    if (idx >= cardCount) break;
    int yPos = 18 + i * 11;
    display.setCursor(2, yPos);
    display.print(idx + 1);
    display.print(F("."));
    display.setCursor(16, yPos);
    display.print(cards[idx].name);
  }

  if (dbScroll > 0)                       display.fillTriangle(122, 18, 126, 18, 124, 15, SSD1306_WHITE);
  if (dbScroll + DB_VISIBLE < cardCount)  display.fillTriangle(122, 60, 126, 60, 124, 63, SSD1306_WHITE);

  display.display();
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
    strftime(fecha, sizeof(fecha), "%d/%m/%Y>%a", &t);
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
  Menu* m = (currentScreen == SCREEN_RFID_MENU) ? &rfidMenu : &mainMenu;
  display.clearDisplay();

  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);   // icono/texto en NEGRO sobre la barra blanca
  display.setTextSize(1);

  // Título del header. El MAIN MENU usa el icono de diskette en vez del primer "="
  if (currentScreen == SCREEN_MENU) {
    display.drawBitmap(3, 3, ICON_TERMINAL, 8, 8, SSD1306_BLACK);
    display.setCursor(13, 3);
    display.print(F("MAIN MENU"));
  } else {
    display.setCursor(4, 3);
    display.print(F("RFID SCANNER"));
  }

  display.setCursor(92, 3);
  display.print("[");
  display.print(m->index + 1);
  display.print("/");
  display.print(m->len);
  display.print("]");

  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < MENU_VISIBLE; i++) {
    int idx = m->scroll + i;
    if (idx >= m->len) break;
    int yPos = 20 + i * 11;

    if (idx == m->index) {
      display.fillRect(0, yPos - 1, 128, 11, SSD1306_WHITE);
      display.drawBitmap(3, yPos, ICON_SKULL, 8, 8, SSD1306_BLACK);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(15, yPos);
      display.print(m->items[idx]);
      display.setTextColor(SSD1306_WHITE);
    } else {
      display.setCursor(15, yPos);
      display.print(m->items[idx]);
    }
  }

  if (m->scroll > 0)                    display.fillTriangle(122, 20, 126, 20, 124, 17, SSD1306_WHITE);
  if (m->scroll + MENU_VISIBLE < m->len) display.fillTriangle(122, 60, 126, 60, 124, 63, SSD1306_WHITE);

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

  display.drawBitmap(70, 3, ICON_RADIATION, 8, 8, SSD1306_WHITE);  // icono 8x8 centrado (antes drawBiohazard)
  drawWifiBars(104, 12);

  display.drawFastHLine(0, 15, 128, SSD1306_WHITE);
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

  const char* title = "RETRO-TERM v0.4";
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
    display.print(F("RETRO-TERM v0.4"));
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

// ═══════════════ MQTT ═══════════════
const char* screenName(Screen s) {
  switch (s) {
    case SCREEN_HOME:      return "HOME";
    case SCREEN_MENU:      return "MENU";
    case SCREEN_RFID_MENU: return "RFIDMENU";
    case SCREEN_RFID:      return "RFID";
    case SCREEN_REGISTER: return "REGISTER";
    case SCREEN_CARDDB:   return "CARDDB";
    case SCREEN_MACROS:   return "MACROS";
  }
  return "?";
}

// arma "retroterm/term01/<suffix>" en out
void mqttTopic(char* out, size_t n, const char* suffix) {
  snprintf(out, n, "%s/%s", topicBase, suffix);
}

int wifiBars() {
  if (WiFi.status() != WL_CONNECTED) return 0;
  long r = WiFi.RSSI();
  return (r > -55) ? 4 : (r > -65) ? 3 : (r > -75) ? 2 : 1;
}

void publishStatus(bool online) {
  char t[64]; mqttTopic(t, sizeof(t), "status");
  JsonDocument doc;
  doc["online"] = online;
  doc["fw"]     = "0.5";
  if (online) {
    doc["ip"]   = WiFi.localIP().toString();
    doc["rssi"] = (int)WiFi.RSSI();
  }
  char buf[192]; serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(t, buf, true);      // retained
}

void publishTelemetry() {
  char t[64]; mqttTopic(t, sizeof(t), "telemetry");
  JsonDocument doc;
  doc["rssi"]   = (int)WiFi.RSSI();
  doc["bars"]   = wifiBars();
  doc["ip"]     = WiFi.localIP().toString();
  doc["heap"]   = (uint32_t)ESP.getFreeHeap();
  doc["cards"]  = cardCount;
  doc["macros"] = macroCount;
  doc["uptime"] = (uint32_t)(millis() / 1000);
  doc["screen"] = screenName(currentScreen);
  struct tm tm;
  if (getLocalTime(&tm, 20)) {
    char clk[16]; strftime(clk, sizeof(clk), "%H:%M:%S", &tm);
    doc["clock"] = clk;
  }
  char buf[256]; serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(t, buf);
}

void publishScan() {
  if (!mqtt.connected()) return;
  char t[64]; mqttTopic(t, sizeof(t), "rfid/scan");
  JsonDocument doc;
  doc["uid"]     = lastUID;
  doc["type"]    = lastType;
  doc["granted"] = lastGranted;
  doc["name"]    = lastGranted ? lastName : "";
  doc["uptime"]  = (uint32_t)(millis() / 1000);
  char buf[192]; serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(t, buf);
}

void publishDB() {
  char t[64]; mqttTopic(t, sizeof(t), "rfid/db");
  JsonDocument doc;
  doc["count"] = cardCount;
  doc["max"]   = MAX_CARDS;
  JsonArray arr = doc["cards"].to<JsonArray>();
  for (int i = 0; i < cardCount; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["uid"]  = cards[i].uid;
    o["name"] = cards[i].name;
  }
  char buf[1536]; serializeJson(doc, buf, sizeof(buf));
  mqtt.publish(t, buf, true);      // retained
}

// macros/config (retenido): {"macros":[{"id":"...","label":"..."}]}
void parseMacrosConfig(JsonDocument& doc) {
  JsonArray arr = doc["macros"].as<JsonArray>();
  macroCount = 0;
  for (JsonObject o : arr) {
    if (macroCount >= MAX_MACROS) break;
    const char* id  = o["id"]    | "";
    const char* lbl = o["label"] | id;
    if (!id[0]) continue;
    strncpy(macros[macroCount].id,    id,  sizeof(macros[macroCount].id) - 1);
    macros[macroCount].id[sizeof(macros[macroCount].id) - 1] = '\0';
    strncpy(macros[macroCount].label, lbl, sizeof(macros[macroCount].label) - 1);
    macros[macroCount].label[sizeof(macros[macroCount].label) - 1] = '\0';
    macroCount++;
  }
  if (macroIndex >= macroCount) macroIndex = macroCount > 0 ? macroCount - 1 : 0;
  if (macroScroll > macroIndex) macroScroll = 0;
  Serial.printf("[MACROS] config recibida: %d macros\n", macroCount);
}

// macro/result: {"id":"...","ok":true,"msg":"QUEUED"}
void parseMacroResult(JsonDocument& doc) {
  macroOk = doc["ok"] | false;
  const char* m = doc["msg"] | (macroOk ? "OK" : "ERR");
  strncpy(macroMsg, m, sizeof(macroMsg) - 1);
  macroMsg[sizeof(macroMsg) - 1] = '\0';
  macroMsgAt = millis();
}

// comandos entrantes: retroterm/term01/cmd/#  +  macros/config  +  macro/result
void mqttCallback(char* topicIn, byte* payload, unsigned int len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload, len);

  char addT[64], delT[64], renT[64], wipeT[64], idT[64];
  mqttTopic(addT,  sizeof(addT),  "cmd/rfid/add");
  mqttTopic(delT,  sizeof(delT),  "cmd/rfid/delete");
  mqttTopic(renT,  sizeof(renT),  "cmd/rfid/rename");
  mqttTopic(wipeT, sizeof(wipeT), "cmd/rfid/wipe");
  mqttTopic(idT,   sizeof(idT),   "cmd/identify");

  if (!strcmp(topicIn, wipeT)) {            // wipe no necesita payload válido
    wipeCards();
    return;
  }
  if (!strcmp(topicIn, idT)) {
    flashSelection();
    return;
  }
  if (err) return;                          // el resto sí necesita JSON válido

  char macCfgT[64], macResT[64];
  mqttTopic(macCfgT, sizeof(macCfgT), "macros/config");
  mqttTopic(macResT, sizeof(macResT), "macro/result");
  if (!strcmp(topicIn, macCfgT)) { parseMacrosConfig(doc); return; }
  if (!strcmp(topicIn, macResT)) { parseMacroResult(doc);  return; }

  const char* uid  = doc["uid"]  | "";
  const char* name = doc["name"] | "";

  if (!strcmp(topicIn, addT)) {
    if (uid[0]) addCardNamed(uid, name[0] ? name : nullptr);
  } else if (!strcmp(topicIn, delT)) {
    if (uid[0]) deleteCardByUid(uid);
  } else if (!strcmp(topicIn, renT)) {
    if (uid[0] && name[0]) renameCard(uid, name);
  }
}

void mqttConnect() {
  if (!MQTT_ENABLED) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastMqttRetry < 5000) return;   // reintenta cada 5 s
  lastMqttRetry = millis();

  char willT[64]; mqttTopic(willT, sizeof(willT), "status");
  const char* willMsg = "{\"online\":false}";
  String cid = String("retroterm-") + DEVICE_ID;

  Serial.printf("[MQTT] Conectando a %s:%d ... ", MQTT_HOST, MQTT_PORT);
  bool ok = strlen(MQTT_USER)
    ? mqtt.connect(cid.c_str(), MQTT_USER, MQTT_PASS, willT, 0, true, willMsg)
    : mqtt.connect(cid.c_str(), NULL, NULL,           willT, 0, true, willMsg);

  if (ok) {
    Serial.println("OK");
    char subT[64]; mqttTopic(subT, sizeof(subT), "cmd/#");
    mqtt.subscribe(subT);
    char m1[64]; mqttTopic(m1, sizeof(m1), "macros/config"); mqtt.subscribe(m1);  // lista (retenida)
    char m2[64]; mqttTopic(m2, sizeof(m2), "macro/result");  mqtt.subscribe(m2);  // resultados
    publishStatus(true);
    publishDB();
    lastTelemetry = 0;            // fuerza telemetría inmediata
  } else {
    // state() -2 = no llegó al broker (IP mal / firewall / broker apagado)
    //         -4 = timeout   5 = credenciales rechazadas
    Serial.printf("FALLO (state=%d)\n", mqtt.state());
  }
}

void mqttLoop() {
  if (!MQTT_ENABLED) return;
  if (!mqtt.connected()) { mqttConnect(); return; }
  mqtt.loop();
  if (dbDirty) { publishDB(); dbDirty = false; }
  if (millis() - lastTelemetry > 5000) {
    publishTelemetry();
    lastTelemetry = millis();
  }
}
