# Rosetta Code Tests

This directory contains implementations of Rosetta Code challenges in FasterBASIC.

## Tests

### 1. Levenshtein Distance
**File:** `levenshtein_distance.bas`  
**Challenge:** https://rosettacode.org/wiki/Levenshtein_distance

Calculates the minimum edit distance between two strings using dynamic programming.

### 2. GOSUB/RETURN Control Flow
**File:** `gosub_if_control_flow.bas`  
**Purpose:** Regression test for GOSUB/RETURN in multiline IF blocks

Tests that GOSUB/RETURN works correctly when called from within:
- Multiline IF...END IF blocks
- Nested IF statements
- WHILE loops containing IF blocks
- Multiple GOSUBs in the same IF block

This test validates the fix for a compiler bug where RETURN would incorrectly jump to after END IF instead of continuing execution within the IF block.

**Expected Output:** `gosub_if_control_flow.expected`

### 3. Mersenne Number Factors
**File:** `mersenne_factors.bas`  
**Challenge:** https://rosettacode.org/wiki/Factors_of_a_Mersenne_number

Finds factors of Mersenne numbers (2^P - 1) using optimized trial division with:
- Binary exponentiation for modular arithmetic
- Mersenne number properties (q = 2kP+1, q ≡ 1 or 7 mod 8)
- Primality testing

**Test Case:** Finds a factor of M929 (2^929 - 1)  
**Result:** Factor 13007 (k=7)  
**Expected Output:** `mersenne_factors.expected`

## Running Tests

### Run a single test:
```bash
./qbe_basic -o test_name tests/rosetta/test_name.bas
./test_name
```

### Verify expected output:
```bash
./test_name > actual.txt
diff -u tests/rosetta/test_name.expected actual.txt
```

### Run all rosetta tests:
```bash
./scripts/run_tests_simple.sh
```

## Test Format

Each test should include:
- `.bas` file - The FasterBASIC source code
- `.expected` file - Expected output for verification
- Comments at the top describing the challenge and algorithm
