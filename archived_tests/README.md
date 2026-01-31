# Archived Tests

This directory contains test files that were used during development and debugging sessions. These tests served their purpose and are kept for historical reference.

## Directory Structure

### gosub_debugging/
Test files used during GOSUB/RETURN implementation and debugging sessions.

**Contents:**
- GOSUB control flow tests
- Nested GOSUB tests
- IF/GOSUB interaction tests
- Bug reproduction cases

**Historical Context:** These tests were created to debug and verify the GOSUB/RETURN stack implementation. The bugs found and fixed are documented in `docs/session_notes/`.

### rosetta_mersenne/
Files from Rosetta Code Mersenne prime factorization implementation.

**Contents:**
- Mersenne prime factorization programs
- Various optimization iterations
- Assembly output from different versions
- Prime number test utilities

**Historical Context:** Used to stress-test the compiler with real-world algorithm implementation. Documented in `docs/ROSETTA_MERSENNE_SOLUTION.md`.

### division_tests/
Tests for integer division, shift optimization, and division operator behavior.

**Contents:**
- Integer division tests (`\` operator)
- Floating-point division tests (`/` operator)
- Shift optimization verification
- Signed division edge cases

**Historical Context:** Used to verify that division operators work correctly and that the compiler optimizes power-of-2 division to shifts.

### Root Level (Other Tests)
Various one-off test files for specific features or bugs:
- Variable access tests
- Function local scope tests
- QBE IL debugging tests
- Global variable tests

## Why These Are Archived

These test files were useful during development but:
1. **Not part of the formal test suite** - The proper test suite is in `tests/`
2. **Debugging artifacts** - Created to reproduce specific bugs
3. **Experiment files** - Used to try different approaches
4. **One-time verification** - Verified a specific fix, no longer needed for regression testing

## Should I Delete These?

**No!** Keep them because:
- They show the development history
- They can be referenced if similar bugs appear
- They document edge cases that were considered
- They may contain useful test patterns for future work

## Accessing Current Tests

For the **active test suite**, see:
- `tests/` - Organized by category (arithmetic, loops, conditionals, etc.)
- `tests/conditionals/` - SELECT CASE tests
- `tests/rosetta/` - Active Rosetta Code implementations
- `scripts/run_tests_simple.sh` - Test runner

## Documentation References

- `docs/session_notes/` - Session-by-session development notes
- `docs/SESSION_*.md` - Specific debugging sessions
- `docs/GOSUB_*.md` - GOSUB implementation documentation
- `docs/SELECT_CASE_*.md` - SELECT CASE implementation documentation

---

**Note:** If you're looking for tests to run, use `tests/` directory, not this archive!