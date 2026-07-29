"""Configuration loader for telnet_tool.

Resolves settings from four sources in priority order
``CLI flag > ENV var > config file > built-in default`` — the leftmost source
that provides a key wins. This mirrors the in-repo ``linux-c/config-demo``
convention.

Config files use a simple ``key = value`` line format with ``#`` and ``;``
comments and blank lines ignored. See ``design.md`` for the full key table.
"""

from __future__ import annotations

import codecs
import os
import re
from dataclasses import dataclass
from typing import Mapping


class ConfigError(Exception):
    """A config value is invalid (maps to ``EX_DATAERR``)."""


class MissingConfigError(ConfigError):
    """A required config key was not provided by any source (``EX_USAGE``)."""


@dataclass(frozen=True)
class Config:
    host: str
    port: int
    commands_file: str
    prompt: str | None
    prompt_regex: str | None
    timeout: float
    encoding: str
    newline: str
    baud_rate: int
    frame_bits: int
    command_delay: float
    log_path: str | None
    verbose: bool


# Defaults applied when no source provides a key. Required keys are ``None``
# here so the validator can detect that nobody supplied them.
DEFAULTS: dict[str, object] = {
    "host": None,
    "port": 23,
    "commands_file": None,
    "prompt": None,
    "prompt_regex": None,
    "timeout": 30.0,
    "encoding": "utf-8",
    "newline": "\n",
    "baud_rate": 0,
    "frame_bits": 10,
    "command_delay": 0.0,
    "log_path": None,
    "verbose": False,
}

_ENV_PREFIX = "TELNET_TOOL_"

_BOOL_TRUE = {"true", "yes", "1", "on"}
_BOOL_FALSE = {"false", "no", "0", "off"}


def parse_config_file(path: str) -> dict[str, str]:
    """Parse a ``key = value`` config file into a string dict.

    Comments (``#`` or ``;``) and blank lines are ignored; keys and values are
    whitespace-trimmed. An empty value is treated as unset (skipped), so a
    ``None``-defaulted required key left empty falls through to lower-priority
    sources or the required-key check rather than becoming a bogus ``""``.
    """
    values: dict[str, str] = {}
    # utf-8-sig transparently strips a UTF-8 BOM (Windows Notepad et al.) and
    # reads BOM-free files identically to utf-8.
    with open(path, "r", encoding="utf-8-sig") as fp:
        for lineno, raw in enumerate(fp, 1):
            line = raw.strip()
            if not line or line[0] in "#;":
                continue
            if "=" not in line:
                raise ConfigError(f"{path}:{lineno}: expected 'key = value'")
            key, _, value = line.partition("=")
            key = key.strip()
            value = value.strip()
            if not key:
                raise ConfigError(f"{path}:{lineno}: empty key")
            if not value:
                continue
            values[key] = value
    return values


def _coerce(key: str, value: object) -> object:
    """Coerce a value to the type implied by the default for ``key``.

    Accepts already-typed input (e.g. a bool from a CLI flag) as-is; string
    input from config files / env vars is parsed. The ``newline`` key is the
    one ``str`` exception: common escape sequences (``\\n``/``\\r``/``\\t``)
    are interpreted via ``unicode_escape`` so a config can express a real
    newline; all other ``str`` keys are taken literally.
    """
    default = DEFAULTS[key]
    if isinstance(default, bool):
        if isinstance(value, bool):
            return value
        low = str(value).lower()
        if low in _BOOL_TRUE:
            return True
        if low in _BOOL_FALSE:
            return False
        raise ConfigError(f"{key}: bad boolean value {value!r} (true/false)")
    # bool is a subclass of int, so the bool branch above must run first.
    if isinstance(default, int):
        if isinstance(value, int) and not isinstance(value, bool):
            return value
        try:
            return int(str(value))
        except ValueError:
            raise ConfigError(f"{key}: bad integer value {value!r}")
    if isinstance(default, float):
        if isinstance(value, float):
            return value
        try:
            return float(str(value))
        except ValueError:
            raise ConfigError(f"{key}: bad float value {value!r}")
    if key == "newline":
        try:
            return codecs.decode(str(value), "unicode_escape")
        except ValueError as e:  # UnicodeDecodeError is a ValueError subclass
            raise ConfigError(f"{key}: bad escape sequence {value!r}") from e
    return str(value)


def _from_env(env: Mapping[str, str]) -> dict[str, str]:
    """Pull ``TELNET_TOOL_*`` env vars into a {config_key: value} dict.

    Empty env values are skipped (treated as unset), mirroring
    ``parse_config_file``.
    """
    out: dict[str, str] = {}
    for key in DEFAULTS:
        envvar = _ENV_PREFIX + key.upper()
        if envvar in env and env[envvar]:
            out[key] = env[envvar]
    return out


def _validate(resolved: dict[str, object]) -> None:
    """Check required keys, prompt mutual exclusion, ranges, and that the
    configured ``encoding`` and ``prompt_regex`` are usable.

    All config-layer failures surface here as ``MissingConfigError`` /
    ``ConfigError`` so the CLI can map them to ``EX_USAGE`` / ``EX_DATAERR``
    at load time — before any network or file I/O.
    """
    if resolved["host"] is None:
        raise MissingConfigError(
            "host is required (set --host, TELNET_TOOL_HOST, or 'host' in config)"
        )
    # Reject structurally-malformed hostnames at load time (e.g. empty labels
    # like 'foo..bar', or over-long labels) so they surface as EX_DATAERR
    # (invalid value) instead of a low-level UnicodeEncodeError from IDNA
    # encoding at connect time (which would map to EX_SOFTWARE).
    try:
        resolved["host"].encode("idna")
    except UnicodeError as e:
        raise ConfigError(f"host: not a valid hostname ({e})") from e
    if resolved["commands_file"] is None:
        raise MissingConfigError(
            "commands_file is required (set --commands, TELNET_TOOL_COMMANDS_FILE, "
            "or 'commands_file' in config)"
        )

    prompt = resolved["prompt"]
    prompt_regex = resolved["prompt_regex"]
    if prompt is not None and prompt_regex is not None:
        raise ConfigError("prompt and prompt_regex are mutually exclusive; set only one")
    if prompt is None and prompt_regex is None:
        raise MissingConfigError(
            "a prompt is required: set 'prompt' (literal) or 'prompt_regex' (regex)"
        )

    encoding = resolved["encoding"]
    try:
        "".encode(encoding)
    except LookupError as e:
        raise ConfigError(f"encoding: {encoding!r} is not a known codec") from e
    if prompt is not None:
        try:
            prompt.encode(encoding)
        except UnicodeEncodeError as e:
            raise ConfigError(f"prompt cannot be encoded in {encoding!r}") from e
    else:
        try:
            re.compile(prompt_regex.encode(encoding))
        except re.error as e:
            raise ConfigError(f"prompt_regex: invalid regex: {e}") from e
        except UnicodeEncodeError as e:
            raise ConfigError(f"prompt_regex cannot be encoded in {encoding!r}") from e

    timeout = resolved["timeout"]
    if not timeout > 0:
        raise ConfigError(f"timeout must be > 0, got {timeout!r}")
    if resolved["baud_rate"] < 0:
        raise ConfigError(f"baud_rate must be >= 0, got {resolved['baud_rate']!r}")
    if resolved["frame_bits"] <= 0:
        raise ConfigError(f"frame_bits must be > 0, got {resolved['frame_bits']!r}")
    if resolved["command_delay"] < 0:
        raise ConfigError(
            f"command_delay must be >= 0, got {resolved['command_delay']!r}"
        )


def load_config(
    overrides: Mapping[str, str | None],
    config_path: str | None,
    env: Mapping[str, str] | None = None,
) -> Config:
    """Build a frozen ``Config`` from all sources in priority order.

    ``overrides`` maps config-key names to CLI values (``None`` = the flag was
    not given; an empty string is treated as unset). ``config_path`` is the
    optional config file path from ``--config`` / ``-c``; a missing file
    raises ``FileNotFoundError`` (mapped to ``EX_NOINPUT`` upstream) — there is
    no silent skip.
    """
    if env is None:
        env = os.environ

    resolved: dict[str, object] = dict(DEFAULTS)

    if config_path is not None:
        file_values = parse_config_file(config_path)
        for key, raw in file_values.items():
            if key not in DEFAULTS:
                raise ConfigError(f"unknown config key {key!r} in {config_path}")
            resolved[key] = _coerce(key, raw)

    for key, raw in _from_env(env).items():
        resolved[key] = _coerce(key, raw)

    for key, raw in overrides.items():
        if raw is None or (isinstance(raw, str) and raw == ""):
            continue
        if key not in DEFAULTS:
            raise ConfigError(f"unknown config key {key!r} in overrides")
        resolved[key] = _coerce(key, raw)

    _validate(resolved)
    return Config(**resolved)
