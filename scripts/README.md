# Scripts

This directory contains utility scripts for building, testing, and maintaining the FasterBASIC QBE compiler.

## Available Scripts

### Build Scripts

- **See [../qbe_basic_integrated/build_qbe_basic.sh](../qbe_basic_integrated/build_qbe_basic.sh)** - Main build script
  - This is the ONLY build script you should use
  - Builds the QBE compiler with FasterBASIC integration
  - Run from `qbe_basic_integrated/` directory

### Test Scripts

- **run_tests_simple.sh** - Run simple test suite
  - Quick smoke tests for basic functionality
  - Useful for rapid iteration during development

- **verify_implementation.sh** - Comprehensive verification
  - Runs full test suite
  - Validates all language features
  - Use before committing changes

### Utility Scripts

- **generate_tests.sh** - Generate test cases
  - Creates test programs programmatically
  - Useful for regression testing

- **organize.sh** - Directory organization
  - One-time script used to reorganize project structure
  - Kept for reference

## Usage

All scripts should be run from the project root directory:

```bash
# From project root
./scripts/run_tests_simple.sh

# Or with full path
cd /path/to/FBCQBE
./scripts/verify_implementation.sh
```

## Building the Compiler

**Important:** Do NOT use any build scripts in this directory. Always use:

```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
```

See [../BUILD.md](../BUILD.md) for detailed build instructions.

## Test Organization

- **Test programs**: [../test_programs/](../test_programs/)
  - `examples/` - Example BASIC programs
  - `scratch/` - Temporary test files (gitignored)
  
- **Test suite**: [../tests/](../tests/)
  - Organized test cases by feature
  - Used by verification scripts

## Adding New Scripts

When adding new scripts:

1. Make them executable: `chmod +x scriptname.sh`
2. Add a header comment explaining the script's purpose
3. Document usage in this README
4. Follow bash best practices (set -e, proper error handling)
5. Test from project root to ensure paths work correctly