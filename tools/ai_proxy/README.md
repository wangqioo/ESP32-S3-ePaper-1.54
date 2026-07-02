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

GLM mode uses `GLM_API_KEY` from the environment. Do not commit real keys,
Wi-Fi passwords, local IP addresses, or captured recordings.

```bash
GLM_API_KEY=... python3 tools/ai_proxy/server.py --host 0.0.0.0 --port 8787 --mode glm
```

Default models follow the Voice Keyboard project:

- `GLM_TRANSCRIBE_MODEL=glm-asr-2512`
- `GLM_TEXT_MODEL=glm-4-flash`

## Device-Safe Output

The proxy returns compact text intended for a 200 x 200 e-paper screen. Before
sending text back to the ESP32, it:

- strips emoji and emoji presentation sequences;
- strips decorative symbol variants that the 14 px device font cannot render;
- collapses whitespace;
- falls back to a short Chinese message when the answer contains no displayable
  text after filtering.

This filtering is done in the proxy so the firmware stays simple and API keys
remain off the device.

## Tests

From the repository root:

```bash
python3 -m unittest tools/ai_proxy/test_proxy_contract.py
```
