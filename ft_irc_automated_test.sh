#!/bin/bash
# ft_irc_automated_test.sh
# Requires: nc, sleep, timeout

SERVER=./ircserv
PORT=6667
PASS=testpass

# Start server
$SERVER $PORT $PASS &
SERVER_PID=$!
sleep 1

function test_case() {
  echo -e "\n=== $1 ==="
  shift
  (echo -e "$@" | nc 127.0.0.1 $PORT -w 2) | tee /tmp/irc_test.log
  sleep 0.5
}

# 1. Registration (PASS/NICK/USER)
test_case "Registration OK" "PASS $PASS\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n"
grep -q "001 alice" /tmp/irc_test.log && echo "✅ Registration OK" || echo "❌ Registration FAIL"

# 2. Wrong PASS
test_case "Wrong PASS" "PASS wrong\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n"
grep -q "464" /tmp/irc_test.log && echo "✅ Wrong PASS rejected" || echo "❌ Wrong PASS accepted"

# 3. JOIN/PART
test_case "JOIN/PART" "PASS $PASS\r\nNICK charlie\r\nUSER charlie 0 * :Charlie\r\nJOIN #test\r\nPART #test\r\n"
grep -q "JOIN :#test" /tmp/irc_test.log && echo "✅ JOIN OK" || echo "❌ JOIN FAIL"

# 4. PRIVMSG to self
test_case "PRIVMSG self" "PASS $PASS\r\nNICK dave\r\nUSER dave 0 * :Dave\r\nPRIVMSG dave :hello\r\n"
grep -q "PRIVMSG dave :hello" /tmp/irc_test.log && echo "✅ PRIVMSG OK" || echo "❌ PRIVMSG FAIL"

# 5. Partial message
(echo -n "PASS $PASS\r\nNICK e" ; sleep 1 ; echo "ve\r\nUSER eve 0 * :Eve\r\n") | nc 127.0.0.1 $PORT -w 2 | tee /tmp/irc_test.log
grep -q "001 eve" /tmp/irc_test.log && echo "✅ Partial message OK" || echo "❌ Partial message FAIL"

# 6. Flood test
for i in {1..100}; do echo "PING"; done | nc 127.0.0.1 $PORT -w 2 | tee /tmp/irc_test.log
grep -q "PONG" /tmp/irc_test.log && echo "✅ Flood handled" || echo "❌ Flood FAIL"

kill $SERVER_PID
echo "All tests done."
