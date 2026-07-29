"""Tests for commands.py: comments, blank lines, trimming, CRLF tolerance."""

from __future__ import annotations

import pytest

from commands import load_commands


def test_basic_commands(tmp_path):
    path = tmp_path / "cmds.txt"
    path.write_text("show version\nshow ip route\n")
    assert load_commands(str(path)) == ["show version", "show ip route"]


def test_comments_and_blank_lines_stripped(tmp_path):
    path = tmp_path / "cmds.txt"
    path.write_text(
        "# header comment\n"
        "show version\n"
        "\n"
        "   \n"
        "; semicolon comment\n"
        "show ip route\n"
    )
    assert load_commands(str(path)) == ["show version", "show ip route"]


def test_trailing_whitespace_trimmed(tmp_path):
    path = tmp_path / "cmds.txt"
    path.write_text("show version   \nshow ip route\t\n")
    assert load_commands(str(path)) == ["show version", "show ip route"]


def test_leading_whitespace_trimmed(tmp_path):
    path = tmp_path / "cmds.txt"
    path.write_text("    show version\n\tshow ip route\n")
    assert load_commands(str(path)) == ["show version", "show ip route"]


def test_crlf_tolerated(tmp_path):
    path = tmp_path / "cmds.txt"
    path.write_bytes(b"show version\r\nshow ip route\r\n")
    assert load_commands(str(path)) == ["show version", "show ip route"]


def test_cr_only_tolerated(tmp_path):
    path = tmp_path / "cmds.txt"
    path.write_bytes(b"show version\rshow ip route\r")
    assert load_commands(str(path)) == ["show version", "show ip route"]


def test_inline_hash_is_not_a_comment(tmp_path):
    # '#' only starts a comment at the start of a line.
    path = tmp_path / "cmds.txt"
    path.write_text("show version # this is part of the command\n")
    assert load_commands(str(path)) == ["show version # this is part of the command"]


def test_file_not_found_raises():
    with pytest.raises(FileNotFoundError):
        load_commands("/nonexistent/telnet_tool/commands.txt")


def test_empty_file_yields_empty_list(tmp_path):
    path = tmp_path / "cmds.txt"
    path.write_text("")
    assert load_commands(str(path)) == []


def test_unicode_line_separator_not_split(tmp_path):
    # NEL (\x85) and LS/PS must NOT act as line separators — only LF/CR/CRLF.
    # (Guards impl-commands #1: the old str.splitlines() over-split here into
    # ["show version", "detail", "show ip route"]; file-object iteration keeps
    # the NEL inside the command.)
    path = tmp_path / "cmds.txt"
    path.write_text("show version\x85detail\nshow ip route\n", encoding="utf-8")
    assert load_commands(str(path)) == ["show version\x85detail", "show ip route"]
