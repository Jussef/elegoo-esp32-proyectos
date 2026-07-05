import WifiBars from './WifiBars'
import { DEVICE_ID } from '../config'

export default function StatusBar({ link, status, telemetry, deviceOnline, onIdentify }) {
  const online = deviceOnline && link === 'online'
  return (
    <header className="statusbar">
      <div className="brand">
        <span className="glyph">☢</span>
        <div>
          <h1>RETRO-TERM</h1>
          <span className="subtitle">// CONTROL DECK · {DEVICE_ID.toUpperCase()}</span>
        </div>
      </div>

      <div className="clock">{telemetry?.clock || '--:--:--'}</div>

      <div className="status-right">
        <WifiBars bars={telemetry?.bars ?? 0} />
        <span className="rssi">{status?.rssi ?? telemetry?.rssi ?? '--'} dBm</span>
        <span className={`led ${online ? 'ok' : 'bad'}`}>
          <i /> {online ? 'ONLINE' : 'OFFLINE'}
        </span>
        <button className="btn ghost" onClick={onIdentify} disabled={!online} title="Parpadea el OLED">
          IDENTIFY
        </button>
      </div>
    </header>
  )
}
