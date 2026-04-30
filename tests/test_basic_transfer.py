import unittest
import subprocess
from framework import RdtTestHarness


class TestBasicTransfer(unittest.TestCase):
    def test_small_file_transfer(self):
        """Tests transfer of a 10KB random file over IPv4 loopback."""
        with RdtTestHarness(port=10001) as harness:
            harness.generate_random_data(10)
            harness.run_server()
            client_ret = harness.run_client()

            self.assertEqual(client_ret, 0, "Client should exit with 0")
            harness.server_proc.wait()
            self.assertEqual(
                harness.server_proc.returncode, 0, "Server should exit with 0"
            )
            self.assertTrue(harness.verify_integrity(), "Data integrity check failed")

    def test_empty_file_transfer(self):
        """Tests transfer of an empty file (Requirement in ASSIGNMENT.md)."""
        with RdtTestHarness(port=10002) as harness:
            harness.generate_random_data(0)
            harness.run_server()
            harness.run_client()

            harness.server_proc.wait()
            self.assertTrue(harness.verify_integrity())

    def test_ipv6_transfer(self):
        """Tests transfer over IPv6 loopback (Requirement in ASSIGNMENT.md)."""
        with RdtTestHarness(port=10003) as harness:
            harness.generate_random_data(5)
            # Override server to use IPv6
            harness.run_server(extra_args=["-a", "::1"])

            # Start client manually to target IPv6 address
            args = [
                "./ipk-rdt",
                "-c",
                "-a",
                "::1",
                "-p",
                "10003",
                "-i",
                harness.input_file,
                "-w",
                "5",
            ]
            harness.client_proc = subprocess.Popen(args, stderr=subprocess.PIPE)
            harness.client_proc.wait()

            harness.server_proc.wait()
            self.assertTrue(harness.verify_integrity())


if __name__ == "__main__":
    unittest.main()
