# Code Generator V2 - Immediate Action Plan

**Date:** February 1, 2026  
**Status:** Integration complete, fixing critical issues  
**Priority:** HIGH - String constant emission blocking all tests

---

## CRITICAL: Fix String Constant Emission

**Priority:** 🔴 **MUST FIX IMMEDIATELY**  
**Estimated Time:** 4-6 hours  
**Blocks:** All PRINT statements with literals, all string operations

### Problem

String constants are being emitted inside functions:

```qbe
export function w $main() {
    data $$str_0 = { b "Hello", b 0 }  # ❌ INVALID
    %t.0 =l call $fb_string_from_cstr(l $str_0)
```

QBE requires all `data` sections at global scope (outside functions).

### Solution Design

**Three-pass approach:**

#### Pass 1: String Collection
- Scan entire program before code generation
- Collect all string literals
- Assign unique labels (`$str_0`, `$str_1`, etc.)
- Store in string constant pool

#### Pass 2: Emit Global String Data
- Emit all string data sections at file start
- Before any function definitions
- Format: `data $str_N = { b "text", b 0 }`

#### Pass 3: Reference Strings in Code
- When emitting string expressions, use pre-defined labels
- No inline data sections
- Just reference: `call $fb_string_from_cstr(l $str_0)`

### Implementation Steps

1. **Add String Pool to QBEBuilder** (30 min)
   - `std::map<std::string, std::string> stringPool_`
   - `registerString(text) → label`
   - `emitStringPool()` method

2. **Update QBECodeGeneratorV2** (45 min)
   - Scan AST for string literals before codegen
   - Call `builder_->registerString()` for each
   - Call `builder_->emitStringPool()` after headers

3. **Update ASTEmitter** (30 min)
   - Remove inline string emission
   - Replace with string pool lookup
   - `emitStringLiteral(text) → label`

4. **Test** (1-2 hours)
   - Hello world
   - Multiple strings
   - Verify QBE accepts IL
   - Verify linking works
   - Verify execution works

### Files to Modify

- `fsh/FasterBASICT/src/codegen_v2/qbe_builder.h` (add string pool)
- `fsh/FasterBASICT/src/codegen_v2/qbe_builder.cpp` (implement methods)
- `fsh/FasterBASICT/src/codegen_v2/qbe_codegen_v2.cpp` (scan + emit)
- `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp` (use pool)

### Success Criteria

- ✅ No `data` sections inside functions
- ✅ All strings in global data section
- ✅ QBE compiles IL without errors
- ✅ `test_hello.bas` compiles to executable
- ✅ Executable runs and prints correct output

---

## HIGH PRIORITY: Implement Core Statement Handlers

**Priority:** 🟡 **HIGH**  
**Estimated Time:** 8-12 hours  
**After:** String fix complete

### 1. IF Statements (2-3 hours)

**Current:**
```cpp
case ASTNodeType::STMT_IF:
    builder_.emitComment("TODO: IF not yet implemented");
```

**Needed:**
- Emit condition evaluation
- Branch to true/false blocks
- Handle THEN clause
- Handle ELSE clause
- Handle ELSEIF chains

**Test:** Simple IF statements work

### 2. FOR Loops (3-4 hours)

**Current:**
```cpp
case ASTNodeType::STMT_FOR:
    builder_.emitComment("TODO: FOR loop not yet implemented");
```

**Needed:**
- Initialize loop variable
- Emit loop header block
- Emit loop condition check
- Emit loop body
- Emit STEP increment
- Emit loop exit

**Test:** Simple FOR loops work

### 3. WHILE Loops (2-3 hours)

**Similar to FOR but simpler**

**Needed:**
- Emit loop header
- Emit condition check
- Emit loop body
- Emit back-edge

**Test:** WHILE loops work

### 4. Variable Operations (1-2 hours)

**Fix:**
- Complete `emitLetStatement`
- Fix variable load/store
- Handle local vs global correctly

**Test:** Variable assignment works

---

## MEDIUM PRIORITY: Expression Evaluation

**Priority:** 🟢 **MEDIUM**  
**Estimated Time:** 6-8 hours  
**After:** Core statements working

### Binary Operators (3-4 hours)

- Addition, subtraction, multiplication, division
- Comparison operators
- Logical operators (AND, OR, NOT)
- Type coercion

### Function Calls (2-3 hours)

- Argument evaluation
- Call instruction
- Return value handling

### Array Access (1-2 hours)

- Index calculation
- Bounds checking (optional)
- Load/store

---

## Test-Driven Approach

### Milestone 1: Hello World ✅
```basic
PRINT "Hello World"
END
```

**Status:** BLOCKED by string constant issue

### Milestone 2: Variables
```basic
DIM X AS INTEGER
X = 42
PRINT X
END
```

### Milestone 3: Arithmetic
```basic
DIM A, B, C AS INTEGER
A = 10
B = 20
C = A + B
PRINT C
END
```

### Milestone 4: IF Statement
```basic
DIM X AS INTEGER
X = 5
IF X > 3 THEN
    PRINT "Greater"
ELSE
    PRINT "Less"
END IF
END
```

### Milestone 5: FOR Loop
```basic
DIM I AS INTEGER
FOR I = 1 TO 10
    PRINT I
NEXT I
END
```

---

## Recommended Work Order

1. **TODAY: Fix string constants** (4-6 hours)
   - Get hello world working end-to-end
   - This unblocks all other work

2. **DAY 2: Variables + arithmetic** (4-6 hours)
   - LET statements
   - Binary operators
   - PRINT variables
   - Test milestone 2 & 3

3. **DAY 3: IF statements** (3-4 hours)
   - Condition evaluation
   - Branching
   - ELSE clause
   - Test milestone 4

4. **DAY 4: FOR loops** (3-4 hours)
   - Loop initialization
   - Loop body
   - STEP handling
   - Test milestone 5

5. **DAY 5: Test suite** (4-6 hours)
   - Run all 125 tests
   - Document results
   - Prioritize next fixes

---

## Quick Wins (After String Fix)

### 1. END Statement ✅
Already works - calls `exit(0)`

### 2. PRINT Newline ✅
Already works - `fb_print_newline()`

### 3. Comments ✅
Already works - emitted as QBE comments

### 4. Empty Programs ✅
Already works - generates minimal main

---

## Debug Tools Needed

### 1. IL Dumper
```bash
./fbc_qbe --emit-qbe test.bas > test.qbe
cat test.qbe  # View generated IL
```

### 2. CFG Visualizer
Already exists - dumps CFG to stderr

### 3. Step-by-Step Trace
Add `--trace-codegen` flag to show:
- Which block being emitted
- Which statement being processed
- What IL being generated

---

## Success Metrics

### Week 1 Goal
- [ ] String constants fixed
- [ ] Hello world compiles and runs
- [ ] Variables work
- [ ] Arithmetic works
- [ ] 10+ simple tests pass

### Week 2 Goal
- [ ] IF statements work
- [ ] FOR loops work
- [ ] Functions work (basic)
- [ ] 30+ tests pass
- [ ] Core language features operational

### Week 3 Goal
- [ ] All control structures work
- [ ] Arrays work
- [ ] 60+ tests pass
- [ ] Ready for broader testing

### Month 1 Goal
- [ ] 90%+ test suite passes
- [ ] Performance acceptable
- [ ] Documentation complete
- [ ] Production ready

---

## Risk Mitigation

### Risk: Runtime Library Missing Functions
**Mitigation:** Check runtime library first, add stubs as needed

### Risk: Type System Incompatibilities
**Mitigation:** Start simple (INTEGER only), add types incrementally

### Risk: QBE IL Syntax Issues
**Mitigation:** Keep QBE reference open, validate IL frequently

### Risk: Time Overruns
**Mitigation:** Focus on critical path, defer optimizations

---

## Questions to Answer

1. ✅ Does code generator compile? **YES**
2. ✅ Does it generate IL? **YES**
3. ⏳ Does IL compile? **NO - string issue**
4. ⏳ Does executable run? **BLOCKED**
5. ⏳ Does it produce correct output? **BLOCKED**

Next questions after string fix:
6. Do variables work?
7. Do expressions work?
8. Do control structures work?

---

## Contact / Support

- CFG documentation: `CFG_V2_COMPLETION_STATUS.md`
- Codegen documentation: `CODEGEN_V2_READY.md`
- Integration status: `CODEGEN_V2_INTEGRATION_STATUS.md`
- Thread history: Agent thread "CFG v2 Unreachable Warnings and Codegen"

---

## Command Reference

```bash
# Build compiler
cd qbe_basic_integrated
./build_qbe_basic.sh

# Compile BASIC program
./fbc_qbe test.bas -o test

# View generated IL (after fixing wrapper)
./fbc_qbe --emit-qbe test.bas

# Run tests
cd ../tests
for f in *.bas; do ../qbe_basic_integrated/fbc_qbe "$f" || echo "FAIL: $f"; done

# Debug build
./build_qbe_basic.sh 2>&1 | grep error

# Check executable
ls -lh fbc_qbe
file fbc_qbe
```

---

**NEXT ACTION: Fix string constant emission. This is the only blocker preventing end-to-end testing.**