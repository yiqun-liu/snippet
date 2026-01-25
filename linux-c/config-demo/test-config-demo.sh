#!/usr/bin/env bash

set -e

cd "$(dirname "$0")"

BUILD_DIR="build"
BINARY="$BUILD_DIR/config-demo"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

passed=0
failed=0

run_test() {
    local name="$1"
    local expected="$2"
    shift 2
    local result
    result=$("$@" 2>&1)

    if echo "$result" | grep -q "$expected"; then
        echo -e "${GREEN}[PASS]${NC} $name"
        passed=$((passed + 1))
    else
        echo -e "${RED}[FAIL]${NC} $name"
        echo "  Expected to contain: $expected"
        echo "  Output: $result"
        failed=$((failed + 1))
    fi
}

echo "=== Configuration Sources Demo Tests ==="
echo ""

make -s

echo "--- Default Values ---"
run_test "Default address" "address:.*localhost" "$BINARY"
run_test "Default port" "port:.*8080" "$BINARY"
run_test "Default log_level" "log_level:.*3.*INFO" "$BINARY"
run_test "Default debug" "debug:.*false" "$BINARY"

echo ""
echo "--- CLI Options ---"
run_test "CLI --address" "address:.*myserver" "$BINARY" --address myserver
run_test "CLI --port" "port:.*9999" "$BINARY" --port 9999
run_test "CLI --log-level" "log_level:.*1.*ERROR" "$BINARY" --log-level 1
run_test "CLI --debug" "debug:.*true" "$BINARY" --debug
run_test "CLI -s short form" "address:.*short" "$BINARY" -s short

echo ""
echo "--- Environment Variables ---"
CONFIG_DEMO_PORT=9000 \
    run_test "ENV CONFIG_DEMO_PORT" "port:.*9000" "$BINARY"

CONFIG_DEMO_DEBUG=true \
    run_test "ENV CONFIG_DEMO_DEBUG" "debug:.*true" "$BINARY"

CONFIG_DEMO_LOG_LEVEL=1 \
    run_test "ENV CONFIG_DEMO_LOG_LEVEL" "log_level:.*1.*ERROR" "$BINARY"

echo ""
echo "--- Configuration Files ---"
echo "port=7777" > /tmp/test-config-demo.conf
run_test "Config file" "port:.*7777" "$BINARY" -c /tmp/test-config-demo.conf

echo "debug=true" >> /tmp/test-config-demo.conf
run_test "Config file multi-line" "debug:.*true" "$BINARY" -c /tmp/test-config-demo.conf

echo "# comment" > /tmp/test-comments.conf
echo "port=5555" >> /tmp/test-comments.conf
echo "; another comment" >> /tmp/test-comments.conf
echo "log_level=2" >> /tmp/test-comments.conf
run_test "Config file with comments" "port:.*5555" "$BINARY" -c /tmp/test-comments.conf

echo ""
echo "--- Priority Tests ---"
CONFIG_DEMO_PORT=8888 \
    run_test "CLI overrides ENV" "port:.*9999" "$BINARY" --port 9999

CONFIG_DEMO_LOG_LEVEL=1 \
    run_test "ENV overrides file" "log_level:.*1.*ERROR" "$BINARY" -c /tmp/test-comments.conf

echo ""
echo "--- Help Output ---"
run_test "Help option" "Usage:" "$BINARY" --help

rm -f /tmp/test-config-demo.conf /tmp/test-comments.conf

echo ""
echo -e "=== Results: ${GREEN}$passed passed${NC}, ${RED}$failed failed${NC} ==="

if [ $failed -gt 0 ]; then
    exit 1
fi
