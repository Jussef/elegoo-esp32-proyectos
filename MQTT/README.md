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
