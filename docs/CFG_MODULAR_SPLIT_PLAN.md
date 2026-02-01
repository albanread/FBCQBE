# CFG Modular Split - Implementation Plan

**Date:** February 1, 2026  
**Status:** READY TO EXECUTE  
**Approach:** Incremental, tested, committed splits of working code

---

## Overview

Split the current working CFG code (`fasterbasic_cfg.cpp`, 2206 lines) into logical, maintainable modules. Each split will be tested and committed before proceeding to the next.

**Key Principle:** Never break the build. Every step must compile and pass tests.

---

## Current Code Structure

### File: `fsh/FasterBASICT/src/fasterbasic_cfg.cpp` (2206 lines)

```
Lines    Section                        Purpose
------   ---------------------------    ----------------------------------------
18-35    Constructor/Destructor         Initialization and cleanup
37-93    Main Build Entry Point         build() - orchestrates CFG construction
95-188   Phase 0: Jump Target Scan      Pre-scan for GOTO/GOSUB targets
190-262  Phase 1: Build Basic Blocks    Create blocks from statements
264-1293 Statement Processing           Process each statement type
         - processStatement()           Main dispatcher
         - processGotoStatement()       GOTO handling
         - processGosubStatement()      GOSUB handling
         - processOnGoto/OnGosub()      ON GOTO/GOSUB
         - processIfStatement()         IF/THEN/ELSE
         - processNestedStatements()    Nested structure handler
         - processForStatement()        FOR loops
         - processWhileStatement()      WHILE loops
         - processRepeatStatement()     REPEAT loops
         - processDoStatement()         DO loops
         - processCaseStatement()       SELECT CASE
         - processTryCatchStatement()   TRY/CATCH/FINALLY
         - processFunctionStatement()   FUNCTION definitions
         - processDefStatement()        DEF FN
         - processSubStatement()        SUB definitions
1295-1956 Phase 2: Build Edges          Wire up control flow edges
1958-2042 Phase 3: Identify Loops       Loop detection and metadata
2044-2058 Phase 4: Identify Subroutines Subroutine analysis
2060-2070 Phase 5: Optimize CFG         Optimizations
2072-2113 Block Management              Block and edge creation helpers
2119-2205 Utility Functions             Reports, type inference
```

---

## Proposed Module Structure

### Phase 1: Extract Core Infrastructure (Files 1-3)

```
cfg/
├── cfg_builder.h              # Main class declaration (copy from fasterbasic_cfg.h)
├── cfg_builder_core.cpp       # Constructor, build(), main entry point
├── cfg_builder_blocks.cpp     # Block creation and management
└── cfg_builder_utils.cpp      # Utility functions (reports, type inference)
```

### Phase 2: Extract Statement Processing (Files 4-7)

```
cfg/
├── cfg_builder_statements.cpp    # processStatement() dispatcher
├── cfg_builder_conditional.cpp   # IF, SELECT CASE
├── cfg_builder_loops.cpp         # FOR, WHILE, REPEAT, DO
└── cfg_builder_functions.cpp     # FUNCTION, DEF FN, SUB
```

### Phase 3: Extract Jump Handling (Files 8-9)

```
cfg/
├── cfg_builder_jumps.cpp      # GOTO, GOSUB, ON GOTO/GOSUB
└── cfg_builder_exception.cpp  # TRY/CATCH/FINALLY, THROW
```

### Phase 4: Extract Edge Building (File 10)

```
cfg/
└── cfg_builder_edges.cpp      # buildEdges(), loop/subroutine analysis
```

---

## Implementation Steps (Detailed)

### Step 1: Create cfg/ Directory and Header

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder.h`

**Actions:**
1. Create `cfg/` directory
2. Copy `fasterbasic_cfg.h` → `cfg/cfg_builder.h`
3. Update include guards
4. Keep all class declarations (no changes to API yet)

**Build changes:** None (header only)

**Test:** Verify no compile errors

**Commit:** "CFG Split Step 1: Create cfg/ directory and header"

---

### Step 2: Extract Core (Constructor, build())

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_core.cpp`

**Extract lines:**
- Lines 1-17: Header comments
- Lines 18-35: Constructor/Destructor
- Lines 37-93: Main build() entry point

**Dependencies:** Calls `collectJumpTargets()`, `buildBlocks()`, `buildEdges()`, etc.

**Build changes:**
```bash
# Add to build_qbe_basic.sh after fasterbasic_semantic.cpp:
"$FASTERBASIC_SRC/cfg/cfg_builder_core.cpp" \
```

**Test:**
```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
# Should fail with undefined references - expected!
```

**Don't commit yet** - not functional alone

---

### Step 3: Extract Block Management

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_blocks.cpp`

**Extract lines:**
- Lines 2072-2113: Block creation functions
  - `createNewBlock()`
  - `finalizeBlock()`
  - `addFallthroughEdge()`
  - `addConditionalEdge()`
  - `addUnconditionalEdge()`
  - `addCallEdge()`
  - `addReturnEdge()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_blocks.cpp" \
```

**Test:** Still won't link (missing other functions)

**Don't commit yet**

---

### Step 4: Extract Utilities

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_utils.cpp`

**Extract lines:**
- Lines 2119-2205: Utility functions
  - `generateReport()`
  - `inferTypeFromName()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_utils.cpp" \
```

**Test:** Still missing most functions

**Don't commit yet**

---

### Step 5: Extract Jump Target Collection

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_jumptargets.cpp`

**Extract lines:**
- Lines 95-188: Jump target scanning
  - `collectJumpTargetsFromStatements()` (helper function)
  - `collectJumpTargets()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_jumptargets.cpp" \
```

**Test:** Getting closer to linking

---

### Step 6: Extract Statement Dispatcher

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_statements.cpp`

**Extract lines:**
- Lines 190-262: `buildBlocks()`
- Lines 264-502: `processStatement()` - main dispatcher
- Lines 640-698: `processNestedStatements()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_statements.cpp" \
```

**Test:** Still missing individual statement processors

---

### Step 7: Extract Jump Handlers

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_jumps.cpp`

**Extract lines:**
- Lines 504-510: `processGotoStatement()`
- Lines 512-530: `processGosubStatement()`
- Lines 532-539: `processOnGotoStatement()`
- Lines 541-548: `processOnGosubStatement()`
- Lines 550-554: `processLabelStatement()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_jumps.cpp" \
```

**Test:** Closer

---

### Step 8: Extract Conditional Handlers

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_conditional.cpp`

**Extract lines:**
- Lines 556-637: `processIfStatement()`
- Lines 816-878: `processCaseStatement()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_conditional.cpp" \
```

**Test:** More functions available

---

### Step 9: Extract Loop Handlers

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_loops.cpp`

**Extract lines:**
- Lines 700-758: `processForStatement()`
- Lines 760-780: `processForInStatement()`
- Lines 782-814: `processWhileStatement()`
- Lines 984-1001: `processRepeatStatement()`
- Lines 1003-1044: `processDoStatement()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_loops.cpp" \
```

**Test:** Getting very close

---

### Step 10: Extract Exception Handler

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_exception.cpp`

**Extract lines:**
- Lines 880-982: `processTryCatchStatement()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_exception.cpp" \
```

**Test:** Almost there

---

### Step 11: Extract Function/Sub Handlers

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_functions.cpp`

**Extract lines:**
- Lines 1046-1138: `processFunctionStatement()`
- Lines 1140-1212: `processDefStatement()`
- Lines 1214-1293: `processSubStatement()`

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_functions.cpp" \
```

**Test:** Should be very close to linking now

---

### Step 12: Extract Edge Builder

**Files to create:**
- `fsh/FasterBASICT/src/cfg/cfg_builder_edges.cpp`

**Extract lines:**
- Lines 1295-1956: `buildEdges()` - wire control flow
- Lines 1958-2042: `identifyLoops()` - loop analysis
- Lines 2044-2058: `identifySubroutines()` - subroutine analysis
- Lines 2060-2070: `optimizeCFG()` - optimizations

**Build changes:**
```bash
# Add to build_qbe_basic.sh:
"$FASTERBASIC_SRC/cfg/cfg_builder_edges.cpp" \
```

**Test:** Should compile and link successfully now!

---

### Step 13: Remove Old Files and Test

**Actions:**
1. Remove `fasterbasic_cfg.cpp` from build script
2. Remove `fasterbasic_cfg.h` from build script
3. Update any includes from `fasterbasic_cfg.h` → `cfg/cfg_builder.h`

**Build changes:**
```bash
# In build_qbe_basic.sh, REMOVE:
# "$FASTERBASIC_SRC/fasterbasic_cfg.cpp" \

# Make sure all cfg/*.cpp files are there
```

**Test:**
```bash
cd qbe_basic_integrated
./build_qbe_basic.sh
# Should compile successfully

# Run smoke test
./qbe_basic ../tests/loops/test_nested_while_if.bas
# Should work as before
```

**Commit:** "CFG Split Complete: Modular structure with all tests passing"

---

## Build Script Template

Final `build_qbe_basic.sh` CFG section should look like:

```bash
clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -I"$FASTERBASIC_SRC/../runtime" -c \
    "$FASTERBASIC_SRC/fasterbasic_lexer.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_parser.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_semantic.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_core.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_blocks.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_utils.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_jumptargets.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_statements.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_jumps.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_conditional.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_loops.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_exception.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_functions.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_edges.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_data_preprocessor.cpp" \
    "$FASTERBASIC_SRC/fasterbasic_ast_dump.cpp" \
    "$FASTERBASIC_SRC/modular_commands.cpp" \
    "$FASTERBASIC_SRC/command_registry_core.cpp" \
    "$FASTERBASIC_SRC/../runtime/ConstantsManager.cpp"
```

---

## Testing Strategy

### After Each File Split

**Compile test:**
```bash
cd qbe_basic_integrated
./build_qbe_basic.sh 2>&1 | tee build.log
# Check for errors
```

**Link test (after all files added):**
```bash
# Should produce executable
ls -lh qbe_basic
```

### After Complete Split

**Smoke tests:**
```bash
# Test 1: Simple program
echo '10 PRINT "Hello"' > /tmp/test.bas
./qbe_basic /tmp/test.bas -o /tmp/test
/tmp/test

# Test 2: Nested control flow (known working)
./qbe_basic ../tests/loops/test_nested_while_if.bas -o /tmp/nested
/tmp/nested

# Test 3: All nested tests
for f in ../tests/loops/test_nested_*.bas; do
    echo "Testing $f..."
    ./qbe_basic "$f" -o /tmp/test || echo "COMPILE FAILED"
    /tmp/test && echo "PASS" || echo "FAIL"
done
```

**Regression test:**
```bash
# Compare behavior before/after split
# Should be identical (no behavior changes)
```

---

## Success Criteria

### For Each Step
- ✅ Code compiles (may have link errors until complete)
- ✅ No new warnings
- ✅ Code moved verbatim (no logic changes)

### For Complete Split
- ✅ Full compilation success
- ✅ All object files created
- ✅ qbe_basic executable built
- ✅ Smoke tests pass
- ✅ All nested loop tests behave identically
- ✅ No regressions in existing functionality
- ✅ Commit created with clear message

---

## Rollback Plan

If anything goes wrong:

```bash
# Restore from git
git checkout fsh/FasterBASICT/src/fasterbasic_cfg.cpp
git checkout fsh/FasterBASICT/src/fasterbasic_cfg.h
git checkout qbe_basic_integrated/build_qbe_basic.sh

# Remove new directory
rm -rf fsh/FasterBASICT/src/cfg/

# Rebuild
cd qbe_basic_integrated
./build_qbe_basic.sh
```

---

## Notes

### Important Points

1. **Do NOT change logic during split** - pure code movement only
2. **Keep includes minimal** - only what each file needs
3. **Maintain namespace** - all in `namespace FasterBASIC`
4. **Copy includes** - each .cpp needs `cfg_builder.h`
5. **Test frequently** - compile after adding each file

### Common Pitfalls

- ❌ Forgetting to add file to build script
- ❌ Circular include dependencies
- ❌ Missing include directives
- ❌ Typos in file paths
- ❌ Wrong namespace or missing namespace

### Tips

- Keep the original file open for reference
- Copy exact line ranges (use `sed -n 'X,Yp'`)
- Add all standard includes to each file
- Verify line counts match
- Test compile after each new file

---

## Timeline Estimate

**Total time:** 3-4 hours for careful, tested execution

- Step 1 (header): 10 min
- Steps 2-12 (file extraction): 15 min each × 11 = 165 min
- Step 13 (cleanup and test): 30 min
- Testing and verification: 30 min
- Commit and documentation: 15 min

**Not a race - accuracy matters more than speed**

---

## After Split Complete

Once we have modular structure working:

1. **Commit immediately** - preserve working state
2. **Review split** - ensure clean separation
3. **Document each module** - add file header comments
4. **Plan improvements** - now we can safely refactor
5. **Apply v2 patterns** - incrementally, one at a time

---

## Next Phase Preview

After modular split is complete and tested:

**Phase 2: Add Context Structures**
- Add LoopContext, SelectContext, TryContext (from v2)
- Refactor to pass contexts instead of using stacks
- Test and commit each change

**Phase 3: Single-Pass Conversion**
- Convert one loop type at a time
- Eliminate two-phase approach
- Fix REPEAT/DO infinite loop bugs

**But first:** Complete the split. One step at a time.

---

**Ready to execute:** Yes
**Next action:** Create cfg/ directory and extract header
**Expected duration:** 3-4 hours total
**Risk level:** Low (pure code movement, no logic changes)