# RETRO-TERM · MQTT Dashboard 🖥️📡

Panel web retrofuturista (estilo Fallout / Blade Runner) para el proyecto
**RETRO-TERM v0.5** en ESP32. Monitorea en vivo y gestiona la base de tarjetas
RFID por MQTT. Aislado de Astro: es una app propia con su propio `package.json`.

```
navegador ──WebSocket:9001──►  ┌───────────┐  ◄──TCP:1883── ESP32 (retro_terminal_esp32.ino)
(React + MQTT.js)              │ Mosquitto │
                              └───────────┘
```

El navegador **solo** puede hablar MQTT sobre WebSocket; el ESP32 habla MQTT
nativo. Los dos se conectan al **mismo broker** y se comunican por los topics
de abajo.

---

## 🚀 Guía de inicio (haz esto en orden)

### Qué necesitas
- **Node + yarn** en la PC (ya lo tienes).
- **Arduino IDE** con las librerías: `PubSubClient`, `ArduinoJson` (v7),
  `Adafruit SSD1306`, `Adafruit GFX`, `MFRC522`.
- ESP32, OLED, joystick, botón y RC522 conectados como en el `.ino`.
- La PC y el ESP32 en la **misma red Wi-Fi**.

### Pasos

**1. Averigua la IP de tu PC** (es donde corre el broker):
```powershell
ipconfig    # busca la "IPv4" de tu Wi-Fi, p.ej. 192.168.0.169
```
> ⚠️ Ignora la de VirtualBox (`192.168.56.x`). Usa la de tu Wi-Fi real.

**2. Arranca el broker** (en una terminal, déjalo abierto):
```bash
cd MQTT
yarn install      # solo la primera vez
yarn broker       # MQTT en 1883 (ESP32) y 9001 (dashboard)
```

**3. Abre los puertos en el Firewall de Windows** ⭐ *(el paso que casi todos olvidan)*

Windows bloquea las conexiones entrantes en redes **Públicas**, así que el ESP32
no puede llegar al broker aunque esté corriendo. Abre **PowerShell como
Administrador** (clic derecho → "Ejecutar como administrador") y pega:
```powershell
New-NetFirewallRule -DisplayName "MQTT 1883" -Direction Inbound -Protocol TCP -LocalPort 1883 -Action Allow
New-NetFirewallRule -DisplayName "MQTT WS 9001" -Direction Inbound -Protocol TCP -LocalPort 9001 -Action Allow
```
*(Alternativa: marca tu Wi-Fi como red "Privada" en Ajustes de Windows. Esto es
de una sola vez.)*

**4. Configura y flashea el ESP32.** En `src/arduino/retro_terminal_esp32.ino`:
```cpp
const char* WIFI_SSID = "tu_wifi";
const char* WIFI_PASS = "tu_password";
const char* MQTT_HOST = "192.168.0.169";  // <- la IP de tu PC del paso 1
```
Abre el **Monitor Serie** (115200 baud). Debe decir:
```
[MQTT] Conectando a 192.168.0.169:1883 ... OK
```
Si dice `FALLO (state=-2)` → no llega al broker: revisa el paso 3 (firewall),
que la IP sea la correcta y que `yarn broker` siga corriendo.

**5. Configura y arranca el dashboard** (otra terminal):
```bash
cd MQTT
# .env: si abres el dashboard en la MISMA PC del broker, usa localhost:
#   VITE_MQTT_HOST=localhost
# si lo abres desde otro dispositivo, pon la IP de la PC (paso 1).
yarn dev          # http://localhost:5173
```

Al abrir la web deberías ver **ONLINE**, el reloj, la señal Wi-Fi y, al acercar
una tarjeta, aparece en el SCAN LOG. ✅

### ¿Sigue sin moverse? Diagnóstico exprés
```powershell
# ¿El broker escucha?  (deben salir 1883 y 9001 LISTENING)
netstat -ano | findstr "1883 9001"
# ¿El ESP32 está conectado?  (debe salir una linea ...:1883  ESTABLISHED)
netstat -ano | findstr 1883
```
- Broker escucha pero **no hay ESTABLISHED en 1883** → firewall / IP del `.ino`.
- El Monitor Serie no dice nada de `[MQTT]` → el ESP32 no pasó del Wi-Fi
  (revisa SSID/PASS) o `MQTT_ENABLED` está en `false`.

---

## 1. Contrato de topics

Base: **`retroterm/<DEVICE_ID>`** — por defecto `retroterm/term01`.
(Cambia `DEVICE_ID` en el `.ino` y `VITE_DEVICE_ID` en el dashboard para tener
varias terminales en el homelab.)

### El ESP32 publica  →  el dashboard escucha

| Topic | Retained | Cuándo | Payload |
|---|---|---|---|
| `.../status` | ✅ (+ LWT) | al conectar / al caer | `{"online":true,"ip":"192.168.1.50","rssi":-52,"fw":"0.5"}` |
| `.../telemetry` | — | cada 5 s | `{"rssi":-52,"bars":4,"ip":"...","heap":210000,"cards":3,"uptime":1234,"clock":"14:05:22","screen":"HOME"}` |
| `.../rfid/scan` | — | en cada lectura de tarjeta | `{"uid":"A1:B2:C3:D4","type":"MIFARE 1KB","granted":true,"name":"AGENT-01","uptime":1234}` |
| `.../rfid/db` | ✅ | al cambiar la base | `{"count":3,"max":20,"cards":[{"uid":"A1:B2:C3:D4","name":"AGENT-01"}]}` |

`status` usa **Last Will**: si el ESP32 se desconecta sin avisar, el broker
publica `{"online":false}` retenido, y el dashboard lo muestra como OFFLINE.

### El dashboard publica  →  el ESP32 escucha (`.../cmd/#`)

| Topic | Payload | Efecto |
|---|---|---|
| `.../cmd/rfid/add` | `{"uid":"A1:B2:C3:D4","name":"AGENT-05"}` | Da de alta (name opcional → auto `AGENT-NN`) |
| `.../cmd/rfid/delete` | `{"uid":"A1:B2:C3:D4"}` | Borra esa tarjeta |
| `.../cmd/rfid/rename` | `{"uid":"...","name":"NUEVO"}` | Renombra |
| `.../cmd/rfid/wipe` | `{}` | Borra TODA la base |
| `.../cmd/identify` | `{}` | Parpadea el OLED para localizar la terminal |

Tras cualquier cambio, el ESP32 re-publica `.../rfid/db` (retained), así el
dashboard siempre queda sincronizado.

---

## 2. Arrancar el broker

Tienes dos opciones. **En un PC sin Docker ni permisos de instalación, usa la A.**

### A) Broker en Node (sin Docker, sin instalar nada) ✅ recomendado ahora

Corre 100% desde `node_modules` (Aedes, un broker MQTT en puro JavaScript).
Soporta *retained* y *Last Will*, igual que Mosquitto.

```bash
cd MQTT
yarn install        # una sola vez (también instala el dashboard)
yarn broker         # levanta MQTT en 1883 (TCP) y 9001 (WebSocket)
```

Déjalo corriendo en una terminal. Verás en el log cada conexión y publicación.

### B) Mosquitto con Docker (para el homelab, cuando lo tengas)

```bash
cd MQTT/broker
docker compose up -d          # 1883 (MQTT) + 9001 (WS)
docker compose logs -f
```

Sin Docker pero con Mosquitto instalado a mano:
`mosquitto -c MQTT/broker/mosquitto.conf -v`

> Anota la **IP de la máquina donde corre el broker** (tu PC/servidor homelab).
> La necesitas en el `.ino` (`MQTT_HOST`) y en el dashboard (`VITE_MQTT_HOST`).
> En Windows la ves con `ipconfig`; en Linux con `ip a`. Si el broker y el
> navegador están en el **mismo** PC, puedes usar `localhost`.

---

## 3. Configurar el ESP32

En `src/arduino/retro_terminal_esp32.ino`, sección `MQTT`:

```cpp
const char* MQTT_HOST = "192.168.1.100";  // IP del broker
const int   MQTT_PORT = 1883;
const char* DEVICE_ID = "term01";
```

Librerías necesarias (Arduino Library Manager):
`PubSubClient` (knolleary) · `ArduinoJson` (v7) · más las que ya usabas
(Adafruit SSD1306/GFX, MFRC522).

---

## 4. Arrancar el dashboard

```bash
cd MQTT
cp .env.example .env          # y edita VITE_MQTT_HOST con la IP del broker
yarn install
yarn dev                      # abre http://localhost:5173
```

Para compilar la versión estática (servible desde cualquier lado del homelab):
`yarn build` → genera `MQTT/dist/`.

---

## 5. Probar sin ESP32 (opcional)

Puedes simular la terminal con `mosquitto_pub` para ver moverse el dashboard:

```bash
mosquitto_pub -h localhost -t retroterm/term01/status    -r -m '{"online":true,"ip":"192.168.1.50","rssi":-48,"fw":"0.5"}'
mosquitto_pub -h localhost -t retroterm/term01/rfid/db   -r -m '{"count":1,"max":20,"cards":[{"uid":"A1:B2:C3:D4","name":"AGENT-01"}]}'
mosquitto_pub -h localhost -t retroterm/term01/rfid/scan     -m '{"uid":"DE:AD:BE:EF","type":"MIFARE 1KB","granted":false,"name":""}'
```
