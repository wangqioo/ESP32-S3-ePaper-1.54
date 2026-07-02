import json
import os
import pathlib
import ssl
import sys
import threading
import unittest
import urllib.error
import urllib.request
import wave
from io import BytesIO

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import server


class ProxyContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.httpd = server.create_server("127.0.0.1", 0, mode="mock")
        cls.port = cls.httpd.server_address[1]
        cls.thread = threading.Thread(target=cls.httpd.serve_forever, daemon=True)
        cls.thread.start()

    @classmethod
    def tearDownClass(cls):
        cls.httpd.shutdown()
        cls.thread.join(timeout=2)

    def post_wav(self, payload=b"RIFF....WAVEfmt "):
        req = urllib.request.Request(
            f"http://127.0.0.1:{self.port}/ask",
            data=payload,
            headers={"Content-Type": "audio/wav"},
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=3) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))

    def test_mock_ask_returns_compact_success_json(self):
        status, body = self.post_wav()
        self.assertEqual(status, 200)
        self.assertEqual(body["ok"], True)
        self.assertIsInstance(body["text"], str)
        self.assertLessEqual(len(body["text"]), 120)
        self.assertIn("request_id", body)

    def test_wrong_path_returns_json_error(self):
        req = urllib.request.Request(
            f"http://127.0.0.1:{self.port}/missing",
            data=b"RIFF....WAVEfmt ",
            headers={"Content-Type": "audio/wav"},
            method="POST",
        )
        with self.assertRaises(urllib.error.HTTPError) as raised:
            urllib.request.urlopen(req, timeout=3)
        self.assertEqual(raised.exception.code, 404)
        body = json.loads(raised.exception.read().decode("utf-8"))
        self.assertEqual(body["ok"], False)
        self.assertIn("error", body)

    def test_invalid_wav_returns_json_error(self):
        req = urllib.request.Request(
            f"http://127.0.0.1:{self.port}/ask",
            data=b"not a wav",
            headers={"Content-Type": "audio/wav"},
            method="POST",
        )
        with self.assertRaises(urllib.error.HTTPError) as raised:
            urllib.request.urlopen(req, timeout=3)
        self.assertEqual(raised.exception.code, 400)
        body = json.loads(raised.exception.read().decode("utf-8"))
        self.assertEqual(body["ok"], False)
        self.assertEqual(body["error"], "not_wav")

    def test_glm_mode_requires_api_key(self):
        old_key = os.environ.pop("GLM_API_KEY", None)
        try:
            with self.assertRaises(RuntimeError):
                server.create_server("127.0.0.1", 0, mode="glm")
        finally:
            if old_key is not None:
                os.environ["GLM_API_KEY"] = old_key


class GlmBuilderTests(unittest.TestCase):
    def test_glm_headers_use_environment_key_and_strip_prefix(self):
        old_key = os.environ.get("GLM_API_KEY")
        os.environ["GLM_API_KEY"] = "GLM:test-key"
        try:
            self.assertEqual(server.glm_headers()["Authorization"], "Bearer test-key")
        finally:
            if old_key is None:
                os.environ.pop("GLM_API_KEY", None)
            else:
                os.environ["GLM_API_KEY"] = old_key

    def test_glm_transcription_request_uses_voice_keyboard_defaults(self):
        boundary, body = server.build_glm_transcription_body(b"RIFF....WAVEfmt ")
        self.assertIn("multipart/form-data", f"multipart/form-data; boundary={boundary}")
        self.assertIn(b'form-data; name="model"', body)
        self.assertIn(b"glm-asr-2512", body)
        self.assertIn(b'filename="assistant.wav"', body)

    def test_glm_transcription_request_converts_stereo_wav_to_mono(self):
        src = BytesIO()
        with wave.open(src, "wb") as wav:
            wav.setnchannels(2)
            wav.setsampwidth(2)
            wav.setframerate(16000)
            wav.writeframes(b"\x00\x00\x64\x00\x00\x00\x9c\xff")

        mono = server.ensure_mono_wav(src.getvalue())

        with wave.open(BytesIO(mono), "rb") as wav:
            self.assertEqual(wav.getnchannels(), 1)
            self.assertEqual(wav.getsampwidth(), 2)
            self.assertEqual(wav.getframerate(), 16000)
            self.assertEqual(wav.readframes(2), b"\x32\x00\xce\xff")

    def test_glm_https_uses_explicit_ssl_context(self):
        ctx = server.build_ssl_context()
        self.assertIsInstance(ctx, ssl.SSLContext)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertTrue(ctx.check_hostname)

    def test_answer_prompt_is_short_screen_prompt(self):
        prompt = server.build_answer_prompt("上海今天冷吗")
        self.assertIn("200x200", prompt)
        self.assertIn("上海今天冷吗", prompt)
        self.assertLess(len(prompt), 260)

    def test_device_text_removes_emoji_and_variation_marks(self):
        self.assertEqual(server.device_text("可以呀 😊👍🏽\ufe0f"), "可以呀")

    def test_device_text_removes_symbol_style_emoji_sequences(self):
        self.assertEqual(server.device_text("好的 ™️ ©️ ®️ ↗️ ❤️ 〰️ ㊗️"), "好的")

    def test_device_text_uses_visible_fallback_when_answer_has_only_emoji(self):
        self.assertEqual(server.device_text("🎉✨"), "我没有得到可显示的回答。")


if __name__ == "__main__":
    unittest.main()
