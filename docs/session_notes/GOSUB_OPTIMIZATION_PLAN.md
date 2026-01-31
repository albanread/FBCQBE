# GOSUB/RETURN Optimization Plan

## Current State (After Bug Fix)
- ✅ GOSUB correctly stores return block ID in custom stack
- ✅ Control flow is correct
- ❌ RETURN uses O(N) linear search through all blocks

## The Core Insight
BASIC GOSUB/RETURN shares global scope - it's NOT a function call with parameters/locals.

## Optimization Strategies

### Strategy 1: Call Stack + Indirect Jump
Use native call mechanism but keep shared scope.

**Challenge:** QBE doesn't support `jmp %register` (indirect jump)

### Strategy 2: Inline Return Dispatch
At compile time, we know which GOSUBs can call each RETURN.

For subroutines called from few places:
```qbe
# Instead of linear search...
PrimeCheck:
    ...
    # RETURN: Only called from blocks 9 and 15
    %ret_id =w loadw $return_stack[sp]
    %is_9 =w ceqw %ret_id, 9
    jnz %is_9, @block_11, @check_15
@check_15:
    %is_15 =w ceqw %ret_id, 15
    jnz %is_15, @block_17, @error
```

**Pros:** Only check reachable return points  
**Cons:** Still doing comparisons

### Strategy 3: Use QBE Functions with Global Variables
Convert all BASIC variables to QBE globals:

```qbe
# Global variable declarations (outside all functions)
data $var_P = { l 0 }
data $var_k = { l 0 }
data $var_q = { l 0 }
...

export function w $main() {
    # Store to globals
    storel 929, $var_P
    
    # GOSUB becomes call
    call $PrimeCheck()
    
    # Load from globals
    %result =l loadl $var_isprime
    ...
}

function $PrimeCheck() {
    # Access globals
    %q =l loadl $var_testq
    
    # Do primality test
    ...
    storel 1, $var_isprime
    
    # RETURN becomes ret
    ret 0
}
```

**Pros:**
- Native call/ret (single instruction)
- No jump tables at all
- Efficient
- Natural mapping to BASIC semantics

**Cons:**
- More complex codegen (separate variable emission phase)
- All variables become globals (but that's how BASIC works anyway!)

### Strategy 4: Computed Jump Table (if QBE optimizes it)
Create array of label addresses and use as jump table:

**Problem:** QBE doesn't support taking address of label or indirect jumps

## Recommended Approach

**Strategy 3** (QBE functions + globals) is the best fit because:

1. BASIC programs already have global scope for all variables
2. GOSUB/RETURN is semantically just call/return with shared state
3. Native call/ret is maximally efficient
4. Clean separation: main + one function per GOSUB target

### Implementation Plan

1. Identify all GOSUB target labels in pre-pass
2. Emit globals for all program variables
3. Emit `$main()` function for main program flow
4. Emit separate function for each GOSUB target label
5. GOSUB → `call $Label()`
6. RETURN → `ret 0`
7. All functions access same globals

### Code Changes Needed

- `qbe_codegen_main.cpp`: Add phase to emit globals before functions
- `qbe_codegen_statements.cpp`: Modify emitGosub to emit `call` instead of stack+jmp
- `qbe_codegen_statements.cpp`: Modify emitReturn to emit `ret 0` instead of jump table
- Add method to determine GOSUB targets and create functions for them

## Expected Performance Improvement

- **Before:** O(N) comparisons per RETURN
- **After:** O(1) native ret instruction

For Mersenne program: ~50 comparisons → 1 instruction = **50x faster returns**
