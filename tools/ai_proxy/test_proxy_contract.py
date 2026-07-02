import json
import os
import pathlib
import sys
import threading
import unittest
import urllib.error
import urllib.request

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

    def test_openai_mode_requires_api_key(self):
        old_key = os.environ.pop("OPENAI_API_KEY", None)
        try:
            with self.assertRaises(RuntimeError):
                server.create_server("127.0.0.1", 0, mode="openai")
        finally:
            if old_key is not None:
                os.environ["OPENAI_API_KEY"] = old_key


class OpenAiBuilderTests(unittest.TestCase):
    def test_openai_headers_use_environment_key(self):
        old_key = os.environ.get("OPENAI_API_KEY")
        os.environ["OPENAI_API_KEY"] = "test-key"
        try:
            self.assertEqual(server.openai_headers()["Authorization"], "Bearer test-key")
        finally:
            if old_key is None:
                os.environ.pop("OPENAI_API_KEY", None)
            else:
                os.environ["OPENAI_API_KEY"] = old_key

    def test_answer_prompt_is_short_screen_prompt(self):
        prompt = server.build_answer_prompt("上海今天冷吗")
        self.assertIn("200x200", prompt)
        self.assertIn("上海今天冷吗", prompt)
        self.assertLess(len(prompt), 260)


if __name__ == "__main__":
    unittest.main()
