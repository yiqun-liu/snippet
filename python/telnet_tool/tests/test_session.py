"""Tests for session.py: shell logic with fake reader/writer + one integration.

The fake async reader/writer isolates the send-and-wait loop from telnetlib3's
IAC negotiation so the control flow (pacing, timeout, strip, log tee) is tested
deterministically and fast. One integration test exercises the real telnetlib3
client against a raw socket server that echoes commands.
"""

from __future__ import annotations

import asyncio
import io
import os
import re
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

import pytest

from config import Config
from session import PromptMatcher, PromptTimeout, shell


# --------------------------------------------------------------------------- #
# Fakes
# --------------------------------------------------------------------------- #
class FakeReader:
    """Serves a pre-baked byte stream; ``readuntil``/``readuntil_pattern`` scan
    it like telnetlib3's binary reader. ``block=True`` makes reads never return
    so ``asyncio.wait_for`` times out."""

    def __init__(self, data: bytes = b"", *, block: bool = False):
        self._data = data
        self._pos = 0
        self._block = block

    async def readuntil(self, sep: bytes) -> bytes:
        if self._block:
            await asyncio.Event().wait()
        idx = self._data.find(sep, self._pos)
        if idx < 0:
            raise asyncio.IncompleteReadError(self._data[self._pos:], None)
        end = idx + len(sep)
        chunk = self._data[self._pos:end]
        self._pos = end
        return chunk

    async def readuntil_pattern(self, pattern: re.Pattern[bytes]) -> bytes:
        if self._block:
            await asyncio.Event().wait()
        m = pattern.search(self._data, self._pos)
        if not m:
            raise asyncio.IncompleteReadError(self._data[self._pos:], None)
        end = m.end()
        chunk = self._data[self._pos:end]
        self._pos = end
        return chunk


class FakeWriter:
    def __init__(self):
        self.sent = bytearray()
        self.write_calls = 0
        self.closed = False

    def write(self, data: bytes) -> None:
        self.sent += data
        self.write_calls += 1

    async def drain(self) -> None:
        pass

    def close(self) -> None:
        self.closed = True

    async def wait_closed(self) -> None:
        pass


def make_config(**kw) -> Config:
    base = dict(
        host="127.0.0.1", port=23, commands_file="/c.txt", prompt="Router#",
        prompt_regex=None, timeout=30.0, encoding="utf-8", newline="\n",
        baud_rate=0, frame_bits=10, command_delay=0.0, log_path=None,
        verbose=False,
    )
    base.update(kw)
    return Config(**base)


# --------------------------------------------------------------------------- #
# PromptMatcher
# --------------------------------------------------------------------------- #
def test_promptmatcher_literal_read_and_strip():
    matcher = PromptMatcher(literal="Router#", regex=None, encoding="utf-8")
    reader = FakeReader(b"Version 1.0\nRouter#")
    captured = asyncio.run(matcher.read_until(reader, timeout=1.0, log_fp=None))
    assert captured == b"Version 1.0\nRouter#"
    assert captured.endswith(b"Router#")
    assert matcher.strip_trailing(captured) == b"Version 1.0\n"


def test_promptmatcher_regex_read_and_strip():
    matcher = PromptMatcher(literal=None, regex=r"R[a-z]+#", encoding="utf-8")
    reader = FakeReader(b"out\nRouter#")
    captured = asyncio.run(matcher.read_until(reader, timeout=1.0, log_fp=None))
    assert captured == b"out\nRouter#"
    assert matcher.strip_trailing(captured) == b"out\n"


def test_promptmatcher_requires_a_prompt():
    with pytest.raises(ValueError):
        PromptMatcher(literal=None, regex=None, encoding="utf-8")


# --------------------------------------------------------------------------- #
# shell — happy path, log tee, strip
# --------------------------------------------------------------------------- #
def test_shell_happy_path_and_log_tees_both_directions():
    cfg = make_config()
    commands = ["show version", "show ip route"]
    # Two responses, each ending in the literal prompt.
    reader = FakeReader(b"Version 1.0\nRouter#routes here\nRouter#")
    writer = FakeWriter()
    out = io.BytesIO()
    log = io.BytesIO()
    matcher = PromptMatcher(literal="Router#", regex=None, encoding="utf-8")

    asyncio.run(shell(reader, writer, config=cfg, commands=commands,
                      matcher=matcher, log_fp=log, out=out))

    # stdout sections + stripped bodies
    expected_out = (
        b"\n=== show version ===\nVersion 1.0\n"
        b"\n=== show ip route ===\nroutes here\n"
    )
    assert out.getvalue() == expected_out

    # raw log: sent (prefixed '>>> ') and received (raw), interleaved
    expected_log = (
        b">>> show version\n"
        b"Version 1.0\nRouter#"
        b">>> show ip route\n"
        b"routes here\nRouter#"
    )
    assert log.getvalue() == expected_log


def test_shell_strip_removes_trailing_prompt_only():
    cfg = make_config(prompt="PROMPT>")
    reader = FakeReader(b"echoed cmd\nline1\nline2\nPROMPT>")
    writer = FakeWriter()
    out = io.BytesIO()
    matcher = PromptMatcher(literal="PROMPT>", regex=None, encoding="utf-8")
    asyncio.run(shell(reader, writer, config=cfg, commands=["cmd"],
                      matcher=matcher, log_fp=None, out=out))
    body = out.getvalue().split(b"=== cmd ===\n", 1)[1]
    assert body == b"echoed cmd\nline1\nline2\n"
    assert not body.endswith(b"PROMPT>")


# --------------------------------------------------------------------------- #
# shell — rate limiting
# --------------------------------------------------------------------------- #
def test_shell_baud_pacing_writes_byte_by_byte():
    cfg = make_config(baud_rate=9600, frame_bits=10)
    commands = ["show version", "show ip route"]
    reader = FakeReader(b"r1\nRouter#r2\nRouter#")
    writer = FakeWriter()
    matcher = PromptMatcher(literal="Router#", regex=None, encoding="utf-8")
    asyncio.run(shell(reader, writer, config=cfg, commands=commands,
                      matcher=matcher, log_fp=None, out=io.BytesIO()))
    # byte-by-byte: one write() per byte of each payload
    expected_calls = sum(len(c + cfg.newline) for c in commands)
    assert writer.write_calls == expected_calls
    # full payload still assembled correctly
    assert writer.sent == b"show version\nshow ip route\n"


def test_shell_no_baud_writes_payload_once_per_command():
    cfg = make_config()  # baud_rate=0
    commands = ["show version", "show ip route"]
    reader = FakeReader(b"r1\nRouter#r2\nRouter#")
    writer = FakeWriter()
    matcher = PromptMatcher(literal="Router#", regex=None, encoding="utf-8")
    asyncio.run(shell(reader, writer, config=cfg, commands=commands,
                      matcher=matcher, log_fp=None, out=io.BytesIO()))
    assert writer.write_calls == len(commands)


def test_shell_command_delay_honored():
    cfg = make_config(command_delay=0.05)
    commands = ["a", "b", "c"]
    reader = FakeReader(b"r1\nRouter#r2\nRouter#r3\nRouter#")
    matcher = PromptMatcher(literal="Router#", regex=None, encoding="utf-8")
    start = time.monotonic()
    asyncio.run(shell(reader, FakeWriter(), config=cfg, commands=commands,
                      matcher=matcher, log_fp=None, out=io.BytesIO()))
    elapsed = time.monotonic() - start
    # 2 inter-command gaps of >= 0.05s each (allow scheduler slack)
    assert elapsed >= 0.10 - 0.01


# --------------------------------------------------------------------------- #
# shell — timeout / fail-fast
# --------------------------------------------------------------------------- #
def test_shell_timeout_raises_prompttimeout_with_command():
    cfg = make_config(timeout=0.05)
    commands = ["show version", "show ip route"]
    reader = FakeReader(block=True)  # readuntil never returns
    log = io.BytesIO()
    matcher = PromptMatcher(literal="Router#", regex=None, encoding="utf-8")
    with pytest.raises(PromptTimeout) as exc_info:
        asyncio.run(shell(reader, FakeWriter(), config=cfg, commands=commands,
                          matcher=matcher, log_fp=log, out=io.BytesIO()))
    assert "show version" in str(exc_info.value)
    # sent bytes were tee'd before the timed-out read; nothing received
    assert log.getvalue() == b">>> show version\n"


def test_shell_regex_prompt_path():
    cfg = make_config(prompt=None, prompt_regex=r"[A-Za-z]+@\S+[#$>] ")
    commands = ["ls"]
    reader = FakeReader(b"file1 file2\nuser@host:~$ ")
    matcher = PromptMatcher(literal=None, regex=r"[A-Za-z]+@\S+[#$>] ", encoding="utf-8")
    out = io.BytesIO()
    asyncio.run(shell(reader, FakeWriter(), config=cfg, commands=commands,
                      matcher=matcher, log_fp=None, out=out))
    body = out.getvalue().split(b"=== ls ===\n", 1)[1]
    assert body == b"file1 file2\n"


# --------------------------------------------------------------------------- #
# Integration: real telnetlib3 client against a raw socket echo server
# --------------------------------------------------------------------------- #
def _start_echo_server():
    """A raw TCP server that (a) sends no initial prompt — matching the
    'caller pre-handles login' scenario — and (b) replies to each line with
    ``out: <line>\\nRouter#``. It ignores the telnetlib3 client's IAC bytes
    (telnetlib3 strips echoed IAC on the client side regardless)."""
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", 0))
    srv.listen(1)
    port = srv.getsockname()[1]
    stop = threading.Event()

    def serve():
        try:
            conn, _ = srv.accept()
            conn.settimeout(0.5)
            buf = b""
            while not stop.is_set():
                try:
                    data = conn.recv(4096)
                except socket.timeout:
                    continue
                if not data:
                    break
                buf += data
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    try:
                        conn.sendall(b"out: " + line + b"\nRouter#")
                    except OSError:
                        break
            conn.close()
        except OSError:
            pass
        finally:
            srv.close()

    t = threading.Thread(target=serve, daemon=True)
    t.start()
    return port, stop


def test_integration_cli_against_raw_server(tmp_path):
    port, stop = _start_echo_server()
    cmds = tmp_path / "cmds.txt"
    cmds.write_text("show version\nshow ip route\n")
    log = tmp_path / "session.log"
    tool = Path(__file__).resolve().parent.parent / "telnet_tool.py"
    try:
        result = subprocess.run(
            [sys.executable, str(tool),
             "--host", "127.0.0.1", "--port", str(port),
             "--prompt", "Router#", "--commands", str(cmds),
             "--log", str(log), "--timeout", "5"],
            capture_output=True, text=True, timeout=20,
        )
    finally:
        stop.set()
    assert result.returncode == 0, result.stderr
    assert "=== show version ===" in result.stdout
    assert "=== show ip route ===" in result.stdout
    log_bytes = log.read_bytes()
    assert b">>> show version" in log_bytes
    assert b"out:" in log_bytes
    assert b"Router#" in log_bytes
