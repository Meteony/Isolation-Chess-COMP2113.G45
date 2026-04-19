#!/usr/bin/env python3
import asyncio
import signal
from dataclasses import dataclass, field
from typing import Optional


# Dead-simple line protocol:
#   Client -> server: JOIN <room> <tag>
#   Client -> server: MOVE <turn> <row> <col>
#   Client -> server: BREAK <turn> <row> <col>
#   Client -> server: CHAT <text>
#
#   Server -> client: WELCOME <1|2>
#   Server -> client: WAITING
#   Server -> client: START <p1_tag> <p2_tag>
#   Server -> client: MOVE <turn> <row> <col>
#   Server -> client: BREAK <turn> <row> <col>
#   Server -> client: CHAT <1|2> <text>
#   Server -> client: INFO <free text>
#   Server -> client: ERROR <free text>

import string

HOST = "0.0.0.0"
PORT = 5050

MAX_WIRE_LINE = 512
MAX_CHAT_LEN = 200

@dataclass
class Client:
    reader: asyncio.StreamReader
    writer: asyncio.StreamWriter
    room: Optional[str] = None
    side: Optional[int] = None
    tag: Optional[str] = None

    @property
    def peername(self) -> str:
        peer = self.writer.get_extra_info("peername")
        return str(peer)


@dataclass
class Room:
    code: str
    p1: Optional[Client] = None
    p2: Optional[Client] = None

    def other(self, client: Client) -> Optional[Client]:
        if self.p1 is client:
            return self.p2
        if self.p2 is client:
            return self.p1
        return None

    def is_empty(self) -> bool:
        return self.p1 is None and self.p2 is None


rooms: dict[str, Room] = {}


async def send_line(client: Client, line: str) -> None:
    client.writer.write((line + "\n").encode("utf-8"))
    await client.writer.drain()
    print(f"[send {client.peername}] {line}")


def get_or_create_room(code: str) -> Room:
    room = rooms.get(code)
    if room is None:
        room = Room(code=code)
        rooms[code] = room
    return room


async def handle_join(client: Client, code: str, tag: str) -> bool:
    room = get_or_create_room(code)

    if room.p1 is None:
        room.p1 = client
        client.room = code
        client.side = 1
        client.tag = tag
        await send_line(client, "WELCOME 1")
        await send_line(client, "WAITING")
        print(f"[room {code}] player 1 joined as {tag}")
        return True

    if room.p2 is None:
        room.p2 = client
        client.room = code
        client.side = 2
        client.tag = tag

        if room.p1 is None or room.p1.tag is None:
            await send_line(client, "ERROR room state invalid")
            return False

        await send_line(client, "WELCOME 2")
        start_line = f"START {room.p1.tag} {room.p2.tag}"
        await send_line(client, start_line)
        await send_line(room.p1, start_line)
        print(f"[room {code}] player 2 joined as {tag}; room ready")
        return True

    await send_line(client, "ERROR room is full")
    return False

def remove_client_from_room(client: Client) -> None:
    if client.room is None:
        return

    room = rooms.get(client.room)
    if room is None:
        return

    peer = room.other(client)

    if room.p1 is client:
        room.p1 = None
    if room.p2 is client:
        room.p2 = None

    print(f"[room {client.room}] player {client.side} left")

    if peer is not None:
        try:
            peer.writer.write(b"ERROR peer disconnected\n")
        except Exception:
            pass
            
    if room.is_empty():
        rooms.pop(room.code, None)
        print(f"[room {room.code}] removed empty room")

    client.room = None
    client.side = None


def valid_room(code: str) -> bool:
    if not code:
        return False
    if len(code) > 64:
        return False
    return all(ch.isalnum() or ch in "-_" for ch in code)

def valid_tag(tag: str) -> bool:
    if not tag:
        return False
    if len(tag) > 24:
        return False
    return all(ch.isalnum() or ch in "._-" for ch in tag)

def sanitize_chat_text(text: str) -> Optional[str]:
    text = text.strip()
    if not text:
        return None
    if len(text) > MAX_CHAT_LEN:
        return None

    out = []
    for ch in text:
        code = ord(ch)
        if code > 127:
            return None
        if ch.isalnum() or ch.isspace() or ch in string.punctuation:
            out.append(ch)
        else:
            return None

    return "".join(out)

async def relay(client: Client, line: str) -> None:
    if client.room is None:
        await send_line(client, "ERROR join first")
        return

    room = rooms.get(client.room)
    if room is None:
        await send_line(client, "ERROR room missing")
        return

    peer = room.other(client)
    if peer is None:
        await send_line(client, "ERROR peer not connected yet")
        return

    if line.startswith("CHAT "):
        text = sanitize_chat_text(line[5:])
        if text is None:
            await send_line(client, "ERROR malformed chat")
            return

        await send_line(peer, f"CHAT {client.side} {text}")
        return

    parts = line.strip().split()
    if len(parts) != 4 or parts[0] not in ("MOVE", "BREAK"):
        await send_line(client, "ERROR malformed action")
        return

    kind, turn, row, col = parts
    try:
        int(turn)
        int(row)
        int(col)
    except ValueError:
        await send_line(client, "ERROR bad integers")
        return

    await send_line(peer, line.strip())

async def handle_client(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
    client = Client(reader=reader, writer=writer)
    print(f"[conn] {client.peername} connected")

    try:
        first = await reader.readline()
        if not first:
            return

        line = first.decode("utf-8", errors="replace").strip()
        print(f"[recv {client.peername}] {line}")

        if len(first) > MAX_WIRE_LINE:
            await send_line(client, "ERROR line too long")
            return

        parts = line.split()
        if len(parts) != 3 or parts[0] != "JOIN":
            await send_line(client, "ERROR expected: JOIN <room> <tag>")
            return

        room = parts[1]
        tag = parts[2]

        if not valid_room(room):
            await send_line(client, "ERROR invalid room code")
            return

        if not valid_tag(tag):
            await send_line(client, "ERROR invalid tag")
            return

        ok = await handle_join(client, room, tag)

        if not ok:
            return

        while True:
            raw = await reader.readline()
            if not raw:
                break             
            if len(raw) > MAX_WIRE_LINE:
                await send_line(client, "ERROR line too long")
                break

            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue
            print(f"[recv {client.peername}] {line}")
            await relay(client, line)

    except asyncio.CancelledError:
        raise
    except Exception as exc:
        print(f"[conn] exception from {client.peername}: {exc}")
    finally:
        remove_client_from_room(client)
        writer.close()
        try:
            await writer.wait_closed()
        except Exception:
            pass
        print(f"[conn] {client.peername} closed")


async def main() -> None:
    server = await asyncio.start_server(handle_client, HOST, PORT)
    sockets = server.sockets or []
    for sock in sockets:
        print(f"[listen] {sock.getsockname()}")

    stop = asyncio.Event()

    def on_signal() -> None:
        print("[server] shutdown signal received")
        stop.set()

    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            loop.add_signal_handler(sig, on_signal)
        except NotImplementedError:
            pass

    async with server:
        await stop.wait()


if __name__ == "__main__":
    asyncio.run(main())
