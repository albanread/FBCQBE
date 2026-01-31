# Logical vs Physical Registers: What the Peephole Sees

**Date:** 2026-01-31  
**Question:** "Are we still using logical temps when we use the peephole optimizer?"  
**Answer:** NO - by the peephole stage, logical temps are replaced with physical registers.

---

## The Compilation Order (from main.c lines 140-147)

```c
T.isel(fn);      // Instruction selection
fillcfg(fn);
filllive(fn);
fillloop(fn);
fillcost(fn);
spill(fn);       // Spilling pass - converts some temps to stack slots
rega(fn);        // Register allocation - converts remaining temps to physical registers
fillcfg(fn);
simpljmp(fn);
fillcfg(fn);
...
T.emitfn(fn, outf);  // Emitter - THIS IS WHERE PEEPHOLE RUNS
```

**Key insight:** The emitter (`T.emitfn`) runs AFTER `rega(fn)`.

---

## What Happens at Each Stage

### Before `spill(fn)`: Logical/Virtual Temps

Instructions use virtual temporaries:

```qbe
%t0 =w mul %t1, %t2
%t3 =w add %t4, %t0
ret %t3
```

These `%t0`, `%t1`, etc. are NOT physical registers - they're just numbered placeholders.

### After `spill(fn)`: Some Temps → Stack Slots

If register pressure is high, some temps get spilled to the stack:

```
%t0 =w mul %x1, %x2    // Still a temp (will be allocated)
%slot5 =w store %t0    // Spilled to stack
%t6 =w load %slot5     // Load from stack
```

The `%slot5` is now type `RSlot`, not `RTmp`.

### After `rega(fn)`: Physical Registers

All remaining `RTmp` values are mapped to physical registers:

```
R3 =w mul R1, R2       // R3 = x3, R1 = x1, R2 = x2
R0 =w add R4, R3       // R0 = x0, R4 = x4
ret R0
```

The `Ref.val` field now contains register numbers:
- `R0` through `R30` (integer registers x0-x30)
- `V0` through `V30` (vector registers v0-v30, or s0-s30/d0-d30)

### At Emitter Stage: Physical Registers + Slots

The emitter sees a mix:
- **Physical registers**: Most values (after rega)
- **Stack slots**: Spilled values (from spill pass)
- **Constants**: Immediate values
- **Memory addresses**: For loads/stores

---

## How to Tell What Type a Ref Is

In `all.h`, there are type-checking macros:

```c
#define rtype(r) ((r).val & 3)  // Bottom 2 bits encode type

enum {
    RTmp = 0,    // Temporary (physical register after rega)
    RSlot = 1,   // Stack slot
    RCon = 2,    // Constant
    RCall = 3,   // Call target
    // ... etc
};
```

After `rega()`, all `RTmp` refs point to physical registers.

---

## What the Emitter Actually Sees

Look at `arm64/emit.c` line 213-217:

```c
case '0':
    r = i->arg[0];
    assert(isreg(r) || req(r, TMP(V31)));  // ← Asserts it's a physical register!
    fputs(rname(r.val, k), e->f);
    break;
```

The `assert(isreg(r))` check **requires** a physical register at this point.

### The `isreg()` Function

From `all.h`:

```c
static inline int
isreg(Ref r)
{
    return rtype(r) == RTmp && r.val >= Tmp0;
}
```

Where `Tmp0` is the start of the physical register range (usually `R0`).

So `isreg(r)` returns true only if:
1. Type is `RTmp` (not slot, not constant)
2. Value is in the physical register range

---

## Example: Tracing a Temp Through the Pipeline

### Input QBE IR:
```qbe
%a =w mul %b, %c
%d =w add %e, %a
ret %d
```

### After Parsing:
```
Tmp #100 = mul Tmp #101, Tmp #102
Tmp #103 = add Tmp #104, Tmp #100
ret Tmp #103
```

### After Register Allocation (rega):
```
R3 = mul R1, R2          // Tmp #100 → R3 (x3)
R0 = add R4, R3          // Tmp #103 → R0 (x0)
ret R0
```

Note: `R3` is an enum value like `(R0 + 3)`, which equals some integer constant.

### In Emitter (what peephole sees):
```c
Ins 1:
  op = Omul
  to.val = 3              // Physical register x3
  arg[0].val = 1          // Physical register x1
  arg[1].val = 2          // Physical register x2

Ins 2:
  op = Oadd
  to.val = 0              // Physical register x0
  arg[0].val = 4          // Physical register x4
  arg[1].val = 3          // Physical register x3
```

**These are physical register numbers!**

### What Peephole Does:
```c
// Just read the register numbers and emit fused instruction:
fprintf(f, "\tmadd\tw%d, w%d, w%d, w%d\n",
    0,  // x0 (from Ins 2 to.val)
    1,  // x1 (from Ins 1 arg[0].val)
    2,  // x2 (from Ins 1 arg[1].val)
    4); // x4 (from Ins 2 arg[0].val)
```

Output:
```assembly
madd w0, w1, w2, w4
```

---

## What About Spilled Values?

If a value was spilled, it becomes type `RSlot`:

```c
case RSlot:
    // This is a stack slot, not a register
    fprintf(e->f, "[x29, %"PRIu64"]", slot(r, e));
    break;
```

**Important:** Spilled values cannot be fused! They're not in registers.

Our fusion check must verify all operands are registers:

```c
if (!isreg(prev->arg[0]) || !isreg(prev->arg[1]) || !isreg(i->arg[0]))
    return 0;  // Cannot fuse - operand is spilled or constant
```

---

## Summary Table

| Stage | What Temps Look Like | Can We Fuse? |
|-------|---------------------|--------------|
| After parse | `%t0`, `%t1` - logical temps | ❌ No - not physical yet |
| After isel | Still logical temps | ❌ No - not physical yet |
| After spill | Mix of temps + `RSlot` | ❌ No - not physical yet |
| After rega | Physical registers (`R0-R30`, `V0-V30`) | ✅ **YES - THIS IS WHERE PEEPHOLE WORKS** |
| In emitter | Physical registers + slots + constants | ✅ Yes - this is our stage |

---

## Key Takeaways

1. **Peephole runs in the emitter, which is AFTER register allocation**

2. **All `RTmp` refs at emitter stage are physical registers**
   - Check: `isreg(r)` returns true
   - Value: `r.val` is a register number (0-30 for x0-x30, etc.)

3. **We don't choose registers - we read them from instructions**
   ```c
   // These values are already physical register numbers:
   int dest_reg = i->to.val;           // e.g., 0 for x0
   int mul_op1 = prev->arg[0].val;     // e.g., 1 for x1
   int mul_op2 = prev->arg[1].val;     // e.g., 2 for x2
   int addend = i->arg[0].val;         // e.g., 4 for x4
   ```

4. **Spilled values are NOT temps - they're `RSlot`**
   - We must check `isreg()` before fusing
   - Cannot fuse if operand is spilled to stack

5. **This is why peephole is simple and safe**
   - No register allocation needed
   - No liveness analysis needed
   - Just pattern matching on already-allocated instructions

---

## Answer to the Original Question

**"Are we still using logical temps when we use the peephole optimizer?"**

**NO.** By the time we reach the peephole (in the emitter), all logical temps have been:
- Either converted to **physical registers** (x0-x30, v0-v30)
- Or spilled to **stack slots** (type `RSlot`)

The peephole sees and works with **physical registers only**.

---

**Verification in Code:**

See `arm64/emit.c` line 213:
```c
assert(isreg(r) || req(r, TMP(V31)));
```

This assertion would **fail** if `r` were still a logical temp. The fact that QBE doesn't crash here proves that by emitter stage, all `RTmp` values are physical registers.

---

**Next:** Ready to implement MADD fusion with confidence that we're working with physical registers!