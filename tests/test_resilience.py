import unittest
import time
import subprocess
from framework import RdtTestHarness
from rdt_proxy import RdtProxy


class TestResilience(unittest.TestCase):
    def run_resilience_test(self, name, size_kb, **proxy_kwargs):
        """Helper to run a transfer through the proxy with specific impairments."""
        proxy_port = 11000
        server_port = 11001

        proxy = RdtProxy(
            listen_port=proxy_port, target_port=server_port, **proxy_kwargs
        )
        proxy.start()

        try:
            with RdtTestHarness(port=server_port, timeout=30) as harness:
                harness.generate_random_data(size_kb)

                # Server listens on its own port
                harness.run_server()

                # Client connects to the PROXY port
                # We need to manually construct the client call to point at proxy_port
                extra_args = ["-a", "127.0.0.1", "-p", str(proxy_port)]
                # Since harness.run_client uses its own port, we override it here
                # actually let's just use harness.run_client with extra_args
                # but we need to make sure -p server_port is not there
                args = [
                    harness.binary,
                    "-c",
                    "-a",
                    "127.0.0.1",
                    "-p",
                    str(proxy_port),
                    "-i",
                    harness.input_file,
                    "-w",
                    str(harness.timeout),
                ]
                harness.client_proc = subprocess.Popen(
                    args, stderr=subprocess.PIPE, stdout=subprocess.PIPE
                )
                client_ret = harness.client_proc.wait()

                if client_ret != 0:
                    _, client_err = harness.client_proc.communicate()
                    _, server_err = harness.server_proc.communicate()
                    print(f"\n--- CLIENT STDERR ({name}) ---\n{client_err.decode()}")
                    print(f"--- SERVER STDERR ({name}) ---\n{server_err.decode()}")

                self.assertEqual(client_ret, 0, f"Client failed in {name}")
                harness.server_proc.wait()
                self.assertTrue(
                    harness.verify_integrity(), f"Integrity failed in {name}"
                )
        finally:
            proxy.stop()

    def test_packet_loss(self):
        """Tests transfer with 15% packet loss."""
        self.run_resilience_test("Loss Test", 50, loss_rate=0.15)

    def test_packet_duplication(self):
        """Tests transfer with 10% packet duplication."""
        self.run_resilience_test("Duplication Test", 100, dup_rate=0.10)

    def test_jitter_and_delay(self):
        """Tests transfer with delay and jitter."""
        self.run_resilience_test("Jitter Test", 50, delay_ms=20, jitter_ms=10)

    def test_packet_reordering(self):
        """Tests transfer with heavy reordering."""
        self.run_resilience_test("Reordering Test", 50, reorder_rate=0.20)

    def test_combined_impairments(self):
        """Tests transfer with loss, duplication and reordering combined."""
        self.run_resilience_test(
            "Combined Test", 50, loss_rate=0.05, dup_rate=0.05, reorder_rate=0.10
        )

    def test_corruption(self):
        """Tests transfer with 10% packet corruption."""
        self.run_resilience_test("Corruption Test", 50, corruption_rate=0.10)

    def test_timeout_termination(self):
        """Tests that the application terminates if no progress is made."""
        proxy_port = 11010
        server_port = 11011

        # Proxy with 100% loss - NO progress possible
        proxy = RdtProxy(listen_port=proxy_port, target_port=server_port, loss_rate=1.0)
        proxy.start()

        try:
            with RdtTestHarness(port=server_port, timeout=2) as harness:
                harness.generate_random_data(10)
                harness.run_server()

                # Client connects to blocked proxy
                args = [
                    harness.binary,
                    "-c",
                    "-a",
                    "127.0.0.1",
                    "-p",
                    str(proxy_port),
                    "-i",
                    harness.input_file,
                    "-w",
                    str(harness.timeout),
                ]
                harness.client_proc = subprocess.Popen(
                    args, stderr=subprocess.PIPE, stdout=subprocess.PIPE
                )

                start_time = time.time()
                client_ret = harness.client_proc.wait()
                end_time = time.time()

                # Check that it terminated with non-zero exit code
                self.assertNotEqual(
                    client_ret, 0, "Client should have failed due to timeout"
                )

                # Check that it took approximately 'timeout' seconds (2s in this case)
                elapsed = end_time - start_time
                self.assertTrue(
                    1.5 <= elapsed <= 5.0,
                    f"Termination took {elapsed}s, expected around 2s",
                )

                harness.server_proc.wait()
                self.assertNotEqual(
                    harness.server_proc.returncode,
                    0,
                    "Server should have failed due to timeout",
                )
        finally:
            proxy.stop()


if __name__ == "__main__":
    unittest.main()
