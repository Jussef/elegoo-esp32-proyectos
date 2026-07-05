import { useEffect, useState } from 'react'

export default function CardDatabase({ db, actions, prefillUid, online }) {
  const [uid, setUid] = useState('')
  const [name, setName] = useState('')
  const [editUid, setEditUid] = useState(null)
  const [editName, setEditName] = useState('')
  const [confirmWipe, setConfirmWipe] = useState(false)

  // Cuando llega un UID desde el SCAN LOG ("+ REGISTRAR").
  useEffect(() => {
    if (prefillUid) {
      setUid(prefillUid)
      setName('')
    }
  }, [prefillUid])

  const submitAdd = (e) => {
    e.preventDefault()
    const u = uid.trim().toUpperCase()
    if (!u) return
    actions.addCard(u, name.trim() || undefined)
    setUid('')
    setName('')
  }

  const saveRename = (u) => {
    if (editName.trim()) actions.renameCard(u, editName.trim())
    setEditUid(null)
  }

  return (
    <section className="panel db">
      <div className="panel-head">
        <h2>▤ CARD DATABASE</h2>
        <span className="muted">{db.count}/{db.max}</span>
      </div>

      <form className="add-form" onSubmit={submitAdd}>
        <input
          className="input mono"
          placeholder="UID  (AA:BB:CC:DD)"
          value={uid}
          onChange={(e) => setUid(e.target.value)}
        />
        <input
          className="input"
          placeholder="NOMBRE (opcional)"
          value={name}
          maxLength={11}
          onChange={(e) => setName(e.target.value)}
        />
        <button className="btn" type="submit" disabled={!online || !uid.trim()}>
          DAR DE ALTA
        </button>
      </form>

      <div className="db-body">
        {db.cards.length === 0 && <p className="empty">Sin tarjetas registradas.</p>}
        {db.cards.map((c, i) => (
          <div key={c.uid} className="card-row">
            <span className="idx">{String(i + 1).padStart(2, '0')}</span>
            {editUid === c.uid ? (
              <input
                className="input inline"
                autoFocus
                value={editName}
                maxLength={11}
                onChange={(e) => setEditName(e.target.value)}
                onKeyDown={(e) => e.key === 'Enter' && saveRename(c.uid)}
                onBlur={() => saveRename(c.uid)}
              />
            ) : (
              <button
                className="card-name"
                title="Renombrar"
                onClick={() => {
                  setEditUid(c.uid)
                  setEditName(c.name)
                }}
              >
                {c.name}
              </button>
            )}
            <code className="uid">{c.uid}</code>
            <button
              className="btn small danger"
              onClick={() => actions.deleteCard(c.uid)}
              disabled={!online}
              title="Borrar tarjeta"
            >
              ✕
            </button>
          </div>
        ))}
      </div>

      <div className="db-foot">
        {!confirmWipe ? (
          <button
            className="btn ghost danger"
            onClick={() => setConfirmWipe(true)}
            disabled={!online || db.count === 0}
          >
            BORRAR TODO
          </button>
        ) : (
          <span className="wipe-confirm">
            ¿Seguro?
            <button className="btn small danger" onClick={() => { actions.wipe(); setConfirmWipe(false) }}>
              SÍ, BORRAR
            </button>
            <button className="btn small" onClick={() => setConfirmWipe(false)}>CANCELAR</button>
          </span>
        )}
      </div>
    </section>
  )
}
