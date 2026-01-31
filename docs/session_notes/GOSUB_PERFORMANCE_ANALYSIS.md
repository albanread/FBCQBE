# GOSUB/RETURN Performance Analysis

## Current Implementation (Custom Stack + Linear Search)

### GOSUB (7 instructions)
```assembly
ldr  w0, [return_sp]           ; Load stack pointer
sxtw x1, w0                     ; Sign extend to 64-bit
mov  x2, #4                     ; 
mul  x1, x1, x2                 ; Multiply by 4 (word size)
adrp x2, return_stack@page      ; Get stack base (2 instructions)
add  x2, x2, return_stack@pageoff
add  x2, x1, x2                 ; Calculate address
mov  w1, #11                    ; Return block ID
str  w1, [x2]                   ; Store to stack
mov  w1, #1                     ;
add  w0, w0, w1                 ; Increment SP
adrp x1, return_sp@page         ; Store SP back (2 instructions)
add  x1, x1, return_sp@pageoff
str  w0, [x1]
bl   Label_Sub                  ; Jump to subroutine
```
**Total: ~15 instructions**

### RETURN (for N blocks)
```assembly
ldr  w0, [return_sp]           ; Load SP
sub  w1, w0, #1                 ; Decrement
str  w1, [return_sp]            ; Store back
sxtw x2, w1                     ; Sign extend
mov  x3, #4
mul  x2, x2, x3                 ; Calculate offset
adrp x3, return_stack@page
add  x3, x3, return_stack@pageoff
add  x3, x2, x3
ldr  w4, [x3]                   ; Load return block ID

; Linear search through all N blocks:
mov  w5, #0
cmp  w4, w5
beq  block_0
mov  w5, #1
cmp  w4, w5
beq  block_1
... (repeat for all N blocks)
mov  w5, #N-1
cmp  w4, w5
beq  block_N-1
b    exit_error
```
**Total: ~10 + (3 × N) instructions**  
**For N=50 blocks: ~160 instructions!**

## Proposed: Native Call/Return with Global Variables

### Variable Access: SSA Local vs QBE Global

#### Option A: SSA Local (current for non-GLOBAL vars)
```qbe
%var_x =d copy d_0.0           ; SSA temporary (stays in register)
%var_x =d add %var_x, d_1.0    ; Direct register operation
```
**Performance: Optimal** - stays in register, no memory access

#### Option B: QBE Global
```qbe
data $var_x = { d 0.0 }

%temp =d loadd $var_x          ; Load from memory
%result =d add %temp, d_1.0    ; Operate
stored %result, $var_x         ; Store back
```
**Performance: Requires memory load/store** - much slower than register

### GOSUB/RETURN with Functions + Globals

#### GOSUB (1-2 instructions)
```assembly
bl   _Sub1                      ; Branch with link
```
**Total: 1 instruction** (plus prologue/epilogue overhead)

#### RETURN (1-2 instructions)
```assembly
ret                             ; Return to caller
```
**Total: 1 instruction**

### Function Prologue/Epilogue Overhead
```assembly
; Prologue (on GOSUB):
stp  x29, x30, [sp, -16]!      ; Save frame pointer and link register
mov  x29, sp                    ; Set frame pointer

; Epilogue (on RETURN):
ldp  x29, x30, [sp], 16        ; Restore frame pointer and link register
ret                             ; Return
```
**Total: 4 instructions per call/return pair**

## Performance Comparison

### For Mersenne Program (50 blocks, 2 GOSUBs per iteration)

Current Implementation per iteration:
- 2 × GOSUB: 2 × 15 = 30 instructions
- 2 × RETURN: 2 × 160 = 320 instructions
- **Total: ~350 instructions**

Proposed (Functions + Globals):
- 2 × GOSUB: 2 × 3 = 6 instructions (bl + prologue)
- 2 × RETURN: 2 × 3 = 6 instructions (epilogue + ret)
- Variable access overhead: ~2x slower (load/store vs register)
- **GOSUB/RETURN: ~12 instructions**

## The Trade-off

**Gains:**
- GOSUB/RETURN: 350 → 12 instructions = **29x faster**

**Losses:**
- Variable access: register → memory = **~2-5x slower per access**

## Critical Question

**How many variable accesses vs GOSUB/RETURN calls?**

In Mersenne program per iteration:
- ~20-30 variable accesses in main loop
- 2 GOSUB/RETURN pairs
- ~50-100 variable accesses inside subroutines

If we use globals:
- GOSUB/RETURN: Save ~338 instructions
- Variable access: Cost ~100-200 extra instructions (load/store overhead)
- **Net: Worse performance!**

## Conclusion

Making all variables global to enable function-based GOSUB would:
- ✅ Make GOSUB/RETURN much faster
- ❌ Make every variable access much slower
- ❌ Net negative performance (more variable access than GOSUB calls)

## Better Alternative

Keep variables as SSA locals (fast), optimize only the RETURN dispatch:
1. **Reduce search space** - only check reachable return points
2. **Use binary search** instead of linear
3. **Inline return jump** when subroutine has only one caller
4. Accept that RETURN is slower - it's not the bottleneck

Or: Keep current implementation - it works correctly, and GOSUB/RETURN is not typically performance-critical in BASIC programs.
