# FasterBASIC QBE Compiler - Developer Guide

**Welcome!** This guide will help you build, use, and develop the FasterBASIC QBE compiler.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Building the Compiler](#building-the-compiler)
3. [Compiling BASIC Programs](#compiling-basic-programs)
4. [Inspecting Generated Code](#inspecting-generated-code)
5. [Running Tests](#running-tests)
6. [Modifying the Runtime](#modifying-the-runtime)
7. [Project Structure](#project-structure)
8. [Development Workflow](#development-workflow)
9. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Prerequisites

- **C++17 compiler** (clang++ or g++)
- **C compiler** (gcc or clang)
- **Standard build tools** (make, as)

### 30-Second Build & Run

```bash
# 1. Build the compiler
cd qbe_basic_integrated
./build_qbe_basic.sh

# 2. Create a test program
cat > hello.bas << 'EOF'
PRINT "Hello, World!"
END
EOF

# 3. Compile and run
./qbe_basic hello.bas > hello.s
gcc hello.s ../fsh/FasterBASICT/runtime_c/*.c -I../fsh/FasterBASICT/runtime_c -lm -o hello
./hello
```

**Output:**
```
Hello, World!
```

---

## Building the Compiler

### Full Build Process

The build script compiles the FasterBASIC compiler frontend and integrates it with the QBE backend:

```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
```

**What this does:**

1. **Compiles FasterBASIC compiler sources** (`src/codegen/*.cpp`, `src/*.cpp`)
   - Lexer, Parser, Semantic Analyzer
   - CFG Builder
   - QBE Code Generator
   
2. **Compiles runtime stubs** (`runtime_stubs.c`)

3. **Builds QBE backend** (if needed)
   - Auto-detects platform (x86-64, ARM64, RISC-V)
   - Compiles QBE object files
   
4. **Links everything** into `qbe_basic` executable

**Output:**
```
=== Building QBE with FasterBASIC Integration ===
Compiling FasterBASIC compiler sources...
  ✓ FasterBASIC compiled
Compiling BASIC runtime library...
  ✓ Runtime library compiled
Compiling FasterBASIC wrapper and frontend...
  ✓ Wrapper compiled
Checking QBE object files...
  ✓ QBE objects ready
Linking QBE with FasterBASIC support...

=== Build Complete ===
Executable: /path/to/qbe_basic_integrated/qbe_basic
```

### Clean Build

```bash
# Remove all built files and rebuild from scratch
cd qbe_basic_integrated
rm -rf obj/*.o qbe_basic
rm -rf qbe_source/*.o
./build_qbe_basic.sh
```

---

## Compiling BASIC Programs

### Basic Workflow

The compiler generates assembly code that must be linked with the runtime:

```bash
# Step 1: Compile BASIC to assembly
./qbe_basic_integrated/qbe_basic program.bas > program.s

# Step 2: Link with runtime
gcc program.s \
    fsh/FasterBASICT/runtime_c/*.c \
    -I fsh/FasterBASICT/runtime_c \
    -lm \
    -o program

# Step 3: Run
./program
```

### Using the -i Flag (Generate QBE IL)

To inspect or save the intermediate QBE IL:

```bash
# Generate QBE IL only (no assembly)
./qbe_basic_integrated/qbe_basic -i program.bas > program.qbe

# Then compile IL to assembly
./qbe_basic_integrated/qbe_basic program.qbe > program.s
```

### Complete Example

```bash
# Create a program
cat > factorial.bas << 'EOF'
FUNCTION Factorial%(N%)
    IF N% <= 1 THEN
        Factorial% = 1
    ELSE
        Factorial% = N% * Factorial%(N% - 1)
    END IF
END FUNCTION

PRINT "5! = "; Factorial%(5)
END
EOF

# Compile
./qbe_basic_integrated/qbe_basic factorial.bas > factorial.s

# Link
gcc factorial.s \
    fsh/FasterBASICT/runtime_c/*.c \
    -I fsh/FasterBASICT/runtime_c \
    -lm \
    -o factorial

# Run
./factorial
```

**Output:**
```
5! = 120
```

---

## Inspecting Generated Code

### View QBE IL (SSA Form)

The `-i` flag generates human-readable QBE Intermediate Language:

```bash
./qbe_basic_integrated/qbe_basic -i program.bas > program.qbe
cat program.qbe
```

**Example QBE IL:**
```qbe
# Variable declarations
%var_A_INT =l copy 0
%var_B_INT =l copy 0

# Line 10: LET A% = 5
%t0 =d copy d_5.000000
%t1 =l dtosi %t0
%var_A_INT =l copy %t1

# Line 20: LET B% = NOT A%
%t3 =w copy %var_A_INT
%t4 =w xor %t3, -1
%t5 =l extsw %t4
%var_B_INT =l copy %t5
```

### View Assembly Output

Generate native assembly for your platform:

```bash
./qbe_basic_integrated/qbe_basic program.bas > program.s
cat program.s
```

**Example Assembly (ARM64):**
```asm
.text
.balign 4
.globl _main
_main:
    hint    #34
    stp     x29, x30, [sp, -16]!
    mov     x29, sp
    bl      _basic_runtime_init
    mov     w0, #-6
    bl      _basic_print_int
    bl      _basic_print_newline
    bl      _basic_runtime_cleanup
    mov     w0, #0
    ldp     x29, x30, [sp], 16
    ret
```

### Debugging Code Generation

Enable verbose output to see what the compiler is doing:

```bash
# Show parsing and CFG construction
./qbe_basic_integrated/qbe_basic -i program.bas 2>&1 | grep "INFO\|Block\|Edge"

# Compare IL and assembly side-by-side
./qbe_basic_integrated/qbe_basic -i program.bas > program.qbe
./qbe_basic_integrated/qbe_basic program.qbe > program.s
diff -y program.qbe program.s | less
```

---

## Running Tests

### Full Test Suite

Run all 56 tests (arithmetic, loops, strings, arrays, types, etc.):

```bash
./test_basic_suite.sh
```

**Output:**
```
═══════════════════════════════════════════════════════════
  ARITHMETIC TESTS
═══════════════════════════════════════════════════════════

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
ℹ INFO: Testing: test_integer_basic
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  [1/4] Compiling BASIC to QBE IL...
  [2/4] Compiling QBE IL to Assembly...
  [3/4] Linking with runtime...
  [4/4] Running executable...
✓ PASS: test_integer_basic
    Output:
      === Integer Arithmetic Tests ===
      10 + 20 = 30
      PASS: Addition
      ...

╔════════════════════════════════════════════════════════════╗
║                    TEST SUMMARY                            ║
╚════════════════════════════════════════════════════════════╝

Total Tests:   56
Passed:        56
Failed:        0
Skipped:       0

═══════════════════════════════════════════════════════════
  ✓ ALL TESTS PASSED!
═══════════════════════════════════════════════════════════
```

### Run a Single Test

```bash
# Method 1: Direct compilation
./qbe_basic_integrated/qbe_basic tests/arithmetic/test_not_basic.bas > /tmp/test.s
gcc /tmp/test.s fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o /tmp/test
/tmp/test

# Method 2: Use the test function (edit test_basic_suite.sh temporarily)
# Add at end: test_basic_file "tests/arithmetic/test_not_basic.bas"
```

### Test Categories

```
tests/
├── arithmetic/       # Math operators, bitwise, MOD, mixed types
├── arrays/          # Array operations, 1D, 2D
├── comparisons/     # Numeric comparisons
├── conditionals/    # IF/THEN/ELSE, SELECT CASE
├── data/            # DATA/READ/RESTORE
├── functions/       # GOSUB, math intrinsics
├── io/              # PRINT formatting
├── loops/           # FOR, WHILE, DO, EXIT statements
├── strings/         # String operations, comparisons
└── types/           # Type conversions, UDTs
```

### Create a New Test

```bash
# 1. Create test file
cat > tests/arithmetic/test_mynew.bas << 'EOF'
10 REM Test: My New Feature
20 PRINT "=== My New Feature Tests ==="
30 LET A% = 42
40 PRINT "A% = "; A%
50 IF A% <> 42 THEN PRINT "ERROR: Failed" : END
60 PRINT "PASS: My test works"
70 END
EOF

# 2. Add to test_basic_suite.sh in appropriate section
# Edit test_basic_suite.sh and add:
# if [ -f "$SCRIPT_DIR/tests/arithmetic/test_mynew.bas" ]; then
#     test_basic_file "$SCRIPT_DIR/tests/arithmetic/test_mynew.bas"
# fi

# 3. Run tests
./test_basic_suite.sh
```

---

## Modifying the Runtime

### Runtime Structure

The runtime library is in `fsh/FasterBASICT/runtime_c/`:

```
runtime_c/
├── basic_runtime.c        # Init, cleanup, main runtime
├── basic_runtime.h        # Header file
├── array_ops.c            # Array operations
├── string_ops.c           # String operations
├── string_utf32.c         # UTF-32 string handling
├── io_ops.c               # Input/output
├── io_ops_format.c        # PRINT USING formatting
├── math_ops.c             # Math functions (SGN, ABS, etc.)
├── conversion_ops.c       # Type conversions
├── memory_mgmt.c          # Memory management
├── basic_data.c           # DATA/READ/RESTORE
└── array_descriptor_runtime.c  # Array descriptors
```

### Rebuild After Runtime Changes

**The runtime is compiled directly during linking**, so no separate rebuild is needed:

```bash
# Just recompile your test program
./qbe_basic_integrated/qbe_basic program.bas > program.s
gcc program.s fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o program
./program
```

### Add a New Runtime Function

**Example: Add a BEEP function**

1. **Add declaration to `basic_runtime.h`:**
```c
void basic_beep(void);
```

2. **Implement in `io_ops.c`:**
```c
void basic_beep(void) {
    printf("\a");  // ASCII bell character
    fflush(stdout);
}
```

3. **Register in codegen** (optional, for intrinsic functions):

Edit `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp`:

```cpp
// In emitFunctionCall()
if (upper == "BEEP" && expr->arguments.empty()) {
    emit("    call $basic_beep()\n");
    m_stats.instructionsGenerated++;
    return "";  // No return value
}
```

4. **Test it:**
```basic
BEEP
PRINT "Did you hear the beep?"
END
```

### Debug Runtime Issues

Add debug output to runtime functions:

```c
// In runtime_c/basic_runtime.c
void basic_print_int(int64_t value) {
    fprintf(stderr, "[DEBUG] Printing int: %lld\n", value);  // Debug
    printf("%lld", value);
}
```

Then recompile and run:
```bash
./qbe_basic_integrated/qbe_basic program.bas > program.s
gcc program.s fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o program
./program 2>&1  # See debug output on stderr
```

---

## Project Structure

```
FBCQBE/
├── START_HERE.md              ← You are here!
├── README.md                  # Project overview
├── test_basic_suite.sh        # Main test runner
│
├── qbe_basic_integrated/      # Compiler build directory
│   ├── build_qbe_basic.sh     # Build script
│   ├── qbe_basic              # Compiler executable (generated)
│   ├── obj/                   # Object files
│   ├── qbe_source/            # QBE backend source
│   ├── fasterbasic_wrapper.cpp   # Integration wrapper
│   └── basic_frontend.cpp     # BASIC frontend
│
├── fsh/FasterBASICT/          # FasterBASIC source
│   ├── src/                   # Compiler source
│   │   ├── fasterbasic_lexer.cpp
│   │   ├── fasterbasic_parser.cpp
│   │   ├── fasterbasic_semantic.cpp
│   │   ├── fasterbasic_cfg.cpp
│   │   └── codegen/           # QBE code generation
│   │       ├── qbe_codegen_main.cpp
│   │       ├── qbe_codegen_expressions.cpp
│   │       ├── qbe_codegen_statements.cpp
│   │       ├── qbe_codegen_helpers.cpp
│   │       └── qbe_codegen_runtime.cpp
│   │
│   └── runtime_c/             # Runtime library
│       ├── basic_runtime.c/h  # Core runtime
│       ├── array_ops.c        # Array operations
│       ├── string_ops.c       # String operations
│       ├── math_ops.c         # Math functions
│       └── io_ops.c           # I/O operations
│
├── tests/                     # Test suite
│   ├── arithmetic/            # Math operator tests
│   ├── loops/                 # Loop tests
│   ├── strings/               # String tests
│   ├── arrays/                # Array tests
│   └── types/                 # Type system tests
│
├── docs/                      # Additional documentation
│   └── NOT_OPERATOR_REFERENCE.md
│
└── test_output/               # Test results (generated)
    ├── test_results.log
    └── *.output
```

---

## Development Workflow

### Typical Development Cycle

1. **Make changes** to compiler source or runtime
2. **Rebuild compiler:**
   ```bash
   cd qbe_basic_integrated
   ./build_qbe_basic.sh
   ```
3. **Test with a simple program:**
   ```bash
   ./qbe_basic ../test_simple.bas > /tmp/test.s
   gcc /tmp/test.s ../fsh/FasterBASICT/runtime_c/*.c -I ../fsh/FasterBASICT/runtime_c -lm -o /tmp/test
   /tmp/test
   ```
4. **Run full test suite:**
   ```bash
   cd ..
   ./test_basic_suite.sh
   ```

### Compiler Development Tips

#### 1. Add Verbose Logging

Edit `qbe_codegen_main.cpp` or other files:

```cpp
void QBECodeGenerator::emitExpression(const Expression* expr) {
    std::cerr << "[DEBUG] Emitting expression type: " 
              << static_cast<int>(expr->getType()) << std::endl;
    // ... rest of function
}
```

#### 2. Test QBE IL Directly

Generate IL and inspect it:

```bash
./qbe_basic_integrated/qbe_basic -i test.bas > test.qbe
cat test.qbe

# Check for QBE errors
./qbe_basic_integrated/qbe_basic test.qbe > test.s 2>&1
```

#### 3. Compare Before/After Changes

```bash
# Before changes
./qbe_basic_integrated/qbe_basic -i test.bas > before.qbe

# Make changes and rebuild
./build_qbe_basic.sh

# After changes
./qbe_basic_integrated/qbe_basic -i test.bas > after.qbe

# Compare
diff -u before.qbe after.qbe
```

#### 4. Use Small Test Cases

Create minimal reproducers:

```bash
cat > minimal.bas << 'EOF'
LET A% = NOT 0
PRINT A%
END
EOF

./qbe_basic_integrated/qbe_basic -i minimal.bas
```

### Runtime Development Tips

#### Test Runtime Functions Standalone

Create a C test program:

```c
#include "fsh/FasterBASICT/runtime_c/basic_runtime.h"

int main() {
    basic_runtime_init();
    
    // Test your function
    basic_beep();
    
    basic_runtime_cleanup();
    return 0;
}
```

```bash
gcc test_runtime.c fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o test_runtime
./test_runtime
```

---

## Troubleshooting

### Compiler Won't Build

**Symptom:** Build script fails with errors

**Solutions:**

1. **Check C++17 support:**
   ```bash
   clang++ --version  # Should be 5.0+
   g++ --version      # Should be 7.0+
   ```

2. **Clean build:**
   ```bash
   cd qbe_basic_integrated
   rm -rf obj/*.o qbe_basic qbe_source/*.o
   ./build_qbe_basic.sh
   ```

3. **Check for missing files:**
   ```bash
   ls -la ../fsh/FasterBASICT/src/codegen/*.cpp
   ls -la ../fsh/FasterBASICT/runtime_c/*.c
   ```

### QBE Errors During Compilation

**Symptom:** Error like `qbe:program.bas:42: invalid instruction`

**This means the codegen emitted invalid QBE IL.**

**Debug steps:**

1. **Generate IL to see the problem:**
   ```bash
   ./qbe_basic_integrated/qbe_basic -i program.bas > program.qbe
   ```

2. **Look for the error line:**
   ```bash
   sed -n '40,45p' program.qbe  # Show lines around error
   ```

3. **Common QBE errors:**
   - **Type mismatch:** `%t1 =w copy %t2` where `%t2` is `l` or `d`
     - Fix: Use proper conversion (`extsw`, `dtosi`, etc.)
   - **Invalid operand:** Using wrong type for instruction
     - Fix: Check QBE IL docs for instruction requirements
   - **Undefined temporary:** Using `%t5` before defining it
     - Fix: Check code generation order

### Runtime Crashes

**Symptom:** Program crashes or produces wrong output

**Debug steps:**

1. **Add debug prints to runtime:**
   ```c
   void basic_print_int(int64_t value) {
       fprintf(stderr, "[PRINT_INT] value=%lld\n", value);
       printf("%lld", value);
   }
   ```

2. **Compile with debug symbols:**
   ```bash
   gcc -g program.s fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o program
   ```

3. **Run with debugger:**
   ```bash
   lldb program  # or gdb program
   (lldb) run
   (lldb) bt     # backtrace on crash
   ```

4. **Check array bounds:**
   - Runtime has bounds checking enabled
   - Look for "Array index out of bounds" messages

### Test Failures

**Symptom:** Tests fail in test suite

**Debug steps:**

1. **See detailed output:**
   ```bash
   cat test_output/test_name.output
   cat test_output/test_name.compile.err
   cat test_output/test_name.link.err
   ```

2. **Run test manually:**
   ```bash
   ./qbe_basic_integrated/qbe_basic tests/arithmetic/test_failing.bas > /tmp/test.s
   gcc /tmp/test.s fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o /tmp/test
   /tmp/test
   ```

3. **Inspect generated IL:**
   ```bash
   ./qbe_basic_integrated/qbe_basic -i tests/arithmetic/test_failing.bas > /tmp/test.qbe
   cat /tmp/test.qbe
   ```

---

## Quick Reference Card

### Build Commands
```bash
# Build compiler
./qbe_basic_integrated/build_qbe_basic.sh

# Clean build
rm -rf qbe_basic_integrated/obj/*.o qbe_basic_integrated/qbe_basic
./qbe_basic_integrated/build_qbe_basic.sh
```

### Compile & Run
```bash
# Complete workflow
./qbe_basic_integrated/qbe_basic program.bas > program.s
gcc program.s fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o program
./program

# Generate QBE IL
./qbe_basic_integrated/qbe_basic -i program.bas > program.qbe
```

### Testing
```bash
# Run all tests
./test_basic_suite.sh

# Run specific test
./qbe_basic_integrated/qbe_basic tests/arithmetic/test_not_basic.bas > /tmp/t.s
gcc /tmp/t.s fsh/FasterBASICT/runtime_c/*.c -I fsh/FasterBASICT/runtime_c -lm -o /tmp/t
/tmp/t
```

### Debugging
```bash
# View QBE IL
./qbe_basic_integrated/qbe_basic -i program.bas | less

# Check QBE errors
./qbe_basic_integrated/qbe_basic program.qbe > program.s 2>&1

# View assembly
./qbe_basic_integrated/qbe_basic program.bas 2>&1 | less
```

---

## Next Steps

1. **Try the examples** in the tests/ directory
2. **Read the documentation:**
   - [NOT_OPERATOR_IMPLEMENTATION.md](NOT_OPERATOR_IMPLEMENTATION.md)
   - [TEST_COVERAGE_EXPANSION.md](TEST_COVERAGE_EXPANSION.md)
   - [MATH_OPERATORS_STATUS.md](MATH_OPERATORS_STATUS.md)
3. **Experiment** with code generation
4. **Add new features** and write tests
5. **Contribute** improvements!

---

## Getting Help

- **Check test suite** for examples: `tests/`
- **Inspect QBE IL** with `-i` flag
- **Add debug output** to codegen or runtime
- **Create minimal test cases** to isolate issues

---

## Additional Resources

- **QBE IL Documentation:** https://c9x.me/compile/doc/il.html
- **QBE IL Tutorial:** https://c9x.me/compile/doc/il-v1.1.html
- **Project Documentation:** See `*.md` files in project root
- **⚠️ CRITICAL:** [docs/CRITICAL_IMPLEMENTATION_NOTES.md](docs/CRITICAL_IMPLEMENTATION_NOTES.md) — Essential reading for exception handling and array descriptor internals

---

**Happy Coding! 🚀**