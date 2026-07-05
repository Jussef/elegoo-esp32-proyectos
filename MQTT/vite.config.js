import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// Dashboard aislado de Astro: su propia app Vite.
export default defineConfig({
  plugins: [react()],
  server: { host: true, port: 5173 },
})
