#!/bin/bash
# IRC Server Test Script - simulates irssi connection
# Usage: ./test_irc.sh [port] [password]

PORT=${1:-6667}
PASS=${2:-admin}

echo "========================================="
echo "  IRC Server Test (simulating irssi)  "
echo "========================================="
echo "Server: localhost:$PORT"
echo "Password: $PASS"
echo ""

# Create a named pipe for bidirectional communication
PIPE=$(mktemp -u)
mkfifo $PIPE

# Function to cleanup on exit
cleanup() {
    rm -f $PIPE
    echo ""
    echo "Test session ended."
}
trap cleanup EXIT

# Start netcat in background, reading from pipe
nc localhost $PORT < $PIPE &
NC_PID=$!

# Give nc time to connect
sleep 0.5

# Send IRC commands through the pipe
exec 3>$PIPE

echo "Sending authentication..."
echo "PASS $PASS" >&3
sleep 0.3

echo "Sending CAP LS..."
echo "CAP LS" >&3
sleep 0.3

echo "Setting username..."
echo "USER testuser 0 * :Test User" >&3
sleep 0.3

echo "Setting nickname..."
echo "NICK testclient" >&3
sleep 0.3

echo "Sending CAP END..."
echo "CAP END" >&3
sleep 0.5

echo "Joining channel #General..."
echo "JOIN #General" >&3
sleep 0.3

echo ""
echo "========================================="
echo "  Testing Multiple PRIVMSG (the fix!)  "
echo "========================================="
echo ""

# Send 5 consecutive PRIVMSG commands
for i in {1..5}; do
    MSG="PRIVMSG #General :Test message number $i"
    echo "Sending: $MSG"
    echo "$MSG" >&3
    sleep 0.2
done

echo ""
echo "All messages sent successfully!"
echo "Press Ctrl+C to exit and check server logs"
echo ""

# Keep connection alive
wait $NC_PID
