// 4 barritas de señal, iguales que las del OLED.
export default function WifiBars({ bars = 0 }) {
  return (
    <span className="wifi" title={`Señal: ${bars}/4`}>
      {[1, 2, 3, 4].map((n) => (
        <i key={n} className={n <= bars ? 'on' : ''} style={{ height: 4 + n * 3 }} />
      ))}
    </span>
  )
}
