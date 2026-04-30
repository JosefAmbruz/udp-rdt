import subprocess
import os
import time
import hashlib

def calculate_sha256(file_path):
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def test_basic_transfer():
    print("[TEST] Basic Loopback Transfer...")
    
    # Create a dummy input file
    input_file = "test_input.bin"
    output_file = "test_output.bin"
    data = os.urandom(1024 * 100)  # 100 KB
    with open(input_file, "wb") as f:
        f.write(data)
    
    if os.path.exists(output_file):
        os.remove(output_file)

    # Start server
    server_proc = subprocess.Popen(
        ["./ipk-rdt", "-s", "-p", "9999", "-o", output_file],
        stderr=subprocess.PIPE
    )
    
    time.sleep(0.5)  # Give server a moment to bind

    # Start client
    client_proc = subprocess.Popen(
        ["./ipk-rdt", "-c", "-a", "127.0.0.1", "-p", "9999", "-i", input_file],
        stderr=subprocess.PIPE
    )

    client_proc.wait()
    server_proc.wait()

    if client_proc.returncode != 0 or server_proc.returncode != 0:
        print(f"FAILED: Non-zero exit code (Client: {client_proc.returncode}, Server: {server_proc.returncode})")
        return False

    if not os.path.exists(output_file):
        print("FAILED: Output file not created")
        return False

    if calculate_sha256(input_file) == calculate_sha256(output_file):
        print("SUCCESS: Data integrity verified.")
        return True
    else:
        print("FAILED: Data corruption detected.")
        return False

if __name__ == "__main__":
    success = test_basic_transfer()
    exit(0 if success else 1)
