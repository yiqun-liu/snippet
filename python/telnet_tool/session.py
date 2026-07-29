"""Telnet session: connect, send-and-wait-for-prompt loop, capture output.

The shell coroutine is the entire protocol — no threads, no pexpect, no PTY.
It talks to telnetlib3's binary (bytes) reader/writer so the configured
``encoding`` controls all encode/decode and the raw log is a true byte stream.

Timeout has no native parameter on ``reader.readuntil`` / ``readuntil_pattern``
so it is enforced with ``asyncio.wait_for``; a miss raises ``PromptTimeout``
which propagates out of the shell for the CLI to map to ``EX_PROTOCOL``.
"""

from __future__ import annotations

import asyncio
import contextlib
import re
import sys
from typing import BinaryIO

import telnetlib3

from config import Config


class PromptTimeout(Exception):
    """The prompt was not seen within the timeout (maps to ``EX_PROTOCOL``)."""


def _trace(config: Config, msg: str) -> None:
    """Emit a verbose trace line to stderr when ``config.verbose`` is set."""
    if config.verbose:
        print(f"telnet_tool: {msg}", file=sys.stderr)


class PromptMatcher:
    """Holds the resolved prompt and dispatches to the matching reader method.

    ``literal`` (``str``) becomes a bytes separator passed to
    ``reader.readuntil``; ``regex`` (``str``) becomes a compiled
    ``re.Pattern[bytes]`` passed to ``reader.readuntil_pattern``. Exactly one
    must be supplied.
    """

    def __init__(self, literal: str | None, regex: str | None, encoding: str):
        if literal is not None:
            self._sep: bytes | None = literal.encode(encoding)
            self._pattern: re.Pattern[bytes] | None = None
        elif regex is not None:
            self._sep = None
            self._pattern = re.compile(regex.encode(encoding))
        else:
            raise ValueError("prompt or prompt_regex is required")

    async def read_until(
        self, reader, timeout: float, log_fp: BinaryIO | None
    ) -> bytes:
        """Read until the prompt; tee received bytes to ``log_fp``; raise on miss."""
        try:
            if self._sep is not None:
                captured = await asyncio.wait_for(
                    reader.readuntil(self._sep), timeout
                )
            else:
                captured = await asyncio.wait_for(
                    reader.readuntil_pattern(self._pattern), timeout
                )
        except asyncio.TimeoutError as e:
            raise PromptTimeout(
                f"prompt not seen within {timeout}s"
            ) from e
        except asyncio.IncompleteReadError as e:
            raise PromptTimeout(
                f"connection closed before prompt seen ({len(e.partial)} bytes pending)"
            ) from e
        if log_fp is not None:
            log_fp.write(captured)
            log_fp.flush()
        return captured

    def strip_trailing(self, captured: bytes) -> bytes:
        """Remove the matched trailing prompt from ``captured``."""
        if self._sep is not None:
            if captured.endswith(self._sep):
                return captured[: -len(self._sep)]
            return captured
        # Regex branch: readuntil_pattern returns bytes ending with the match;
        # scan to the last match and verify it sits at the end.
        last = None
        for match in self._pattern.finditer(captured):
            last = match
        if last is not None and last.end() == len(captured):
            return captured[: last.start()]
        return captured


async def shell(
    reader,
    writer,
    *,
    config: Config,
    commands: list[str],
    matcher: PromptMatcher,
    log_fp: BinaryIO | None = None,
    out: BinaryIO | None = None,
) -> None:
    """Run the send-and-wait loop. Does not own the connection lifecycle.

    ``out`` is the binary stream for human-readable sections (defaults to
    ``sys.stdout.buffer``); ``log_fp`` is the optional raw-byte audit file.
    """
    if out is None:
        out = sys.stdout.buffer

    _trace(config, f"starting session: {len(commands)} command(s)")
    for cmd in commands:
        payload = (cmd + config.newline).encode(config.encoding)

        # Send: when baud_rate > 0, pace each byte at the device's line rate.
        # writer.drain() is backpressure (flow control), not rate limiting.
        if config.baud_rate:
            byte_delay = config.frame_bits / config.baud_rate
            _trace(config, f"baud-pacing {len(payload)} bytes at {config.baud_rate} baud")
            for ch in payload:
                writer.write(bytes([ch]))
                await writer.drain()
                await asyncio.sleep(byte_delay)
        else:
            writer.write(payload)
            await writer.drain()
        _trace(config, f"sent: {cmd!r}")
        if log_fp is not None:
            log_fp.write(b">>> " + payload)
            log_fp.flush()

        # Read until the prompt; tee received bytes to the log. A miss raises
        # PromptTimeout, annotated with the offending command for diagnostics.
        try:
            captured = await matcher.read_until(reader, config.timeout, log_fp)
        except PromptTimeout as e:
            raise PromptTimeout(f"{e}; after sending: {cmd!r}") from e
        _trace(config, f"prompt seen: {len(captured)} bytes")

        body = matcher.strip_trailing(captured)
        out.write(f"\n=== {cmd} ===\n".encode(config.encoding))
        out.write(body)
        out.flush()

        if config.command_delay:
            _trace(config, f"inter-command delay {config.command_delay}s")
            await asyncio.sleep(config.command_delay)


async def _connect_and_run(
    config: Config,
    commands: list[str],
    matcher: PromptMatcher,
    *,
    log_fp: BinaryIO | None,
    out: BinaryIO | None = None,
) -> None:
    """Open the telnet connection, run the shell, and own the lifecycle."""
    reader, writer = await telnetlib3.open_connection(
        config.host,
        config.port,
        encoding=False,
        connect_minwait=0,
        connect_maxwait=1.0,
        connect_timeout=config.timeout,
    )
    _trace(config, f"connected to {config.host}:{config.port}")
    try:
        await shell(
            reader,
            writer,
            config=config,
            commands=commands,
            matcher=matcher,
            log_fp=log_fp,
            out=out,
        )
    finally:
        writer.close()
        with contextlib.suppress(Exception):
            await writer.wait_closed()


def run_session(
    config: Config,
    commands: list[str],
    *,
    out: BinaryIO | None = None,
) -> None:
    """Synchronous entry: open the log, build the matcher, drive the session.

    Raises ``PromptTimeout`` (→ EX_PROTOCOL) on a missed prompt and lets
    connection errors (→ EX_UNAVAILABLE) propagate from ``open_connection``.
    """
    log_fp = open(config.log_path, "wb") if config.log_path else None
    try:
        matcher = PromptMatcher(config.prompt, config.prompt_regex, config.encoding)
        asyncio.run(_connect_and_run(config, commands, matcher, log_fp=log_fp, out=out))
    finally:
        if log_fp is not None:
            log_fp.close()
