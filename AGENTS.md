# AGENTS.md - Code Snippets Repository Guidelines

This is a personal playground for programming experiments with readable, production-ready code snippets across C, Python, shell, and assembly. Code here is meant to be copy-pasted into production projects.

## Build Commands

```bash
# Individual subdirectory
cd <subdirectory> && make

# Clean build artifacts
make clean
```

Each Makefile supports: `make`, `make prepare`, `make test`, `make clean`.

## Code Style

### C
- Tabs for indentation
- Macros: `UPPER_SNAKE_CASE`
- Structs: `snake_case_t` or `struct snake_case`
- Functions/variables: `snake_case`
- Member names: `snake_case`
- Compiler: `-Wall -Wextra -Wfloat-equal -fno-common`

### Shell
- `#!/usr/bin/bash`
- Functions: `snake_case`
- Quote variables: `"$var"`
- Write errors to stderr: `>&2`

### Python
- PEP 8 conventions
- Imports: stdlib → third-party → local
- Type hints when beneficial

## Project Structure
- `c/` - Basic C demos
- `linux-c/` - Linux-specific C (sockets, sysfs, pthread)
- `kernel-module/` - Kernel modules
- `shell/` - Shell script demos
- `python/` - Python demos
- `assembly/` - ARM64 examples
- `concurrency/` - Concurrency algorithms

## Important
- Some tests require root privileges (klog-demo, va-to-pa)
- Use `timeout` for operations that may hang
