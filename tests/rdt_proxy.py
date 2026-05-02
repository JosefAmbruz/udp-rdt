import socket
import threading
import random
import time
import select

class RdtProxy:
    """
    A simple UDP proxy to simulate network impairments.
    Listens on one port and forwards to another, applying loss, delay, etc.
    """
    def __init__(self, listen_port, target_port, loss_rate=0.0, dup_rate=0.0, 
                 delay_ms=0, jitter_ms=0, reorder_rate=0.0, corruption_rate=0.0):
        self.listen_port = listen_port
        self.target_port = target_port
        self.loss_rate = loss_rate
        self.dup_rate = dup_rate
        self.delay_ms = delay_ms
        self.jitter_ms = jitter_ms
        self.reorder_rate = reorder_rate
        self.corruption_rate = corruption_rate
        self.running = False
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.client_addr = None
        self.server_addr = ('127.0.0.1', target_port)

    def start(self):
        self.sock.bind(('127.0.0.1', self.listen_port))
        self.running = True
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False
        self.sock.close()
        self.thread.join(timeout=1)

    def _run(self):
        while self.running:
            try:
                # Use select for non-blocking read to allow stopping
                ready = select.select([self.sock], [], [], 0.5)
                if not ready[0]:
                    continue
                
                data, addr = self.sock.recvfrom(2048)
                
                # If it's from target port, it's a response to be sent to client
                if addr == self.server_addr:
                    if self.client_addr:
                        self._handle_packet(data, self.client_addr)
                else:
                    # It's from client, save client addr and forward to target
                    self.client_addr = addr
                    self._handle_packet(data, self.server_addr)
            except Exception:
                if self.running:
                    break

    def _handle_packet(self, data, target_addr):
        # Loss
        if random.random() < self.loss_rate:
            return

        # Corruption
        if random.random() < self.corruption_rate and len(data) > 20:
            data = bytearray(data)
            # Flip a bit in the payload (after 20 byte header)
            idx = random.randint(20, len(data) - 1)
            data[idx] ^= 0xFF
            data = bytes(data)

        # Duplication
        copies = 1
        if random.random() < self.dup_rate:
            copies = 2

        for _ in range(copies):
            # Delay and Jitter
            delay = self.delay_ms / 1000.0
            if self.jitter_ms > 0:
                delay += random.uniform(-self.jitter_ms, self.jitter_ms) / 1000.0
            
            # Reordering (simulated by randomizing delay slightly more)
            if random.random() < self.reorder_rate:
                delay += random.uniform(0.05, 0.2) # Add significant random delay

            if delay > 0:
                threading.Timer(delay, self._send, args=(data, target_addr)).start()
            else:
                self._send(data, target_addr)

    def _send(self, data, target_addr):
        try:
            # Create a temporary socket to send, or just use the main one 
            # if it's thread-safe (UDP is usually fine)
            self.sock.sendto(data, target_addr)
        except:
            pass
