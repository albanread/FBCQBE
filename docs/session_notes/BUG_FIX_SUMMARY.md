# Bug Fix Summary: GOSUB/RETURN in Multiline IF Blocks

## Problem
When GOSUB was called from within a multiline IF...END IF block, the RETURN statement would jump to after the END IF instead of continuing with statements after the GOSUB within the IF block.

## Root Cause
In `fasterbasic_cfg.cpp`, the `buildEdges()` function was incorrectly calculating the return continuation block for GOSUB statements using `block->id + 1`. This assumption breaks when GOSUB is inside control structures (IF, WHILE, etc.) because:

1. `processGosubStatement()` creates a new block for the return point
2. Due to how nested statements are processed, this new block may not be sequential
3. The buildEdges function assumed the next block ID would be sequential

Example:
- Block 1 (IF THEN) contains GOSUB
- processGosubStatement creates Block 3 for statements after GOSUB
- Block 2 is created for ELSE branch
- buildEdges tried to connect Block 1 → Block 2 (wrong!)
- Should connect Block 1 → Block 3 (correct!)

## Solution
Added a map (`m_gosubReturnMap`) to track the relationship between GOSUB blocks and their return continuation blocks:

### Changes Made

**File: `fsh/FasterBASICT/src/fasterbasic_cfg.h`**
- Added member variable: `std::map<int, int> m_gosubReturnMap;`

**File: `fsh/FasterBASICT/src/fasterbasic_cfg.cpp`**

1. In `processGosubStatement()` (line ~517):
   - Record the mapping: `m_gosubReturnMap[currentBlock->id] = nextBlock->id;`

2. In `buildEdges()` GOSUB case (line ~1478):
   - Changed from: `addFallthroughEdge(block->id, block->id + 1);`
   - Changed to: Use the recorded return block from the map

## Test Results

### Before Fix
```
A
S
C
```
(Missing "B" - the statement after GOSUB)

### After Fix
```
A
S
B
C
```
(All statements execute correctly)

## Rosetta Code Challenge Result
With the bug fixed, successfully found a factor of M929:

**Factor: 13007**
- k = 7  
- Formula: q = 2 × 7 × 929 + 1 = 13007
- Verification: 2^929 mod 13007 = 1 ✓

## Files Modified
- `fsh/FasterBASICT/src/fasterbasic_cfg.h` - Added m_gosubReturnMap
- `fsh/FasterBASICT/src/fasterbasic_cfg.cpp` - Updated processGosubStatement and buildEdges

## Test Files Created
- `bug_minimal.bas` - Minimal reproduction case
- `test_gosub_return_bug.bas` - Comprehensive test suite
- `test_prime.bas` - Isolated primality test
- `BUG_REPORT_GOSUB_RETURN.md` - Original bug report
