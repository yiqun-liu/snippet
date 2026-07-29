# telnet_tool

A portable, single-dependency CLI that runs a fixed list of telnet commands
against any remote CLI emitting a recognizable prompt. The caller supplies the
prompt; the tool owns the connect / send / wait-until-prompt loop. Fail-fast on
the first missed prompt; full raw-byte audit log for post-mortem.

Pure-Python and cross-platform via [`telnetlib3`][telnetlib3] (Python's stdlib
`telnetlib` was removed in 3.13). See [`design.md`](design.md) for the full
design, alternatives considered, and references.

## Download

Fetch just this directory from the parent repo (Linux, GNU tar):

```bash
curl -sL https://codeload.github.com/yiqun-liu/snippet/tar.gz/refs/heads/main \
    | tar xzf - --strip-components=2 --wildcards '*/python/telnet_tool'
cd telnet_tool
make prepare        # = uv sync — installs telnetlib3 + pytest into .venv
```

Alternatives when `--wildcards` isn't available (e.g. macOS bsdtar):

```bash
# pure git (portable, fetches only the needed subdir)
git clone --depth 1 --filter=blob:none --sparse https://github.com/yiqun-liu/snippet.git tt \
    && cd tt && git sparse-checkout set python/telnet_tool \
    && mv python/telnet_tool ../telnet_tool && cd .. && rm -rf tt

# or, with Node:
npx degit yiqun-liu/snippet/python/telnet_tool telnet_tool
```

## Architecture

```
telnet_tool/
├── telnet_tool.py    # CLI entry: arg parsing, orchestration, exit codes
├── config.py         # Source-priority loader: CLI > ENV > File > Defaults
├── commands.py       # Commands-file parser (newline-separated, # / ; comments)
├── session.py        # PromptMatcher + async shell coroutine + run_session()
├── design.md         # Design doc
├── pyproject.toml    # uv project (telnetlib3; pytest as dev dep)
├── Makefile
├── README.md
└── tests/
    ├── test_config.py
    ├── test_commands.py
    └── test_session.py   # fake reader/writer + raw-socket integration
```

## Configuration Priority

```
CLI flag > ENV var > config file > built-in default
```

The leftmost source that provides a key wins — mirroring the in-repo
[`linux-c/config-demo/`](../../linux-c/config-demo/README.md) convention.

## Configuration Keys

| Key             | Type  | Default  | CLI flag           | ENV var                     |
|-----------------|-------|----------|--------------------|-----------------------------|
| `host`          | str   | —        | `--host`           | `TELNET_TOOL_HOST`          |
| `port`          | int   | `23`     | `--port`           | `TELNET_TOOL_PORT`          |
| `commands_file` | str   | —        | `--commands`       | `TELNET_TOOL_COMMANDS_FILE` |
| `prompt`        | str   | —        | `--prompt`         | `TELNET_TOOL_PROMPT`        |
| `prompt_regex`  | str   | —        | `--prompt-regex`   | `TELNET_TOOL_PROMPT_REGEX`  |
| `timeout`       | float | `30.0`   | `--timeout`        | `TELNET_TOOL_TIMEOUT`        |
| `encoding`      | str   | `utf-8`  | `--encoding`       | `TELNET_TOOL_ENCODING`      |
| `newline`       | str   | `\n`     | `--newline`        | `TELNET_TOOL_NEWLINE`       |
| `baud_rate`     | int   | `0`      | `--baud-rate`      | `TELNET_TOOL_BAUD_RATE`      |
| `frame_bits`    | int   | `10`     | `--frame-bits`     | `TELNET_TOOL_FRAME_BITS`     |
| `command_delay` | float | `0.0`    | `--command-delay`  | `TELNET_TOOL_COMMAND_DELAY` |
| `log_path`      | str   | —        | `--log`            | `TELNET_TOOL_LOG_PATH`      |
| `verbose`       | bool  | `false`  | `--verbose` / `-v` | `TELNET_TOOL_VERBOSE`       |

`prompt` (literal substring, matched by telnetlib3's `readuntil`) and
`prompt_regex` (regex, matched by `readuntil_pattern`) are mutually exclusive —
exactly one must be set. `baud_rate = 0` disables per-byte pacing; setting it
paces each byte at the device's line rate (`byte_delay = frame_bits /
baud_rate`; `frame_bits` defaults to 10 = the 8N1 serial frame).

## Usage

```bash
# Install dependencies into .venv
make prepare        # = uv sync

# Run with defaults from a config file
uv run python telnet_tool.py -c router.conf

# Override anything on the CLI (CLI > ENV > file > default)
uv run python telnet_tool.py -c router.conf --commands show-cmds.txt --log session.log

# Everything via CLI flags
uv run python telnet_tool.py --host 192.168.10.1 --port 23 \
    --prompt 'Router#' --commands cmds.txt --timeout 5 --log session.log

# Environment variables
TELNET_TOOL_HOST=192.168.10.1 TELNET_TOOL_PROMPT='Router#' \
    uv run python telnet_tool.py --commands cmds.txt
```

Each command's captured output is printed to stdout under a `=== <cmd> ===`
header; the trailing prompt is stripped (echo and `\r\n` are kept as-is). The
full raw byte stream — sent bytes prefixed `>>> `, received bytes verbatim —
is tee'd to `--log <path>`.

## Config File Format

`key = value` lines; `#` and `;` start full-line comments; blank lines are
ignored; values are whitespace-trimmed.

```ini
# /etc/telnet_tool/router.conf
host          = 192.168.10.1
port          = 23
prompt        = Router#
commands_file = /etc/telnet_tool/show-cmds.txt
baud_rate     = 9600          ; 9600-baud serial console: pace input
frame_bits    = 10            ; 8N1 frame (1 start + 8 data + 1 stop)
command_delay = 0.5           ; extra gap between commands
newline       = \r\n          ; strict telnet devices want CRLF
log_path      = /var/log/telnet_tool/router.log
```

## Commands File Format

A newline-separated list of commands. Blank lines and full-line comments
(`#` or `;`) are skipped; each command is whitespace-trimmed; CRLF/LF/CR line
endings are all tolerated.

```
# daily health checks
show version
show ip route
show processes memory
```

## Rate Limiting (Console Devices)

Some telnet targets are serial consoles with a fixed baud rate; sending faster
than the line can drain drops characters. Set `baud_rate` to the device's line
rate and the tool paces each byte at `byte_delay = frame_bits / baud_rate`
(≈1.04 ms/byte at 9600 baud). `command_delay` adds an orthogonal inter-command
gap for device processing time. Both default to 0 (no throttling).

`asyncio.sleep` granularity (~1 ms) means high-baud pacing over-throttles
(safe); low-baud pacing — the drop-prevention case — is achievable. See
`design.md` for the full rationale.

## Exit Codes

Failures propagate internally as Python exceptions; `main()` maps each to a
`sysexits.h` code and prints a one-line message to stderr. argparse usage
errors (unknown flag, missing value) are remapped from argparse's default 2 to
`EX_USAGE` by an `ArgumentParser` subclass.

| sysexits.h       | Code | Meaning                                    |
|------------------|------|--------------------------------------------|
| `EX_OK`          | 0    | All commands ran; every prompt seen        |
| `EX_USAGE`       | 64   | Bad CLI args / missing required config      |
| `EX_DATAERR`     | 65   | Config parse error / invalid value          |
| `EX_NOINPUT`     | 66   | Expected input file missing or unreadable   |
| `EX_UNAVAILABLE` | 69   | Remote unreachable / connection refused     |
| `EX_PROTOCOL`    | 76   | Prompt not seen within `timeout`           |
| `EX_SOFTWARE`    | 70   | Catch-all (unexpected exception)           |

`timeout` is also used as the TCP-connect timeout, so an unreachable host
fails within `timeout` seconds.

## Build / Test

```bash
make             # prepare (uv sync) + smoke test (--help)
make prepare     # uv sync
make test        # uv run pytest
make clean       # remove .venv, .pytest_cache, __pycache__
```

[telnetlib3]: https://telnetlib3.readthedocs.io/en/latest/
