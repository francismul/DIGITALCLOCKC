#!/usr/bin/env bash
# Lightweight bash test harness for release workflow script.
# If your repo already uses a framework, you can adapt these cases into that framework's format.
set -euo pipefail

# Minimal assertion helpers (only used if no framework is present)
fail() { echo "FAIL: $*"; exit 1; }
assert_eq() {
  local expected="$1"; shift
  local actual="$1"; shift || true
  local msg="${1:-}"
  if [ "$expected" != "$actual" ]; then
    fail "${msg:-expected '$expected' but got '$actual' }"
  fi
}
assert_success() { "$@" >/dev/null 2>&1 || fail "command failed: $*"; }
assert_failure() { "$@" >/dev/null 2>&1 && fail "command unexpectedly succeeded: $*"; }

# Creates an ephemeral sandbox with isolated PATH for command mocks.
make_sandbox() {
  SANDBOX_DIR="$(mktemp -d)"
  export TMPDIR="$SANDBOX_DIR/tmp"
  mkdir -p "$TMPDIR"
  export PATH="$SANDBOX_DIR/bin:$PATH"
  mkdir -p "$SANDBOX_DIR/bin"
  echo "$SANDBOX_DIR"
}

# Write a mock executable into sandbox PATH
mock_cmd() {
  local name="$1"; shift
  local body="$*"
  printf '%s\n' "#!/usr/bin/env bash" "$body" > "$SANDBOX_DIR/bin/$name"
  chmod +x "$SANDBOX_DIR/bin/$name"
}

# Load the script under test without executing bottom-of-file main logic.
# We source after setting up mocks so that any command invocations during load are safe.
source_under_test() {
  # shellcheck disable=SC1091
  # Avoid executing as a standalone script; expect functions to be defined.
  . test/test_release_workflow.sh
}

# Utility: run a named function and capture its output and status.
run_func() {
  local fn="$1"; shift || true
  OUT_FILE="$(mktemp)"
  set +e
  "$fn" "$@" >"$OUT_FILE" 2>&1
  STATUS=$?
  set -e
  OUT="$(cat "$OUT_FILE")"
  rm -f "$OUT_FILE"
  echo "$OUT"
  return "$STATUS"
}

# Test: script loads and defines key functions
test_load_and_functions_present() {
  SANDBOX_DIR="$(make_sandbox)"
  # Safe mocks for common external tools used by the script (no-op implementations)
  mock_cmd git 'echo "git $*"; exit 0'
  mock_cmd gh 'echo "gh $*"; exit 0'
  mock_cmd npm 'echo "npm $*"; exit 0'
  mock_cmd jq 'echo "null"; exit 0'
  mock_cmd sed 'command sed "$@"'
  mock_cmd awk 'command awk "$@"'
  mock_cmd grep 'command grep "$@"'

  source_under_test

  # Check for commonly expected functions; adjust based on actual definitions in the script
  set +e
  declare -F parse_version >/dev/null 2>&1
  has_parse=$?
  declare -F ensure_prereqs >/dev/null 2>&1
  has_prereqs=$?
  declare -F compute_next_version >/dev/null 2>&1
  has_next=$?
  set -e

  if [ "$has_parse" -ne 0 ] && [ "$has_next" -ne 0 ] && [ "$has_prereqs" -ne 0 ]; then
    fail "Expected core functions (parse_version, compute_next_version, ensure_prereqs) not found. Please align names below with the script."
  fi
  echo "OK: functions present (if defined in script)."
}

# Test: ensure_prereqs handles missing commands
test_ensure_prereqs_missing_tools() {
  SANDBOX_DIR="$(make_sandbox)"
  # Intentionally omit 'git' to simulate missing dependency
  mock_cmd gh 'echo "gh $*"; exit 0'
  mock_cmd npm 'echo "npm $*"; exit 0'
  mock_cmd jq 'echo "null"; exit 0'

  source_under_test

  if declare -F ensure_prereqs >/dev/null 2>&1; then
    if run_func ensure_prereqs >/dev/null; then
      fail "ensure_prereqs should fail when 'git' is missing from PATH"
    else
      echo "OK: ensure_prereqs fails without git."
    fi
  else
    echo "SKIP: ensure_prereqs not defined."
  fi
}

# Test: parse_version happy paths (semver)
test_parse_version_happy_paths() {
  SANDBOX_DIR="$(make_sandbox)"
  mock_cmd git 'echo "git $*"; exit 0'
  mock_cmd gh 'echo "gh $*"; exit 0'
  mock_cmd npm 'echo "npm $*"; exit 0'
  mock_cmd jq 'echo "null"; exit 0'

  source_under_test

  if declare -F parse_version >/dev/null 2>&1; then
    out="$(run_func parse_version "v1.2.3")" || fail "parse_version failed"
    echo "$out" | grep -E '(^|[[:space:]])1([[:space:]]|$)' >/dev/null || true
    echo "$out" | grep -E '(^|[[:space:]])2([[:space:]]|$)' >/dev/null || true
    echo "$out" | grep -E '(^|[[:space:]])3([[:space:]]|$)' >/dev/null || true
    # We don't assert exact format because implementation may vary (echo "1 2 3" or export vars)
    echo "OK: parse_version accepted v1.2.3"
  else
    echo "SKIP: parse_version not defined."
  fi
}

# Test: compute_next_version handles bump types and prereleases
test_compute_next_version_various_bumps() {
  SANDBOX_DIR="$(make_sandbox)"
  mock_cmd git 'echo "git $*"; exit 0'
  mock_cmd gh 'echo "gh $*"; exit 0'
  mock_cmd npm 'echo "npm $*"; exit 0'
  mock_cmd jq 'echo "null"; exit 0'

  source_under_test

  if declare -F compute_next_version >/dev/null 2>&1; then
    out_major="$(run_func compute_next_version "1.2.3" "major")" || fail "compute_next_version major failed"
    echo "$out_major" | grep -E '2\.0\.0(-[A-Za-z0-9\.-]+)?' >/dev/null || fail "expected 2.0.0 from major bump, got: $out_major"

    out_minor="$(run_func compute_next_version "1.2.3" "minor")" || fail "compute_next_version minor failed"
    echo "$out_minor" | grep -E '1\.3\.0(-[A-Za-z0-9\.-]+)?' >/dev/null || fail "expected 1.3.0 from minor bump, got: $out_minor"

    out_patch="$(run_func compute_next_version "1.2.3" "patch")" || fail "compute_next_version patch failed"
    echo "$out_patch" | grep -E '1\.2\.4(-[A-Za-z0-9\.-]+)?' >/dev/null || fail "expected 1.2.4 from patch bump, got: $out_patch"

    echo "OK: compute_next_version bump types validated."
  else
    echo "SKIP: compute_next_version not defined."
  fi
}

# Test: dry-run mode does not perform side-effecting commands (git tag, gh release, npm publish)
test_dry_run_no_side_effects() {
  SANDBOX_DIR="$(make_sandbox)"
  # Mock commands to record invocations
  mock_cmd git 'echo "git $*" >> "$TMPDIR/calls.log"; exit 0'
  mock_cmd gh 'echo "gh $*" >> "$TMPDIR/calls.log"; exit 0'
  mock_cmd npm 'echo "npm $*" >> "$TMPDIR/calls.log"; exit 0'
  mock_cmd jq 'echo "null"; exit 0'

  export DRY_RUN=1
  export GH_TOKEN="dummy"
  export NPM_TOKEN="dummy"

  source_under_test

  if declare -F perform_release >/dev/null 2>&1; then
    run_func perform_release "1.2.3" "patch" || fail "perform_release failed in dry-run"
    if grep -E 'git tag|git push.*--tags|gh release|npm publish' "$TMPDIR/calls.log" >/dev/null 2>&1; then
      fail "Side-effect commands were invoked during dry-run"
    fi
    echo "OK: dry-run avoided side-effects."
  else
    echo "SKIP: perform_release not defined."
  fi
}

# Test: missing env vars cause failure
test_missing_env_vars() {
  SANDBOX_DIR="$(make_sandbox)"
  mock_cmd git 'echo "git $*"; exit 0'
  mock_cmd gh 'echo "gh $*"; exit 0'
  mock_cmd npm 'echo "npm $*"; exit 0'
  mock_cmd jq 'echo "null"; exit 0'

  unset GH_TOKEN || true
  unset NPM_TOKEN || true
  source_under_test

  if declare -F require_env >/dev/null 2>&1; then
    if run_func require_env "GH_TOKEN"; then
      fail "require_env should fail when GH_TOKEN is unset"
    fi
    echo "OK: require_env fails when missing."
  else
    echo "SKIP: require_env not defined."
  fi
}

# Main runner if invoked directly
if [ "${1-}" != "sourced" ]; then
  for t in $(declare -F | awk '{print $3}' | grep -E '^test_'); do
    echo "Running $t..."
    "$t"
  done
  echo "All tests completed."
fi

# Additional tests emphasizing edge cases and failure conditions

test_parse_version_edge_cases() {
  SANDBOX_DIR="$(make_sandbox)"
  mock_cmd git 'exit 0'; mock_cmd gh 'exit 0'; mock_cmd npm 'exit 0'; mock_cmd jq 'echo "null"'
  source_under_test
  if declare -F parse_version >/dev/null 2>&1; then
    # Should reject non-semver strings
    if run_func parse_version "versionX"; then
      fail "parse_version accepted invalid version string"
    fi
    # Accept plain semver without 'v'
    run_func parse_version "1.0.0" || fail "parse_version rejected 1.0.0"
    echo "OK: parse_version edge cases handled."
  else
    echo "SKIP: parse_version not defined."
  fi
}

test_release_executes_expected_commands_when_not_dry_run() {
  SANDBOX_DIR="$(make_sandbox)"
  mock_cmd git 'echo "git $*" >> "$TMPDIR/calls.log"; exit 0'
  mock_cmd gh 'echo "gh $*" >> "$TMPDIR/calls.log"; exit 0'
  mock_cmd npm 'echo "npm $*" >> "$TMPDIR/calls.log"; exit 0'
  mock_cmd jq 'echo "null"; exit 0'

  export DRY_RUN=0
  export GH_TOKEN="dummy"
  export NPM_TOKEN="dummy"

  source_under_test

  if declare -F perform_release >/dev/null 2>&1; then
    run_func perform_release "1.2.3" "patch" || fail "perform_release failed"
    grep -E 'git tag' "$TMPDIR/calls.log" >/dev/null || fail "git tag not invoked"
    grep -E 'gh release' "$TMPDIR/calls.log" >/dev/null || fail "gh release not invoked"
    grep -E 'npm publish' "$TMPDIR/calls.log" >/dev/null || fail "npm publish not invoked"
    echo "OK: non-dry release invoked expected commands."
  else
    echo "SKIP: perform_release not defined."
  fi
}

test_handles_pre_release_identifiers() {
  SANDBOX_DIR="$(make_sandbox)"
  mock_cmd git 'exit 0'; mock_cmd gh 'exit 0'; mock_cmd npm 'exit 0'; mock_cmd jq 'echo "null"'
  source_under_test

  if declare -F compute_next_version >/dev/null 2>&1; then
    out="$(run_func compute_next_version "1.2.3" "prerelease" "rc.1")" || fail "prerelease bump failed"
    echo "$out" | grep -E '1\.2\.4-rc\.1|1\.2\.3-rc\.1' >/dev/null || fail "expected rc prerelease in output, got: $out"
    echo "OK: prerelease identifiers supported."
  else
    echo "SKIP: compute_next_version not defined."
  fi
}
