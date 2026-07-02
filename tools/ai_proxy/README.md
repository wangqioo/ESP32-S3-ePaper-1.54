# AI Voice Proxy

This local Mac proxy keeps API keys off the ESP32 firmware.

Run mock mode:

```bash
python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode mock
```

Health check:

```bash
curl http://127.0.0.1:8787/health
```

Ask endpoint:

```bash
curl -X POST http://127.0.0.1:8787/ask \
  -H 'Content-Type: audio/wav' \
  --data-binary @example.wav
```

OpenAI mode uses `OPENAI_API_KEY` from the environment. Do not commit real keys,
Wi-Fi passwords, local IP addresses, or captured recordings.
