# FasterBASIC Compiler Debugging Guide

This guide explains how to debug compilation issues using the FasterBASIC compiler toolchain.

## Quick Start

Use the `fbc` wrapper script for a user-friendly interface:

```bash
./fbc program.bas              # Compile to executable
./fbc program.bas --ast        # View Abstract Syntax Tree
./fbc program.bas --cfg        # View Control Flow Graph
./fbc program.bas --asm        # Generate assembly
```

## Compilation Pipeline

Understanding the compilation pipeline is crucial for debugging:

```
┌─────────┐    ┌─────────┐    ┌──────────┐    ┌─────┐    ┌──────────┐    ┌────────┐    ┌────────────┐
│ .bas    │ -> │ Lexer   │ -> │ Parser   │ -> │ AST │ -> │ Semantic │ -> │ CFG    │ -> │ QBE IL     │
│ Source  │    │ Tokens  │    │          │    │     │    │ Analysis │    │        │    │            │
└─────────┘    └─────────┘    └──────────┘    └─────┘    └──────────┘    └────────┘    └────────────┘
                                                                                               │
                                                                                               v
┌─────────────┐  ┌────────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────────────────┐
│ Executable  │<-│ Linker     │<-│ Assembler│<-│ Assembly │<-│ QBE Backend              │
│ a.out       │  │            │  │          │  │ .s       │  │                          │
└─────────────┘  └────────────┘  └──────────┘  └──────────┘  └──────────────────────────┘
```

## Debugging Tools

### 1. AST Dump (`--ast` or `-A`)

**What it shows:** The Abstract Syntax Tree - how the parser interpreted your source code.

**When to use:** 
- Parser errors
- Unexpected statement interpretation
- Verifying IF/THEN/ELSE structure
- Checking how loops are parsed

**Example:**
```bash
./fbc program.bas --ast
```

**Output:**
```
=== AST DUMP ===
Program with 4 lines

Line 10 (1 statements):
  STMT_DIM
Line 20 (1 statements):
  STMT_LET (variable=x)
Line 30 (1 statements):
  STMT_IF (hasGoto=0, thenStmts=1, elseIfClauses=0, elseStmts=0)
    THEN branch:
      STMT_PRINT (items=1)
Line 40 (1 statements):
  STMT_PRINT (items=1)
```

**What to look for:**
- **Statement counts:** `thenStmts=1` means the IF has 1 statement in THEN branch
- **Structure:** Nested statements should appear indented
- **Line numbers:** Verify they match your source

### 2. CFG Dump (`--cfg` or `-G`)

**What it shows:** The Control Flow Graph - how execution flows through your program.

**When to use:**
- Control flow bugs (wrong branches taken)
- GOSUB/GOTO issues
- Loop problems
- Code after END not executing

**Example:**
```bash
./fbc program.bas --cfg
```

**Output:**
```
MAIN PROGRAM CFG:
  Total blocks: 2

--------------------
Block 0 (Entry)
  Lines: 10, 20, 30, 40
  Statements:
    [0] DIM (line 10)
    [1] LET/ASSIGNMENT (line 20)
    [2] IF (line 30) - then:1 else:0
    [3] PRINT (line 40)
  Successors: 1, 1
```

**What to look for:**
- **Blocks:** Each block is a sequence of statements with one entry, one exit
- **Statements:** Which statements are in which blocks
- **Successors:** Where control flow goes next
  - `Successors: 1, 1` means both branches go to block 1
  - `Successors: 2, 3` means true branch → block 2, false branch → block 3
- **IF statement info:** `then:1 else:0` shows counts of THEN and ELSE statements

### 3. Symbol Table Dump (`--symbols` or `-S`)

**What it shows:** All variables, arrays, types, and their properties.

**When to use:**
- Type mismatch errors
- Variable not declared errors
- Array dimensioning issues

**Example:**
```bash
./fbc program.bas --symbols
```

### 4. QBE IL Output (`--il` or `-i`)

**What it shows:** The intermediate representation before assembly generation.

**When to use:**
- Understanding code generation
- Debugging optimizer issues
- Low-level control flow problems

**Example:**
```bash
./fbc program.bas --il -o program.qbe
cat program.qbe
```

### 5. Assembly Output (`--asm` or `-s`)

**What it shows:** The generated assembly language code.

**When to use:**
- Performance analysis
- Verifying code generation
- Understanding low-level behavior

**Example:**
```bash
./fbc program.bas --asm -o program.s
less program.s
```

## Common Debugging Scenarios

### Scenario 1: IF Statement Not Working

**Symptoms:** Condition appears to be ignored, wrong branch executes.

**Debug steps:**

1. **Check the AST:**
   ```bash
   ./fbc program.bas --ast
   ```
   Look for the IF statement. Verify:
   - Is `thenStmts` > 0?
   - Are the statements nested under "THEN branch"?
   - Is it marked as `isMultiLine=true` or `false`?

2. **Check the CFG:**
   ```bash
   ./fbc program.bas --cfg
   ```
   Look for:
   - Is the IF statement in a block?
   - What are the successors? Should be two different blocks for true/false branches
   - Are THEN statements in the CFG blocks or just in the AST?

3. **Check the assembly:**
   ```bash
   ./fbc program.bas --asm | grep -A 20 "jnz\|cbz\|cbnz"
   ```
   Look for conditional branch instructions. If there are none, the condition isn't being tested!

**Example fix:** If AST shows `thenStmts=1` but CFG shows no THEN statements in separate blocks, the issue is likely in CFG building (single-line IF handling).

### Scenario 2: GOSUB Doesn't Execute Subroutine

**Symptoms:** Subroutine after END never runs, or GOSUB goes to wrong place.

**Debug steps:**

1. **Check CFG for GOSUB targets:**
   ```bash
   ./fbc program.bas --cfg | grep -A 5 "GOSUB"
   ```
   Look for the GOSUB statement and its target line number.

2. **Check that target line is in a block:**
   ```bash
   ./fbc program.bas --cfg | grep "Line: 1000"
   ```
   If the line after END isn't in any block, it's unreachable!

3. **Verify CFG doesn't create fallthrough after END:**
   Look at the block containing END - it should have NO successors or only an exit successor.

### Scenario 3: Array Access Returns Wrong Values

**Symptoms:** Reading array element returns unexpected value.

**Debug steps:**

1. **Check array declaration in symbols:**
   ```bash
   ./fbc program.bas --symbols | grep -A 3 "myArray"
   ```
   Verify dimensions are correct.

2. **Add debug PRINT statements:**
   ```basic
   PRINT "i="; i; " j="; j; " arr(i,j)="; arr(i, j)
   ```

3. **Check if indices are in range:**
   Add bounds checking:
   ```basic
   IF i < 0 OR i > 10 THEN PRINT "Index i out of range: "; i
   ```

4. **Check the assembly for array access:**
   ```bash
   ./fbc program.bas --asm | grep -A 30 "loadl\|loadsw"
   ```
   Look for the offset calculation - should be `base + (i * rowsize + j) * elementsize`

### Scenario 4: Loop Doesn't Execute

**Symptoms:** FOR loop body never runs or runs wrong number of times.

**Debug steps:**

1. **Check CFG loop structure:**
   ```bash
   ./fbc program.bas --cfg
   ```
   Look for blocks marked `[LOOP HEADER]` and `[LOOP EXIT]`.

2. **Add debug output in loop:**
   ```basic
   FOR i = 1 TO 10
       PRINT "Loop iteration: "; i
       ' ... rest of loop
   NEXT i
   ```

3. **Check loop variable bounds:**
   ```basic
   PRINT "Starting loop: start="; startval; " end="; endval
   ```

## Direct Tool Usage (Advanced)

If you need more control than `fbc` provides, use `qbe_basic` directly:

### qbe_basic Options

```
-h              Print help
-o <file>       Output filename
-i              Stop at QBE IL generation
-s              Stop at assembly generation  
-c              Stop at object file generation
-A              Dump AST and exit
-G              Dump CFG and exit
-S              Dump symbol table and exit
-D              Enable debug mode
-t <target>     Target architecture (arm64_apple, amd64_sysv, etc.)
-d <flags>      QBE internal debug flags
```

### Examples

```bash
# Generate IL only
./qbe_basic -i input.bas > output.qbe

# Generate assembly only  
./qbe_basic -s input.bas > output.s

# Compile with debug output
./qbe_basic -D input.bas > output.s

# Target specific architecture
./qbe_basic -t amd64_sysv input.bas > output.s
```

## Environment Variables

For even deeper debugging, set these before compilation:

```bash
# Enable AST tracing
export TRACE_AST=1
./qbe_basic input.bas

# Enable CFG tracing  
export TRACE_CFG=1
./qbe_basic input.bas

# Enable symbol table tracing
export TRACE_SYMBOLS=1
./qbe_basic input.bas

# Combine multiple
export TRACE_AST=1 TRACE_CFG=1 TRACE_SYMBOLS=1
./qbe_basic input.bas
```

## Understanding the Three Types of IF

A common source of confusion is the difference between:

### 1. Single-line IF-THEN
```basic
IF x = 1 THEN PRINT "yes"
```
- **AST:** `isMultiLine=false`, `thenStmts=1`
- **CFG:** Statement kept in AST, emitted with conditional branch by codegen
- **Codegen:** Emits `jnz` with inline THEN/ELSE blocks

### 2. Multi-line IF-THEN-END IF
```basic
IF x = 1 THEN
    PRINT "yes"
    PRINT "indeed"
END IF
```
- **AST:** `isMultiLine=true`, `thenStmts=2`
- **CFG:** Creates separate blocks for THEN/ELSE branches with successor edges
- **Codegen:** Uses CFG successors for branching, emits nested statements from AST

### 3. IIF() Expression
```basic
result = IIF(x > 0, 10, -10)
```
- **AST:** `EXPR_IIF` (not a statement!)
- **CFG:** Not a statement, no CFG involvement
- **Codegen:** Emits conditional expression evaluation with phi nodes

## Tips and Best Practices

1. **Start with AST:** Always check AST first - if parser got it wrong, everything downstream will be wrong.

2. **Then CFG:** If AST is correct but behavior is wrong, check CFG to see how control flow was built.

3. **Finally Assembly:** Only look at assembly if AST and CFG are correct but output is still wrong.

4. **Use minimal test cases:** Reduce your program to the smallest example that shows the bug.

5. **Compare working vs broken:** If something works in one context but not another, compare their AST/CFG side-by-side.

6. **Add debug output:** Liberal use of PRINT statements in your BASIC code can help isolate issues.

7. **Check one stage at a time:** Don't try to debug AST, CFG, and codegen simultaneously.

## Example: Complete Debug Session

Let's debug why this program prints "YES" when it shouldn't:

```basic
10 DIM x AS INTEGER
20 x = 0
30 IF x = 1 THEN PRINT "YES"
40 PRINT "DONE"
```

**Step 1: Check AST**
```bash
./fbc test.bas --ast
```
Result: IF statement has `thenStmts=1` ✓ Correct!

**Step 2: Check CFG**
```bash
./fbc test.bas --cfg
```
Result: Block 0 contains IF with "then:1 else:0" and has successors to block 1 twice.
Issue: No separate blocks for THEN/ELSE! The PRINT("YES") should only execute conditionally.

**Step 3: Check Assembly**
```bash
./fbc test.bas --asm | grep -A 20 jnz
```
Result: No `jnz` instructions found! The conditional branch is missing.

**Diagnosis:** The code generator is not emitting conditional branches for single-line IF statements.

**Fix:** Check `emitIf()` in `qbe_codegen_statements.cpp` - ensure it emits `jnz` for single-line IFs.

## Getting Help

If you're stuck:

1. Create a minimal test case (< 10 lines)
2. Run through all debug stages (AST, CFG, assembly)
3. Document what you expected vs what you got at each stage
4. Check if there's a similar working example
5. Open an issue with your findings

## Common Error Messages

### "failed to compile BASIC file"
- Check syntax errors in your .bas file
- Try `--ast` to see if parsing succeeded

### "cannot open '-o'"
- Put flags before filename: `./qbe_basic -o output input.bas`
- Or use `fbc` wrapper which handles this correctly

### "Runtime library not found"
- The runtime C files must be in `fsh/FasterBASICT/runtime_c/`
- Use `fbc` which handles paths automatically

### "Semantic errors: Duplicate line number"
- You have two statements with the same line number
- Each line number must be unique
- Use `--ast` to see which lines are duplicated

## File Locations

- **Compiler:** `qbe_basic`
- **Wrapper:** `fbc`
- **Runtime:** `fsh/FasterBASICT/runtime_c/*.c`
- **Tests:** `tests/` directory
- **Source:** `fsh/FasterBASICT/src/`

## Rebuilding After Changes

If you modify compiler source:

```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
cp qbe_basic ..
cd ..
```

Now your `fbc` wrapper will use the updated compiler.