import unittest
import subprocess
import os
import time
from framework import RdtTestHarness


class TestIoModes(unittest.TestCase):
    def test_stdin_to_file(self):
        """Tests reading from stdin and writing to a file."""
        with RdtTestHarness(port=10010) as harness:
            harness.generate_random_data(50)  # 50KB
            harness.run_server()

            # Client reads from stdin (default when -i is not provided or is "-")
            with open(harness.input_file, "rb") as f_in:
                args = [
                    harness.binary,
                    "-c",
                    "-a",
                    "127.0.0.1",
                    "-p",
                    "10010",
                    "-w",
                    "5",
                ]
                harness.client_proc = subprocess.Popen(
                    args, stdin=f_in, stderr=subprocess.PIPE, stdout=subprocess.PIPE
                )
                harness.client_proc.wait()

            harness.server_proc.wait()
            self.assertTrue(harness.verify_integrity())

    def test_file_to_stdout(self):
        """Tests reading from a file and writing to stdout."""
        with RdtTestHarness(port=10011) as harness:
            harness.generate_random_data(50)

            # Server writes to stdout
            args = [
                harness.binary,
                "-s",
                "-p",
                "10011",
                "-w",
                "5",
            ]
            harness.server_proc = subprocess.Popen(
                args, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
            time.sleep(0.2)

            harness.run_client()

            stdout_data, _ = harness.server_proc.communicate()
            with open(harness.input_file, "rb") as f_in:
                expected_data = f_in.read()

            self.assertEqual(stdout_data, expected_data)

    def test_stdin_to_stdout(self):
        """Tests reading from stdin and writing to stdout."""
        with RdtTestHarness(port=10014) as harness:
            harness.generate_random_data(50)

            # Server writes to stdout
            args_s = [
                harness.binary,
                "-s",
                "-p",
                "10014",
                "-w",
                "5",
            ]
            harness.server_proc = subprocess.Popen(
                args_s, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
            time.sleep(0.2)

            # Client reads from stdin
            with open(harness.input_file, "rb") as f_in:
                args_c = [
                    harness.binary,
                    "-c",
                    "-a",
                    "127.0.0.1",
                    "-p",
                    "10014",
                    "-w",
                    "5",
                ]
                harness.client_proc = subprocess.Popen(
                    args_c, stdin=f_in, stderr=subprocess.PIPE, stdout=subprocess.PIPE
                )
                harness.client_proc.wait()

            stdout_data, _ = harness.server_proc.communicate()
            with open(harness.input_file, "rb") as f_in:
                expected_data = f_in.read()

            self.assertEqual(stdout_data, expected_data)

    def test_baseline_file_transfer(self):
        """Tests transfer of a 50kB file."""
        with RdtTestHarness(port=10012) as harness:
            harness.generate_random_data(50)  # 1MB
            harness.run_server()
            client_ret = harness.run_client()

            self.assertEqual(client_ret, 0)
            harness.server_proc.wait()
            self.assertTrue(harness.verify_integrity())

    def test_large_file_transfer(self):
        """Tests transfer of a 1MB file."""
        with RdtTestHarness(port=10013) as harness:
            harness.generate_random_data(1024)  # 1MB
            harness.run_server()
            client_ret = harness.run_client()

            self.assertEqual(client_ret, 0)
            harness.server_proc.wait()
            self.assertTrue(harness.verify_integrity())

    def test_larger_file_transfer(self):
        """Tests transfer of a 50MB file."""
        with RdtTestHarness(port=10014, timeout=30) as harness:
            harness.generate_random_data(50 * 1024)  # 50MB
            harness.run_server()
            client_ret = harness.run_client()

            self.assertEqual(client_ret, 0)
            harness.server_proc.wait()
            self.assertTrue(harness.verify_integrity())


if __name__ == "__main__":
    unittest.main()
