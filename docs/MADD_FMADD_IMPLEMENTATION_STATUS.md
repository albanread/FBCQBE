# MADD/FMADD Fusion Implementation Status

**Date:** 2024-01-31  
**Status:** 🟡 **PARTIALLY WORKING** - Fusion detection works, arg[2] emission issue  
**Priority:** CRITICAL

---

## Executive Summary

We implemented automatic MADD/FMADD fusion in the QBE ARM64 backend. The fusion logic successfully detects multiply-add patterns and emits fused instructions, but there is a **critical issue with the third operand (accumulator) not being emitted in the assembly output**.

**Current Status:**
- ✅ Opcode definitions added (Oamadd, Oamsub, Oafmadd, Oafmsub)
- ✅ Emission patterns added with %2 token for third argument
- ✅ Pattern detection in isel.c correctly identifies fusion opportunities
- ✅ Fused instructions ARE being emitted (we see `madd`, `fmadd`, etc. in output)
- ❌ **Third argument (accumulator) is missing** - trailing comma in assembly

**Assembly Output:**
```assembly
madd	x0, x0, x1,    # ← Missing third operand!
fmadd	d0, d0, d1,    # ← Missing third operand!
```

**Expected Output:**
```assembly
madd	x0, x0, x1, x2    # x0 = x2 + (x0 * x1)
fmadd	d0, d0, d1, d2    # d0 = d2 + (d0 * d1)
```

---

## Changes Made

### 1. Extended Instruction Structure (`all.h`)

**File:** `qbe_basic_integrated/qbe_source/all.h`

**Change:** Extended `Ins` structure to support 3 arguments:

```c
struct Ins {
    uint op:30;
    uint cls:2;
    Ref to;
    Ref arg[3];  /* Extended to 3 args for MADD/FMADD/MSUB/FMSUB */
};
```

**Previous:** `arg[2]` (only 2 arguments)  
**Reason:** ARM64 MADD/FMADD instructions require 3 source operands: multiply operands + accumulator

---

### 2. Added New Opcodes (`ops.h`)

**File:** `qbe_basic_integrated/qbe_source/ops.h`

**Change:** Added 4 new internal opcodes for fused multiply-add/subtract:

```c
/* Fused Multiply-Add/Sub (ARM64) */
O(amadd,   T(w,l,e,e, w,l,e,e), F(0,0,0,0,0,0,0,0,0,0)) X(0,0,0) V(0)
O(amsub,   T(w,l,e,e, w,l,e,e), F(0,0,0,0,0,0,0,0,0,0)) X(0,0,0) V(0)
O(afmadd,  T(e,e,s,d, e,e,s,d), F(0,0,0,0,0,0,0,0,0,0)) X(0,0,0) V(0)
O(afmsub,  T(e,e,s,d, e,e,s,d), F(0,0,0,0,0,0,0,0,0,0)) X(0,0,0) V(0)
```

- `Oamadd` / `Oamsub` - Integer multiply-add/subtract (32-bit/64-bit)
- `Oafmadd` / `Oafmsub` - Floating-point multiply-add/subtract (single/double)

**Naming:** Prefix `a` denotes ARM64-specific internal opcodes

---

### 3. Added Assembly Emission Patterns (`arm64/emit.c`)

**File:** `qbe_basic_integrated/qbe_source/arm64/emit.c`

**Change 1:** Added emission patterns in `omap[]` array:

```c
{ Oamadd,  Ki, "madd %=, %0, %1, %2" },
{ Oamsub,  Ki, "msub %=, %0, %1, %2" },
{ Oafmadd, Ka, "fmadd %=, %0, %1, %2" },
{ Oafmsub, Ka, "fmsub %=, %0, %1, %2" },
```

**Change 2:** Added support for `%2` token in `emitf()` function:

```c
case '2':
    r = i->arg[2];
    /* Skip empty third argument (for non-3-arg instructions) */
    if (req(r, R))
        break;
    switch (rtype(r)) {
    default:
        die("invalid third argument");
    case RTmp:
        assert(isreg(r));
        fputs(rname(r.val, k), e->f);
        break;
    case RCon:
        pc = &e->fn->con[r.val];
        n = pc->bits.i;
        assert(pc->type == CBits);
        if (n >> 24) {
            assert(arm64_logimm(n, k));
            fprintf(e->f, "#%"PRIu64, n);
        } else if (n & 0xfff000) {
            assert(!(n & ~0xfff000ull));
            fprintf(e->f, "#%"PRIu64", lsl #12", n>>12);
        } else {
            assert(!(n & ~0xfffull));
            fprintf(e->f, "#%"PRIu64, n);
        }
        break;
    }
    break;
```

**Purpose:** Handle third argument in format strings like `"madd %=, %0, %1, %2"`

---

### 4. Implemented Fusion Detection (`arm64/isel.c`)

**File:** `qbe_basic_integrated/qbe_source/arm64/isel.c`

**Change:** Added peephole optimization in `sel()` function (before default `emiti(i)` fallthrough):

```c
/* Try to fuse multiply-add patterns: add(x, mul(a,b)) -> madd(a,b,x) */
if (i.op == Oadd || i.op == Osub) {
    /* Try both argument positions (add is commutative) */
    for (int idx = 0; idx < 2; idx++) {
        Ref r = i.arg[idx];
        
        /* Only fuse subtract when mul is arg[1]: sub(acc, mul) */
        if (i.op == Osub && idx == 0)
            continue;
        
        if (rtype(r) != RTmp)
            continue;
        
        Tmp *t = &fn->tmp[r.val];
        
        /* Check single-use and has definition */
        if (t->nuse != 1 || !t->def)
            continue;
        
        Ins *def = t->def;
        
        /* Check if definition is multiply with matching class */
        if (def->op != Omul || def->cls != i.cls)
            continue;
        
        /* FUSION OPPORTUNITY FOUND */
        Ref acc = i.arg[1 - idx];  /* The other argument (accumulator) */
        int fused_op;
        
        if (i.cls == Kw || i.cls == Kl) {
            /* Integer madd/msub */
            fused_op = (i.op == Oadd) ? Oamadd : Oamsub;
        } else if (i.cls == Ks || i.cls == Kd) {
            /* Float fmadd/fmsub */
            fused_op = (i.op == Oadd) ? Oafmadd : Oafmsub;
        } else {
            continue;  /* Unsupported class */
        }
        
        /* Emit fused instruction: result = acc + (arg0 * arg1) */
        /* ARM64 syntax: madd Xd, Xn, Xm, Xa  means  Xd = Xa + (Xn * Xm) */
        emit(fused_op, i.cls, i.to, def->arg[0], def->arg[1]);
        
        /* Save pointer to the fused instruction before fixarg modifies curi */
        Ins *fused_ins = curi;
        fused_ins->arg[2] = acc;  /* Set accumulator as 3rd argument */
        
        /* Fix up all three arguments - fixarg may emit more instructions */
        fixarg(&fused_ins->arg[0], i.cls, 0, fn);
        fixarg(&fused_ins->arg[1], i.cls, 0, fn);
        fixarg(&fused_ins->arg[2], i.cls, 0, fn);
        
        /* Mark multiply instruction as dead */
        def->op = Onop;
        
        return;
    }
}
```

**Algorithm:**
1. Detect `add(x, mul(a,b))` or `sub(x, mul(a,b))` patterns
2. Verify the multiply result has single use (`t->nuse == 1`)
3. Verify class matches (Kw/Kl for integer, Ks/Kd for float)
4. Emit fused instruction with accumulator as third operand
5. Mark original multiply as dead (Onop)

---

### 5. Updated emit() Helper (`util.c`)

**File:** `qbe_basic_integrated/qbe_source/util.c`

**Change:** Initialize third argument slot to R (empty):

```c
void
emit(int op, int k, Ref to, Ref arg0, Ref arg1)
{
    if (curi == insb)
        die("emit, too many instructions");
    *--curi = (Ins){
        .op = op, .cls = k,
        .to = to, .arg = {arg0, arg1, R}
    };
}
```

**Reason:** With `arg[3]`, we must initialize all three slots. The third slot defaults to R (empty ref).

---

## The Problem

### Symptom

Assembly output shows fused instructions but with **trailing comma** (missing third operand):

```assembly
_TestMaddInt:
    hint    #34
    stp     x29, x30, [sp, -16]!
    mov     x29, sp
    madd    x0, x0, x1,    # ← WRONG: Missing accumulator!
    ldp     x29, x30, [sp], 16
    ret
```

### Root Cause Analysis

**The accumulator register (arg[2]) is not being emitted.**

We set `fused_ins->arg[2] = acc` in `isel.c`, but when the emitter runs in `emit.c`, it finds `arg[2] == R` (empty ref) and skips printing it:

```c
case '2':
    r = i->arg[2];
    if (req(r, R))    // ← This is TRUE - arg[2] is empty!
        break;        // ← Skips printing, leaving trailing comma
```

### Why is arg[2] empty?

**Theory 1: Instruction gets copied**
- QBE may copy instructions during register allocation or other passes
- The copy might not preserve arg[2]

**Theory 2: fixarg() invalidates the pointer**
- `fixarg()` calls `emit()` which modifies `curi`
- Even though we saved `fused_ins` pointer, the instruction buffer might be reallocated
- Or the instruction gets copied/moved

**Theory 3: Instruction buffer structure**
- Instructions are emitted in reverse order into a buffer (`insb[]`)
- The buffer might be copied to another location before emission
- The copy operation might only copy arg[0] and arg[1]

### Evidence

1. **Fusion detection works:** The `madd`/`fmadd` opcodes ARE in the output
2. **Pattern matching works:** Only the right patterns trigger fusion
3. **arg[2] gets set:** We can trace that `fused_ins->arg[2] = acc` executes
4. **But arg[2] is empty at emission time:** The emitter sees `R` in arg[2]

---

## Test Results

### Test File
`tests/arithmetic/test_madd_simple.bas`

### QBE IL Output (Correct)
```qbe
export function l $TestMaddInt(l %a, l %b, l %c) {
@start
    %t94 =l mul %a, %b
    %t95 =l add %c, %t94
    ret %t95
}
```

Pattern: `mul` followed by `add` with single-use temporary → **Should fuse**

### Assembly Output (Incorrect)
```assembly
_TestMaddInt:
    madd    x0, x0, x1,    # Missing x2 (accumulator)
```

**Expected:**
```assembly
_TestMaddInt:
    madd    x0, x0, x1, x2    # x0 = x2 + (x0 * x1)
```

---

## Debugging Attempts

### Attempt 1: Fixed iarg pointer invalidation
**Problem:** `iarg = curi->arg` was getting invalidated when `fixarg()` changed `curi`  
**Fix:** Use `curi->arg[N]` directly instead of cached pointer  
**Result:** Didn't help - arg[2] still empty

### Attempt 2: Saved fused_ins pointer before fixarg
**Problem:** `fixarg()` might invalidate `curi` pointer  
**Fix:** Save `Ins *fused_ins = curi` before calling `fixarg()`  
**Result:** Didn't help - arg[2] still empty

### Attempt 3: Added req(r, R) check in emitter
**Problem:** Emitter crashed on empty arg[2]  
**Fix:** Skip printing if `req(r, R)` is true  
**Result:** No crash, but arg[2] not printed (because it's empty!)

---

## Next Steps to Fix

### Investigation Needed

1. **Trace instruction lifecycle**
   - Add debug prints in `isel.c` when setting arg[2]
   - Add debug prints in `emit.c` when reading arg[2]
   - Find where arg[2] gets lost

2. **Check instruction copying**
   - Search for code that copies `Ins` structures
   - Check if `icpy()`, `idup()`, or similar functions exist
   - Verify they copy all 3 arguments

3. **Check register allocation**
   - The register allocator (`rega.c`) might process instructions
   - It might copy instructions and only preserve arg[0..1]

4. **Check instruction buffer management**
   - How does `insb[]` get copied to block's `ins[]` array?
   - Does `idup()` or similar copy all arguments?

### Potential Fixes

**Option 1: Find and fix instruction copy code**
```c
// In any code that copies instructions:
dest->arg[0] = src->arg[0];
dest->arg[1] = src->arg[1];
dest->arg[2] = src->arg[2];  // ← Add this!
```

**Option 2: Store accumulator differently**
```c
// Instead of using arg[2], use a different field?
// Or store in the temporary's metadata?
```

**Option 3: Emit accumulator load explicitly**
```c
// Instead of trying to use arg[2], emit a sequence:
//   mov temp, accumulator
//   madd dest, arg0, arg1, temp
// This uses only 2-arg instructions
```

---

## Files Modified

1. `qbe_basic_integrated/qbe_source/all.h` - Extended Ins.arg[3]
2. `qbe_basic_integrated/qbe_source/ops.h` - Added opcodes
3. `qbe_basic_integrated/qbe_source/arm64/emit.c` - Added emitters + %2 support
4. `qbe_basic_integrated/qbe_source/arm64/isel.c` - Fusion detection
5. `qbe_basic_integrated/qbe_source/util.c` - Initialize arg[2] in emit()

---

## References

- ARM64 MADD instruction: `madd Xd, Xn, Xm, Xa` → `Xd = Xa + (Xn * Xm)`
- ARM64 FMADD instruction: `fmadd Dd, Dn, Dm, Da` → `Dd = Da + (Dn * Dm)`
- QBE documentation: `qbe_source/doc/il.txt`
- Investigation document: `docs/MADD_FMADD_FUSION_INVESTIGATION.md`

---

**Status:** Ready for debugging - Need to find where arg[2] gets lost between isel and emit phases.