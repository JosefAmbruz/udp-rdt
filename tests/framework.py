import subprocess
import os
import time
import hashlib
import random
import string
import shutil
from typing import Optional, List


class RdtTestHarness:
    """
    A robust test harness for udp-rdt that ensures clean process lifecycle
    and data integrity verification.
    """

    def __init__(self, port: int = 9999, timeout: int = 5, binary: Optional[str] = None):
        self.binary = binary or os.environ.get("RDT_BIN", "./udp-rdt")
        self.port = port
        self.timeout = timeout
        self.server_proc: Optional[subprocess.Popen] = None
        self.client_proc: Optional[subprocess.Popen] = None
        self.temp_dir = (
            f"temp_test_{''.join(random.choices(string.ascii_lowercase, k=5))}"
        )
        self.input_file = os.path.join(self.temp_dir, "input.bin")
        self.output_file = os.path.join(self.temp_dir, "output.bin")

    def __enter__(self):
        if not os.path.exists(self.temp_dir):
            os.makedirs(self.temp_dir)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.cleanup()

    def cleanup(self):
        """Forcefully terminate processes and remove temporary files."""
        for proc in [self.server_proc, self.client_proc]:
            if proc:
                if proc.poll() is None:
                    proc.terminate()
                    try:
                        proc.wait(timeout=2)
                    except subprocess.TimeoutExpired:
                        proc.kill()
                
                # Close the communication pipes to prevent ResourceWarnings
                if proc.stdout:
                    proc.stdout.close()
                if proc.stderr:
                    proc.stderr.close()

        if os.path.exists(self.temp_dir):
            shutil.rmtree(self.temp_dir)

    def generate_random_data(self, size_kb: int):
        """Generates random binary data for transfer."""
        with open(self.input_file, "wb") as f:
            f.write(os.urandom(size_kb * 1024))

    def calculate_sha256(self, file_path: str) -> str:
        """Calculates SHA256 hash of a file."""
        sha256_hash = hashlib.sha256()
        with open(file_path, "rb") as f:
            for byte_block in iter(lambda: f.read(4096), b""):
                sha256_hash.update(byte_block)
        return sha256_hash.hexdigest()

    def run_server(self, extra_args: List[str] = None):
        """Starts the server process."""
        args = [
            self.binary,
            "-s",
            "-p",
            str(self.port),
            "-o",
            self.output_file,
            "-w",
            str(self.timeout),
        ]
        if extra_args:
            args.extend(extra_args)

        self.server_proc = subprocess.Popen(
            args, stderr=subprocess.PIPE, stdout=subprocess.PIPE
        )
        time.sleep(0.2)  # Wait for bind

    def run_client(self, extra_args: List[str] = None):
        """Starts the client process and waits for completion."""
        args = [
            self.binary,
            "-c",
            "-a",
            "127.0.0.1",
            "-p",
            str(self.port),
            "-i",
            self.input_file,
            "-w",
            str(self.timeout),
        ]
        if extra_args:
            args.extend(extra_args)

        self.client_proc = subprocess.Popen(
            args, stderr=subprocess.PIPE, stdout=subprocess.PIPE
        )
        return self.client_proc.wait()

    def verify_integrity(self) -> bool:
        """Compares input and output file hashes."""
        if not os.path.exists(self.output_file):
            return False
        return self.calculate_sha256(self.input_file) == self.calculate_sha256(
            self.output_file
        )
