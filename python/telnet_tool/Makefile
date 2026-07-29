# telnet_tool — portable telnet CLI (Python, uv-managed)
# Dependencies live in .venv via uv; tests run through `uv run pytest`.

.PHONY: all prepare test clean

all: prepare
	uv run python telnet_tool.py --help >/dev/null
	@echo "telnet_tool ready (uv sync done; --help OK)"

prepare:
	uv sync

test:
	@echo -e '\033[0;32m[test]:started\033[0m'
	uv run pytest
	@echo -e '\033[0;32m[test]:passed\033[0m'

clean:
	rm -rf .venv .pytest_cache
	find . -name '__pycache__' -type d -prune -exec rm -rf {} +
