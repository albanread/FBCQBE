# Testing Guide

## Quick Start - Run All Tests

**To run the complete test suite, use this command from the project root:**

```bash
./run_tests.sh
```

**That's it!** This is the ONLY test command you need to remember.

---

## Test Results

The test runner will show:
- ✅ **PASS** - Test compiled, ran, and produced correct output
- ❌ **COMPILE FAIL** - Test failed to compile (compiler error or abort)
- ❌ **LINK FAIL** - Test compiled but failed to link
- ❌ **CRASH** - Test compiled and ran but crashed
- ⏱️ **TIMEOUT** - Test took too long to run

### Current Baseline

**Expected results (as of 2024-01-31):**
```
Total Tests:   107
Passed:        ~81  (75.7%)
Failed:        ~26  (24.3%)
```

Some tests are known to fail due to:
- GOSUB/RETURN implementation issues (several tests)
- SELECT CASE incomplete implementation (several tests)
- SUB statement implementation issues
- Edge cases in type coercion

---

## Test Output

### See Summary Only

```bash
./run_tests.sh | tail -20
```

### Save Full Results

```bash
./run_tests.sh > test_results.txt 2>&1
cat test_results.txt
```

### Watch Tests Run in Real-Time

```bash
./run_tests.sh | tee test_results.txt
```

---

## Test Categories

Tests are organized by feature in the `tests/` directory:

```
tests/
├── arithmetic/       # Math operators (+, -, *, /, MOD, ^, bitwise)
├── arrays/          # Array operations (1D, 2D, REDIM, ERASE)
├── conditionals/    # IF/THEN/ELSE, SELECT CASE
├── exceptions/      # TRY/CATCH/FINALLY, error handling
├── functions/       # GOSUB/RETURN, math intrinsics (SIN, COS, etc.)
├── loops/           # FOR/NEXT, WHILE/WEND, DO/LOOP, EXIT
├── rosetta/         # Rosetta Code solutions and benchmarks
├── strings/         # String operations, slicing, comparison
├── types/           # Type conversions, UDTs (User-Defined Types)
└── ...              # Additional categories
```

---

## Running Individual Tests

### Method 1: Direct Compilation (Fastest)

```bash
# Compile to executable and run
./qbe_basic -o /tmp/test tests/arithmetic/test_integer_basic.bas
/tmp/test
```

### Method 2: Step-by-Step (For Debugging)

```bash
# Step 1: Compile BASIC to assembly
./qbe_basic -c -o /tmp/test.s tests/arithmetic/test_integer_basic.bas

# Step 2: Link with runtime
gcc /tmp/test.s fsh/FasterBASICT/runtime_c/*.c \
    -I fsh/FasterBASICT/runtime_c -lm -o /tmp/test

# Step 3: Run
/tmp/test
```

### Method 3: Inspect QBE IL (Intermediate Representation)

```bash
# Generate QBE IL only
./qbe_basic -i -o /tmp/test.qbe tests/arithmetic/test_integer_basic.bas
cat /tmp/test.qbe
```

---

## Creating New Tests

### 1. Choose a Category

Pick the appropriate test category in `tests/`:
- Arithmetic operations → `tests/arithmetic/`
- String operations → `tests/strings/`
- Control flow → `tests/conditionals/` or `tests/loops/`
- etc.

### 2. Create Test File

Create a `.bas` file with a descriptive name:

```bash
cat > tests/arithmetic/test_my_feature.bas << 'EOF'
10 REM Test: My Feature Description
20 PRINT "=== My Feature Tests ==="
30 
40 REM Test case 1
50 LET A% = 42
60 IF A% <> 42 THEN PRINT "ERROR: Test 1 failed" : END
70 PRINT "PASS: Test 1"
80 
90 REM Test case 2
100 LET B% = A% * 2
110 IF B% <> 84 THEN PRINT "ERROR: Test 2 failed" : END
120 PRINT "PASS: Test 2"
130 
140 PRINT "All tests passed!"
150 END
EOF
```

### 3. Test Your Test

```bash
# Run it manually first
./qbe_basic -o /tmp/mytest tests/arithmetic/test_my_feature.bas
/tmp/mytest
```

### 4. Verify It's Picked Up

```bash
# Run the full suite - your test should appear
./run_tests.sh | grep test_my_feature
```

The test runner automatically discovers all `.bas` files in the `tests/` directory.

---

## Test Best Practices

### ✅ DO:
- Use descriptive test names: `test_feature_description.bas`
- Print clear pass/fail messages
- Test one feature/behavior per file
- Include a REM comment at the top describing the test
- End with `END` statement
- Print "PASS" for successful tests

### ❌ DON'T:
- Rely on exact timing (tests may run on different systems)
- Print excessive output (keeps test runs fast)
- Use infinite loops (tests will timeout)
- Leave tests that always fail (fix or remove them)

### Example Good Test:

```basic
10 REM Test: Integer Division
20 PRINT "Testing integer division..."
30 LET A% = 10
40 LET B% = 3
50 LET C% = A% \ B%
60 IF C% <> 3 THEN PRINT "ERROR: Expected 3, got "; C% : END
70 PRINT "PASS"
80 END
```

---

## Debugging Test Failures

### 1. Identify the Failing Test

```bash
./run_tests.sh | grep "FAIL\|CRASH"
```

### 2. Run the Test Manually

```bash
./qbe_basic -o /tmp/debug tests/category/test_name.bas
/tmp/debug
```

### 3. Check Compiler Output

```bash
# See if there are compiler warnings/errors
./qbe_basic tests/category/test_name.bas > /tmp/out.s 2>&1
cat /tmp/out.s
```

### 4. Inspect Generated IL

```bash
# Look at the intermediate representation
./qbe_basic -i -o /tmp/debug.qbe tests/category/test_name.bas
cat /tmp/debug.qbe
```

### 5. Check Assembly

```bash
# See what assembly is generated
./qbe_basic -c -o /tmp/debug.s tests/category/test_name.bas
cat /tmp/debug.s
```

---

## Performance Testing

### Run Tests with Timing

```bash
time ./run_tests.sh
```

### Profile a Specific Test

```bash
# macOS
time ./qbe_basic -o /tmp/test tests/rosetta/addition_chain_exponentiation.bas
time /tmp/test

# Linux (with perf)
perf stat ./qbe_basic -o /tmp/test tests/rosetta/addition_chain_exponentiation.bas
perf stat /tmp/test
```

---

## Continuous Integration

### Before Committing Code

```bash
# Always run the full test suite
./run_tests.sh

# Check for regressions
# If fewer tests pass than baseline (~81), investigate!
```

### Baseline Tracking

Keep track of test pass rates:
```bash
./run_tests.sh | grep "Total Tests\|Passed" > baseline.txt
```

Compare before and after changes:
```bash
# Before changes
./run_tests.sh | grep "Passed:" > before.txt

# Make changes...

# After changes
./run_tests.sh | grep "Passed:" > after.txt
diff before.txt after.txt
```

---

## Test Runner Details

The test runner (`./run_tests.sh`) automatically:
1. Discovers all `.bas` files in `tests/` directory
2. Compiles each test with the QBE compiler
3. Links with the runtime library
4. Runs the executable
5. Captures output and exit codes
6. Categorizes results (PASS/FAIL/CRASH/TIMEOUT)
7. Generates a summary report

**Test timeout:** 10 seconds per test (configurable in script)

---

## Troubleshooting

### "Command not found: ./run_tests.sh"

```bash
# Make sure you're in the project root
cd /path/to/FBCQBE
./run_tests.sh
```

### "Permission denied"

```bash
# Make the script executable
chmod +x ./run_tests.sh
./run_tests.sh
```

### "qbe_basic not found"

```bash
# Build the compiler first
cd qbe_basic_integrated
./build_qbe_basic.sh
cd ..
./run_tests.sh
```

### All Tests Failing

```bash
# Rebuild from scratch
cd qbe_basic_integrated
rm -rf obj/*.o qbe_basic
./build_qbe_basic.sh
cd ..
./run_tests.sh
```

---

## Additional Resources

- **[START_HERE.md](START_HERE.md)** - Main developer guide
- **[BUILD.md](BUILD.md)** - Build instructions
- **[OPTIMIZATION_SUMMARY.md](OPTIMIZATION_SUMMARY.md)** - Compiler optimizations
- **[tests/README.md](tests/README.md)** - Test directory details (if exists)

---

**Last Updated:** 2024-01-31  
**Test Suite Version:** 107 tests  
**Baseline Pass Rate:** ~81/107 (75.7%)