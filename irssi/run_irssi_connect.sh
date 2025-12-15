#!/usr/bin/env zsh
# Wrapper to start irssi with a specific config and force connect/rawlog commands
# Usage: ./run_irssi_connect.sh /full/path/to/irssi_config [nick]

CONFIG_PATH="$1"
NICK="${2:-nickme}"
HOME_DIR="${TMPDIR:-/tmp}/irssi_${USER}_${NICK}"
LOG_DIR="$(pwd)/client_logs"

if [ -z "$CONFIG_PATH" ]; then
  echo "Usage: $0 /full/path/to/irssi_config [nick]"
  exit 1
fi

mkdir -p "$HOME_DIR"
mkdir -p "$LOG_DIR"

cp -f "$CONFIG_PATH" "$HOME_DIR/config"

DESIRED_USER="${NICK}-user"
if grep -q "user_name" "$HOME_DIR/config" 2>/dev/null; then
  sed -i -E "s/(user_name\s*=\s*\")[^\"]*(\";)/\1${DESIRED_USER}\2/" "$HOME_DIR/config"
else
  sed -i "/core = {/a\
    user_name = \"${DESIRED_USER}\";" "$HOME_DIR/config"
fi
if grep -q "real_name" "$HOME_DIR/config" 2>/dev/null; then
  sed -i -E "s/(real_name\s*=\s*\")[^\"]*(\";)/\1${DESIRED_USER}\2/" "$HOME_DIR/config"
else
  sed -i "/core = {/a\
    real_name = \"${DESIRED_USER}\";" "$HOME_DIR/config"
fi

EXPECT_BIN="$(command -v expect 2>/dev/null)"
if [ -n "$EXPECT_BIN" ]; then
  EXPECT_SCRIPT="$HOME_DIR/run_irssi_expect.exp"
  cat > "$EXPECT_SCRIPT" <<EOF
#!/usr/bin/expect -f
set timeout -1
spawn irssi --home "$HOME_DIR" --config "$HOME_DIR/config"
sleep 1
send -- "/set user_name ${DESIRED_USER}\r"
send -- "/set real_name ${DESIRED_USER}\r"
sleep 0.5
send -- "/connect 127.0.0.1 6667 -password start\r"
sleep 1
send -- "/rawlog start $LOG_DIR/${NICK}.raw\r"
interact
EOF
  chmod +x "$EXPECT_SCRIPT"
  echo "Starting Irssi with home: $HOME_DIR using expect wrapper -> $EXPECT_SCRIPT"
  exec "$EXPECT_BIN" "$EXPECT_SCRIPT"
else
  echo "expect not found; attempting to start Irssi with CLI connect options"
  echo "Starting Irssi with home: $HOME_DIR"
  # Ensure config contains the desired user_name/real_name before launch
  if grep -q "user_name" "$HOME_DIR/config" 2>/dev/null; then
    sed -i -E "s/(user_name\s*=\s*\")[^\"]*(\";)/\1${DESIRED_USER}\2/" "$HOME_DIR/config"
  else
    sed -i "/core = {/a\
    user_name = \"${DESIRED_USER}\";" "$HOME_DIR/config"
  fi
  if grep -q "real_name" "$HOME_DIR/config" 2>/dev/null; then
    sed -i -E "s/(real_name\s*=\s*\")[^\"]*(\";)/\1${DESIRED_USER}\2/" "$HOME_DIR/config"
  else
    sed -i "/core = {/a\
    real_name = \"${DESIRED_USER}\";" "$HOME_DIR/config"
  fi

  exec irssi -c 127.0.0.1 -p 6667 -w start -n "$NICK" --home "$HOME_DIR" --config "$HOME_DIR/config"
fi
