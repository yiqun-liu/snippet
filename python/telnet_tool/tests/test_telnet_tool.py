"""Unit tests for telnet_tool.py: CLI dest<->key contract, exception->exit-code
mapping, and the argparse error -> EX_USAGE remap. These exercise the pure
seams (build_parser/main) without a network or subprocess."""

from __future__ import annotations

import socket

import pytest

import telnet_tool
from config import Config, ConfigError, DEFAULTS, MissingConfigError, load_config
from session import PromptTimeout
from telnet_tool import EX_DATAERR, EX_NOINPUT, EX_PROTOCOL, EX_SOFTWARE, EX_UNAVAILABLE, EX_USAGE, build_parser, main


_BASE = dict(
    host="h", port=23, commands_file="/c", prompt="#", prompt_regex=None,
    timeout=30.0, encoding="utf-8", newline="\n", baud_rate=0, frame_bits=10,
    command_delay=0.0, log_path=None, verbose=False,
)


def _ok_cfg() -> Config:
    return Config(**_BASE)


# --- dest<->DEFAULTS-key contract (struct-telnet_tool #1) -------------------
def test_build_parser_dest_matches_defaults():
    # Every DEFAULTS key must have a matching argparse dest, or the override
    # loop in main() would silently drop that key (getattr default None).
    args = build_parser().parse_args([])
    for key in DEFAULTS:
        assert hasattr(args, key), f"argparse parser missing dest for {key!r}"


def test_cli_overrides_flow_into_config():
    # A full CLI flag set (every key except the mutually-exclusive prompt_regex)
    # must reach load_config and populate the Config fields — a renamed dest
    # would make getattr(args, key) None, skip the override, and fail here.
    argv = [
        "--host", "h", "--port", "23", "--commands", "/c", "--prompt", "#",
        "--timeout", "5", "--encoding", "utf-8", "--newline", r"\n",
        "--baud-rate", "9600", "--frame-bits", "10", "--command-delay", "0",
        "--log", "/l", "-v",
    ]
    args = build_parser().parse_args(argv)
    overrides = {k: getattr(args, k) for k in DEFAULTS if getattr(args, k) is not None}
    cfg = load_config(overrides, None)

    assert cfg.host == "h"
    assert cfg.port == 23
    assert cfg.commands_file == "/c"
    assert cfg.prompt == "#"
    assert cfg.timeout == 5.0
    assert cfg.encoding == "utf-8"
    assert cfg.newline == "\n"
    assert cfg.baud_rate == 9600
    assert cfg.frame_bits == 10
    assert cfg.command_delay == 0.0
    assert cfg.log_path == "/l"
    assert cfg.verbose is True


# --- exception -> exit-code mapping (struct-telnet_tool #2, impl #2) ----------
def _raiser(exc):
    def _run(config, commands, *, out=None):
        raise exc
    return _run


@pytest.mark.parametrize("exc,expected", [
    (MissingConfigError("m"), EX_USAGE),
    (ConfigError("c"), EX_DATAERR),
    (FileNotFoundError(2, "No such file or directory", "/p"), EX_NOINPUT),
    (PermissionError(13, "Permission denied", "/p"), EX_NOINPUT),
    (IsADirectoryError(21, "Is a directory", "/p"), EX_NOINPUT),
    (PromptTimeout("t"), EX_PROTOCOL),
    (ConnectionError("cc"), EX_UNAVAILABLE),
    (socket.gaierror(-2, "Name or service not known"), EX_UNAVAILABLE),
    (socket.herror(-2, "host error"), EX_UNAVAILABLE),
    (RuntimeError("unexpected"), EX_SOFTWARE),
])
def test_main_maps_exceptions_to_exit_codes(exc, expected, monkeypatch):
    monkeypatch.setattr(telnet_tool, "load_config", lambda *a, **k: _ok_cfg())
    monkeypatch.setattr(telnet_tool, "load_commands", lambda *a, **k: [])
    monkeypatch.setattr(telnet_tool, "run_session", _raiser(exc))

    code = main(["--host", "h", "--prompt", "#", "--commands", "/c"])
    assert code == expected, f"{type(exc).__name__} -> {code}, want {expected}"


def test_main_returns_ex_ok_on_success(monkeypatch):
    monkeypatch.setattr(telnet_tool, "load_config", lambda *a, **k: _ok_cfg())
    monkeypatch.setattr(telnet_tool, "load_commands", lambda *a, **k: [])
    monkeypatch.setattr(telnet_tool, "run_session", lambda *a, **k: None)
    assert main(["--host", "h", "--prompt", "#", "--commands", "/c"]) == 0


# --- argparse error -> EX_USAGE (impl-telnet_tool #1) ------------------------
def test_argparse_error_exits_ex_usage():
    # An unknown flag makes argparse call error() -> exit(2), which the
    # _ArgumentParser subclass remaps to EX_USAGE (64).
    with pytest.raises(SystemExit) as exc_info:
        main(["--bogus-flag"])
    assert exc_info.value.code == EX_USAGE
