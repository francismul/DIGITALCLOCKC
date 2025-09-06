#!/usr/bin/env bash
set -euo pipefail
echo "Running shell tests..."
# Execute all *.spec.sh tests
status=0
for f in $(find test -maxdepth 1 -type f -name '*.spec.sh' | sort); do
  echo "=> $f"
  bash "$f" || status=$?
done
exit $status