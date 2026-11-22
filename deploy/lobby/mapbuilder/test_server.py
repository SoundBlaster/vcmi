import unittest
from types import SimpleNamespace
from unittest.mock import patch

import server


class RateLimitTests(unittest.TestCase):
    def setUp(self):
        server.history.clear()

    def test_limit_is_per_client_and_expires(self):
        with patch.object(server.time, "monotonic", return_value=1000):
            for _ in range(server.RATE_LIMIT):
                self.assertTrue(server.allowed("192.0.2.1"))
            self.assertFalse(server.allowed("192.0.2.1"))
            self.assertTrue(server.allowed("192.0.2.2"))
        with patch.object(server.time, "monotonic", return_value=1001 + server.RATE_WINDOW):
            self.assertTrue(server.allowed("192.0.2.1"))
            self.assertNotIn("192.0.2.2", server.history)

    def test_new_clients_are_rejected_at_capacity(self):
        with patch.object(server, "MAX_CLIENTS", 2), patch.object(server.time, "monotonic", return_value=1000):
            self.assertTrue(server.allowed("192.0.2.1"))
            self.assertTrue(server.allowed("192.0.2.2"))
            self.assertFalse(server.allowed("192.0.2.3"))
            self.assertTrue(server.allowed("192.0.2.1"))
            self.assertEqual(len(server.history), 2)

    def test_client_address_is_validated_and_normalized(self):
        request = SimpleNamespace(headers={"X-Real-IP": "2001:0db8::1"}, client_address=("127.0.0.1", 10))
        self.assertEqual(server.Handler.client(request), "2001:db8::1")
        request.headers["X-Real-IP"] = "not an IP address"
        self.assertEqual(server.Handler.client(request), "127.0.0.1")


if __name__ == "__main__":
    unittest.main()
