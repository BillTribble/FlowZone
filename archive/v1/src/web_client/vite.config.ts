/// <reference types="vitest" />
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vitejs.dev/config/
export default defineConfig({
    base: './',
    plugins: [react()],
    server: {
        port: 5173,
        strictPort: true, // Fail if port is already in use
        host: 'localhost'
    },
    test: {
        environment: 'jsdom',
        globals: true,
        setupFiles: []
    }
})
