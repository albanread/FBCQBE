# FOR Loop Implementation in FasterBASIC QBE Codegen V2

## Overview

This document describes the implementation of FOR loops in the FasterBASIC compiler's QBE code generator (Version 2, CFG-aware). The implementation correctly handles:

- Loop variable initialization
- Limit and step values evaluated once at loop entry
- Proper comparison operators for positive and negative steps
- Loop variable mutation during loop body (BASIC allows this)
- Loop variable persistence after loop exit

## BASIC FOR Loop Semantics

A FOR loop in BASIC has three key values:

1. **Index (loop variable)**: Mutable during the loop, survives after loop exit
2. **Limit**: Constant during the loop (evaluated once at init)
3. **Step**: Constant during the loop (evaluated once at init, default = 1)

### Important BASIC Rules

- The loop variable can be modified inside the loop body (e.g., `I = limit` to exit early)
- The loop variable retains its value after the loop exits
- All three values (index, limit, step) are treated as **integers** (word type in QBE)
- The comparison operator depends on the step sign:
  - **Positive step** (step >= 0): continue while `index <= limit`
  - **Negative step** (step < 0): continue while `index >= limit`

## CFG Structure

The CFG builder (`cfg_builder_loops.cpp`) creates the following block structure for FOR loops:

```
Entry → Init → Header → Body → Increment → Header (back-edge)
                  ↓
                Exit
```

- **Init block**: Initialize loop variable, allocate and store limit/step
- **Header block**: Evaluate condition (index vs limit based on step sign)
- **Body block**: Execute loop statements
- **Increment block**: Add step to index
- **Exit block**: Reached when condition is false

## Code Generation

### 1. Initialization (`emitForInit`)

Located in: `fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp`

The initialization phase:

1. **Evaluate start expression** and store to loop variable
2. **Allocate and store limit** value (constant during loop):
   ```qbe
   %limit_addr =l alloc4 4
   %limit_val =w <evaluate end expression>
   storew %limit_val, %limit_addr
   ```
3. **Allocate and store step** value (constant during loop, default = 1):
   ```qbe
   %step_addr =l alloc4 4
   %step_val =w <evaluate step expression or 1>
   storew %step_val, %step_addr
   ```

The addresses are stored in `forLoopTempAddresses_` map for later access by condition and increment code.

### 2. Condition Check (`emitForCondition`)

The condition logic:

1. **Load current values**:
   - Loop variable (may have been modified in body)
   - Limit (from allocated storage)
   - Step (from allocated storage)

2. **Check step sign**:
   ```qbe
   %step_is_neg =w csltw %step, 0
   ```

3. **Compute both conditions**:
   - **Positive case**: `index <= limit` is `!(index > limit)`
   - **Negative case**: `index >= limit` is `!(index < limit)`

4. **Select correct condition** using bitwise operations:
   ```qbe
   # Positive: !(index > limit)
   %loop_gt_limit =w csgtw %index, %limit
   %pos_cond =w xor %loop_gt_limit, 1
   
   # Negative: !(index < limit)
   %loop_lt_limit =w csltw %index, %limit
   %neg_cond =w xor %loop_lt_limit, 1
   
   # Select: if step_is_neg then neg_cond else pos_cond
   %neg_part =w and %step_is_neg, %neg_cond
   %not_step_is_neg =w xor %step_is_neg, 1
   %pos_part =w and %not_step_is_neg, %pos_cond
   %result =w or %neg_part, %pos_part
   ```

This approach avoids branches and phi nodes by using arithmetic selection.

### 3. Increment (`emitForIncrement`)

The increment phase:

1. **Load current index** (may have been modified in body)
2. **Load step** (from allocated storage)
3. **Add step to index**:
   ```qbe
   %current =w loadw %var_index
   %step_val =w loadw %step_addr
   %new_val =w add %current, %step_val
   storew %new_val, %var_index
   ```

## Example: Simple FOR Loop

### Source Code
```basic
10 FOR I = 1 TO 5
20   PRINT I
30 NEXT I
40 END
```

### Generated QBE IL (Key Parts)

```qbe
@block_0  # Entry
    %var_I =l alloc4 4
    storew 0, %var_I
    jmp @block_2

@block_2  # For_Init
    storew 1, %var_I           # I = 1
    %t.0 =l alloc4 4
    storew 5, %t.0             # limit = 5
    %t.1 =l alloc4 4
    %t.2 =w copy 1
    storew %t.2, %t.1          # step = 1
    jmp @block_3

@block_3  # For_Header
    %t.3 =w loadw %var_I       # load I
    %t.4 =w loadw %t.0         # load limit
    %t.5 =w loadw %t.1         # load step
    %t.6 =w csltw %t.5, 0      # step < 0?
    %t.7 =w csgtw %t.3, %t.4   # I > limit?
    %t.8 =w xor %t.7, 1        # !(I > limit) = (I <= limit)
    %t.9 =w csltw %t.3, %t.4   # I < limit?
    %t.10 =w xor %t.9, 1       # !(I < limit) = (I >= limit)
    %t.11 =w and %t.6, %t.10   # step<0 AND I>=limit
    %t.12 =w xor %t.6, 1       # step>=0
    %t.13 =w and %t.12, %t.8   # step>=0 AND I<=limit
    %t.14 =w or %t.11, %t.13   # final condition
    jnz %t.14, @block_4, @block_6

@block_4  # For_Body
    %t.15 =w loadw %var_I
    call $basic_print_int(w %t.15)
    call $basic_print_newline()
    jmp @block_5

@block_5  # For_Increment
    %t.16 =w loadw %var_I      # load I (may be modified)
    %t.17 =w loadw %t.1        # load step
    %t.18 =w add %t.16, %t.17  # I = I + step
    storew %t.18, %var_I
    jmp @block_3               # back to header

@block_6  # For_Exit
    ret 0
```

### Output
```
1
2
3
4
5
```

## Key Design Decisions

### 1. **Evaluate Limit and Step Once**

Per BASIC semantics, limit and step are constant during the loop. We allocate temporary storage at init and load from it each iteration. This ensures:
- Consistent behavior even if the expressions have side effects
- Efficient code (no re-evaluation)
- Correct semantics (BASIC standard)

### 2. **Handle Variable Step Sign**

Since BASIC allows step to be an expression (not just a constant), we check the sign at runtime and select the appropriate comparison:
- Positive/zero step: `<=`
- Negative step: `>=`

### 3. **Allow Loop Variable Mutation**

The loop variable is stored in memory (not an SSA temporary), so it can be:
- Modified in the loop body
- Loaded fresh at each increment
- Accessed after loop exit

### 4. **Use Bitwise Selection**

Instead of branches and phi nodes, we use bitwise AND/OR to select the correct condition. This:
- Avoids creating extra basic blocks
- Keeps the CFG structure clean
- Works correctly in QBE IL

## Files Modified

1. **`fsh/FasterBASICT/src/codegen_v2/ast_emitter.cpp`**:
   - `emitForInit()`: Initialize loop, allocate limit/step storage
   - `emitForCondition()`: Generate condition check with step sign handling
   - `emitForIncrement()`: Add step to index
   - `emitEndStatement()`: Emit `ret 0` to terminate blocks

2. **`fsh/FasterBASICT/src/codegen_v2/ast_emitter.h`**:
   - Added `forLoopTempAddresses_` map to track temp storage

## Testing

Test case: `test_simple_for.bas`
```basic
10 FOR I = 1 TO 5
20   PRINT I
30 NEXT I
40 END
```

Result: ✓ Prints 1, 2, 3, 4, 5 correctly

## Future Enhancements

1. **Optimize constant step**: If step is a compile-time constant, we could skip the sign check
2. **STEP 0 handling**: Currently undefined behavior; could detect and error
3. **EXIT FOR**: Already handled by CFG (jumps to exit block)
4. **Nested loops**: Should work with current implementation (each loop has unique temp addresses)

## Commit Information

**Commit**: Fix FOR loop implementation with proper limit/step evaluation and sign handling  
**Date**: February 1, 2026  
**Files**: ast_emitter.cpp, ast_emitter.h

**Key changes**:
- Evaluate limit and step once at init (allocate temps)
- Handle negative step with runtime sign check
- Use bitwise selection to choose comparison operator
- Fix END statement to emit `ret 0`
- Store temp addresses in `forLoopTempAddresses_` map

## ARM64 Assembly Analysis

The integrated compiler generates efficient ARM64 assembly. Here's the generated code for our test loop:

```asm
_main:
	hint	#34                      # BTI (Branch Target Identification)
	stp	x29, x30, [sp, -64]!     # Save frame pointer and link register
	mov	x29, sp                  # Set up frame pointer
	str	x19, [x29, 56]           # Save callee-saved registers
	str	x20, [x29, 48]           # x19 = loop variable (I)
	str	x21, [x29, 40]           # x20 = step
	                                 # x21 = limit
	
	# Initialize limit and step on stack
	add	x1, x29, #24
	mov	w0, #5                   # limit = 5
	str	w0, [x1]
	add	x1, x29, #28
	mov	w0, #1                   # step = 1
	str	w0, [x1]
	
	# Load initial values into registers
	mov	w21, #1                  # I = 1
	mov	w19, #5                  # limit in register
	mov	w20, #1                  # step in register

L2:  # Loop header
	cmp	w20, #0                  # Check if step < 0
	cset	w2, lt                   # w2 = (step < 0)
	
	cmp	w21, w19                 # Compare I with limit
	cset	w0, gt                   # w0 = (I > limit)
	mov	w1, #1
	eor	w1, w0, w1               # w1 = !(I > limit) = (I <= limit)
	
	cmp	w21, w19                 # Compare again for negative case
	cset	w0, lt                   # w0 = (I < limit)
	mov	w3, #1
	eor	w0, w0, w3               # w0 = !(I < limit) = (I >= limit)
	
	# Select condition based on step sign
	and	w0, w2, w0               # w0 = step_is_neg AND (I >= limit)
	mov	w3, #1
	eor	w2, w2, w3               # w2 = !step_is_neg
	and	w1, w1, w2               # w1 = !step_is_neg AND (I <= limit)
	orr	w0, w0, w1               # w0 = final condition
	
	cmp	w0, #0
	beq	L4                       # Exit if condition false
	
	# Loop body - print I
	mov	w0, w21
	bl	_basic_print_int
	bl	_basic_print_newline
	
	# Increment
	add	w21, w21, w20            # I = I + step
	b	L2                        # Back to loop header

L4:  # Exit
	mov	w0, #0                   # Return 0
	ldr	x19, [x29, 56]           # Restore callee-saved registers
	ldr	x20, [x29, 48]
	ldr	x21, [x29, 40]
	ldp	x29, x30, [sp], 64       # Restore frame pointer and return
	ret
```

### Key Observations

1. **Register allocation**: QBE keeps the loop variable (I), limit, and step in registers (w21, w19, w20) for the entire loop, avoiding memory accesses in the hot path.

2. **Efficient condition**: The bitwise selection we used in QBE IL translates directly to efficient ARM instructions (cmp, cset, eor, and, orr).

3. **No redundant loads**: After the initial setup, the loop runs entirely in registers - only the print functions access memory.

4. **Compact loop body**: The actual loop body (print and increment) is just 5 instructions plus the branch.

5. **Stack frame**: 64 bytes allocated for local variables and saved registers.

The generated code is quite efficient - QBE's register allocator did a good job keeping hot values in registers throughout the loop.

## References

- CFG Builder: `fsh/FasterBASICT/src/cfg/cfg_builder_loops.cpp`
- QBE Builder: `fsh/FasterBASICT/src/codegen_v2/qbe_builder.h`
- Type Manager: `fsh/FasterBASICT/src/codegen_v2/type_manager.h`
