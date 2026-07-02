#!/usr/bin/env python3
import argparse
import json
import os
import ssl
import wave
import time
import urllib.error
import urllib.request
import uuid
from io import BytesIO
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


MAX_WAV_BYTES = 2 * 1024 * 1024
DEVICE_TEXT_FALLBACK = "我没有得到可显示的回答。"


def json_bytes(body):
    return json.dumps(body, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


def is_device_unsupported_char(ch):
    code = ord(ch)
    return (
        0x1F000 <= code <= 0x1FAFF
        or code in (0x00A9, 0x00AE, 0x2122, 0x2139)
        or 0x2190 <= code <= 0x21FF
        or 0x2300 <= code <= 0x23FF
        or 0x2460 <= code <= 0x24FF
        or 0x2600 <= code <= 0x27BF
        or 0x2934 <= code <= 0x2935
        or 0x2B00 <= code <= 0x2BFF
        or 0x3030 <= code <= 0x303D
        or 0x3297 <= code <= 0x3299
        or 0xFE00 <= code <= 0xFE0F
        or 0xE0100 <= code <= 0xE01EF
        or 0x200D == code
        or 0x20E3 == code
    )


def device_text(text):
    kept = []
    for ch in text:
        code = ord(ch)
        if 0xFE00 <= code <= 0xFE0F:
            if kept and is_device_unsupported_char(kept[-1]):
                kept.pop()
            continue
        if code == 0x20E3:
            if kept:
                kept.pop()
            continue
        if is_device_unsupported_char(ch):
            continue
        kept.append(ch)
    cleaned = "".join(kept)
    cleaned = " ".join(cleaned.split())
    return cleaned.strip() or DEVICE_TEXT_FALLBACK


class AssistantProxyHandler(BaseHTTPRequestHandler):
    server_version = "AiVoiceProxy/0.1"

    def log_message(self, fmt, *args):
        print("%s - %s" % (self.address_string(), fmt % args))

    def send_json(self, status, body):
        payload = json_bytes(body)
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        if self.path == "/health":
            self.send_json(200, {"ok": True, "mode": self.server.mode})
            return
        self.send_json(404, {"ok": False, "error": "not_found"})

    def do_POST(self):
        request_id = uuid.uuid4().hex[:12]
        started = time.monotonic()
        if self.path != "/ask":
            self.send_json(404, {"ok": False, "error": "not_found", "request_id": request_id})
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.send_json(400, {"ok": False, "error": "bad_content_length", "request_id": request_id})
            return
        if length <= 0:
            self.send_json(400, {"ok": False, "error": "empty_audio", "request_id": request_id})
            return
        if length > MAX_WAV_BYTES:
            self.send_json(413, {"ok": False, "error": "audio_too_large", "request_id": request_id})
            return

        audio = self.rfile.read(length)
        if not audio.startswith(b"RIFF") or b"WAVE" not in audio[:16]:
            self.send_json(400, {"ok": False, "error": "not_wav", "request_id": request_id})
            return

        if self.server.mode == "mock":
            text = "Mock answer: upload works. GLM mode can be enabled next."
        else:
            text = ask_glm(audio, request_id)

        elapsed_ms = int((time.monotonic() - started) * 1000)
        print(f"request_id={request_id} mode={self.server.mode} bytes={length} elapsed_ms={elapsed_ms}")
        self.send_json(200, {"ok": True, "text": device_text(text)[:180], "request_id": request_id})


def glm_api_key():
    key = os.environ.get("GLM_API_KEY") or os.environ.get("ZHIPUAI_API_KEY", "")
    key = key.strip()
    if key.startswith("GLM:"):
        key = key[4:].strip()
    if not key:
        raise RuntimeError("GLM_API_KEY is required")
    return key


def glm_headers():
    key = glm_api_key()
    return {"Authorization": f"Bearer {key}"}


def build_answer_prompt(transcript):
    return (
        "You are a concise assistant for a 200x200 e-paper device. "
        "Answer in the same language as the user. Keep the answer under 80 Chinese characters "
        "or under 120 English characters. Do not use emoji, emoticons, stickers, or decorative symbols.\n\n"
        "User said:\n" + transcript.strip()
    )


def multipart_body(fields, files):
    boundary = "----ai-voice-" + uuid.uuid4().hex
    chunks = []
    for name, value in fields.items():
        chunks.append(f"--{boundary}\r\n".encode())
        chunks.append(f'Content-Disposition: form-data; name="{name}"\r\n\r\n'.encode())
        chunks.append(str(value).encode())
        chunks.append(b"\r\n")
    for name, filename, content_type, data in files:
        chunks.append(f"--{boundary}\r\n".encode())
        chunks.append(f'Content-Disposition: form-data; name="{name}"; filename="{filename}"\r\n'.encode())
        chunks.append(f"Content-Type: {content_type}\r\n\r\n".encode())
        chunks.append(data)
        chunks.append(b"\r\n")
    chunks.append(f"--{boundary}--\r\n".encode())
    return boundary, b"".join(chunks)


def ensure_mono_wav(audio):
    try:
        with wave.open(BytesIO(audio), "rb") as src:
            channels = src.getnchannels()
            sample_width = src.getsampwidth()
            frame_rate = src.getframerate()
            frames = src.readframes(src.getnframes())
    except wave.Error:
        return audio

    if channels == 1:
        return audio
    if channels < 1 or sample_width != 2:
        return audio

    mono = bytearray()
    frame_size = channels * sample_width
    for offset in range(0, len(frames) - frame_size + 1, frame_size):
        total = 0
        for channel in range(channels):
            start = offset + channel * sample_width
            total += int.from_bytes(frames[start:start + sample_width], "little", signed=True)
        sample = max(-32768, min(32767, round(total / channels)))
        mono.extend(int(sample).to_bytes(sample_width, "little", signed=True))

    out = BytesIO()
    with wave.open(out, "wb") as dst:
        dst.setnchannels(1)
        dst.setsampwidth(sample_width)
        dst.setframerate(frame_rate)
        dst.writeframes(bytes(mono))
    return out.getvalue()


def build_ssl_context():
    try:
        import certifi
    except ImportError as exc:
        raise RuntimeError("certifi is required for GLM HTTPS requests: pip install certifi") from exc
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ctx.verify_mode = ssl.CERT_REQUIRED
    ctx.check_hostname = True
    ctx.load_verify_locations(cafile=certifi.where())
    return ctx


def post_json(url, body):
    data = json.dumps(body).encode("utf-8")
    headers = {"Content-Type": "application/json", **glm_headers()}
    req = urllib.request.Request(url, data=data, headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=45, context=build_ssl_context()) as resp:
        return json.loads(resp.read().decode("utf-8"))


def build_glm_transcription_body(audio):
    audio = ensure_mono_wav(audio)
    boundary, body = multipart_body(
        {"model": os.environ.get("GLM_TRANSCRIBE_MODEL", "glm-asr-2512"), "stream": "false"},
        [("file", "assistant.wav", "audio/wav", audio)],
    )
    return boundary, body


def transcribe_wav(audio):
    boundary, body = build_glm_transcription_body(audio)
    headers = {"Content-Type": f"multipart/form-data; boundary={boundary}", **glm_headers()}
    req = urllib.request.Request("https://open.bigmodel.cn/api/paas/v4/audio/transcriptions", data=body, headers=headers, method="POST")
    with urllib.request.urlopen(req, timeout=60, context=build_ssl_context()) as resp:
        result = json.loads(resp.read().decode("utf-8"))
    text = result.get("text", "").strip()
    if not text:
        raise RuntimeError("empty transcription")
    return text


def answer_text(transcript):
    body = {
        "model": os.environ.get("GLM_TEXT_MODEL", os.environ.get("GLM_MODEL", "glm-4-flash")),
        "messages": [
            {"role": "system", "content": "You are a concise assistant for a 200x200 e-paper device. Answer in the same language as the user. Keep the answer under 80 Chinese characters or under 120 English characters. Do not use emoji, emoticons, stickers, or decorative symbols."},
            {"role": "user", "content": transcript.strip()},
        ],
        "temperature": 0.2,
        "max_tokens": 120,
    }
    result = post_json("https://open.bigmodel.cn/api/paas/v4/chat/completions", body)
    choices = result.get("choices") or []
    message = choices[0].get("message", {}) if choices else {}
    text = (message.get("content") or "").strip()
    if not text:
        raise RuntimeError("empty answer")
    return device_text(text)


def ask_glm(audio, request_id):
    try:
        transcript = transcribe_wav(audio)
        print(f"request_id={request_id} transcript_chars={len(transcript)}")
        return answer_text(transcript)
    except urllib.error.HTTPError as exc:
        detail = exc.read().decode("utf-8", errors="replace")[:300]
        raise RuntimeError(f"glm_http_{exc.code}:{detail}") from exc


def create_server(host, port, mode):
    if mode == "glm":
        glm_api_key()
    httpd = ThreadingHTTPServer((host, port), AssistantProxyHandler)
    httpd.mode = mode
    return httpd


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8787)
    parser.add_argument("--mode", choices=("mock", "glm"), default=os.environ.get("AI_PROXY_MODE", "mock"))
    args = parser.parse_args()
    httpd = create_server(args.host, args.port, args.mode)
    print(f"AI voice proxy listening on http://{args.host}:{args.port} mode={args.mode}")
    httpd.serve_forever()


if __name__ == "__main__":
    main()
