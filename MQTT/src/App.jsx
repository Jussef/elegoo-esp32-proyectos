import { useMemo, useState } from 'react'
import { useMqtt } from './useMqtt'
import StatusBar from './components/StatusBar'
import StatsRow from './components/StatsRow'
import ScanFeed from './components/ScanFeed'
import CardDatabase from './components/CardDatabase'

export default function App() {
  const { link, status, telemetry, db, scans, deviceOnline, actions } = useMqtt()
  const [prefillUid, setPrefillUid] = useState(null)

  const online = deviceOnline && link === 'online'
  const knownUids = useMemo(() => new Set(db.cards.map((c) => c.uid)), [db])

  // "+ REGISTRAR" desde el feed -> precarga el UID en el formulario de la base.
  const registerFromFeed = (uid) => setPrefillUid(uid + '#' + Date.now())
  const cleanPrefill = prefillUid ? prefillUid.split('#')[0] : ''

  return (
    <div className="app">
      <div className="scanlines" aria-hidden />
      <StatusBar
        link={link}
        status={status}
        telemetry={telemetry}
        deviceOnline={deviceOnline}
        onIdentify={actions.identify}
      />
      <StatsRow link={link} status={status} telemetry={telemetry} db={db} />

      <main className="grid">
        <ScanFeed scans={scans} knownUids={knownUids} onRegister={registerFromFeed} />
        <CardDatabase db={db} actions={actions} prefillUid={cleanPrefill} online={online} />
      </main>

      <footer className="foot">
        RETRO-TERM v0.5 · MQTT link <b className={link === 'online' ? 'ok' : 'bad'}>{link}</b>
        {' · '}fw {status?.fw || '—'}
      </footer>
    </div>
  )
}
