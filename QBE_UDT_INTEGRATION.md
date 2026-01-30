# QBE Integration with User-Defined Types (UDTs)

**Purpose:** Explain how QBE's intermediate language enables FasterBASIC's UDT implementation

---

## Overview

QBE (Quick Backend) doesn't have native struct/aggregate types, but it provides all the necessary primitives to implement User-Defined Types efficiently. This document explains the mapping between BASIC UDTs and QBE instructions.

---

## 1. Core QBE Features Used

### 1.1 Memory Allocation

**QBE Instruction:** `alloc4`, `alloc8`, `alloc16`

```qbe
%ptr =l alloc8 24    # Allocate 24 bytes, 8-byte aligned
```

**Usage:**
- Allocate stack space for UDT variables
- Alignment ensures efficient memory access
- `alloc8` is typical for structs (8-byte alignment matches pointers and doubles)

**FasterBASIC Example:**
```basic
TYPE Point
    X AS INTEGER
    Y AS DOUBLE
END TYPE
DIM P AS Point
```

**Generated QBE:**
```qbe
%P =l alloc8 16    # 4 bytes (int) + 4 padding + 8 bytes (double)
```

---

### 1.2 Pointer Arithmetic

**QBE Instruction:** `add`

```qbe
%field_ptr =l add %base_ptr, 8    # Offset by 8 bytes
```

**Usage:**
- Calculate field addresses within structs
- Offsets computed at compile-time
- No runtime overhead

**FasterBASIC Example:**
```basic
P.Y = 3.14    ' Y is at offset 8
```

**Generated QBE:**
```qbe
%base =l copy %P           # Base pointer
%y_ptr =l add %base, 8     # Y field at offset 8
stored d_3.14, %y_ptr      # Store double
```

---

### 1.3 Typed Memory Access

**QBE Instructions:** `loadw`, `loadl`, `loads`, `loadd`

```qbe
%val =w loadw %ptr    # Load 32-bit word
%val =l loadl %ptr    # Load 64-bit long
%val =d loadd %ptr    # Load 64-bit double
```

**Usage:**
- Type-safe field access
- QBE enforces correct operand types
- Prevents integer/float confusion

**FasterBASIC Example:**
```basic
Z = P.X    ' X is INTEGER (4 bytes)
```

**Generated QBE:**
```qbe
%x_ptr =l add %P, 0        # X at offset 0
%z =w loadw %x_ptr         # Load 32-bit int
```

---

### 1.4 Typed Memory Write

**QBE Instructions:** `storew`, `storel`, `stores`, `stored`

```qbe
storew 42, %ptr      # Store 32-bit word
stored d_3.14, %ptr  # Store 64-bit double
```

**Usage:**
- Type-safe field assignment
- Matches load instruction types
- Ensures data integrity

---

## 2. UDT Memory Layout

### 2.1 Simple Struct

**BASIC:**
```basic
TYPE Point
    X AS INTEGER    ' 4 bytes
    Y AS DOUBLE     ' 8 bytes
END TYPE
```

**Memory Layout:**
```
Offset  Size  Field   Type
------  ----  -----   ----
0       4     X       INTEGER
4       4     (pad)   -
8       8     Y       DOUBLE
------
Total: 16 bytes
```

**Why Padding?**
- DOUBLE requires 8-byte alignment
- Padding inserted after X to align Y
- Matches C struct layout

**QBE Access Pattern:**
```qbe
# DIM P AS Point
%P =l alloc8 16

# P.X = 10
%x_ptr =l add %P, 0
storew 10, %x_ptr

# P.Y = 20.5
%y_ptr =l add %P, 8
stored d_20.5, %y_ptr
```

---

### 2.2 String Fields

**BASIC:**
```basic
TYPE Person
    Name AS STRING    ' 8 bytes (pointer)
    Age AS INTEGER    ' 4 bytes
END TYPE
```

**Memory Layout:**
```
Offset  Size  Field   Type
------  ----  -----   ----
0       8     Name    STRING (pointer)
8       4     Age     INTEGER
12      4     (pad)   -
------
Total: 16 bytes
```

**QBE Pattern:**
```qbe
# DIM P AS Person
%P =l alloc8 16

# P.Name = "Alice"
%str =l call $basic_string_create(l %.str.Alice)
%name_ptr =l add %P, 0
storel %str, %name_ptr        # Store pointer

# P.Age = 25
%age_ptr =l add %P, 8
storew 25, %age_ptr
```

**String Cleanup:**
When UDT goes out of scope or is reassigned, strings must be freed:
```qbe
%name_ptr =l add %P, 0
%str =l loadl %name_ptr
call $basic_string_free(l %str)
```

---

### 2.3 Nested UDTs

**BASIC:**
```basic
TYPE Inner
    Value AS INTEGER
END TYPE

TYPE Outer
    Item AS Inner
    Count AS INTEGER
END TYPE
```

**Memory Layout:**
```
Outer:
Offset  Size  Field   Type
------  ----  -----   ----
0       4     Item.Value  INTEGER
4       4     Count       INTEGER
------
Total: 8 bytes
```

**QBE Pattern:**
```qbe
# DIM O AS Outer
%O =l alloc8 8

# O.Item.Value = 99
%item_ptr =l add %O, 0         # Item at offset 0
%value_ptr =l add %item_ptr, 0 # Value at offset 0 within Item
storew 99, %value_ptr

# O.Count = 5
%count_ptr =l add %O, 4
storew 5, %count_ptr
```

**Optimization:**
Nested offsets are summed at compile-time:
```qbe
# O.Item.Value = 99 (optimized)
%value_ptr =l add %O, 0    # 0 + 0 = 0
storew 99, %value_ptr
```

---

## 3. Arrays of UDTs

### 3.1 Element Access

**BASIC:**
```basic
TYPE Point
    X AS INTEGER
    Y AS DOUBLE
END TYPE
DIM Points(10) AS Point
Points(5).X = 100
```

**QBE Pattern:**
```qbe
# Get array descriptor
%desc =l load %Points_desc_ptr

# Get data pointer
%data =l loadl %desc           # offset 0 = data pointer

# Calculate element address
# index=5, elementSize=16
%offset =l mul 5, 16           # 80 bytes
%elem_ptr =l add %data, %offset

# Access field X (offset 0 within element)
%x_ptr =l add %elem_ptr, 0
storew 100, %x_ptr
```

**Element Size Calculation:**
```c
// Compile-time computation
struct Point { int x; double y; };
size_t elementSize = sizeof(struct Point);  // 16 bytes
```

---

### 3.2 Array Descriptor for UDTs

**ArrayDescriptor Structure:**
```c
typedef struct {
    void* data;          // Pointer to UDT array
    int lowerBound1;
    int upperBound1;
    int lowerBound2;
    int upperBound2;
    int elementSize;     // Size of one UDT instance
    int dimensions;
    int typeSuffix;
} ArrayDescriptor;
```

**QBE Allocation:**
```qbe
# DIM Points(10) AS Point
# elementSize = 16, count = 10

%desc =l alloc8 64                 # Descriptor

%count =l mul 10, 16               # Total bytes
%data =l call $malloc(l %count)    # Allocate data
storel %data, %desc                # Store data pointer

%lb_ptr =l add %desc, 8
storew 0, %lb_ptr                  # Lower bound

%ub_ptr =l add %desc, 16
storew 9, %ub_ptr                  # Upper bound

%es_ptr =l add %desc, 40
storew 16, %es_ptr                 # Element size

%dims_ptr =l add %desc, 48
storew 1, %dims_ptr                # Dimensions
```

---

## 4. Type Safety in QBE

### 4.1 QBE Type System

QBE has four base types:
- `w` - 32-bit word (int)
- `l` - 64-bit long (pointer, int64)
- `s` - 32-bit single (float)
- `d` - 64-bit double

**Every instruction validates operand types:**
```qbe
%x =w loadw %ptr     ✅ OK: ptr is 'l', result is 'w'
%y =d add %x, %y     ❌ ERROR: add expects same types
%z =d extsw %x       ✅ OK: explicit conversion
%z =d add %z, %y     ✅ OK: both 'd'
```

---

### 4.2 FasterBASIC → QBE Type Mapping

| BASIC Type | Size | QBE Type | Load/Store |
|------------|------|----------|------------|
| BYTE       | 1    | `w`      | `loadw`/`storew` |
| SHORT      | 2    | `w`      | `loadw`/`storew` |
| INTEGER    | 4    | `w`      | `loadw`/`storew` |
| LONG       | 8    | `l`      | `loadl`/`storel` |
| SINGLE     | 4    | `s`      | `loads`/`stores` |
| DOUBLE     | 8    | `d`      | `loadd`/`stored` |
| STRING     | 8    | `l`      | `loadl`/`storel` (pointer) |
| UDT        | varies | `l`    | (pointer) |

---

### 4.3 Type Coercion

**Mixed-type field access:**
```basic
TYPE Data
    Count AS INTEGER
    Total AS DOUBLE
END TYPE
DIM D AS Data
D.Total = D.Count + 10.5
```

**Generated QBE:**
```qbe
%count_ptr =l add %D, 0
%count =w loadw %count_ptr     # Load int

%count_d =d extsw %count       # Convert int → double
%tmp =d add %count_d, d_10.5   # Add doubles

%total_ptr =l add %D, 8
stored %tmp, %total_ptr        # Store double
```

---

## 5. Comparison with Other Backends

### 5.1 LLVM

**LLVM Approach:**
```llvm
%Point = type { i32, double }
%p = alloca %Point
%x_ptr = getelementptr %Point, %Point* %p, i32 0, i32 0
store i32 42, i32* %x_ptr
```

**Advantages:**
- Named struct types
- Type-safe GEP instruction
- Better optimization potential

**Disadvantages:**
- Much more complex
- Requires type declarations
- Harder to debug

---

### 5.2 QBE

**QBE Approach:**
```qbe
%p =l alloc8 16
%x_ptr =l add %p, 0
storew 42, %x_ptr
```

**Advantages:**
- Simple, explicit
- No type declarations needed
- Easy to verify correctness
- Still type-safe (operand types validated)

**Disadvantages:**
- Manual offset calculation
- Less optimization potential
- Larger IL code

---

## 6. Performance Characteristics

### 6.1 Memory Access Speed

**Direct field access - zero overhead:**
```basic
Sum = Sum + P.X
```

**Generated assembly (ARM64):**
```asm
ldr w8, [sp, #16]      ; Load P.X
add w8, w8, w9         ; Add to Sum
str w8, [sp, #24]      ; Store Sum
```

**No function calls, no indirection - same as C struct access**

---

### 6.2 Offset Calculation

**Compile-time:**
```cpp
// FasterBASIC compiler
size_t offset = calculateFieldOffset("Point", "Y");  // Returns 8
```

**Result:**
```qbe
%y_ptr =l add %P, 8    # Offset 8 is a constant
```

**No runtime computation - hardcoded offset in generated code**

---

### 6.3 Cache Efficiency

**Struct layout matches CPU cache lines:**
```
Point { X, Y }    = 16 bytes → fits in one cache line (64 bytes)
Array of Points   = sequential memory → good spatial locality
```

**Hot loops with UDTs:**
```basic
FOR i = 0 TO 1000
    Points(i).X = i * 2
    Points(i).Y = i * 3.14
NEXT
```

**QBE generates tight loop with prefetch-friendly access pattern**

---

## 7. Advanced QBE Patterns

### 7.1 SIMD Optimization Potential

**Pair of Doubles (SIMD Vector):**
```basic
TYPE Vec2D
    X AS DOUBLE
    Y AS DOUBLE
END TYPE
```

**Memory layout = 16 bytes = perfect for SSE/NEON**

**Future QBE enhancement (not yet implemented):**
```qbe
%vec =:vec2d loadd %ptr    # Load 2 doubles as vector
%result =:vec2d add %vec, %vec
stored %result, %out       # Store 2 doubles
```

**Current workaround:** QBE's optimizer may auto-vectorize loops

---

### 7.2 Atomic Operations

**For thread-safe UDT fields:**
```qbe
%val =w loadw %ptr
%new =w add %val, 1
%old =w casw %ptr, %val, %new    # Compare-and-swap
```

**Enables lock-free data structures with UDTs**

---

### 7.3 Alignment Control

**QBE respects natural alignment:**
```qbe
%p =l alloc8 16    # 8-byte aligned
%p =l alloc16 32   # 16-byte aligned (for SIMD)
```

**Allows fine-tuning for performance-critical structs**

---

## 8. Debugging and Verification

### 8.1 QBE IL is Human-Readable

**Easy to verify correctness:**
```qbe
# P.X = 42
%x_ptr =l add %P, 0    # ← Check offset is correct
storew 42, %x_ptr      # ← Check type is 'w' (32-bit)
```

**Can spot bugs visually:**
- Wrong offset → field corruption
- Wrong type → type mismatch error from QBE
- Missing alignment → potential crash

---

### 8.2 QBE Error Messages

**Type mismatch example:**
```qbe
%x =w loadw %ptr
stored %x, %out    # ERROR: stored expects 'd', got 'w'
```

**QBE catches errors early - before assembly generation**

---

## 9. Limitations and Workarounds

### 9.1 No Named Types

**QBE doesn't have:**
```qbe
type :Point = { w, d }    # ← Not in QBE
```

**FasterBASIC solution:**
- Track type definitions in compiler symbol table
- Generate offsets at compile-time
- Emit raw pointer arithmetic

**Result:** Equivalent performance, just more verbose IL

---

### 9.2 No Struct Return

**QBE doesn't support:**
```qbe
%result =:Point call $GetPoint()    # ← Not in QBE
```

**Workaround (C calling convention):**
```qbe
# Caller allocates return space
%ret =l alloc8 16
call $GetPoint(l %ret)
# Function writes result to ret pointer
```

---

### 9.3 No Bitfields

**Cannot pack multiple fields into one word:**
```basic
TYPE Flags
    IsActive AS BYTE    ' Wastes 3 bytes
    IsVisible AS BYTE   ' Wastes 3 bytes
END TYPE
```

**Workaround:** Use INTEGER and bit operations

---

## 10. Future Enhancements

### 10.1 SIMD Auto-Vectorization

**Detect patterns:**
```basic
TYPE Color
    R AS SINGLE
    G AS SINGLE
    B AS SINGLE
    A AS SINGLE
END TYPE
```

**Generate vector loads:**
```qbe
%color =:vec4f loads %ptr    # Load 4 floats as SIMD vector
```

---

### 10.2 Struct Packing Attribute

**BASIC syntax:**
```basic
TYPE PackedData PACKED
    Flag AS BYTE
    Value AS INTEGER
END TYPE
```

**QBE generation:**
```qbe
# No padding, total size = 5 bytes
%flag_ptr =l add %p, 0
%value_ptr =l add %p, 1    # No alignment padding
```

---

### 10.3 Alignment Attribute

**BASIC syntax:**
```basic
TYPE AlignedBuffer ALIGN(64)
    Data(1024) AS BYTE
END TYPE
```

**QBE generation:**
```qbe
%buf =l alloc64 1024    # 64-byte aligned (cache line)
```

---

## 11. Summary

### How QBE Enables UDTs

| Requirement | QBE Feature |
|-------------|-------------|
| Allocate struct | `alloc8` instruction |
| Field access | Pointer arithmetic (`add`) |
| Type safety | Typed loads/stores |
| Nested structs | Pointer indirection |
| Arrays of UDTs | `malloc` + element size |
| Performance | Zero overhead abstractions |

### Key Insights

1. **QBE's simplicity is a strength** - Easy to understand generated code
2. **Type safety without complexity** - Operand types validated, no type declarations needed
3. **Performance matches C** - Direct memory access, no runtime overhead
4. **Debugging friendly** - Human-readable IL, clear error messages

### Bottom Line

QBE may not have native struct types, but it provides **exactly the right primitives** to implement User-Defined Types efficiently. The resulting implementation is:
- ✅ Fast (zero overhead)
- ✅ Type-safe (QBE validates everything)
- ✅ Debuggable (clear IL output)
- ✅ Maintainable (simple codegen logic)

**QBE is an excellent choice for FasterBASIC's UDT implementation.**

---

**End of Document**