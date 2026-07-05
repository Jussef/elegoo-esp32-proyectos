/*
 * Broker MQTT para RETRO-TERM — SIN Docker, SIN instalar nada.
 * Corre 100% desde node_modules (Aedes, MQTT en puro JavaScript).
 *
 *   yarn broker
 *
 * Escucha en dos puertos:
 *   1883 -> MQTT nativo (TCP)      -> lo usa el ESP32
 *   9001 -> MQTT sobre WebSocket   -> lo usa el dashboard (navegador)
 *
 * Soporta mensajes retained y Last Will (LWT), que es lo que necesita
 * el contrato de topics (status/db retenidos, offline por LWT).
 */
const aedes = require('aedes')()
const net = require('net')
const http = require('http')
const websocketStream = require('websocket-stream')

const TCP_PORT = 1883
const WS_PORT = 9001

net.createServer(aedes.handle).listen(TCP_PORT, () => {
  console.log(`[broker] MQTT (TCP)       -> puerto ${TCP_PORT}   · para el ESP32`)
})

const httpServer = http.createServer()
websocketStream.createServer({ server: httpServer }, aedes.handle)
httpServer.listen(WS_PORT, () => {
  console.log(`[broker] MQTT (WebSocket) -> puerto ${WS_PORT}   · para el dashboard`)
  console.log('[broker] Listo. Ctrl+C para detener.\n')
})

// --- logs bonitos ---
aedes.on('client', (c) => console.log(`  [+] conectado:    ${c.id}`))
aedes.on('clientDisconnect', (c) => console.log(`  [-] desconectado: ${c.id}`))
aedes.on('publish', (packet, client) => {
  if (client && packet.topic) {
    const body = packet.payload ? packet.payload.toString().slice(0, 80) : ''
    console.log(`      ${client.id} → ${packet.topic}  ${body}`)
  }
})
