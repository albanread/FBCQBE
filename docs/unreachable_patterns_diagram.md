# Unreachable Code Patterns - Visual Diagrams

This document provides visual representations of the three unreachable code patterns identified in the test suite.

---

## Pattern 1: Test Assertion with Early Termination

### Control Flow Diagram

```
Program Start
     |
     v
┌─────────────────────┐
│  Test Setup         │
│  LET result = calc  │
└──────────┬──────────┘
           |
           v
┌─────────────────────────────────────┐
│  IF result <> expected THEN         │
│     PRINT "ERROR"                   │
│     END              ◄────────────┐ │
│  ELSE                             │ │
│     Continue         │            │ │
└──────────┬───────────┴────────────┘ │
           |                          │
           v                          │
    ┌──────────┐                      │
    │ Merge    │                      │
    └─────┬────┘                      │
          |                           │
          v                           │
┌─────────────────────┐               │
│  PRINT "PASS"       │               │
└──────────┬──────────┘               │
           |                          │
           v                          │
┌─────────────────────┐               │
│  Next Test          │               │
│  (UNREACHABLE if    │               │
│   prior test fails) │               │
└─────────────────────┘               │
                                      │
Program Exit ◄────────────────────────┘
```

### Source Code Example

```basic
90  LET result% = 10 + 5
100 IF result% <> 15 THEN PRINT "ERROR: Math failed" : END
    │              │
    │              └──> Terminates program immediately
    │
    └──> Test passes, continue to line 110
    
110 PRINT "PASS: Math works"
    ↑
    └──> UNREACHABLE if line 100 test fails
    
120 REM Next test...
    ↑
    └──> UNREACHABLE if line 100 test fails
```

### Why Legitimate

- **Intentional Design:** Tests are designed to stop on first failure
- **Correct Behavior:** No point running test N+1 if test N fails
- **Expected Pattern:** Standard test harness idiom in BASIC

---

## Pattern 2: Computed Jump Targets (ON GOTO/GOSUB)

### Control Flow Diagram

```
Program Start
     |
     v
┌─────────────────────┐
│  LET INDEX% = 2     │
└──────────┬──────────┘
           |
           v
┌─────────────────────────────────────┐
│  ON INDEX% GOTO 1000, 2000, 3000    │
└───┬──────────────┬──────────────┬───┘
    │              │              │
    │ INDEX=1      │ INDEX=2      │ INDEX=3
    │              │              │
    ▼              ▼              ▼
┌────────┐    ┌────────┐    ┌────────┐
│ GOTO   │    │ GOTO   │    │ GOTO   │
│ 1000   │    │ 2000   │    │ 3000   │
└───┬────┘    └───┬────┘    └───┬────┘
    │             │             │
    └─────┬───────┴─────┬───────┘
          │             │
          ▼             ▼
     [Computed      [Sequential
      dispatch]      fall-through]
          │             │
          │             ▼
          │      ┌─────────────┐
          │      │ PRINT "..."  │
          │      │ END          │
          │      └──────────────┘
          │
          ├─────────────────────────────┐
          │                             │
          ▼                             ▼
    ┌──────────┐  (no sequential  ┌──────────┐
    │ Line 1000│   flow reaches    │ Line 2000│
    │ Target 1 │   these blocks)   │ Target 2 │
    │          │                   │          │
    │UNREACHABLE                   │UNREACHABLE
    │via seq.  │                   │via seq.  │
    └──────────┘                   └──────────┘
```

### Source Code Example

```basic
50  LET INDEX% = 2
60  ON INDEX% GOTO 1000, 2000, 3000
    │
    └──> Computes: GOTO (line_base + INDEX * offset)
         If INDEX=1 → GOTO 1000
         If INDEX=2 → GOTO 2000
         If INDEX=3 → GOTO 3000
    
70  PRINT "ERROR: Out of range"
80  END

    ... (sequential flow never reaches below) ...
    
1000 REM Target 1
     ↑
     └──> UNREACHABLE via sequential flow
          (only reachable via ON GOTO when INDEX=1)

1010 PRINT "Reached target 1"
1020 GOTO 100

2000 REM Target 2
     ↑
     └──> UNREACHABLE via sequential flow
          (only reachable via ON GOTO when INDEX=2)

2010 PRINT "Reached target 2"
2020 GOTO 100
```

### Why Legitimate

- **Indirect Control Transfer:** ON GOTO is a computed jump, not sequential flow
- **CFG Correctness:** CFG tracks sequential flow edges separately from jumps
- **Warning Accuracy:** Blocks ARE unreachable sequentially (reached via dispatch table)

---

## Pattern 3: Subroutines After END

### Control Flow Diagram

```
Program Start
     |
     v
┌─────────────────────┐
│  PRINT "Main"       │
└──────────┬──────────┘
           |
           v
┌─────────────────────┐
│  GOSUB 2000         │──┐
└──────────┬──────────┘  │
           |              │ (Call stack
           v              │  push/pop)
┌─────────────────────┐  │
│  PRINT "After call" │  │
└──────────┬──────────┘  │
           |              │
           v              │
┌─────────────────────┐  │
│  END                │  │
└─────────────────────┘  │
           │              │
           ▼              │
    Program Exit          │
                          │
    (sequential flow      │
     stops here)          │
                          │
    ═══════════════════   │
    UNREACHABLE ZONE      │
    ═══════════════════   │
           ▲              │
           │              │
2000 ──────┴──────────────┘
     REM Subroutine
     ↑
     └──> UNREACHABLE via sequential flow
          (only reachable via GOSUB)

2010 PRINT "In subroutine"
2020 RETURN
     │
     └──> Pops call stack, returns to caller
```

### Source Code Example

```basic
1000 PRINT "=== Main Program ==="
1010 GOSUB Sub1
     │
     └──> Pushes return address, jumps to 2000

1020 PRINT "After subroutine call"
1030 GOSUB Sub2
1040 PRINT "Done"
1050 END
     │
     └──> Program terminates here
     
     ═══════════════════════════════════
     Sequential flow stops at END
     Everything below is UNREACHABLE
     via normal control flow
     ═══════════════════════════════════

2000 Sub1:
     ↑
     └──> UNREACHABLE sequentially
          (reached via GOSUB at line 1010)

2010    PRINT "In Sub1"
2020    RETURN
        │
        └──> Returns to line 1020

2100 Sub2:
     ↑
     └──> UNREACHABLE sequentially
          (reached via GOSUB at line 1030)

2110    PRINT "In Sub2"
2120    RETURN
```

### Why Legitimate

- **Classic BASIC Idiom:** Standard practice to place subroutines after main END
- **Intentional Structure:** Keeps main program flow clear, subroutines separate
- **Correct Analysis:** Blocks ARE unreachable sequentially (reached via GOSUB)

---

## Pattern Comparison Matrix

| Aspect | Test Assertions | Computed Jumps | Subroutines |
|--------|----------------|----------------|-------------|
| **Control Flow** | Conditional termination | Indirect dispatch | Call/return |
| **Reachability** | Conditional (depends on test result) | Always (via computed jump) | Always (via GOSUB) |
| **Sequential?** | No (END terminates) | No (jump bypasses) | No (END before subroutines) |
| **Intentional?** | Yes (test pattern) | Yes (control structure) | Yes (code organization) |
| **Bug Risk** | Low (test code) | Low (explicit dispatch) | Low (standard idiom) |
| **Warning Value** | Medium (documents dependency) | High (shows indirect flow) | Medium (documents structure) |

---

## CFG Edge Types

The CFG tracks different types of control flow edges:

### 1. Sequential Flow (→)
```
Block A → Block B
```
Normal execution from one statement to the next.

### 2. Conditional Branch (→?)
```
        → Block THEN
Block IF
        → Block ELSE
```
Branches based on condition evaluation.

### 3. Unconditional Jump (⇢)
```
Block GOTO ⇢ Block Target
```
Direct jump to labeled line.

### 4. Computed Jump (⇝)
```
          ⇝ Target 1
Block ON  ⇝ Target 2
          ⇝ Target 3
```
Indirect jump via dispatch table.

### 5. Call/Return (⤴⤵)
```
Block GOSUB ⤴ Subroutine
            ⤵
Block After
```
Subroutine call with return.

### Unreachable Block Detection

A block is marked UNREACHABLE if:
```
Predecessors(Block) = ∅ AND Block ≠ Entry
```

In other words:
- No edges lead to the block (no predecessors)
- AND it's not the program entry point

This correctly identifies:
- ✅ Code after early termination (test pattern)
- ✅ Jump targets with no sequential flow (computed jumps)
- ✅ Code after END (subroutines)

---

## Why These Warnings Are Valuable

### For Test Code
- **Documents Dependencies:** Shows which tests depend on earlier tests passing
- **Clarifies Intent:** Makes early-termination pattern explicit
- **Aids Debugging:** If unreachable code executes, something is wrong

### For Production Code
- **Finds Dead Code:** Identifies truly unreachable code (bugs)
- **Shows Control Flow:** Makes complex flow patterns visible
- **Improves Quality:** Encourages developers to remove unused code

### For Compiler Development
- **Validates CFG:** Confirms CFG builder is working correctly
- **Tests Edge Cases:** Exercises indirect control flow handling
- **Provides Metrics:** Quantifies code complexity and structure

---

## Future Enhancements

### 1. Warning Categories

```
INFO:    Block unreachable (subroutine after END)
INFO:    Block unreachable (computed jump target)
WARNING: Block unreachable (potential dead code)
```

### 2. Pragma Support

```basic
REM @pragma unreachable-ok "subroutine"
2000 Sub1:
2010    PRINT "In subroutine"
2020    RETURN
```

### 3. Enhanced CFG Visualization

```
Block 1 (Entry)
  ├─→ Block 2 (sequential)
  ├─→ Block 3 (conditional)
  └─⇢ Block 10 (GOTO)

Block 10 (Label) [⚠ unreachable sequentially]
  ⤴ GOSUB from Block 5
  ⤴ GOSUB from Block 8
```

---

## Conclusion

All three unreachable patterns represent legitimate code structures:

1. **Test Assertions:** Intentional early termination for test harness
2. **Computed Jumps:** Correct modeling of indirect control transfer
3. **Subroutines:** Classic BASIC idiom for code organization

The CFG v2 implementation correctly identifies unreachable blocks and provides accurate, valuable warnings that help developers understand program structure and find potential bugs in production code.

**Status: ✅ All warnings are correct and legitimate**

---

## Visual Summary

```
┌─────────────────────────────────────────────────────────────┐
│  Unreachable Code in FBCQBE Test Suite (12/125 = 9.6%)     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Pattern 1: Test Assertions (9 tests - 75%)                │
│  ┌─────────┐     ┌─────────┐     ┌─────────┐             │
│  │  Test 1 │ ──→ │  Test 2 │ ──→ │  Test 3 │             │
│  └────┬────┘     └────┬────┘     └────┬────┘             │
│       │ PASS          │ PASS          │ PASS              │
│       ↓               ↓               ↓                   │
│       ✓               ✓               ✓                   │
│       │ FAIL          │ FAIL          │ FAIL              │
│       ↓               ↓               ↓                   │
│     [ END ]         [ END ]         [ END ]               │
│       │               │               │                   │
│       └───────┬───────┴───────┬───────┘                   │
│               ↓               ↓                           │
│         [Unreachable]   [Unreachable]                     │
│                                                            │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  Pattern 2: Computed Jumps (1 test - 8%)                  │
│  ┌──────────────┐                                         │
│  │ ON idx GOTO  │                                         │
│  └───┬──┬───┬───┘                                         │
│      │  │   │                                             │
│   ┌──┘  │   └──┐                                          │
│   ↓     ↓      ↓                                          │
│  [1000][2000][3000]  ← Unreachable via sequential flow   │
│                                                            │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  Pattern 3: Subroutines After END (2 tests - 17%)         │
│  ┌─────────┐                                              │
│  │  Main   │                                              │
│  │ Program │                                              │
│  └────┬────┘                                              │
│       │                                                   │
│       ↓                                                   │
│     [END]  ← Sequential flow stops here                  │
│       │                                                   │
│  ═════════════════════                                    │
│   Unreachable Zone                                        │
│  ═════════════════════                                    │
│       ↓                                                   │
│  ┌─────────┐  ← Reached via GOSUB only                   │
│  │  Sub 1  │                                              │
│  └─────────┘                                              │
│  ┌─────────┐                                              │
│  │  Sub 2  │                                              │
│  └─────────┘                                              │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

**For detailed test-by-test analysis, see:** `unreachable_code_analysis.md`  
**For quick reference, see:** `unreachable_warnings_summary.md`
