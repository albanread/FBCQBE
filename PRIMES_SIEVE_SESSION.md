# Prime Sieve Implementation and CFG Debugging Session

**Date**: 2025-01-30  
**Objective**: Implement Sieve of Eratosthenes and analyze generated assembly efficiency  
**Status**: Completed with important compiler bug discovered and documented

---

## Summary

This session successfully:

1. ✅ Created a working prime sieve using the Sieve of Eratosthenes algorithm
2. ✅ Discovered and diagnosed a critical compiler bug with nested control flow
3. ✅ Implemented `--trace-cfg` (-G) diagnostic tool for CFG visualization
4. ✅ Analyzed generated ARM64 assembly for efficiency
5. ✅ Documented workarounds and created comprehensive bug report

---

## Prime Sieve Results

### Working Implementation

File: `tests/test_primes_sieve_working.bas`

**Performance**:
- Finds all primes up to 10,000
- Result: **1,229 primes** (correct!)
- Execution time: **0.254 seconds**
- Generated assembly: 587 lines

**Output**:
```
Found 1229 primes up to 10000
(Expected: 1229)

First 25 primes:
2 3 5 7 11 13 17 19 23 29 31 37 41 43 47 53 59 61 67 71 73 79 83 89 97

Last 10 primes up to 10000:
9973 9967 9949 9941 9931 9929 9923 9907 9901 9887
```

### Assembly Analysis

The generated ARM64 assembly is reasonably efficient:

**Good**:
- Proper register allocation (uses x19-x28, d8)
- Efficient array indexing with bounds checking
- Minimal function call overhead in tight loops
- Good instruction selection

**Could be optimized**:
- Loop conditions use floating-point comparisons (`scvtf`, `fcmpe`) instead of integer comparisons
- Some redundant register moves
- Array bounds checking adds overhead (but necessary for safety)

**Key assembly sequence (inner loop)**:
```asm
L14:
    scvtf   d0, x23          ; Convert loop counter to float
    fcmpe   d0, d1           ; Compare (could be integer cmp)
    bgt     L19              ; Branch if done
    sxtw    x25, w23         ; Sign extend for array index
    ; ... bounds checking ...
    str     w0, [x1]         ; sieve[j] = 0
    add     x23, x24, x23    ; j = j + i
```

---

## Critical Bug Discovered

### Issue: WHILE Loops Inside IF Statements

**Symptom**: Inner WHILE loop executes only **once** instead of looping until condition becomes false.

**Affected Code Pattern**:
```basic
WHILE i <= 3
    IF sieve(i) = 1 THEN
        WHILE j <= 10    ' ← Only runs once!
            sieve(j) = 0
            j = j + 2
        WEND
    END IF
    i = i + 1
WEND
```

**Root Cause**: Multi-line IF statements store their body as nested children of the IF AST node, not as sequential statements. The CFG builder doesn't recursively process these nested control-flow structures, so nested WHILE loops are invisible to the CFG builder and don't get proper loop headers or back-edges.

### CFG Diagnostic Tool

Implemented `./qbe_basic -G file.bas` to dump CFG structure before code generation.

**Example output showing the bug**:
```
Block 5 (WHILE Loop Body)
  [11] PRINT
  [12] IF (line 1120) - then:9 else:0  ← Inner WHILE trapped here!
  [13] LET/ASSIGNMENT
  [14] WEND (outer)
```

The IF shows `then:9`, meaning 9 statements nested inside (including the inner WHILE/WEND), but these don't appear as separate CFG blocks.

**Comparison with working nested WHILE**:
```
Block 3 (WHILE Loop Header)  ← Inner loop gets its own block
  [7] WHILE (inner)
  Successors: 4, 5

Block 4 (WHILE Loop Body)
  [8] PRINT
  [9] LET/ASSIGNMENT
  [10] WEND (inner)
  Successors: 3
```

---

## Workaround Used

To avoid the bug, restructure code using single-line `IF...THEN GOTO`:

**Before (broken)**:
```basic
WHILE i <= limit
    IF sieve(i) = 1 THEN
        j = i * i
        WHILE j <= 10000
            sieve(j) = 0
            j = j + i
        WEND
    END IF
    i = i + 1
WEND
```

**After (working)**:
```basic
WHILE i <= limit
    IF sieve(i) <> 1 THEN GOTO skip_marking
    
    j = i * i
    WHILE j <= 10000
        sieve(j) = 0
        j = j + i
    WEND
    
skip_marking:
    i = i + 1
WEND
```

Single-line IF with GOTO doesn't create nested statement structure, so the inner WHILE is processed normally by the CFG builder.

---

## Test Files Created

1. **`tests/test_primes_sieve.bas`** - Original broken version (demonstrates bug)
2. **`tests/test_primes_sieve_working.bas`** - Working version using GOTO workaround
3. **`tests/test_while_nested_simple.bas`** - Simple nested WHILE (works fine)
4. **`tests/test_while_if_nested.bas`** - Minimal reproduction of the bug
5. **`tests/test_while_array_bug.bas`** - Test case for array operations in nested loops

---

## Documentation Created

1. **`NESTED_WHILE_IF_BUG.md`** - Comprehensive bug report with:
   - Symptom description
   - Root cause analysis
   - CFG trace examples
   - Multiple workarounds
   - Proposed fixes
   - Test cases

2. **`PRIMES_SIEVE_SESSION.md`** - This document

---

## Key Findings

### What Works

✅ Simple nested WHILE loops (no IF)  
✅ FOR loops (likely has same issue but untested)  
✅ Single-line IF statements  
✅ WHILE loops with array operations  
✅ GOTO-based control flow  

### What Doesn't Work

❌ WHILE inside multi-line IF  
❌ Likely FOR inside multi-line IF (untested)  
❌ Any nested control-flow inside IF body  

---

## Required Fixes

Three possible approaches (in order of preference):

### Option A: Parser Flattening
Make the parser convert multi-line IF/ENDIF into sequential statements rather than nested children. Requires representing IF as multiple statement types (IF_START, IF_THEN, IF_ELSE, IF_END).

**Pros**: Clean separation, maintains CFG builder simplicity  
**Cons**: Larger parser change, affects AST structure

### Option B: Recursive CFG Processing
Modify `CFGBuilder::processIfStatement()` to recursively process nested control-flow statements (WHILE, FOR, DO, etc.).

**Pros**: Minimal code change  
**Cons**: Need to prevent double-processing, complexity in CFG builder

### Option C: AST Restructuring
Change how multi-line IF bodies are represented - use flat statement lists instead of nested arrays.

**Pros**: Cleaner AST model  
**Cons**: Large refactoring, affects many parts of compiler

---

## Tools Added

### CFG Trace Option

**Usage**: `./qbe_basic -G file.bas`

**Output**: Dumps complete CFG structure including:
- All basic blocks with IDs and labels
- Statement lists with global indices
- Successor edges (control flow)
- Loop header identification
- Statement line numbers
- Special structures (FOR, WHILE, DO loops)

**Location**: 
- Implementation: `qbe_basic_integrated/fasterbasic_wrapper.cpp` (`dumpCFG()`)
- Command-line: `qbe_basic_integrated/qbe_source/main.c` (`-G` option)

---

## Next Steps

### High Priority

1. **Fix the nested WHILE/IF bug** - Choose and implement one of the three fix strategies
2. **Add regression tests** to CI for nested control structures
3. **Test FOR loops** inside IF to confirm they have the same issue
4. **Document the -G flag** in help text and README

### Medium Priority

1. **Optimize loop condition checks** - Use integer comparisons instead of float
2. **Add more CFG diagnostic info** - Show edge types (conditional vs unconditional)
3. **Create CFG visualizer** - Generate GraphViz .dot files for visual debugging
4. **Profile the sieve** - Identify hotspots for further optimization

### Nice to Have

1. **Implement loop unrolling** for small fixed-iteration loops
2. **Add strength reduction** (replace multiplies with adds in loops)
3. **Better register allocation** in nested loops
4. **Optional bounds check elimination** for proven-safe array accesses

---

## Lessons Learned

1. **CFG visualization is essential** for debugging control-flow issues
2. **AST structure matters** - nested vs. flat affects code generation significantly
3. **Simple tests first** - nested WHILE without IF worked, helped isolate the problem
4. **Incremental debugging** - started complex (prime sieve), reduced to minimal case
5. **Documentation pays off** - clear bug reports help future contributors

---

## Impact Assessment

**User Impact**: Medium-High
- Common pattern (loop inside conditional)
- Has clear workaround (use GOTO)
- Documented with examples

**Code Quality**: High Priority Fix
- Breaks expected behavior
- Hard to debug without CFG trace tool
- Affects real-world algorithms

**Performance**: Low Impact
- Workaround has same performance
- Bug doesn't cause crashes, just incorrect logic

---

## References

- CFG Builder: `fsh/FasterBASICT/src/fasterbasic_cfg.cpp`
- Parser: `fsh/FasterBASICT/src/fasterbasic_parser.cpp`
- Bug Report: `NESTED_WHILE_IF_BUG.md`
- Working Sieve: `tests/test_primes_sieve_working.bas`
- CFG Trace Tool: `qbe_basic_integrated/fasterbasic_wrapper.cpp:dumpCFG()`
