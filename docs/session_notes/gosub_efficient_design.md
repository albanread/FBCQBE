# Efficient GOSUB/RETURN Design

## Current Implementation (Inefficient)
- Stores block ID (integer) on custom return_stack array
- RETURN does linear search: compare ID against 0, 1, 2, 3... until match
- O(N) comparisons for N blocks
- For Mersenne program (~50 blocks): ~50 comparisons per RETURN

## Problem
QBE IL only supports direct jumps to labels (`jmp @label`), not indirect jumps.

## Proposed Solution: Use Native Call Stack

Instead of storing block IDs and doing linear search, store actual return addresses and use indirect jump.

### Option 1: QBE `call`/`ret` with shared globals
```qbe
# Declare all variables as globals (outside function)
data $var_x = { w 0 }
data $var_y = { w 0 }

function w $main() {
    # Load from globals
    %x =w loadw $var_x
    
    # GOSUB → call
    call $Sub1()
    
    # Continue after return
    ...
}

function $Sub1() {
    # Access same globals
    %x =w loadw $var_x
    storew %x, $var_y
    ret 0
}
```

**Pros:** Native call/return, very efficient
**Cons:** All variables become globals, more complex codegen

### Option 2: QBE Indirect Jump (if supported)
Check if QBE supports `jmp %register` style indirect jumps.

### Option 3: Switch-style Jump Table (better than linear)
Instead of linear search, use binary search or computed jump:
```qbe
# Compute offset and use as jump table index
%offset =l mul %block_id, 8
%table_addr =l add $jump_table, %offset  
%target =l loadl %table_addr
jmp %target  # if indirect jmp supported
```

### Option 4: Hybrid - Functions for subroutines, pass context
Make each GOSUB target a QBE function, pass variables as struct pointer:
```qbe
type :context = { w, w, w, ... }  # All program variables

function w $main() {
    %ctx =l alloc8 1000  # Allocate context on stack
    ...
    call $Sub1(l %ctx)   # Pass context
}

function $Sub1(l %ctx) {
    # Access variables through context pointer
    %x_ptr =l add %ctx, 0
    %x =w loadw %x_ptr
    ret 0
}
```

## Recommendation

Start with **Option 1** (globals + QBE functions). Benefits:
- Uses native call/return (single instruction each)
- No jump tables or linear search
- Simple to implement
- Variables already tracked by symbol table

Implementation:
1. Emit all BASIC variables as QBE `data` declarations (globals)
2. Emit main program as `$main()` function  
3. Emit each GOSUB target label as separate QBE function
4. GOSUB → `call $Label()`
5. RETURN → `ret 0`

All functions access the same global variables - same semantics as BASIC GOSUB/RETURN, but with efficient native call/return.
