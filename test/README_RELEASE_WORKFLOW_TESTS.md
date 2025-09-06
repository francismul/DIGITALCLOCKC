These tests validate the behavior of test/test_release_workflow.sh using a lightweight bash harness.
Framework note: If the repository already uses a shell testing framework (e.g., Bats, shunit2), adapt the test cases in test/test_release_workflow.spec.sh into that framework's idioms.
Tests include:
- If implemented, verify the presence of core functions (parse_version, compute_next_version, ensure_prereqs, perform_release, require_env).
- Mock external commands (git, gh, npm, jq) by shadowing them in a temporary PATH to prevent side effects.
- Validate that a dry run avoids side effects.
- Validate that a non-dry run invokes the expected commands.
- Exercise semver parsing, bump logic, and prerelease handling.
Run locally:
  bash test/run.sh