#!/bin/bash

# Configuration
PORT=12005
INPUT_FILE="test_input_valgrind.bin"
OUTPUT_FILE="test_output_valgrind.bin"
FILE_SIZE_KB=50

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

function cleanup {
    rm -f $INPUT_FILE $OUTPUT_FILE valgrind_server.log valgrind_client.log
}

trap cleanup EXIT
cleanup

echo "Creating test file..."
dd if=/dev/urandom of=$INPUT_FILE bs=1024 count=$FILE_SIZE_KB status=none

echo "Starting Server under Valgrind..."
valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=100 \
    --log-file=valgrind_server.log \
    ./ipk-rdt -s -p $PORT -o $OUTPUT_FILE -w 5 &
SERVER_PID=$!

sleep 1

echo "Starting Client under Valgrind..."
valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=100 \
    --log-file=valgrind_client.log \
    ./ipk-rdt -c -a 127.0.0.1 -p $PORT -i $INPUT_FILE -w 5
CLIENT_RET=$?

# Wait for server
wait $SERVER_PID
SERVER_RET=$?

SUCCESS=true

# Check exit codes (100 means valgrind found leaks)
if [ $CLIENT_RET -eq 100 ]; then
    echo -e "${RED}FAIL: Memory leaks detected in Client!${NC}"
    cat valgrind_client.log
    SUCCESS=false
fi

if [ $SERVER_RET -eq 100 ]; then
    echo -e "${RED}FAIL: Memory leaks detected in Server!${NC}"
    cat valgrind_server.log
    SUCCESS=false
fi

if [ "$SUCCESS" = true ]; then
    echo -e "${GREEN}SUCCESS: No memory leaks detected by Valgrind.${NC}"
    exit 0
else
    exit 1
fi
