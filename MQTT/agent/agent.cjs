/*
 * AGENTE RETRO-TERM — el "cerebro" del stream deck.
 * ─────────────────────────────────────────────────────────
 * Corre en el homelab (o en tu PC). Se conecta al MISMO broker
 * que el ESP32 y:
 *   1) publica la lista de MACROS (retenida) -> el OLED la muestra
 *   2) escucha macro/fire  -> ejecuta la acción real
 *   3) responde macro/result -> el OLED muestra el resultado
 *
 *   yarn agent           (o: node agent/agent.cjs)
 *
 * Requisitos por acción:
 *   - "docker restart": el comando `docker` debe existir en ESTA máquina
 *     y tu usuario debe poder usarlo (agrega el nombre real del contenedor).
 *   - "health": esta máquina debe poder alcanzar por red a los servicios.
 * ─────────────────────────────────────────────────────────
 */
const mqtt = require('mqtt')
const net = require('net')
const { execFile } = require('child_process')

// ╔══════════════════ CONFIG (edita esto) ══════════════════╗
const DEVICE_ID = process.env.DEVICE_ID  || 'term01'
const BROKER    = process.env.AGENT_MQTT || 'mqtt://localhost:1883'

// Servicios que revisa "HOMELAB HEALTH" (nombre corto para el OLED + host:puerto)
const SERVICES = [
  { name: 'PROXMOX', host: '192.168.0.10', port: 8006 },
  { name: 'PIHOLE',  host: '192.168.0.11', port: 80 },
  { name: 'NAS',     host: '192.168.0.12', port: 5000 },
]

// Contenedor que reinicia "RESTART <cont>" (pon el nombre real de tu contenedor)
const DOCKER_CONTAINER = 'nginx'
// ╚═════════════════════════════════════════════════════════╝

const base = `retroterm/${DEVICE_ID}`

// ---- Acciones ----
function checkTcp({ host, port }, timeout = 1500) {
  return new Promise((resolve) => {
    const sock = new net.Socket()
    let done = false
    const finish = (ok) => { if (!done) { done = true; sock.destroy(); resolve(ok) } }
    sock.setTimeout(timeout)
    sock.once('connect', () => finish(true))
    sock.once('timeout', () => finish(false))
    sock.once('error',   () => finish(false))
    sock.connect(port, host)
  })
}

async function healthAll() {
  let up = 0, downName = null
  for (const s of SERVICES) {
    const ok = await checkTcp(s)
    if (ok) up++
    else if (!downName) downName = s.name
  }
  const total = SERVICES.length
  return up === total
    ? { ok: true,  msg: `OK ${up}/${total}` }
    : { ok: false, msg: `DOWN ${downName}` }
}

function dockerRestart(name) {
  return new Promise((resolve) => {
    execFile('docker', ['restart', name], { timeout: 12000 }, (err, _out, stderr) => {
      if (err) {
        const m = (stderr || err.message || 'ERR').split('\n')[0].slice(0, 16)
        resolve({ ok: false, msg: m })
      } else {
        resolve({ ok: true, msg: 'RESTARTED' })
      }
    })
  })
}

// ---- Registro de MACROS (id + etiqueta OLED + acción) ----
// Agregar una macro nueva = una línea aquí. El ESP32 no se reflashea.
const MACROS = [
  { id: 'health-all',     label: 'HOMELAB HEALTH',  run: () => healthAll() },
  { id: 'docker-restart', label: `RESTART ${DOCKER_CONTAINER}`, run: () => dockerRestart(DOCKER_CONTAINER) },
]

// ---- MQTT ----
const client = mqtt.connect(BROKER, { clientId: `agent-${DEVICE_ID}` })

client.on('connect', () => {
  console.log(`[agent] conectado a ${BROKER} como agent-${DEVICE_ID}`)
  const cfg = { macros: MACROS.map((m) => ({ id: m.id, label: m.label })) }
  client.publish(`${base}/macros/config`, JSON.stringify(cfg), { retain: true })
  client.subscribe(`${base}/macro/fire`)
  console.log(`[agent] macros publicadas: ${MACROS.map((m) => m.id).join(', ')}`)
})

client.on('message', async (topic, payload) => {
  if (topic !== `${base}/macro/fire`) return
  let id
  try { id = JSON.parse(payload.toString()).id } catch { return }

  console.log(`[agent] ▶ fire: ${id}`)
  const macro = MACROS.find((m) => m.id === id)
  let res
  if (!macro) res = { ok: false, msg: 'NO EXISTE' }
  else {
    try { res = await macro.run() }
    catch (e) { res = { ok: false, msg: 'ERR' }; console.error(e) }
  }
  client.publish(`${base}/macro/result`, JSON.stringify({ id, ...res }))
  console.log(`[agent]   ${res.ok ? '✅' : '❌'} ${id} -> ${res.msg}`)
})

client.on('error', (e) => console.error('[agent] MQTT error:', e.message))
