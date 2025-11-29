#!/usr/bin/env bash
set -euo pipefail

# Simple runner for the Python test suite
PYTHON=${PYTHON:-python3}
SCRIPT_DIR=$(dirname "$0")

echo "Running IRC server test suite..."
${PYTHON} -m unittest discover -s "${SCRIPT_DIR}" -p "test_all.py"
