# QBE Aggregate Types Analysis - Actual Capabilities

**Date:** January 2025  
**Purpose:** Correct understanding of QBE's native aggregate type support  
**Status:** 🔴 FasterBASIC is NOT using QBE aggregate types (should consider it)

---

## Executive Summary

**CRITICAL DISCOVERY:** QBE **DOES** have native aggregate/struct types, but FasterBASIC is currently **NOT using them**. Instead, FasterBASIC uses manual pointer arithmetic for all UDT field access.

**Impact:** 
- ✅ Current approach works and is explicit
- ❌ Missing potential optimizations from QBE's ABI integration
- ❌ More verbose IL code
- ⚠️ Function parameters/returns would benefit from aggregate types

---

## QBE Aggregate Type Features (From Official Docs)

### 1. Type Definitions

**Syntax:**
```qbe
type :typename = ['align' NUMBER] '{' (SUBTY [NUMBER])+ '}'
```

**Examples from QBE docs:**

```qbe
# Four floats
type :fourfloats = { s, s, d, d }

# Byte followed by 100 words (array syntax)
type :abyteandmanywords = { b, w 100 }

# Explicit alignment (for SIMD)
type :cryptovector = align 16 { w 4 }

# Nested aggregate types
type :point = { w, w }
type :rect = { :point, :point }
```

### 2. Union Types

**QBE supports unions:**
```qbe
type :union_name = {
    { b }      # Variation 1: single byte
    { s }      # Variation 2: single short
    { w, w }   # Variation 3: two words
}
```

Size and alignment = maximum of all variations.

### 3. Opaque Types

**When structure is unknown but size/alignment known:**
```qbe
type :opaque = align 16 { 32 }   # 32 bytes, 16-byte aligned
```

---

## How QBE Uses Aggregate Types

### Function Parameters (Pass by Pointer)

**From QBE docs:**
> "When an argument has an aggregate type, a pointer to the aggregate is passed by the caller."

**Example:**
```qbe
type :one = { w }

function w $getone(:one %p) {
@start
        %val =w loadw %p     # %p is a POINTER to the struct
        ret %val
}
```

**Key Point:** Aggregate parameters are automatically passed as pointers.

### Function Returns

**ABI Integration:**
- Small structs (≤ 16 bytes) can be returned in registers
- QBE handles the ABI details automatically
- Large structs use hidden pointer parameter (MEMORY class)

**From abi.txt:**
> "If the size of an object is larger than two eightbytes or if contains unaligned fields, it has class MEMORY."

**Register return examples:**
- `{ w, w }` → returned in `%rax`, `%rdx` (INTEGER class)
- `{ d, d }` → returned in `%xmm0`, `%xmm1` (SSE class)
- `{ w, d }` → returned in `%rax`, `%xmm0` (mixed class)

### Data Definitions

**Aggregate types can be used in data sections:**
```qbe
type :point = { w, w }

data $origin = :point { w 0, w 0 }
```

---

## FasterBASIC's Current Approach (NOT Using Aggregate Types)

### What FasterBASIC Does Now

**For UDTs:**
```basic
TYPE Point
    X AS INTEGER
    Y AS DOUBLE
END TYPE
DIM P AS Point
P.X = 42
```

**FasterBASIC generates:**
```qbe
# Allocate raw memory
%P =l alloc8 16

# Manual pointer arithmetic
%x_ptr =l add %P, 0        # Field X at offset 0
storew 42, %x_ptr
```

**No type definitions emitted!** FasterBASIC computes offsets at compile-time and uses raw pointer math.

---

## What FasterBASIC COULD Do (Using Aggregate Types)

### Approach 1: Define Types But Still Use Pointer Math

```qbe
# Define the type (for documentation/debugging)
type :Point = { w, d }

# Allocate using the type
%P =l alloc8 16

# Still use manual pointer math (current approach)
%x_ptr =l add %P, 0
storew 42, %x_ptr
```

**Benefit:** Type definition makes IL more readable, but no functional change.

### Approach 2: Use Types for Function Parameters

```qbe
type :Point = { w, d }

# Function that takes Point by pointer (automatic in QBE)
function w $getX(:Point %p) {
@start
        %x =w loadw %p      # Load field X (offset 0)
        ret %x
}
```

**Calling:**
```qbe
%P =l alloc8 16
# ... initialize P ...
%result =w call $getX(:Point %P)   # Pass pointer to Point
```

**Benefits:**
- Type safety at IL level
- QBE validates parameter types
- Clearer function signatures

### Approach 3: Full Aggregate Type Usage

```qbe
type :Point = { w, d }

# Return small struct in registers (ABI handled by QBE)
function :Point $makePoint(w %x, d %y) {
@start
        %p =l alloc8 16
        storew %x, %p
        %y_ptr =l add %p, 8
        stored %y, %y_ptr
        
        # QBE handles returning in registers (RAX, XMM0)
        ret %p      # Actually returns the STRUCT, not pointer
}
```

**Note:** Return semantics are complex - QBE docs say aggregate return uses hidden pointer parameter for large structs.

---

## Advantages of Using QBE Aggregate Types

### 1. Type Safety

**Current (no types):**
```qbe
%p =l alloc8 16
%val =w call $function(l %p)    # What type is this pointer to?
```

**With types:**
```qbe
type :Point = { w, d }
%p =l alloc8 16
%val =w call $function(:Point %p)  # Clearly a Point
```

QBE can validate that callers and functions agree on types.

### 2. ABI Compliance

**For function returns:**
- Small structs returned in registers automatically
- QBE generates correct code for each platform (x86_64, ARM64, RISC-V)
- No manual register manipulation needed

**Example:**
```qbe
type :Vec2 = { d, d }

function :Vec2 $addVec(:Vec2 %a, :Vec2 %b) {
    # QBE handles register passing/return automatically
}
```

### 3. Clearer IL Code

**Readable type names instead of opaque pointers:**
```qbe
# Current
function w $distance(l %p1, l %p2)

# With types
function d $distance(:Point %p1, :Point %p2)
```

### 4. Potential Optimization

QBE's optimizer may:
- Recognize aggregate access patterns
- Optimize struct copies (blit optimization)
- Better register allocation for struct operations

---

## Disadvantages of Using QBE Aggregate Types

### 1. Complexity

**Current approach is simple and explicit:**
- Offsets computed at compile-time by FasterBASIC
- Direct pointer arithmetic (easy to debug)
- No hidden behavior

**Aggregate types add complexity:**
- Need to emit type definitions
- Understand QBE ABI rules for parameter passing
- Return value semantics are subtle

### 2. Loss of Control

**Manual pointer math gives full control:**
- Exact instruction sequence generated
- No surprises from QBE's ABI handling
- Easy to generate from templates

**Aggregate types delegate to QBE:**
- Less predictable code generation
- Platform-specific ABI differences
- Harder to debug codegen issues

### 3. Limited Field Access

**QBE doesn't have field-by-name access:**
```qbe
type :Point = { w, d }
%p =l alloc8 16

# Still need manual offset calculation!
%x =w loadw %p           # Field 0 at offset 0
%y_ptr =l add %p, 8      # Field 1 at offset 8
%y =d loadd %y_ptr
```

**No improvement over current approach for field access.**

### 4. No Nested Member Access

**FasterBASIC supports:**
```basic
O.Item.Value = 99
```

**With aggregate types:**
```qbe
type :Inner = { w }
type :Outer = { :Inner, w }

# Still need pointer math for nested access
%o =l alloc8 8
%item_ptr =l add %o, 0      # Offset to Item
%value_ptr =l add %item_ptr, 0  # Offset to Value
storew 99, %value_ptr
```

**No syntactic improvement.**

---

## Recommendation: Hybrid Approach

### Use Aggregate Types For:

1. **Function Signatures** (type safety and ABI)
   ```qbe
   type :Point = { w, d }
   function :Point $createPoint(w %x, d %y)
   function d $distance(:Point %p1, :Point %p2)
   ```

2. **Documentation** (IL readability)
   ```qbe
   type :Player = { w, w, d, d, l }  # id, health, x, y, name_ptr
   ```

3. **Data Definitions** (globals)
   ```qbe
   type :Config = { w, w, d }
   data $config = :Config { w 800, w 600, d 60.0 }
   ```

### Continue Using Pointer Math For:

1. **Field Access** (current approach works well)
   ```qbe
   %x_ptr =l add %P, 0
   %x =w loadw %x_ptr
   ```

2. **Local Variables** (simpler allocation)
   ```qbe
   %p =l alloc8 16    # No type needed for local UDT
   ```

3. **Nested Member Access** (no benefit from types)
   ```qbe
   %value_ptr =l add %outer, 0    # Compute offset directly
   ```

---

## Implementation Impact on FasterBASIC

### Changes Required for Hybrid Approach

#### 1. Emit Type Definitions

**New codegen phase:**
```cpp
void QBECodeGenerator::emitTypeDefinitions() {
    for (const auto& [name, typeSymbol] : m_symbolTable.types) {
        emit("type :" + name + " = { ");
        
        bool first = true;
        for (const auto& field : typeSymbol.fields) {
            if (!first) emit(", ");
            emit(getQBETypeChar(field));
            first = false;
        }
        
        emit(" }\n");
    }
    emit("\n");
}
```

**Generated:**
```qbe
type :Point = { w, d }
type :Person = { l, w }     # name_ptr, age
type :Outer = { :Inner, w } # Nested type
```

#### 2. Update Function Signatures

**Current:**
```qbe
function w $process(l %p)   # Generic pointer
```

**With types:**
```qbe
function w $process(:Point %p)   # Typed parameter
```

**Codegen change:**
```cpp
std::string QBECodeGenerator::emitFunctionSignature(const FunctionSymbol& func) {
    // ... 
    for (const auto& param : func.parameters) {
        if (param.type == VariableType::USER_DEFINED) {
            sig += ":" + param.typeName + " %" + param.name;
        } else {
            sig += getQBEType(param.type) + " %" + param.name;
        }
    }
    // ...
}
```

#### 3. Handle Struct Returns (Complex)

**Small struct return:**
```qbe
type :Vec2 = { d, d }

function :Vec2 $getOrigin() {
@start
    # Allocate local struct
    %p =l alloc8 16
    stored d_0.0, %p
    %y_ptr =l add %p, 8
    stored d_0.0, %y_ptr
    
    # Return struct (QBE copies to registers)
    ret %p
}
```

**Large struct return (needs hidden parameter):**
```qbe
function :LargeStruct $getStruct(l %ret_ptr) {
    # Write result to ret_ptr
    # ...
    ret %ret_ptr
}
```

**Codegen complexity:** Must understand size thresholds and ABI rules.

---

## Performance Comparison

### Benchmark: Field Access

**Test:** Access 1 million struct fields in a loop.

**Current approach (pointer math):**
```qbe
@loop
    %x_ptr =l add %P, 0
    %x =w loadw %x_ptr
    # ... use x ...
    jmp @loop
```

**With aggregate types (same code):**
```qbe
type :Point = { w, d }

@loop
    %x_ptr =l add %P, 0    # Still need pointer math
    %x =w loadw %x_ptr
    # ... use x ...
    jmp @loop
```

**Result:** Identical performance - aggregate types don't help field access.

### Benchmark: Function Call with Struct Parameter

**Current approach:**
```qbe
function w $getX(l %p) { ... }
%result =w call $getX(l %P)
```

**With aggregate types:**
```qbe
type :Point = { w, d }
function w $getX(:Point %p) { ... }
%result =w call $getX(:Point %P)
```

**Result:** Identical performance - both pass pointer in register.

### Benchmark: Struct Return (Small)

**Current approach (manual):**
```qbe
# Return pointer to heap-allocated struct
function l $makePoint() {
    %p =l call $malloc(l 16)
    # ... initialize ...
    ret %p
}
# Caller must free
```

**With aggregate types (potential win):**
```qbe
type :Point = { w, d }
function :Point $makePoint() {
    %p =l alloc8 16
    # ... initialize ...
    ret %p    # QBE returns in RAX, XMM0 (no heap allocation!)
}
```

**Result:** Aggregate return **avoids heap allocation** - significant win!

---

## Testing with QBE

### Experiment 1: Simple Aggregate Type

**File: `test_agg.qbe`**
```qbe
type :Point = { w, d }

function w $test() {
@start
    %p =l alloc8 16
    storew 42, %p
    %y_ptr =l add %p, 8
    stored d_3.14, %y_ptr
    
    %x =w loadw %p
    ret %x
}

export function w $main() {
@start
    %result =w call $test()
    ret %result
}
```

**Compile and run:**
```bash
cd fsh/qbe
./obj/qbe test_agg.qbe > test_agg.s
cc test_agg.s -o test_agg
./test_agg
echo $?   # Should print 42
```

**Result:** ✅ Works - QBE accepts aggregate types

### Experiment 2: Aggregate Parameter

**File: `test_param.qbe`**
```qbe
type :Point = { w, d }

function w $getX(:Point %p) {
@start
    %x =w loadw %p
    ret %x
}

export function w $main() {
@start
    %p =l alloc8 16
    storew 100, %p
    %y_ptr =l add %p, 8
    stored d_50.5, %y_ptr
    
    %result =w call $getX(:Point %p)
    ret %result
}
```

**Result:** ✅ Should work - pointer passed in RDI

### Experiment 3: Aggregate Return (COMPLEX)

**File: `test_return.qbe`**
```qbe
type :Point = { w, d }

function :Point $makePoint(w %x, d %y) {
@start
    %p =l alloc8 16
    storew %x, %p
    %y_ptr =l add %p, 8
    stored %y, %y_ptr
    ret %p
}

export function w $main() {
@start
    %pt =:Point call $makePoint(w 42, d d_3.14)
    # How to access %pt fields?
    ret 0
}
```

**Problem:** How does QBE represent the returned aggregate in the caller?

**Answer (from ABI docs):** 
- `%pt` would be split across registers (RAX for int, XMM0 for double)
- This is **very complex** - better to avoid for now

---

## Conclusion and Recommendation

### Current Status

✅ **FasterBASIC's approach is working and reasonable**
- Explicit pointer arithmetic
- Predictable codegen
- Simple to debug

### Why NOT Using Aggregate Types Is OK

1. **Field access doesn't benefit** - still need pointer math
2. **Local variables don't benefit** - same allocation code
3. **Current approach is simpler** - no ABI complexity

### Where Aggregate Types WOULD Help

1. **Function signatures** - type safety and clarity
2. **Small struct returns** - register return (avoid heap alloc)
3. **IL readability** - named types instead of generic pointers

### Recommended Action Plan

**Phase 1: Add Type Definitions (Low Risk)**
- Emit `type :TypeName = { ... }` definitions
- Don't change any codegen yet
- Benefit: More readable IL

**Phase 2: Type Function Parameters (Medium Risk)**
- Change function signatures to use aggregate types
- Keep pointer-passing semantics
- Benefit: Type safety

**Phase 3: Consider Struct Returns (High Risk)**
- Only for small structs (≤ 16 bytes)
- Requires understanding ABI thoroughly
- Benefit: Avoid heap allocation

### Final Verdict

**Do NOT change current UDT implementation for field access.**

**DO consider using aggregate types for function parameters/returns when implementing SUB/FUNCTION UDT support.**

**Priority: LOW** - Fix the two critical bugs first (string fields, UDT arrays), then consider aggregate types as an optimization.

---

## Appendix: QBE Aggregate Type Quick Reference

### Syntax Summary

```qbe
# Regular type
type :name = { w, w, d }

# With alignment
type :name = align 16 { w 4 }

# Union type
type :name = {
    { w }
    { d }
    { l }
}

# Opaque type
type :name = align 8 { 32 }

# Nested types
type :inner = { w }
type :outer = { :inner, d }
```

### Size and Alignment Rules

| Type | Size | Alignment |
|------|------|-----------|
| `b` (byte) | 1 | 1 |
| `h` (half) | 2 | 2 |
| `w` (word) | 4 | 4 |
| `l` (long) | 8 | 8 |
| `s` (single) | 4 | 4 |
| `d` (double) | 8 | 8 |
| Aggregate | Sum of fields (with padding) | Max of field alignments |

### Function Parameter/Return Rules

**Parameters:**
- Aggregate parameters → pointer passed (automatic)
- Caller passes address in register (RDI, RSI, etc.)

**Returns:**
- Small aggregate (≤ 16 bytes) → registers (RAX, XMM0, etc.)
- Large aggregate (> 16 bytes) → hidden pointer parameter
- Rules defined by System V ABI (see abi.txt)

---

**Bottom Line:** QBE has aggregate types, FasterBASIC doesn't use them (yet), and that's fine for now. Consider them later for function signatures and returns.