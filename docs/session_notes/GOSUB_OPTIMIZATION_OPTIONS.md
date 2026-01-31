# GOSUB/RETURN Optimization Options (Without Globals)

## Constraint
Variables MUST stay as SSA locals (in registers) for performance.
GOSUB targets MUST stay within the same QBE function (same scope).

## Current State
- ✅ GOSUB/RETURN works correctly (bug fixed)
- ❌ RETURN uses O(N) linear search through all blocks
- For 50 blocks: ~160 instructions per RETURN

## Optimization Options

### Option 1: Binary Search Jump Table
Replace linear search with binary search.

**Current (Linear):**
```qbe
if block_id == 0 goto block_0
if block_id == 1 goto block_1
... (N comparisons)
```

**Binary Search:**
```qbe
if block_id < 25 goto lower_half
if block_id < 37 goto mid_upper
... (log2(N) comparisons)
```

**Performance:** O(N) → O(log₂N)
- 50 blocks: 50 comparisons → 6 comparisons = **8x faster**

### Option 2: Reduce Search Space
Only check blocks that are reachable as return points.

**Analysis:** Use CFG to determine which blocks can be RETURN targets.
Only include those in the jump table.

**Example:** If only 5 blocks are GOSUB return points:
- Current: Check all 50 blocks
- Optimized: Check only 5 blocks = **10x faster**

### Option 3: Specialized RETURN per Subroutine
Generate different RETURN code based on which subroutine we're in.

**Example:**
```qbe
@PrimeCheck:
    ...
    ; This RETURN only called from blocks 11 and 17
    %ret_id =w loadw $return_stack[sp]
    %is_11 =w ceqw %ret_id, 11
    jnz %is_11, @block_11, @check_17
@check_17:
    jnz ..., @block_17, @error
```

**Performance:** Only check 2 blocks instead of 50 = **25x faster**

### Option 4: Accept Current Performance
GOSUB/RETURN is not typically in tight inner loops.
The ~160 instruction overhead is acceptable for occasional calls.

**Argument:** Profile-driven - measure real programs to see if GOSUB/RETURN
is actually a bottleneck before optimizing.

## Recommendation

Implement **Option 3** (Specialized RETURN) because:
1. Simple to implement (use CFG analysis)
2. Dramatic speedup (only check reachable return points)
3. Scales well (each subroutine optimized independently)
4. No downside (doesn't affect variable access)

### Implementation
1. In CFG, track which blocks can call each subroutine
2. In codegen, generate tailored jump table for each RETURN
3. Each RETURN only checks its reachable callers

For most subroutines with 1-3 call sites: 
**50 comparisons → 1-3 comparisons = 16-50x faster**
