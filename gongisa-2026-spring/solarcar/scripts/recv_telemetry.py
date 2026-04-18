import socket
import datetime
import sys

UDP_IP = "0.0.0.0"
UDP_PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 5005

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"UDP Log Server on port {UDP_PORT}")
print("-" * 80)

try:
    while True:
        data, addr = sock.recvfrom(4096)
        text = data.decode("utf-8", errors="replace").strip()
        ts = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"[{ts}] {addr[0]}: {text}")

except KeyboardInterrupt:
    print("\nServer stopped.")
finally:
    sock.close()
