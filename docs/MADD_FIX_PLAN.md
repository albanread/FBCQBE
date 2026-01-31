# MADD/FMADD Fix Plan - Complete arg[2] Propagation

**Date:** 2024-01-31  
**Status:** 🔧 Ready to implement  
**Issue:** Third argument (arg[2]) not propagating from isel to emit

---

## Root Cause

QBE has **hard-coded assumptions** throughout the codebase that instructions have exactly 2 arguments. Even though we extended `Ins.arg[3]`, the following code still only processes/copies the first 2:

1. **Loops that iterate `n < 2`** - Don't process arg[2]
2. **emiti() function** - Only passes arg[0] and arg[1] to emit()
3. **Register allocator** - Only tracks uses of arg[0] and arg[1]
4. **Instruction copying** - Various passes copy instructions but miss arg[2]

---

## Required Changes

### 1. Fix emit() and emiti() in util.c

**Current:**
```c
void emit(int op, int k, Ref to, Ref arg0, Ref arg1);
void emiti(Ins i) {
    emit(i.op, i.cls, i.to, i.arg[0], i.arg[1]);
}
```

**Problem:** emiti() only passes 2 arguments to emit()

**Fix Option A: Update emit() signature (Breaking change)**
```c
void emit(int op, int k, Ref to, Ref arg0, Ref arg1, Ref arg2);
```
Then update ALL ~500 call sites to pass R as 6th argument.

**Fix Option B: Create emit3() helper (Recommended)**
```c
void emit3(int op, int k, Ref to, Ref arg0, Ref arg1, Ref arg2)
{
    if (curi == insb)
        die("emit, too many instructions");
    *--curi = (Ins){
        .op = op, .cls = k,
        .to = to, .arg = {arg0, arg1, arg2}
    };
}
```

**Update emiti():**
```c
void emiti(Ins i)
{
    if (req(i.arg[2], R))
        emit(i.op, i.cls, i.to, i.arg[0], i.arg[1]);
    else
        emit3(i.op, i.cls, i.to, i.arg[0], i.arg[1], i.arg[2]);
}
```

---

### 2. Fix gcm.c - Instruction Scheduling

**File:** `qbe_source/gcm.c`  
**Function:** `schedins()`  
**Line:** ~320

**Current:**
```c
for (i=i0; i<i1; i++)
    for (n=0; n<2; n++) {  // ← Hard-coded 2!
        if (rtype(i->arg[n]) != RTmp)
            continue;
        // ...
    }
```

**Fix:**
```c
for (i=i0; i<i1; i++)
    for (n=0; n<3; n++) {  // ← Changed to 3
        if (rtype(i->arg[n]) != RTmp)
            continue;
        // ...
    }
```

---

### 3. Fix gvn.c - Global Value Numbering

**File:** `qbe_source/gvn.c`

**Function:** `replaceuse()` (Line ~89)

**Current:**
```c
case UIns:
    i = u->u.ins;
    for (n=0; n<2; n++)  // ← Hard-coded 2!
        if (req(i->arg[n], r1))
            i->arg[n] = r2;
```

**Fix:**
```c
case UIns:
    i = u->u.ins;
    for (n=0; n<3; n++)  // ← Changed to 3
        if (req(i->arg[n], r1))
            i->arg[n] = r2;
```

**Function:** `normins()` (Line ~148)

**Current:**
```c
for (n=0; n<2; n++) {  // ← Hard-coded 2!
    if (!KWIDE(argcls(i, n)))
    if (isconbits(fn, i->arg[n], &v))
    // ...
}
```

**Fix:**
```c
for (n=0; n<3; n++) {  // ← Changed to 3
    if (!KWIDE(argcls(i, n)))
    if (isconbits(fn, i->arg[n], &v))
    // ...
}
```

---

### 4. Fix mem.c - Memory Optimization

**File:** `qbe_source/mem.c`  
**Function:** `coalesce()` (Line ~430)

**Current:**
```c
arg = u->u.ins->arg;
for (n=0; n<2; n++)  // ← Hard-coded 2!
    if (req(arg[n], TMP(s->t)))
        arg[n] = TMP(s->s->t);
```

**Fix:**
```c
arg = u->u.ins->arg;
for (n=0; n<3; n++)  // ← Changed to 3
    if (req(arg[n], TMP(s->t)))
        arg[n] = TMP(s->s->t);
```

---

### 5. Fix parse.c - Type Checking

**File:** `qbe_source/parse.c`  
**Function:** `typecheck()` (Line ~850)

**Current:**
```c
for (i=b->ins; i<&b->ins[b->nins]; i++)
    for (n=0; n<2; n++) {  // ← Hard-coded 2!
        k = optab[i->op].argcls[n][i->cls];
        r = i->arg[n];
        // ...
    }
```

**Fix:**
```c
for (i=b->ins; i<&b->ins[b->nins]; i++)
    for (n=0; n<3; n++) {  // ← Changed to 3
        if (n >= 2 && req(i->arg[n], R))
            continue;  // Skip empty 3rd arg for 2-arg instructions
        k = optab[i->op].argcls[n][i->cls];
        r = i->arg[n];
        // ...
    }
```

---

### 6. Fix spill.c - Register Spilling (CRITICAL!)

**File:** `qbe_source/spill.c`  
**Function:** `spill()` (Multiple locations)

**Location 1: Line ~443**

**Current:**
```c
j = T.memargs(i->op);
for (n=0; n<2; n++)  // ← Hard-coded 2!
    if (rtype(i->arg[n]) == RMem)
        j--;
for (n=0; n<2; n++)  // ← Hard-coded 2!
    switch (rtype(i->arg[n])) {
```

**Fix:**
```c
j = T.memargs(i->op);
for (n=0; n<3; n++)  // ← Changed to 3
    if (rtype(i->arg[n]) == RMem)
        j--;
for (n=0; n<3; n++)  // ← Changed to 3
    switch (rtype(i->arg[n])) {
```

**Location 2: Line ~470**

**Current:**
```c
for (n=0; n<2; n++)  // ← Hard-coded 2!
    if (rtype(i->arg[n]) == RTmp) {
        t = i->arg[n].val;
```

**Fix:**
```c
for (n=0; n<3; n++)  // ← Changed to 3
    if (rtype(i->arg[n]) == RTmp) {
        t = i->arg[n].val;
```

---

### 7. Update arm64/isel.c to use emit3()

**File:** `qbe_source/arm64/isel.c`  
**Function:** `sel()`

**Current:**
```c
emit(fused_op, i.cls, i.to, def->arg[0], def->arg[1]);
Ins *fused_ins = curi;
fused_ins->arg[2] = acc;
```

**Fix:**
```c
emit3(fused_op, i.cls, i.to, def->arg[0], def->arg[1], acc);
```

Then fix the fixarg calls:
```c
fixarg(&curi->arg[0], i.cls, 0, fn);
fixarg(&curi->arg[1], i.cls, 0, fn);
fixarg(&curi->arg[2], i.cls, 0, fn);
```

---

### 8. Add emit3() prototype to all.h

**File:** `qbe_source/all.h`

Add after the emit() declaration:
```c
void emit(int, int, Ref, Ref, Ref);
void emit3(int, int, Ref, Ref, Ref, Ref);  // ← Add this
void emiti(Ins);
```

---

## Implementation Order

1. ✅ **util.c** - Add emit3() function
2. ✅ **util.c** - Update emiti() to check arg[2]
3. ✅ **all.h** - Add emit3() prototype
4. ✅ **spill.c** - Fix all n<2 loops (CRITICAL - This is likely the smoking gun!)
5. ✅ **gcm.c** - Fix scheduling loop
6. ✅ **gvn.c** - Fix replaceuse() and normins()
7. ✅ **mem.c** - Fix coalesce()
8. ✅ **parse.c** - Fix typecheck()
9. ✅ **arm64/isel.c** - Use emit3() instead of manual arg[2] setting
10. ✅ **Test** - Rebuild and verify assembly output

---

## Testing After Fix

### Test Command
```bash
cd qbe_basic_integrated
./build.sh
./qbe_basic ../tests/arithmetic/test_madd_simple.bas > test_madd_simple.s
grep -E "(fmadd|fmsub|madd|msub)" test_madd_simple.s
```

### Expected Output (CORRECT)
```assembly
madd    x0, x0, x1, x2    # ✓ All 4 operands present!
fmadd   d0, d0, d1, d2    # ✓ All 4 operands present!
msub    x0, x0, x1, x2    # ✓ All 4 operands present!
fmsub   d0, d0, d1, d2    # ✓ All 4 operands present!
```

### Current Output (BROKEN)
```assembly
madd    x0, x0, x1,       # ✗ Missing 4th operand
fmadd   d0, d0, d1,       # ✗ Missing 4th operand
```

---

## Why This Fixes It

The **register allocator and spiller** run AFTER instruction selection. They:

1. Scan all instructions looking at arg[0] and arg[1]
2. Build use/def chains
3. Allocate physical registers
4. **Copy instructions** to new blocks

When they copy instructions, they only look at the first 2 arguments because of the `n<2` loops. This means:

- arg[2] never gets its register allocated
- arg[2] is effectively lost during the copy
- By the time emit runs, arg[2] is empty (R)

By changing all `n<2` to `n<3`, we ensure:
- arg[2] gets tracked through all passes
- arg[2] gets a register assigned
- arg[2] survives the journey from isel to emit

---

## Files to Modify

| File | Lines | Changes |
|------|-------|---------|
| `all.h` | 1 | Add emit3() prototype |
| `util.c` | ~15 | Add emit3(), update emiti() |
| `spill.c` | 3 locations | Change n<2 to n<3 |
| `gcm.c` | 1 location | Change n<2 to n<3 |
| `gvn.c` | 2 locations | Change n<2 to n<3 |
| `mem.c` | 1 location | Change n<2 to n<3 |
| `parse.c` | 1 location | Change n<2 to n<3 + skip empty |
| `arm64/isel.c` | 1 location | Use emit3() |

**Total: 8 files, ~10 locations**

---

## Verification Checklist

After implementing all changes:

- [ ] Rebuild with `./build.sh`
- [ ] Compile test: `./qbe_basic test_madd_simple.bas > test.s`
- [ ] Check assembly: `grep madd test.s` shows 4 operands
- [ ] Run test: `as test.s -o test.o && ld test.o -o test && ./test`
- [ ] Verify correctness: Results match expected values
- [ ] Run full test suite: `cd tests && ./run_tests.sh`

---

## Success Criteria

✅ Assembly output shows complete instructions:
```assembly
madd    x0, x0, x1, x2
fmadd   d0, d0, d1, d2
msub    x0, x0, x1, x2
fmsub   d0, d0, d1, d2
```

✅ Test programs produce correct numerical results

✅ No regressions in existing tests

---

**Status:** Ready to implement systematically, file by file.