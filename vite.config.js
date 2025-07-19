import { defineConfig } from 'vite'

export default defineConfig({
  root: '.',
  server: {
    port: 3000,
    headers: {
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Opener-Policy': 'same-origin',
    },
    fs: {
      allow: ['..']
    }
  },
  optimizeDeps: {
    exclude: ['jwwlib']
  },
  assetsInclude: ['**/*.wasm']
})