# GOSUB/RETURN Fix Verification

## The Fix
Added `m_gosubReturnMap` to track GOSUB return continuation blocks in CFG building.

## QBE IL Evidence

### GOSUB from Block 9 (inside IF THEN block)
```qbe
@block_9
    # Block 9 (IF THEN)
    %var_testq =d copy %var_q
    %t57 =w loadw $return_sp
    %t58 =l extsw %t57
    %t59 =l mul %t58, 4
    %t60 =l add $return_stack, %t59
    storew 11, %t60              ← Stores block 11 as return address
    %t61 =w add %t57, 1
    storew %t61, $return_sp
    # GOSUB to block_27
    jmp @block_27                ← Jumps to subroutine

@block_10
    # Block 10 (IF ELSE)         ← This is the ELSE block, NOT the return point!
    jmp @block_19

@block_11                        ← This is the correct return point!
    # Block 11
    %t62 =d copy d_1.000000
    %t63 =d sltof %var_isprime_INT
    %t65 =w ceqd %t63, %t62
    ...continues with IF isprime% = 1 check...
```

**Key Point:** Block 9 stores return address **11** (not 10), which is correct!

### RETURN Implementation (from block_27 - PrimeCheck)
```qbe
@block_28
    # Block 28 (IF THEN)
    %t136 =w loadw $return_sp     ← Load return stack pointer
    %t137 =w sub %t136, 1         ← Decrement it
    storew %t137, $return_sp
    %t138 =l extsw %t137
    %t139 =l mul %t138, 4
    %t140 =l add $return_stack, %t139
    %t141 =w loadw %t140          ← Load return block ID from stack
    %t142 =w copy 0
    %t143 =w ceqw %t141, %t142
    jnz %t143, @block_0, @L1      ← Jump table dispatch

@L1                                ← Jump table comparing return address
    %t144 =w copy 1
    %t145 =w ceqw %t141, %t144
    jnz %t145, @block_1, @L2
@L2
    %t146 =w copy 2
    %t147 =w ceqw %t141, %t146
    jnz %t147, @block_2, @L3
...continues checking for block 11...
```

## ARM64 Assembly

The ARM64 code shows the same pattern - return stack management and jump table dispatch for RETURN.

### GOSUB Sequence (ARM64)
```asm
	adrp	x0, _return_sp@page
	add	x0, x0, _return_sp@pageoff
	ldr	w0, [x0]                  ; Load return_sp
	sxtw	x1, w0
	mov	x2, #4
	mul	x1, x1, x2                ; Multiply by 4
	adrp	x2, _return_stack@page
	add	x2, x2, _return_stack@pageoff
	add	x2, x1, x2                ; Calculate stack address
	mov	w1, #11                   ; Return block ID = 11
	str	w1, [x2]                  ; Store to stack
	mov	w1, #1
	add	w0, w0, w1                ; Increment return_sp
	adrp	x1, _return_sp@page
	add	x1, x1, _return_sp@pageoff
	str	w0, [x1]                  ; Save return_sp
```

## Verification

✅ **Block numbering is correct** - GOSUB stores the actual continuation block ID, not id+1  
✅ **Jump table includes all return points** - RETURN can dispatch to any valid block  
✅ **Tests pass** - All GOSUB/IF interaction tests produce correct output  
✅ **Mersenne challenge solved** - Found factor 13007 of M929

## Conclusion

The fix correctly tracks GOSUB return points through the CFG, ensuring proper control flow even when GOSUB is nested within IF blocks, WHILE loops, or other control structures.
