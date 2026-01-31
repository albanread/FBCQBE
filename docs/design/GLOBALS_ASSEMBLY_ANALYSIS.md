# GLOBALS Assembly Analysis: Local vs Global Variables

## Executive Summary

Comparing the generated assembly code for local (SSA) vs global (memory-backed) variables reveals a critical insight:

**QBE performs aggressive constant folding and dead code elimination on SSA temporaries, completely eliminating calculations when possible. Global variables, being memory-backed, cannot be optimized away and require explicit load/store operations.**

## Test Programs

Two identical programs, differing only in variable declaration:

### Local Version (DIM)
```basic
DIM a%, b%, c%, d%
a% = 10: b% = 20: c% = 30: d% = 40
a% = a% + b%
c% = c% * 2
d% = d% + c%
PRINT a%; c%; d%
```

### Global Version (GLOBAL)
```basic
GLOBAL a%, b%, c%, d%
a% = 10: b% = 20: c% = 30: d% = 40
a% = a% + b%
c% = c% * 2
d% = d% + c%
PRINT a%; c%; d%
```

Both compute the same result: `30, 60, 100`

## Assembly Output Comparison

### Local Variables: 71 lines (ARM64)

**Complete optimization!** QBE computed everything at compile time:

```asm
_main:
    bl   _basic_runtime_init
    mov  x0, #30              # Result pre-computed: 10 + 20
    bl   _basic_print_int
    mov  x0, #60              # Result pre-computed: 30 * 2
    bl   _basic_print_int
    mov  x0, #100             # Result pre-computed: 40 + 60
    bl   _basic_print_int
    bl   _basic_print_newline
    bl   _basic_runtime_cleanup
    ret
```

**Analysis:**
- **0 load/store operations**
- **0 arithmetic operations** at runtime
- **All calculations performed at compile time**
- Only function calls remain (print, init, cleanup)
- Tiny code size (16 instructions)

### Global Variables: 144 lines (ARM64)

**No optimization** - every operation explicit:

```asm
_main:
    bl   _basic_runtime_init
    mov  x0, #4
    bl   _basic_global_init
    
    # a% = 10
    bl   _basic_global_base
    mov  x1, x0
    mov  x0, #10
    str  x0, [x1]
    
    # b% = 20
    bl   _basic_global_base
    mov  x1, #8
    add  x1, x0, x1
    mov  x0, #20
    str  x0, [x1]
    
    # c% = 30
    bl   _basic_global_base
    mov  x1, #16
    add  x1, x0, x1
    mov  x0, #30
    str  x0, [x1]
    
    # d% = 40
    bl   _basic_global_base
    mov  x1, #24
    add  x1, x0, x1
    mov  x0, #40
    str  x0, [x1]
    
    # a% = a% + b%  (READ a%, READ b%, ADD, WRITE a%)
    bl   _basic_global_base
    ldr  x19, [x0]            # Load a%
    bl   _basic_global_base
    mov  x1, #8
    add  x0, x0, x1
    ldr  x0, [x0]             # Load b%
    add  x19, x19, x0         # Add
    bl   _basic_global_base
    str  x19, [x0]            # Store a%
    
    # c% = c% * 2  (READ c%, MULTIPLY, WRITE c%)
    bl   _basic_global_base
    mov  x1, #16
    add  x0, x0, x1
    ldr  x0, [x0]             # Load c%
    scvtf d0, x0              # Convert to float
    ldr  d1, [Lfp0]           # Load constant 2.0
    fmul d0, d0, d1           # Multiply
    fcvtzs x19, d0            # Convert back to int
    bl   _basic_global_base
    mov  x1, #16
    add  x0, x0, x1
    str  x19, [x0]            # Store c%
    
    # d% = d% + c%  (READ d%, READ c%, ADD, WRITE d%)
    bl   _basic_global_base
    mov  x1, #24
    add  x0, x0, x1
    ldr  x19, [x0]            # Load d%
    bl   _basic_global_base
    mov  x1, #16
    add  x0, x0, x1
    ldr  x0, [x0]             # Load c%
    add  x19, x19, x0         # Add
    bl   _basic_global_base
    mov  x1, #24
    add  x0, x0, x1
    str  x19, [x0]            # Store d%
    
    # Print results
    bl   _basic_global_base
    ldr  x0, [x0]
    bl   _basic_print_int
    # ... (similar for other prints)
```

**Analysis:**
- **13 function calls** to `basic_global_base()`
- **10 load operations** (ldr)
- **7 store operations** (str)
- **3 arithmetic operations** at runtime
- All calculations explicit in assembly
- Much larger code size (~80 instructions)

## Detailed Instruction Breakdown

### Per-Operation Cost

| Operation | Local (SSA) | Global (Memory) | Difference |
|-----------|-------------|-----------------|------------|
| **Variable initialization** | 0 instructions | 4-5 instructions each | ∞ |
| **Read variable** | 0 (optimized away) | 3 instructions (call, add, ldr) | ∞ |
| **Write variable** | 0 (optimized away) | 3 instructions (call, add, str) | ∞ |
| **Arithmetic** | 0 (compile-time) | 1-4 instructions (runtime) | ∞ |
| **Total code size** | 71 lines | 144 lines | **2x larger** |

### Global Variable Access Pattern (Each Access)

```asm
bl   _basic_global_base      # Call runtime function (3-5 cycles)
mov  x1, #OFFSET             # Load immediate offset (1 cycle)
add  x1, x0, x1              # Calculate address (1 cycle)
ldr  x0, [x1]                # Load from memory (3-5 cycles)
# Total: ~8-12 cycles per read
```

### Local Variable "Access" Pattern

```asm
# (nothing - optimized away completely)
```

## Why Such a Huge Difference?

### SSA Temporaries Enable Optimization

QBE's SSA form allows it to:
1. **Track data flow** precisely
2. **Constant propagation**: `a = 10; b = a + 5` → `b = 15`
3. **Dead code elimination**: Remove unused calculations
4. **Constant folding**: Evaluate expressions at compile time
5. **Copy propagation**: Replace variable reads with known values

### Memory Variables Prevent Optimization

Global variables are **opaque** to the optimizer because:
1. **Memory is observable** - can't prove no aliasing
2. **Function calls can modify** - conservative assumption
3. **Side effects possible** - must preserve order
4. **No SSA form** - can't track value flow
5. **Runtime address** - can't analyze at compile time

## Performance Implications

### Local Variables (Best Case)
- **Compilation time**: Higher (optimization analysis)
- **Code size**: Minimal (dead code eliminated)
- **Runtime**: Fastest (everything pre-computed)
- **Memory**: Zero runtime memory access
- **Register pressure**: Zero (no runtime variables)

### Global Variables (Worst Case)
- **Compilation time**: Lower (no optimization)
- **Code size**: Large (every operation explicit)
- **Runtime**: Slower (loads, stores, function calls)
- **Memory**: High traffic (every access goes to RAM)
- **Register pressure**: Moderate (temporaries for addresses)

## Real-World Scenario: 32 Variables

### Local Version
```
Total assembly: 106 lines
Actual code: ~20 instructions (mostly prints)
All 32 variables optimized away completely
```

### Global Version
```
Total assembly: 887 lines
Actual code: ~600 instructions
32 variables × ~20 operations each = massive code
Every single operation generates loads/stores
```

## When Globals Make Sense

Despite worse optimization, globals are necessary for:

### 1. **Cross-Function Sharing**
```basic
GLOBAL counter%

SUB Increment()
    SHARED counter%
    counter% = counter% + 1
END SUB
```
Without globals: Must pass by reference (not yet implemented)

### 2. **Dynamic/Runtime Values**
```basic
GLOBAL userInput$
INPUT "Name: ", userInput$
' Value unknown at compile time
```

### 3. **Program State**
```basic
GLOBAL gameScore%, lives%, level%
' Persistent state across function calls
```

## Optimization Opportunities for Globals

### 1. **Base Pointer Caching** (Easy)
Current:
```asm
bl _basic_global_base  # Call for every access
```

Optimized:
```asm
bl _basic_global_base  # Call once per block
mov x20, x0            # Cache in register
# Reuse x20 for all globals in block
```
**Savings**: N-1 function calls for N accesses

### 2. **Address Caching** (Medium)
For frequently-accessed globals:
```asm
bl _basic_global_base
add x20, x0, #OFFSET   # Calculate once
# Reuse x20 for multiple reads/writes
```

### 3. **Smart Escape Analysis** (Hard)
If global is only used in single function:
```basic
GLOBAL temp%
SUB OnlyUser()
    SHARED temp%
    temp% = 100  ' Could be optimized like local
END SUB
```
Compiler could promote to register if no other accesses exist.

## Recommendations

### For Performance-Critical Code
```basic
' Use local variables when possible
SUB FastCalculation()
    DIM a%, b%, c%, result%
    ' QBE will optimize aggressively
    a% = 10
    b% = 20
    result% = a% + b% * c%
    ' If all constants, may compile to single value
END SUB
```

### For Shared State
```basic
' Use globals when necessary
GLOBAL sharedData%, programState%

SUB UpdateState()
    SHARED programState%
    programState% = programState% + 1
    ' Accept the load/store overhead
END SUB
```

### Hybrid Approach
```basic
GLOBAL importantData%

SUB ProcessData()
    SHARED importantData%
    DIM localCopy%, result%
    
    ' Copy to local once
    localCopy% = importantData%
    
    ' Work with local (optimizable)
    result% = localCopy% * 2 + 10
    
    ' Write back once
    importantData% = result%
END SUB
```

## Conclusion

The assembly analysis reveals **QBE is an extremely powerful optimizer** for SSA temporaries:
- **100% of calculations eliminated** when values are compile-time known
- **Perfect constant folding** and dead code elimination
- **Minimal code size** and maximum runtime performance

Globals pay a **significant performance penalty**:
- **~2x code size** for simple programs
- **~8x code size** for complex programs (32 variables)
- **Every operation explicit**: no optimization possible
- **High memory traffic**: every access is load/store

**Trade-off**: Globals provide essential **cross-scope sharing** at the cost of **optimization opportunities**.

For FasterBASIC users:
- **Use DIM for local computation** → Let QBE optimize
- **Use GLOBAL for shared state** → Accept the overhead
- **Cache frequently-accessed globals** → Minimize traffic

---

**Analysis Date**: January 2025  
**Platform**: ARM64 (Apple Silicon)  
**Compiler**: QBE + FasterBASIC  
**Optimization Level**: QBE default (-O2 equivalent)