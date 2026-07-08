import socket
import selectors
import struct

HOST = "0.0.0.0"
PORT = 5000
MAX_CLIENTS = 3

sel = selectors.DefaultSelector()
clients = {}
next_id = 1

# === MUST MATCH C++ PACKED STRUCT (37 bytes) ===
HEADER = struct.Struct("<B32sI")  # type, senderId[32], size
HEADER_SIZE = HEADER.size


class Client:
    def __init__(self, sock, cid):
        self.sock = sock
        self.id = cid

        self.header_buf = bytearray()
        self.payload_buf = bytearray()

        self.expected_size = None
        self.packet_type = None
        self.sender_id = None


def broadcast(sender: Client, packet: bytes):
    for c in clients.values():
        if c.id == sender.id:
            continue
        try:
            c.sock.sendall(HEADER.pack(
                sender.packet_type or 0,
                sender.sender_id or b"",
                len(packet)
            ))
            c.sock.sendall(packet)
        except OSError:
            pass

    print(f"Forwarded packet from {sender.id} size={len(packet)}")


def accept(server):
    global next_id

    sock, addr = server.accept()

    if len(clients) >= MAX_CLIENTS:
        print("Server full, rejecting client")
        sock.close()
        return

    sock.setblocking(False)

    client = Client(sock, next_id)
    clients[sock] = client
    sel.register(sock, selectors.EVENT_READ)

    print(f"Client {next_id} connected from {addr}")
    next_id += 1


def disconnect(client: Client):
    print(f"Client {client.id} disconnected")
    try:
        sel.unregister(client.sock)
    except Exception:
        pass

    clients.pop(client.sock, None)
    client.sock.close()


def read_exact(sock, buffer: bytearray, n: int):
    """Try to fill buffer up to n bytes"""
    try:
        chunk = sock.recv(n - len(buffer))

        if not chunk:
            return False

        buffer.extend(chunk)
        return True

    except BlockingIOError:
        return True

    except (ConnectionResetError, ConnectionAbortedError, BrokenPipeError, OSError):
        return False


def handle(client: Client):
    sock = client.sock

    if client.expected_size is None:
        ok = read_exact(sock, client.header_buf, HEADER_SIZE)
        if not ok:
            disconnect(client)
            return

        if len(client.header_buf) < HEADER_SIZE:
            return

        packet_type, sender_id, size = HEADER.unpack(client.header_buf)

        sender_id = sender_id.split(b"\x00", 1)[0]

        if size == 0 or size > 5 * 1024 * 1024:
            print("Invalid packet size:", size)
            client.header_buf.clear()
            return

        client.packet_type = packet_type
        client.sender_id = sender_id
        client.expected_size = size
        client.payload_buf.clear()

    try:
        chunk = sock.recv(client.expected_size - len(client.payload_buf))

        if not chunk:
            disconnect(client)
            return

        client.payload_buf.extend(chunk)

    except BlockingIOError:
        return

    if len(client.payload_buf) < client.expected_size:
        return

    broadcast(client, bytes(client.payload_buf))

    # reset
    client.header_buf.clear()
    client.payload_buf.clear()
    client.expected_size = None


server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((HOST, PORT))
server.listen(MAX_CLIENTS)
server.setblocking(False)

sel.register(server, selectors.EVENT_READ)
print(f"Relay server listening on {PORT}")


while True:
    events = sel.select(timeout=0)

    for key, _ in events:
        if key.fileobj is server:
            accept(server)
        else:
            handle(clients[key.fileobj])