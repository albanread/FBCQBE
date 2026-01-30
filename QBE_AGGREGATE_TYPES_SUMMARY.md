# QBE Aggregate Types - Summary and Recommendations

**Date:** January 2025  
**Status:** 🟡 FasterBASIC NOT using QBE aggregate types (acceptable, but opportunity exists)

---

## Key Discovery

**QBE DOES have native aggregate/struct types**, but FasterBASIC currently uses manual pointer arithmetic instead.

---

## QBE Aggregate Type Capabilities (Official)

### 1. Type Definitions

```qbe
type :Point = { w, d }              # word + double
type :Array = { w 100 }             # 100 words (array syntax)
type :Aligned = align 16 { w 4 }    # Explicit alignment
type :Nested = { :Point, :Point }   # Nested types
```

### 2. Union Types

```qbe
type :Variant = {
    { w }       # Option 1
    { d }       # Option 2
    { l, l }    # Option 3
}
```

### 3. Opaque Types

```qbe
type :Opaque = align 16 { 32 }   # 32 bytes, alignment only
```

---

## How QBE Uses Aggregate Types

### Function Parameters (Automatic Pointer Passing)

```qbe
type :Point = { w, d }

function w $getX(:Point %p) {
@start
    %x =w loadw %p      # %p is pointer to struct
    ret %x
}
```

**Key:** Aggregate parameters are automatically passed as pointers.

### Function Returns (ABI-Aware)

**Small structs (≤ 16 bytes):**
- Returned in registers automatically
- `{ w, w }` → RAX, RDX
- `{ d, d }` → XMM0, XMM1
- `{ w, d }` → RAX, XMM0

**Large structs (> 16 bytes):**
- Use hidden pointer parameter (MEMORY class)
- QBE handles ABI automatically

---

## FasterBASIC's Current Approach (Manual)

### What It Does

```basic
TYPE Point
    X AS INTEGER
    Y AS DOUBLE
END TYPE
DIM P AS Point
P.X = 42
```

### Generated QBE (Current)

```qbe
# No type definition emitted
%P =l alloc8 16

# Manual pointer arithmetic
%x_ptr =l add %P, 0
storew 42, %x_ptr
```

**Offsets computed at compile-time, raw pointer math used.**

---

## Why Current Approach Works

### Advantages

1. **Simple and Explicit**
   - Direct pointer arithmetic
   - Predictable code generation
   - Easy to debug

2. **Full Control**
   - Exact instruction sequence
   - No ABI surprises
   - Platform-independent templates

3. **Works for All Field Access**
   - Nested members: `O.Item.Value`
   - Array elements: `Points(i).X`
   - No special cases

### What Aggregate Types DON'T Help With

**Field access still requires pointer math:**
```qbe
type :Point = { w, d }
%p =l alloc8 16

# Still need manual offsets!
%x =w loadw %p           # Field 0
%y_ptr =l add %p, 8      # Field 1 at +8
%y =d loadd %y_ptr
```

**No field-by-name syntax in QBE.**

---

## Where Aggregate Types WOULD Help

### 1. Function Signatures (Type Safety)

**Current:**
```qbe
function w $distance(l %p1, l %p2)  # What type are these pointers?
```

**With types:**
```qbe
type :Point = { w, d }
function d $distance(:Point %p1, :Point %p2)  # Clear types
```

**Benefit:** QBE validates parameter types.

### 2. Small Struct Returns (Performance)

**Current (requires heap):**
```qbe
function l $makePoint() {
    %p =l call $malloc(l 16)    # Heap allocation
    # ... initialize ...
    ret %p                       # Caller must free
}
```

**With aggregate types:**
```qbe
type :Point = { w, d }
function :Point $makePoint() {
    %p =l alloc8 16              # Stack allocation
    # ... initialize ...
    ret %p                       # Returned in RAX+XMM0
}
```

**Benefit:** Avoid heap allocation, return in registers.

### 3. IL Readability

**Type names instead of generic pointers:**
```qbe
type :Player = { w, w, d, d, l }  # id, health, x, y, name

function :Player $createPlayer(w %id)
function w $getHealth(:Player %p)
```

**Benefit:** Self-documenting code.

---

## Recommendation: Hybrid Approach

### Phase 1: Add Type Definitions (Safe, Low Effort)

**Emit definitions for documentation:**
```qbe
type :Point = { w, d }
type :Person = { l, w }
type :Outer = { :Inner, w }
```

**Keep current codegen unchanged.**

**Benefit:** IL becomes more readable, no behavior change.

---

### Phase 2: Use Types for Function Parameters (When Implemented)

**When adding SUB/FUNCTION UDT support:**
```qbe
type :Point = { w, d }

function w $process(:Point %p) {
    # Pointer passed automatically
}
```

**Benefit:** Type safety, clearer signatures.

---

### Phase 3: Consider Struct Returns (Advanced)

**Only for small structs:**
```qbe
type :Vec2 = { d, d }

function :Vec2 $add(:Vec2 %a, :Vec2 %b) {
    # QBE handles register return
}
```

**Benefit:** Performance (no heap allocation).

**Risk:** Complex ABI rules, platform differences.

---

## What NOT to Change

### Keep Pointer Math For:

1. **Field Access**
   ```qbe
   %x_ptr =l add %P, 0
   %x =w loadw %x_ptr
   ```
   **Reason:** Aggregate types don't improve this.

2. **Local Variables**
   ```qbe
   %p =l alloc8 16
   ```
   **Reason:** No benefit, adds complexity.

3. **Nested Member Access**
   ```qbe
   %value_ptr =l add %outer, 0
   ```
   **Reason:** Manual offset calculation still needed.

---

## Priority Assessment

### Immediate (HIGH PRIORITY)
❗ **Fix the 2 critical bugs:**
1. String field type inference
2. UDT array descriptor initialization

**Estimate:** 4-6 hours

---

### Short-Term (MEDIUM PRIORITY)
🔧 **Expand test coverage:**
- Test member assignment on LHS
- Test all numeric types
- Test REDIM with UDT arrays

**Estimate:** 4-6 hours

---

### Medium-Term (LOW PRIORITY)
📝 **Consider aggregate types:**
- Phase 1: Emit type definitions (1-2 hours)
- Phase 2: Use in function signatures (4-6 hours)
- Phase 3: Struct returns (8-12 hours)

**Total:** 13-20 hours

---

## Decision Matrix

| Use Case | Current Approach | With Aggregate Types | Winner |
|----------|-----------------|---------------------|--------|
| Field access | Manual offset | Manual offset | **Tie** |
| Local variables | `alloc8` | `alloc8` | **Tie** |
| Function params | Generic `l` | Typed `:Name` | **Aggregate** |
| Function returns | Heap/pointer | Register return | **Aggregate** |
| IL readability | Generic | Named types | **Aggregate** |
| Simplicity | ✅ Simple | ⚠️ More complex | **Current** |
| Debugging | ✅ Explicit | ⚠️ ABI magic | **Current** |

**Overall:** Current approach is fine for now, aggregate types add value for functions.

---

## Conclusion

### FasterBASIC's UDT Implementation is Sound

✅ **Current manual pointer arithmetic approach:**
- Works correctly
- Simple to understand
- Easy to debug
- Predictable code generation

### QBE Aggregate Types are Underutilized

📊 **Potential benefits:**
- Type safety in function signatures
- Performance gains for struct returns
- Better IL documentation

### Recommended Action

**NOW:** Fix the 2 critical bugs (string fields, UDT arrays)  
**NEXT:** Expand test coverage, verify stability  
**LATER:** Consider aggregate types when implementing UDT function parameters/returns

**Bottom Line:** Don't change what's working. Use aggregate types strategically when implementing new features.

---

## Quick Reference: QBE Aggregate Syntax

### Type Definition
```qbe
type :TypeName = { FIELD_TYPES }
```

### Example Types
```qbe
type :Point = { w, w }                    # Two words
type :Rect = { :Point, :Point }           # Nested
type :Buffer = align 16 { b 256 }         # Array with alignment
type :Opaque = align 8 { 64 }             # Size only
```

### Function Usage
```qbe
function w $func(:Point %p)               # Parameter
function :Point $create()                 # Return (small struct)
```

### Limitations
- ❌ No field-by-name access (`p.x` syntax)
- ❌ No automatic field offset calculation
- ❌ Must use pointer arithmetic for members
- ✅ ABI-aware parameter passing
- ✅ Register returns for small structs

---

**Status:** Document complete. Proceed with bug fixes, revisit aggregate types later.