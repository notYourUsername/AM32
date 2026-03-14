#!/usr/bin/env bash
# pre-push-check.sh — run the same checks as CI before pushing
# Usage: bash scripts/pre-push-check.sh
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

echo "==> [1/3] volatile qualifier audit"
python3 tests/check_volatile.py

echo "==> [2/3] cppcheck Src/"
cppcheck \
  --enable=warning,performance,portability \
  --error-exitcode=1 \
  --suppress=missingInclude \
  --suppress=missingIncludeSystem \
  --suppress=unusedFunction \
  --inline-suppr \
  -I Inc/ \
  -D SEQURE_4IN1_F051 \
  Src/main.c Src/dshot.c

echo "==> [2/3] cppcheck DroneCAN/"
cppcheck \
  --enable=warning,performance,portability \
  --error-exitcode=1 \
  --suppress=missingInclude \
  --suppress=missingIncludeSystem \
  --suppress=unusedFunction \
  --inline-suppr \
  -I Inc/ \
  -i Src/DroneCAN/dsdl_generated \
  -i Src/DroneCAN/libcanard \
  Src/DroneCAN/

echo "==> [3/3] unit tests"
make -C tests all run

echo ""
echo "all checks passed - safe to push"
