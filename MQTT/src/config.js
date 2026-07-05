// Configuración leída de .env (con valores por defecto sensatos).
const env = import.meta.env

export const MQTT = {
  host: env.VITE_MQTT_HOST || 'localhost',
  port: Number(env.VITE_MQTT_PORT || 9001),
  path: env.VITE_MQTT_PATH || '/mqtt',
  user: env.VITE_MQTT_USER || undefined,
  pass: env.VITE_MQTT_PASS || undefined,
  secure: env.VITE_MQTT_SECURE === 'true', // ws:// por defecto, wss:// si se pide
}

export const DEVICE_ID = env.VITE_DEVICE_ID || 'term01'
export const BASE = `retroterm/${DEVICE_ID}`

// Topics que el ESP32 publica y nosotros escuchamos.
export const TOPICS = {
  status: `${BASE}/status`,
  telemetry: `${BASE}/telemetry`,
  scan: `${BASE}/rfid/scan`,
  db: `${BASE}/rfid/db`,
}

// Topics de comando que nosotros publicamos.
export const CMD = {
  add: `${BASE}/cmd/rfid/add`,
  del: `${BASE}/cmd/rfid/delete`,
  rename: `${BASE}/cmd/rfid/rename`,
  wipe: `${BASE}/cmd/rfid/wipe`,
  identify: `${BASE}/cmd/identify`,
}

export const brokerUrl = () =>
  `${MQTT.secure ? 'wss' : 'ws'}://${MQTT.host}:${MQTT.port}${MQTT.path}`
