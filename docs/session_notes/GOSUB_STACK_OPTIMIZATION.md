# GOSUB Return Stack Optimization

## The Requirement
- Return stack is NECESSARY (multiple call sites, nesting, recursion)
- Must push return address on GOSUB
- Must pop and jump on RETURN

## Current Implementation
**Stores:** Block ID (integer 0, 1, 2, 3...)  
**Dispatch:** Linear search through all blocks

```qbe
; GOSUB
storew 11, $return_stack[sp]    ; Store block ID
sp++
jmp @subroutine

; RETURN  
sp--
%id =w loadw $return_stack[sp]  ; Load block ID
; Linear search:
if %id == 0 goto @block_0
if %id == 1 goto @block_1
... (N comparisons)
```

**Problem:** O(N) dispatch is slow

## What Can We Store Instead?

### Option A: Block ID (current)
**Dispatch:** Must search/compare to find label  
**Overhead:** O(N) or O(log N) comparisons

### Option B: Label Address (pointer)
**Dispatch:** Indirect jump `jmp %address`  
**Problem:** QBE doesn't support indirect jumps!

### Option C: Use Case Numbers for Switch
**Dispatch:** Some backends optimize switches efficiently  
**In QBE:** Still generates comparisons

## The Core Problem

**QBE does not support indirect jumps.**

You cannot do:
```qbe
%addr =l loadl $return_stack[sp]
jmp %addr        ; ERROR: jmp only takes @label, not %temp
```

## Available QBE Jump Instructions

From QBE IL spec:
```
JUMP := 'jmp' @IDENT              # Unconditional to label
      | 'jnz' VAL, @IDENT, @IDENT # Conditional
      | 'ret' [VAL]               # Return from function
      | 'hlt'                     # Halt
```

No `jmp %register` or `jmp (%pointer)` support!

## Optimization Options Within QBE Constraints

### Option 1: Binary Search
```qbe
; Instead of linear:
if id < 25 goto lower_half
if id < 12 goto lower_quarter
...
```
**Performance:** O(log₂N) = 6 comparisons for 50 blocks

### Option 2: Sparse Jump Table
Only include return blocks (not all blocks):
```qbe
; If only blocks 11, 14, 22, 35, 41 are return points:
if id == 11 goto @block_11
if id == 14 goto @block_14
if id == 22 goto @block_22
if id == 35 goto @block_35
if id == 41 goto @block_41
```
**Performance:** 5 comparisons instead of 50

### Option 3: Backend-Specific Optimization
Generate switch statement and hope backend optimizes it:
```qbe
; QBE might pass this to backend as switch
; ARM64/x86 compilers can generate jump tables
```

But QBE likely just generates comparisons anyway.

## Real Solution?

**Accept the limitation or extend QBE.**

Since we can't use indirect jumps in QBE, our options are:
1. Live with linear search (works, not ideal)
2. Binary search (log₂N, good improvement)
3. Sparse table (only return blocks, best we can do)
4. Extend QBE to support indirect jumps (major undertaking)

## Recommendation

Implement **Option 2: Sparse Jump Table**
- Analyze CFG to find all GOSUB return blocks
- Only include those in dispatch table
- Typical programs: 5-10 return blocks instead of 50 total blocks
- **10x speedup** with minimal code changes

This is the best optimization possible within QBE's constraints.
