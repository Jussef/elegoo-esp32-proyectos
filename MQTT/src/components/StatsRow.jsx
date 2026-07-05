function fmtUptime(s) {
  if (s == null) return '--'
  const d = Math.floor(s / 86400)
  const h = Math.floor((s % 86400) / 3600)
  const m = Math.floor((s % 3600) / 60)
  if (d) return `${d}d ${h}h`
  if (h) return `${h}h ${m}m`
  return `${m}m ${s % 60}s`
}

function Tile({ label, value, hint }) {
  return (
    <div className="tile">
      <span className="tile-label">{label}</span>
      <span className="tile-value">{value}</span>
      {hint && <span className="tile-hint">{hint}</span>}
    </div>
  )
}

export default function StatsRow({ link, status, telemetry, db }) {
  const linkText = { online: 'CONECTADO', connecting: 'ENLAZANDO…', offline: 'SIN BROKER' }[link]
  return (
    <section className="stats">
      <Tile label="BROKER" value={linkText} />
      <Tile label="IP DEVICE" value={status?.ip || telemetry?.ip || '---.---.---.---'} />
      <Tile label="PANTALLA" value={telemetry?.screen || '--'} hint="OLED actual" />
      <Tile label="UPTIME" value={fmtUptime(telemetry?.uptime)} />
      <Tile
        label="MEMORIA"
        value={telemetry?.heap != null ? `${(telemetry.heap / 1024).toFixed(0)} KB` : '--'}
        hint="heap libre"
      />
      <Tile label="TARJETAS" value={`${db.count}/${db.max}`} hint="en la base" />
    </section>
  )
}
