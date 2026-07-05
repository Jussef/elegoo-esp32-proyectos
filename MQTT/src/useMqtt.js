import { useEffect, useRef, useState, useCallback } from 'react'
import mqtt from 'mqtt'
import { MQTT, TOPICS, CMD, brokerUrl } from './config'

const MAX_SCANS = 60

// Hook central: mantiene la conexión al broker y todo el estado del dashboard.
export function useMqtt() {
  const [link, setLink] = useState('connecting') // connecting | online | offline
  const [status, setStatus] = useState(null)      // {online, ip, rssi, fw}
  const [telemetry, setTelemetry] = useState(null)
  const [db, setDb] = useState({ count: 0, max: 20, cards: [] })
  const [scans, setScans] = useState([])
  const clientRef = useRef(null)

  useEffect(() => {
    const client = mqtt.connect(brokerUrl(), {
      username: MQTT.user,
      password: MQTT.pass,
      reconnectPeriod: 3000,
      connectTimeout: 8000,
      clientId: 'retroterm-web-' + Math.random().toString(16).slice(2, 8),
    })
    clientRef.current = client

    client.on('connect', () => {
      setLink('online')
      client.subscribe(Object.values(TOPICS))
    })
    client.on('reconnect', () => setLink('connecting'))
    client.on('offline', () => setLink('offline'))
    client.on('error', () => setLink('offline'))

    client.on('message', (topic, payload) => {
      let data
      try {
        data = JSON.parse(payload.toString())
      } catch {
        return
      }
      if (topic === TOPICS.status) setStatus(data)
      else if (topic === TOPICS.telemetry) setTelemetry(data)
      else if (topic === TOPICS.db) setDb(data)
      else if (topic === TOPICS.scan) {
        setScans((prev) =>
          [{ ...data, id: Date.now() + Math.random(), at: Date.now() }, ...prev].slice(0, MAX_SCANS)
        )
      }
    })

    return () => client.end(true)
  }, [])

  const send = useCallback((topic, obj) => {
    const c = clientRef.current
    if (c && c.connected) c.publish(topic, JSON.stringify(obj || {}))
  }, [])

  const actions = {
    addCard: (uid, name) => send(CMD.add, name ? { uid, name } : { uid }),
    deleteCard: (uid) => send(CMD.del, { uid }),
    renameCard: (uid, name) => send(CMD.rename, { uid, name }),
    wipe: () => send(CMD.wipe, {}),
    identify: () => send(CMD.identify, {}),
  }

  // El dispositivo está "vivo" si el broker no lo marcó offline (LWT).
  const deviceOnline = !!status?.online

  return { link, status, telemetry, db, scans, deviceOnline, actions }
}
