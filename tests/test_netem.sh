#!/bin/bash

# Configuration
INTERFACE="lo"
PORT=11005
INPUT_FILE="test_input_netem.bin"
OUTPUT_FILE="test_output_netem.bin"
FILE_SIZE_MB=5
RDT_BIN="${RDT_BIN:-./udp-rdt}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Check for permissions before attempting anything requiring sudo
if [ "$(id -u)" -ne 0 ] && ! sudo -n true 2>/dev/null; then
    echo -e "${RED}Skipping netem test: Root privileges or passwordless sudo required for tc netem.${NC}"
    exit 0
fi

SUDO_CMD=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO_CMD="sudo"
fi

function cleanup {
    echo "Cleaning up..."
    $SUDO_CMD tc qdisc del dev $INTERFACE root 2>/dev/null
    rm -f $INPUT_FILE $OUTPUT_FILE
}

# Ensure we start clean
cleanup

echo "Creating $FILE_SIZE_MB MB test file..."
dd if=/dev/urandom of=$INPUT_FILE bs=1M count=$FILE_SIZE_MB status=none

echo "Setting up tc netem impairments on $INTERFACE..."
# Simulate: 10% loss, 5% duplication, 20ms delay with 10ms jitter, and 10% reordering
$SUDO_CMD tc qdisc add dev $INTERFACE root netem \
    loss 10% \
    duplicate 5% \
    delay 20ms 10ms \
    reorder 10% 50% 2>/dev/null

if [ $? -ne 0 ]; then
    echo -e "${RED}Skipping netem test: sudo/tc netem setup failed (likely missing permissions).${NC}"
    exit 0
fi

echo -e "Starting Server (writing to $OUTPUT_FILE)..."
$RDT_BIN -s -p $PORT -o $OUTPUT_FILE -w 10 &
SERVER_PID=$!

sleep 1

echo -e "Starting Client (sending $INPUT_FILE)..."
START_TIME=$(date +%s%3N)
$RDT_BIN -c -a 127.0.0.1 -p $PORT -i $INPUT_FILE -w 10
CLIENT_RET=$?
END_TIME=$(date +%s%3N)

# Wait for server to finish
wait $SERVER_PID
SERVER_RET=$?

# Calculate elapsed time using awk (more portable than bc)
ELAPSED=$(awk "BEGIN {print ($END_TIME - $START_TIME) / 1000}")

echo "---------------------------------------"
echo "Transfer finished in ${ELAPSED}s"

# Verification
SUCCESS=true

if [ $CLIENT_RET -ne 0 ]; then
    echo -e "${RED}FAIL: Client exited with $CLIENT_RET${NC}"
    SUCCESS=false
fi

if [ $SERVER_RET -ne 0 ]; then
    echo -e "${RED}FAIL: Server exited with $SERVER_RET${NC}"
    SUCCESS=false
fi

if [ -f "$OUTPUT_FILE" ]; then
    DIFF=$(diff $INPUT_FILE $OUTPUT_FILE)
    if [ "$DIFF" == "" ]; then
        echo -e "${GREEN}SUCCESS: Files are identical.${NC}"
    else
        echo -e "${RED}FAIL: Files differ!${NC}"
        SUCCESS=false
    fi
else
    echo -e "${RED}FAIL: Output file not created.${NC}"
    SUCCESS=false
fi

cleanup

if [ "$SUCCESS" = true ]; then
    exit 0
else
    exit 1
fi
