#!/usr/bin/env python3
import argparse
import json
import os
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


MAX_WAV_BYTES = 2 * 1024 * 1024


def json_bytes(body):
    return json.dumps(body, ensure_ascii=False, separators=(",", ":")).encode("utf-8")


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
            text = "Mock answer: upload works. OpenAI mode can be enabled next."
        else:
            text = ask_openai(audio, request_id)

        elapsed_ms = int((time.monotonic() - started) * 1000)
        print(f"request_id={request_id} mode={self.server.mode} bytes={length} elapsed_ms={elapsed_ms}")
        self.send_json(200, {"ok": True, "text": text[:180], "request_id": request_id})


def ask_openai(audio, request_id):
    raise RuntimeError("openai mode is not implemented yet")


def create_server(host, port, mode):
    if mode == "openai" and not os.environ.get("OPENAI_API_KEY"):
        raise RuntimeError("OPENAI_API_KEY is required for openai mode")
    httpd = ThreadingHTTPServer((host, port), AssistantProxyHandler)
    httpd.mode = mode
    return httpd


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8787)
    parser.add_argument("--mode", choices=("mock", "openai"), default=os.environ.get("AI_PROXY_MODE", "mock"))
    args = parser.parse_args()
    httpd = create_server(args.host, args.port, args.mode)
    print(f"AI voice proxy listening on http://{args.host}:{args.port} mode={args.mode}")
    httpd.serve_forever()


if __name__ == "__main__":
    main()
