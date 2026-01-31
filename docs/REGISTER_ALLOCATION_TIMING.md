# Register Allocation Timing and MADD Fusion

**Date:** 2026-01-31  
**Question:** "How do we know what register we can use?"  
**Answer:** We don't need to choose - they're already chosen!

---

## The Key Insight

**By the time we reach the emitter (`arm64/emit.c`), all registers are already allocated.**

The register allocator (`rega.c`) runs **before** the emitter. This is why the peephole approach works so well.

---

## QBE Compilation Pipeline (Simplified)

```
Source Code (QBE IR)
    ↓
Parsing (parse.c)
    ↓
SSA Construction (ssa.c)
    ↓
Optimizations (fold.c, etc.)
    ↓
Instruction Selection (arm64/isel.c)
    ↓
Register Allocation (rega.c)  ← REGISTERS ASSIGNED HERE
    ↓
Spilling (spill.c)
    ↓
Emitter (arm64/emit.c)  ← WE WORK HERE (registers already known!)
    ↓
Assembly Output
```

---

## Before Register Allocation

Instructions use **virtual temporaries**:

```qbe
@start
    %t1 =w mul %a, %b      # Virtual temp %t1
    %t2 =w add %c, %t1     # Virtual temp %t2
    ret %t2
```

These `%t1`, `%t2` are not real registers - they're just numbered temporaries.

---

## After Register Allocation

The same instructions now reference **physical registers**:

```qbe
@start
    %x3 =w mul %x1, %x2    # x3 = physical register (R0+3)
    %x0 =w add %x4, %x3    # x0 = physical register (R0+0)
    ret %x0
```

The `Ref` structures now contain actual hardware register numbers:
- `R0` through `R30` for integer registers (ARM64 x0-x30)
- `V0` through `V30` for vector/float registers (ARM64 v0-v30, or s0-s30/d0-d30)

---

## Visual Example: How We "Know" What Registers to Use

```
BEFORE EMITTER (after rega.c):
┌─────────────────────────────────────────────────────────┐
│ Instruction 1 (in memory):                              │
│   op:      Omul                                          │
│   cls:     Kw (word/32-bit)                              │
│   to.val:  3        ← This is register R0+3 = x3       │
│   arg[0].val: 1     ← This is register R0+1 = x1       │
│   arg[1].val: 2     ← This is register R0+2 = x2       │
│                                                          │
│ Instruction 2 (in memory):                              │
│   op:      Oadd                                          │
│   cls:     Kw                                            │
│   to.val:  0        ← This is register R0+0 = x0       │
│   arg[0].val: 4     ← This is register R0+4 = x4       │
│   arg[1].val: 3     ← This is register R0+3 = x3       │
└─────────────────────────────────────────────────────────┘
                            ↓
                    PEEPHOLE DETECTS:
                    • ADD uses result of MUL (both have .val=3)
                    • x3 is single-use
                    • Can fuse!
                            ↓
┌─────────────────────────────────────────────────────────┐
│ EMITTER OUTPUT:                                          │
│   fprintf(f, "\tmadd\t%s, %s, %s, %s\n",                │
│           rname(0, Kw),  // "w0"  ← i->to.val           │
│           rname(1, Kw),  // "w1"  ← prev->arg[0].val    │
│           rname(2, Kw),  // "w2"  ← prev->arg[1].val    │
│           rname(4, Kw)); // "w4"  ← i->arg[0].val       │
│                                                          │
│ ASSEMBLY OUTPUT:                                         │
│   madd w0, w1, w2, w4                                   │
│   # w0 = w4 + (w1 * w2)                                 │
└─────────────────────────────────────────────────────────┘

KEY INSIGHT: All the register numbers (0,1,2,3,4) are ALREADY
             in the instructions! We just read them and emit
             the fused instruction. No choosing needed!
```

---

## How Registers Are Represented

In `all.h`, a `Ref` is just a number:

```c
typedef struct Ref {
    uint32_t val;  // Register number or temp ID
} Ref;
```

After register allocation:
- `r.val >= R0` means it's a physical register
- You can check: `isreg(r)` returns true

---

## In the Emitter: Reading Registers

Look at `arm64/emit.c` lines 207-217:

```c
case '=':
case '0':
    r = c == '=' ? i->to : i->arg[0];
    assert(isreg(r) || req(r, TMP(V31)));
    fputs(rname(r.val, k), e->f);  // ← Just print the register name!
    break;
case '1':
    r = i->arg[1];
    switch (rtype(r)) {
    case RTmp:
        assert(isreg(r));
        fputs(rname(r.val, k), e->f);  // ← Again, just print it!
        break;
    ...
```

**Key point**: The emitter doesn't *allocate* registers. It just *reads* the register numbers that are already in the instruction and prints their names.

---

## Example: MADD Fusion in Action

### Input Instructions (after register allocation):

```
Ins 1: op=Omul, to=x3, arg[0]=x1, arg[1]=x2
       // mul x3, x1, x2

Ins 2: op=Oadd, to=x0, arg[0]=x4, arg[1]=x3
       // add x0, x4, x3
```

### Peephole Pattern Matching:

```c
// We see instruction 2 (ADD) and look back at instruction 1 (MUL)
if (i->op == Oadd && prev->op == Omul) {
    // Check if ADD's second arg is the MUL's result
    if (req(i->arg[1], prev->to)) {
        // All registers are RIGHT THERE in the instructions!
        // prev->arg[0] = x1
        // prev->arg[1] = x2
        // i->arg[0] = x4
        // i->to = x0
        
        // Emit: madd x0, x1, x2, x4
        fprintf(e->f, "\tmadd\t%s, %s, %s, %s\n",
            rname(i->to.val, k),        // x0 (destination)
            rname(prev->arg[0].val, k), // x1 (multiply operand 1)
            rname(prev->arg[1].val, k), // x2 (multiply operand 2)
            rname(i->arg[0].val, k));   // x4 (addend)
    }
}
```

### Output Assembly:

```assembly
madd x0, x1, x2, x4
```

Instead of:

```assembly
mul x3, x1, x2
add x0, x4, x3
```

---

## What About Scratch Registers?

### The MSUB Example

Look at how modulo already uses a scratch register (line 57 in `arm64/emit.c`):

```c
{ Orem, Ki, "sdiv %?, %0, %1\n\tmsub\t%=, %?, %1, %0" }
```

The `%?` token means "use a designated scratch register":

```c
case '?':
    if (KBASE(k) == 0)
        fputs(rname(IP1, k), e->f);  // IP1 = x16 for integers
    else
        fputs(rname(V31, k), e->f);  // V31 for floats
    break;
```

### Why MADD Doesn't Need Scratch Registers

MSUB needs a scratch because it's a **macro** (two instructions):
```assembly
sdiv x16, x0, x1    # Needs temp for division result
msub x0, x16, x1, x0
```

MADD is **one instruction** - all operands come from existing registers:
```assembly
madd x0, x1, x2, x4  # No temp needed!
```

---

## Register Availability: The Real Constraint

### The Question Isn't "What register can we use?"

It's: **"Can we fuse these specific instructions with their specific registers?"**

### Conditions for Fusion:

1. **Single-use check**: The multiply result must only be used once
   ```c
   if (fn->tmp[prev->to.val].nuse == 1) {
       // OK to fuse - no other instruction needs this value
   }
   ```

2. **Register compatibility**: All operands must be registers (not constants or memory)
   ```c
   if (isreg(prev->arg[0]) && isreg(prev->arg[1]) && 
       isreg(i->arg[0]) && isreg(i->arg[1])) {
       // OK - all are registers
   }
   ```

3. **Type compatibility**: All must be same width (w/l for int, s/d for float)
   ```c
   if (prev->cls == i->cls) {
       // OK - matching types
   }
   ```

4. **No interference**: Instructions must be adjacent or provably reorderable
   - In peephole approach: we only look at adjacent instructions
   - This automatically guarantees safety

---

## Comparison with Our Failed Attempt

### What We Tried Before (WRONG):

- Added new 3-arg instructions throughout the pipeline
- Tried to influence register allocation
- Required changes to `icpy()`, `rega()`, `spill()`, etc.
- Register allocator didn't know about 3-arg semantics

### What We're Doing Now (RIGHT):

- Work at emitter level after allocation is done
- Read registers from already-allocated instructions
- No pipeline changes needed
- Register allocator doesn't need to know about fusion

---

## Code Example: Complete Fusion Function

```c
static int
try_madd_fusion(Ins *i, Ins *prev, E *e)
{
    // Pattern: ADD dest, src1, mul_result
    //          where mul_result = MUL arg1, arg2 (single use)
    
    if (i->op != Oadd || prev->op != Omul)
        return 0;  // Not the right pattern
    
    if (i->cls != prev->cls)
        return 0;  // Type mismatch
    
    if (KBASE(i->cls) != 0)
        return 0;  // Only integer MADD for now (TODO: FMADD)
    
    // Check if ADD uses MUL result
    int mul_in_arg0 = req(i->arg[0], prev->to);
    int mul_in_arg1 = req(i->arg[1], prev->to);
    
    if (!mul_in_arg0 && !mul_in_arg1)
        return 0;  // MUL result not used by ADD
    
    // Check single use
    if (e->fn->tmp[prev->to.val].nuse != 1)
        return 0;  // MUL result used elsewhere
    
    // Determine operand order
    // MADD: dest = addend + (mul_op1 * mul_op2)
    Ref addend = mul_in_arg0 ? i->arg[1] : i->arg[0];
    
    // All registers are already allocated - just emit!
    fprintf(e->f, "\tmadd\t%s, %s, %s, %s\n",
        rname(i->to.val, i->cls),        // destination
        rname(prev->arg[0].val, i->cls), // multiply operand 1
        rname(prev->arg[1].val, i->cls), // multiply operand 2
        rname(addend.val, i->cls));      // addend
    
    return 1;  // Fused successfully!
}
```

---

## Summary

### The Answer to "How do we know what register we can use?"

**We don't choose registers - we discover them!**

1. Register allocator has already assigned physical registers
2. Instructions contain register numbers (x0, x1, v0, etc.)
3. We read these numbers and emit the fused instruction
4. No register allocation needed at emitter level

### Why This Works

- **Timing**: We work after register allocation is complete
- **Simplicity**: Just pattern matching on already-allocated instructions
- **Safety**: Register allocator already handled all the hard stuff
- **Proven**: MSUB already works this way for modulo operations

### What We Do Need to Check

- ✅ Single-use constraint (multiply result used once)
- ✅ Type compatibility (same width)
- ✅ Register vs. constant (all operands must be registers)
- ✅ Adjacency (or provable reorderability)

### What We Don't Need to Worry About

- ❌ Choosing which register to use
- ❌ Register allocation conflicts
- ❌ Spilling decisions
- ❌ Register pressure

**The hard work is already done by the time we see the instructions!**

---

**Next:** Ready to implement the peephole fusion in `arm64/emit.c`
