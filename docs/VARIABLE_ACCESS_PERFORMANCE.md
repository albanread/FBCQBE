# Variable Access Performance: SSA Locals vs Globals

## Test Case: Simple Loop
```basic
LET sum& = 0
LET i& = 1
WHILE i& <= 10
    LET sum& = sum& + i&
    LET i& = i& + 1
WEND
```

## SSA Locals (Current - Non-GLOBAL variables)

### Loop Body Assembly
```assembly
L1:
    ; Compare i <= 10
    ldr   d2, [constant_10]     ; 2 instructions (adrp + ldr)
    fcmpe d1, d2                 ; 1 instruction
    bgt   L3                     ; 1 instruction (exit loop)
    
    ; sum = sum + i
    fadd  d0, d0, d1             ; 1 instruction (registers!)
    
    ; i = i + 1  
    ldr   d2, [constant_1]       ; 2 instructions (adrp + ldr)
    fadd  d1, d1, d2             ; 1 instruction
    
    b     L1                     ; 1 instruction (loop back)
```

**Per iteration: ~9 instructions**
- Variables d0, d1 stay in **FP registers** 
- Only constant loads from memory
- **Zero variable memory access**

## Global Variables

### Loop Body Assembly
```assembly
L1:
    ; Load i from global vector
    adrp  x0, __global_vector@page       ; 2 instructions
    add   x0, x0, __global_vector@pageoff
    ldr   d1, [x0]                        ; Load i (memory access!)
    
    ; Compare i <= 10
    ldr   d2, [constant_10]               ; 2 instructions
    fcmpe d1, d2                          ; 1 instruction
    bgt   L3                              ; 1 instruction
    
    ; Load sum from global vector  
    adrp  x0, __global_vector@page+16    ; 2 instructions
    add   x0, x0, __global_vector@pageoff+16
    ldr   d0, [x0]                        ; Load sum (memory access!)
    
    ; sum = sum + i
    fadd  d0, d1, d0                      ; 1 instruction
    
    ; Store sum back
    str   d0, [x0]                        ; Store sum (memory access!)
    
    ; i = i + 1
    ldr   d2, [constant_1]                ; 2 instructions
    fadd  d1, d1, d2                      ; 1 instruction
    
    ; Store i back
    adrp  x0, __global_vector@page       ; 2 instructions
    add   x0, x0, __global_vector@pageoff
    str   d1, [x0]                        ; Store i (memory access!)
    
    b     L1                              ; 1 instruction
```

**Per iteration: ~19 instructions**
- 4 memory accesses per iteration (2 loads, 2 stores)
- **2x more instructions**

## Performance Impact

### Instruction Count
- SSA Locals: 9 instructions/iteration
- Globals: 19 instructions/iteration  
- **Overhead: 2.1x slower**

### Memory Access
- SSA Locals: 0 variable memory accesses (registers only!)
- Globals: 4 memory accesses per iteration
- **Memory bottleneck: Cache misses, memory latency**

### Real-World Impact
On modern CPUs with:
- Register operations: 0.25-1 cycle
- L1 cache hit: 4-5 cycles
- L2 cache hit: 12-15 cycles

**Estimated real performance: 3-5x slower with globals**

## Conclusion

Converting all variables to globals for GOSUB optimization would:
- Make GOSUB/RETURN 29x faster (350 → 12 instructions)
- Make variable access 3-5x slower (registers → memory)

Since typical BASIC programs have **far more variable accesses than GOSUB calls**, this would be a **net performance loss**.

## Recommendation

**Keep SSA locals for variables.** The current GOSUB/RETURN implementation is correct after our bug fix. The linear search in RETURN is not ideal, but acceptable because:
1. GOSUB/RETURN is not typically in tight loops
2. Variable access is far more frequent and must stay fast
3. The trade-off heavily favors fast variable access
