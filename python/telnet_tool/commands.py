"""Commands-file parser for telnet_tool.

The commands file is a newline-separated list of commands. Blank lines and
full-line comments (``#`` or ``;``) are ignored; each command is whitespace-
trimmed. CRLF, LF, and CR line endings are all tolerated via universal-newline
file iteration — mirroring the line-ending handling in ``config.py``.
"""

from __future__ import annotations


def load_commands(path: str) -> list[str]:
    """Read a commands file into a list of trimmed command strings.

    A line is a comment if its first non-whitespace character is ``#`` or ``;``;
    blank lines are skipped. Remaining lines are stripped of leading and
    trailing whitespace. Raises ``FileNotFoundError`` (``EX_NOINPUT``) if
    the file is missing.
    """
    commands: list[str] = []
    # Iterate the file object directly: universal-newlines mode translates only
    # \r\n / \r / \n to \n (not the Unicode separators str.splitlines() also
    # splits on, e.g. \x85 / \u2028, which would silently split mid-command).
    with open(path, "r", encoding="utf-8-sig") as fp:
        for raw in fp:
            stripped = raw.strip()
            if not stripped or stripped[0] in "#;":
                continue
            commands.append(stripped)
    return commands
