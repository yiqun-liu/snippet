#!/usr/bin/env python3
"""telnet_tool — portable telnet CLI: send commands, wait for prompt, capture.

Usage: telnet_tool -c <config> [--commands <path>] [--host <ip>] [--port <n>]
       [--prompt <str> | --prompt-regex <str>] [--log <path>] [other options]

See design.md for the full configuration-key table and exit-code map.
"""

from __future__ import annotations

import argparse
import socket
import sys

from config import ConfigError, DEFAULTS, MissingConfigError, load_config
from commands import load_commands
from session import PromptTimeout, run_session

__version__ = "0.1.0"

# sysexits.h subset — the exit-code authority for this tool.
EX_OK = 0
EX_USAGE = 64
EX_DATAERR = 65
EX_NOINPUT = 66
EX_UNAVAILABLE = 69
EX_SOFTWARE = 70
EX_PROTOCOL = 76


class _ArgumentParser(argparse.ArgumentParser):
    """argparse exits 2 on usage errors; remap 2 -> EX_USAGE (64) so the
    documented sysexits.h contract holds. --help/----version use status 0 and
    pass through unchanged."""

    def exit(self, status: int = 0, message: str | None = None) -> None:
        if status == 2:
            status = EX_USAGE
        super().exit(status, message)


def build_parser() -> argparse.ArgumentParser:
    parser = _ArgumentParser(
        prog="telnet_tool",
        description=(
            "Run a list of telnet commands against a remote CLI, waiting for "
            "a caller-supplied prompt after each command."
        ),
    )
    parser.add_argument(
        "-c", "--config", metavar="PATH",
        help="config file (key = value; # and ; comments)",
    )
    parser.add_argument("--host", metavar="IP", help="remote host")
    parser.add_argument("--port", metavar="N", help="telnet port (default 23)")
    parser.add_argument(
        "--commands", dest="commands_file", metavar="PATH",
        help="newline-separated commands file",
    )
    parser.add_argument(
        "--prompt", metavar="STR",
        help="literal prompt substring (e.g. 'Router#')",
    )
    parser.add_argument(
        "--prompt-regex", dest="prompt_regex", metavar="PAT",
        help="regex prompt (e.g. '[A-Za-z]+@\\S+[#$>] ')",
    )
    parser.add_argument("--timeout", metavar="SEC", help="per-command prompt timeout (default 30)")
    parser.add_argument("--encoding", metavar="ENC", help="text encoding (default utf-8)")
    parser.add_argument(
        "--newline", metavar="SEQ",
        help="newline sequence sent after each command (default \\n)",
    )
    parser.add_argument("--baud-rate", dest="baud_rate", metavar="N", help="pace input at this baud rate (default 0 = off)")
    parser.add_argument("--frame-bits", dest="frame_bits", metavar="N", help="serial frame bits for baud pacing (default 10 = 8N1)")
    parser.add_argument("--command-delay", dest="command_delay", metavar="SEC", help="inter-command delay in seconds (default 0)")
    parser.add_argument("--log", dest="log_path", metavar="PATH", help="raw byte-stream audit log")
    parser.add_argument(
        "-v", "--verbose", action="store_true", default=None,
        help="verbose tracing to stderr",
    )
    parser.add_argument("--version", action="version", version=f"telnet_tool {__version__}")
    return parser


def main(argv: list[str] | None = None) -> int:
    """Run the CLI lifecycle: parse args, load config + commands, run the
    telnet session. Returns one of the ``EX_*`` sysexits.h codes above;
    failures from ``load_config`` / ``load_commands`` / ``run_session`` are
    mapped to the matching code and reported on stderr."""
    parser = build_parser()
    args = parser.parse_args(argv)
    config_path = args.config

    overrides: dict[str, object] = {}
    for key in DEFAULTS:
        value = getattr(args, key, None)
        if value is not None:
            overrides[key] = value

    try:
        cfg = load_config(overrides, config_path)
        commands = load_commands(cfg.commands_file)
        run_session(cfg, commands)
    except MissingConfigError as e:
        print(f"telnet_tool: {e}", file=sys.stderr)
        return EX_USAGE
    except ConfigError as e:
        print(f"telnet_tool: config error: {e}", file=sys.stderr)
        return EX_DATAERR
    except (FileNotFoundError, PermissionError, IsADirectoryError) as e:
        print(f"telnet_tool: {e.strerror}: {e.filename}", file=sys.stderr)
        return EX_NOINPUT
    except PromptTimeout as e:
        print(f"telnet_tool: {e}", file=sys.stderr)
        return EX_PROTOCOL
    except (ConnectionError, socket.gaierror, socket.herror) as e:
        print(f"telnet_tool: connection error: {e}", file=sys.stderr)
        return EX_UNAVAILABLE
    except Exception as e:
        print(f"telnet_tool: unexpected: {type(e).__name__}: {e}", file=sys.stderr)
        return EX_SOFTWARE
    return EX_OK


if __name__ == "__main__":
    sys.exit(main())
