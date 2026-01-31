# Test Programs

This directory contains test programs for the FasterBASIC QBE compiler.

## Directory Structure

- **examples/** - Example BASIC programs demonstrating language features
  - Simple, well-commented programs
  - Good starting point for learning FasterBASIC
  - Intended to be committed to repository

- **scratch/** - Temporary test files and experiments
  - Development and debugging programs
  - Work-in-progress code
  - NOT committed (gitignored)

## Examples

Example programs demonstrate specific language features:

```bash
# Run an example
cd ../
./qbe_basic test_programs/examples/hello.bas -o hello
./hello
```

## Scratch Area

The `scratch/` directory is for temporary test files during development:

- Quick experiments
- Debugging specific issues
- One-off test cases
- Minimal reproducers for bugs

Files in `scratch/` are automatically gitignored.

## Official Test Suite

For comprehensive testing, use the official test suite in [../tests/](../tests/):

- **tests/rosetta/** - Rosetta Code implementations
- **tests/features/** - Feature-specific tests
- **tests/regression/** - Regression tests for fixed bugs

## Creating Test Programs

### Simple Test

```basic
10 REM Test basic arithmetic
20 DIM x AS INTEGER
30 x = 5 + 3
40 PRINT "Result: "; x
50 END
```

### Compile and Run

```bash
# From project root
./qbe_basic test_programs/scratch/mytest.bas -o mytest
./mytest
```

### Debug Output

```bash
# Generate QBE IL only
./qbe_basic -i -o mytest.qbe test_programs/scratch/mytest.bas

# Generate assembly only  
./qbe_basic -c -o mytest.s test_programs/scratch/mytest.bas

# Trace CFG
./qbe_basic -G test_programs/scratch/mytest.bas
```

## Test Program Guidelines

When creating test programs:

1. **Use clear variable names** - Makes debugging easier
2. **Add comments** - Explain what you're testing
3. **Keep it minimal** - Test one thing at a time
4. **Print results** - Verify expected vs actual output
5. **Handle errors** - Use TRY/CATCH when appropriate

## Example Program Template

```basic
10 REM Test: [Brief description]
20 REM Expected: [What should happen]
30
40 DIM variable AS INTEGER
50
60 REM Setup
70 variable = 42
80
90 REM Test
100 PRINT "Testing..."
110 PRINT "Result: "; variable
120
130 REM Verify
140 IF variable = 42 THEN
150     PRINT "PASS"
160 ELSE
170     PRINT "FAIL: Expected 42, got "; variable
180 END IF
190
200 END
```

## Running Multiple Tests

Use the test scripts in [../scripts/](../scripts/):

```bash
# Quick test
./scripts/run_tests_simple.sh

# Full verification
./scripts/verify_implementation.sh
```

## See Also

- [../START_HERE.md](../START_HERE.md) - Getting started guide
- [../BUILD.md](../BUILD.md) - Build instructions
- [../docs/DEBUGGING_GUIDE.md](../docs/DEBUGGING_GUIDE.md) - Debugging tips