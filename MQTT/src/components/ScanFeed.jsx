function clock(ts) {
  return new Date(ts).toLocaleTimeString('es-MX', { hour12: false })
}

export default function ScanFeed({ scans, knownUids, onRegister }) {
  return (
    <section className="panel feed">
      <div className="panel-head">
        <h2>◉ SCAN LOG</h2>
        <span className="muted">{scans.length} eventos</span>
      </div>

      <div className="feed-body">
        {scans.length === 0 && <p className="empty">Esperando lecturas RFID…</p>}
        {scans.map((s) => (
          <div key={s.id} className={`scan ${s.granted ? 'granted' : 'denied'}`}>
            <span className="scan-verdict">{s.granted ? 'GRANTED' : 'DENIED'}</span>
            <div className="scan-info">
              <code className="uid">{s.uid}</code>
              <span className="scan-meta">
                {s.granted ? s.name || 'AGENT' : s.type || 'UNKNOWN'} · {clock(s.at)}
              </span>
            </div>
            {!s.granted && !knownUids.has(s.uid) && (
              <button className="btn small" onClick={() => onRegister(s.uid)}>
                + REGISTRAR
              </button>
            )}
          </div>
        ))}
      </div>
    </section>
  )
}
