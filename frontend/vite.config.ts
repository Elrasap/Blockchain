import { defineConfig } from "vite";
import react from "@vitejs/plugin-react-swc";

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      // proxy API calls to your node during development
      "/ping": "http://localhost:8080",
      "/dnd": "http://localhost:8080"
    }
  }
});

