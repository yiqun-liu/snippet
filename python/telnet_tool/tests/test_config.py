"""Tests for config.py: source priority, parsing, coercion, validation."""

from __future__ import annotations

import pytest

from config import ConfigError, MissingConfigError, load_config, parse_config_file


def _write_config(tmp_path, text: str) -> str:
    path = tmp_path / "telnet.conf"
    path.write_text(text)
    return str(path)


def test_defaults_when_nothing_provided(tmp_path):
    cfg = load_config(
        overrides={"host": "10.0.0.1", "commands_file": "/c.txt", "prompt": "#"},
        config_path=None,
        env={},
    )
    assert cfg.port == 23
    assert cfg.timeout == 30.0
    assert cfg.encoding == "utf-8"
    assert cfg.newline == "\n"
    assert cfg.baud_rate == 0
    assert cfg.frame_bits == 10
    assert cfg.command_delay == 0.0
    assert cfg.verbose is False
    assert cfg.prompt_regex is None
    assert cfg.log_path is None


def test_file_values_override_defaults(tmp_path):
    path = _write_config(tmp_path, "host = 1.2.3.4\nport = 7777\nprompt = R#\ncommands_file = /c.txt\n")
    cfg = load_config(overrides={}, config_path=path, env={})
    assert cfg.host == "1.2.3.4"
    assert cfg.port == 7777
    assert cfg.prompt == "R#"


def test_env_overrides_file(tmp_path):
    path = _write_config(tmp_path, "port = 7777\nhost = 1.2.3.4\nprompt = #\ncommands_file = /c.txt\n")
    cfg = load_config(overrides={}, config_path=path, env={"TELNET_TOOL_PORT": "8888"})
    assert cfg.port == 8888


def test_cli_overrides_env_and_file(tmp_path):
    path = _write_config(tmp_path, "port = 7777\nhost = 1.2.3.4\nprompt = #\ncommands_file = /c.txt\n")
    env = {"TELNET_TOOL_PORT": "8888"}
    cfg = load_config(overrides={"port": "9999"}, config_path=path, env=env)
    assert cfg.port == 9999


def test_missing_host_raises_missing(tmp_path):
    with pytest.raises(MissingConfigError, match="host"):
        load_config(overrides={"commands_file": "/c.txt", "prompt": "#"}, config_path=None, env={})


def test_missing_commands_file_raises_missing(tmp_path):
    with pytest.raises(MissingConfigError, match="commands_file"):
        load_config(overrides={"host": "1.2.3.4", "prompt": "#"}, config_path=None, env={})


def test_missing_prompt_raises_missing():
    with pytest.raises(MissingConfigError, match="prompt"):
        load_config(overrides={"host": "1.2.3.4", "commands_file": "/c.txt"}, config_path=None, env={})


def test_prompt_and_prompt_regex_mutually_exclusive():
    with pytest.raises(ConfigError, match="mutually exclusive"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#", "prompt_regex": ".*#"},
            config_path=None,
            env={},
        )


def test_bad_integer_raises_config_error():
    with pytest.raises(ConfigError, match="bad integer"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#", "port": "not-a-number"},
            config_path=None,
            env={},
        )


def test_bad_float_raises_config_error():
    with pytest.raises(ConfigError, match="bad float"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#", "timeout": "oops"},
            config_path=None,
            env={},
        )


def test_bad_boolean_raises_config_error():
    with pytest.raises(ConfigError, match="bad boolean"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#", "verbose": "maybe"},
            config_path=None,
            env={},
        )


def test_verbose_bool_value_passed_through():
    # Simulates the CLI store_true passing a real bool.
    cfg = load_config(
        overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#", "verbose": True},
        config_path=None,
        env={},
    )
    assert cfg.verbose is True


def test_newline_escape_interpreted():
    cfg = load_config(
        overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#", "newline": r"\r\n"},
        config_path=None,
        env={},
    )
    assert cfg.newline == "\r\n"


def test_config_file_comments_and_blank_lines(tmp_path):
    # Full-line '#' and ';' comments and blank lines are ignored.
    path = _write_config(tmp_path, (
        "# a comment\n"
        "\n"
        "host = 10.0.0.1\n"
        "port = 5000\n"
        "; full-line comment\n"
        "prompt = $\n"
        "commands_file = /c.txt\n"
    ))
    cfg = load_config(overrides={}, config_path=path, env={})
    assert cfg.host == "10.0.0.1"
    assert cfg.port == 5000
    assert cfg.prompt == "$"


def test_unknown_config_key_raises(tmp_path):
    path = _write_config(tmp_path, "host = 1.2.3.4\nbogus = x\n")
    with pytest.raises(ConfigError, match="unknown config key"):
        load_config(overrides={}, config_path=path, env={})


def test_parse_config_file_no_equals_raises(tmp_path):
    path = _write_config(tmp_path, "host 1.2.3.4\n")
    with pytest.raises(ConfigError, match="expected 'key = value'"):
        load_config(overrides={}, config_path=path, env={})


def test_baud_rate_and_frame_bits_loaded():
    cfg = load_config(
        overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#",
                   "baud_rate": "9600", "frame_bits": "11"},
        config_path=None, env={},
    )
    assert cfg.baud_rate == 9600
    assert cfg.frame_bits == 11


# --- empty values are treated as unset (impl-config #2, #4) --------------------
def test_empty_value_in_config_file_falls_through_to_required_check(tmp_path):
    # `host =` (empty) is skipped, so host stays None -> MissingConfigError, not
    # a bogus Config(host="") that fails later at connect time.
    path = _write_config(tmp_path, "host =\nport = 7777\nprompt = #\ncommands_file = /c.txt\n")
    with pytest.raises(MissingConfigError, match="host"):
        load_config(overrides={}, config_path=path, env={})


def test_empty_env_value_treated_as_unset():
    with pytest.raises(MissingConfigError, match="host"):
        load_config(
            overrides={"commands_file": "/c.txt", "prompt": "#"},
            config_path=None,
            env={"TELNET_TOOL_HOST": ""},
        )


def test_empty_cli_override_treated_as_unset():
    # --host "" is skipped, so host falls through to the required-key check.
    with pytest.raises(MissingConfigError, match="host"):
        load_config(
            overrides={"host": "", "commands_file": "/c.txt", "prompt": "#"},
            config_path=None,
            env={},
        )


# --- BOM handling (impl-config #3) -------------------------------------------
def test_bom_stripped_from_config_file(tmp_path):
    path = tmp_path / "bom.conf"
    path.write_text(
        "\ufeffhost = 1.2.3.4\nport = 7777\nprompt = #\ncommands_file = /c.txt\n",
        encoding="utf-8",
    )
    cfg = load_config(overrides={}, config_path=str(path), env={})
    assert cfg.host == "1.2.3.4"
    assert cfg.port == 7777


# --- malformed newline escape is a ConfigError, not a raw UnicodeDecodeError --
def test_bad_escape_in_newline_raises_config_error():
    with pytest.raises(ConfigError, match="bad escape"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#",
                       "newline": r"\xZZ"},
            config_path=None, env={},
        )


# --- range validation (impl-session #3) --------------------------------------
@pytest.mark.parametrize("key,bad", [
    ("baud_rate", "-9600"),
    ("frame_bits", "0"),
    ("frame_bits", "-1"),
    ("command_delay", "-0.5"),
    ("timeout", "0"),
    ("timeout", "-5"),
])
def test_non_positive_numeric_knobs_rejected(key, bad):
    with pytest.raises(ConfigError):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#",
                       key: bad},
            config_path=None, env={},
        )


# --- encoding / regex validation at load time (impl-session #2) --------------
def test_invalid_encoding_rejected():
    with pytest.raises(ConfigError, match="encoding"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#",
                       "encoding": "utf-8-bogus"},
            config_path=None, env={},
        )


def test_invalid_prompt_regex_rejected():
    with pytest.raises(ConfigError, match="prompt_regex"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt_regex": "["},
            config_path=None, env={},
        )


# --- malformed hostname rejected at load time (IDNA-incompatible) ------------
@pytest.mark.parametrize("bad_host", ["foo..bar", "a" * 300])
def test_malformed_hostname_rejected(bad_host):
    with pytest.raises(ConfigError, match="host"):
        load_config(
            overrides={"host": bad_host, "commands_file": "/c.txt", "prompt": "#"},
            config_path=None, env={},
        )


# --- unknown CLI override key is rejected (impl-config #5) -------------------
def test_unknown_override_key_rejected():
    with pytest.raises(ConfigError, match="unknown config key"):
        load_config(
            overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#",
                       "bogus": "x"},
            config_path=None, env={},
        )


# --- bool vocabulary (struct-config #5) -------------------------------------
@pytest.mark.parametrize("raw,expected", [
    ("true", True), ("TRUE", True), ("Yes", True), ("1", True), ("on", True),
    ("false", False), ("FALSE", False), ("no", False), ("0", False), ("OFF", False),
])
def test_bool_vocabulary(raw, expected):
    cfg = load_config(
        overrides={"host": "1.2.3.4", "commands_file": "/c.txt", "prompt": "#",
                   "verbose": raw},
        config_path=None, env={},
    )
    assert cfg.verbose is expected
