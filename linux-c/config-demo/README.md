# Configuration Sources Demo

A modular demonstration of configuration loading from multiple sources following the priority order used by popular open-source projects (CLI > ENV > File > Defaults).

## Architecture

```
config-demo/
├── config.h              # Core types + inline helper (config_set_string)
├── config_source.h       # Source function declarations
├── cli_options.h/c       # CLI options array + print_usage
├── config-demo.c         # Main entry point + orchestration
├── cli_source.c          # CLI argument parsing (apply_cli_args)
├── env_source.c          # Environment variable parsing (apply_env_config)
├── file_source.c         # Config file parsing (apply_file_config)
├── defaults_source.c     # Built-in defaults (apply_defaults)
├── Makefile
└── test-config-demo.sh   # Test suite
```

## Configuration Priority

```
CLI Options > Environment Variables > Config Files > Built-in Defaults
```

## Supported Options

| Option | Type | CLI Flag | Environment | Config Key | Default |
|--------|------|----------|-------------|------------|---------|
| `address` | string | `-s`, `--address` | `CONFIG_DEMO_ADDRESS` | `address` | `localhost` |
| `port` | int | `-p`, `--port` | `CONFIG_DEMO_PORT` | `port` | `8080` |
| `log_level` | enum | `-l`, `--log-level` | `CONFIG_DEMO_LOG_LEVEL` | `log_level` | `3` (INFO) |
| `debug` | bool | `-d`, `--debug` | `CONFIG_DEMO_DEBUG` | `debug` | `false` |
| `config` | string | `-c`, `--config` | `CONFIG_DEMO_CONFIG` | N/A | auto-detect |

**Log levels**: 1=ERROR, 2=WARN, 3=INFO

## Usage

```bash
# Build
make

# Run tests
make test

# Run with defaults
./build/config-demo

# Environment variables
CONFIG_DEMO_PORT=9000 CONFIG_DEMO_LOG_LEVEL=1 ./build/config-demo

# CLI options
./build/config-demo --address 192.168.1.1 --port 9090 --debug

# Config file
echo "port=7000" > /tmp/test.conf
./build/config-demo -c /tmp/test.conf
```

## Config File Format

Config files use simple key=value format with whitespace trimming:

```ini
address = 127.0.0.1
port = 8080
log_level = 2
debug = true
```

Lines starting with `#` or `;` are treated as comments.

## Build Options

```bash
make           # Build the demo
make test      # Run test suite
make clean     # Clean build artifacts
```
