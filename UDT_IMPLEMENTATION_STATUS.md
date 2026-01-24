# User-Defined Types (UDT) Implementation Status

## Overview

Successfully implemented core support for user-defined types (structs/records) in the FasterBASIC QBE backend!

**Status**: ✅ **FUNCTIONAL** - Basic UDT operations working, member access, nested types, and arrays operational

---

## What's Implemented

### ✅ Phase 1: Memory Layout (COMPLETE)

**Location**: `qbe_codegen_helpers.cpp`

- `calculateTypeSize()` - Computes struct size with proper alignment
- `calculateFieldOffset()` - Computes field offsets within structs
- `getFieldOffset()` - Handles nested member chains (e.g., `Player.Position.X`)
- `inferMemberAccessType()` - Resolves type of member access expressions
- `getVariableTypeName()` - Maps variable names to UDT type names
- `getTypeSymbol()` - Looks up type definitions from symbol table

**Features**:
- Recursive size calculation for nested types
- 8-byte alignment for all fields
- Caching for performance (avoids recalculation)
- Proper handling of built-in types (INT=4, DOUBLE=8, STRING=8)

### ✅ Phase 2: Variable Declaration (COMPLETE)

**Location**: `qbe_codegen_main.cpp` - `emitMainFunction()`

Variables of user-defined types are allocated on the stack using QBE's `alloc8`:

```qbe
%var_Player =l alloc8 32    # sizeof(Entity) = 32 bytes
%var_P =l alloc8 16         # sizeof(Point) = 16 bytes
```

**Features**:
- Automatic size calculation
- Stored as pointers (QBE `l` type)
- Type name cached in `m_varTypeNames` map

### ✅ Phase 3: Member Access - Read (COMPLETE)

**Location**: `qbe_codegen_expressions.cpp` - `emitMemberAccessExpr()`

Reading members works for both simple and nested cases:

```basic
x = Player.Position.X    ' Nested member access
```

**Generated QBE**:
```qbe
%t1 =l add %var_Player, 8      # offset to Position field
%t2 =l add %t1, 0              # offset to X within Position
%x =d loadd %t2                # load double value
```

**Features**:
- Pointer arithmetic for offset calculation
- Type-specific load instructions (`loadw`, `loadd`, `loadl`)
- Recursive handling for nested types
- Returns pointer for nested UDTs (allows chaining)

### ✅ Phase 4: Member Assignment - Write (COMPLETE)

**Location**: `qbe_codegen_statements.cpp` - `emitLet()`

Writing to members with proper type coercion:

```basic
Player.Position.X = 100
Player.Name = "Hero"
Player.Health = 100
```

**Generated QBE**:
```qbe
# Player.Position.X = 100
%t1 =d copy d_100.0
%t2 =l add %var_Player, 8      # offset to Position
%t3 =l add %t2, 0              # offset to X
stored %t1, %t3                # store double

# Player.Name = "Hero"
%t4 =l copy $str.5
%t5 =l copy %var_Player        # offset 0
storel %t4, %t5                # store pointer
```

**Features**:
- Walks member chain computing cumulative offsets
- Type conversion at assignment (DOUBLE → INT, etc.)
- Type-specific store instructions (`storew`, `stored`, `storel`)
- Handles nested member chains

### ✅ Semantic Analyzer Updates (COMPLETE)

**Location**: `fasterbasic_semantic.cpp` - `processDimStatement()`

Modified to create **variables** (not arrays) for scalar UDT declarations:

```basic
DIM Player AS Entity    ' Creates variable (not 0-dimensional array)
```

**Logic**:
- Detects `dimensions.empty() && hasAsType`
- Creates `VariableSymbol` with `type = USER_DEFINED`
- Stores `typeName` for later lookup
- Validates type exists in `m_symbolTable.types`

---

## Generated Code Quality

### Example: Simple Point Type

**BASIC**:
```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

DIM P AS Point
P.X = 10.5
P.Y = 20.5
PRINT P.X, P.Y
```

**Generated QBE**:
```qbe
# Declaration
%var_P =l alloc8 16             # 2 doubles = 16 bytes

# P.X = 10.5
%t0 =d copy d_10.5
%t1 =l copy %var_P              # base pointer
stored %t0, %t1                 # store at offset 0

# P.Y = 20.5
%t2 =d copy d_20.5
%t3 =l add %var_P, 8            # offset 8 for Y
stored %t2, %t3

# PRINT P.X
%t4 =l copy %var_P
%t5 =d loadd %t4
call $basic_print_double(d %t5)

# PRINT P.Y
%t6 =l add %var_P, 8
%t7 =d loadd %t6
call $basic_print_double(d %t7)
```

### Example: Nested Types

**BASIC**:
```basic
TYPE Vector2D
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

TYPE Entity
    Name AS STRING
    Position AS Vector2D
    Health AS INTEGER
END TYPE

DIM Player AS Entity
Player.Name = "Hero"
Player.Position.X = 100
Player.Health = 100
```

**Memory Layout**:
```
Entity (32 bytes total):
  offset 0:  Name (STRING, 8 bytes)
  offset 8:  Position (Vector2D, 16 bytes)
    offset 8:  Position.X (DOUBLE, 8 bytes)
    offset 16: Position.Y (DOUBLE, 8 bytes)
  offset 24: Health (INTEGER, 4 bytes)
  offset 28: padding (4 bytes for alignment)
```

**Generated QBE**:
```qbe
%var_Player =l alloc8 32

# Player.Name = "Hero"
%t0 =l copy $str.0
%t1 =l copy %var_Player         # offset 0
storel %t0, %t1

# Player.Position.X = 100
%t2 =d copy d_100.0
%t3 =l add %var_Player, 8       # offset to Position
%t4 =l add %t3, 0               # offset to X within Position
stored %t2, %t4

# Player.Health = 100
%t5 =d copy d_100.0             # literal is DOUBLE
%t6 =w dtosi %t5                # convert to INT
%t7 =l add %var_Player, 24      # offset to Health
storew %t6, %t7
```

---

## What's NOT Implemented Yet

### ✅ Phase 5: Arrays of UDTs (1D, Global Scope)

**Status**: ✅ **COMPLETE**

Arrays of user-defined types are now supported for global/module-level 1D arrays with constant dimensions:

```basic
DIM Enemies(10) AS Sprite       ' ✅ Working!
Enemies(1).Health = 30          ' ✅ Implemented
```

**Implementation Details**:
- Arrays allocated on heap with `malloc` (not stack)
- Element access uses pointer arithmetic: `base + (index * elementSize)`
- Returns pointer to element for member access
- Automatic cleanup with `free()` in exit block

**See**: `UDT_ARRAYS_IMPLEMENTATION.md` for complete documentation

**Current Limitations**:
- Only 1D arrays supported (multi-dimensional coming later)
- Only global scope (function-local arrays planned)
- Constant dimensions only (dynamic sizing planned)

### ❌ Multi-dimensional Arrays of UDTs

**Status**: NOT STARTED (but straightforward extension)

```basic
DIM Grid(8, 8) AS Cell          ' Not working yet
Grid(4, 4).Value = 42           ' Not implemented
```

**What's needed**:
- Row-major offset calculation: `(row * cols + col) * elementSize`
- Index validation for multiple dimensions

### ❌ Function Parameters (UDT by reference)

**Status**: NOT STARTED

Passing UDTs and UDT arrays to functions:

```basic
FUNCTION UpdateEntity(E AS Entity)
    E.Health = E.Health - 10
END FUNCTION

SUB ProcessArray(arr() AS Point)
    arr(0).x = 100
END SUB
```

**Design Decision**: Pass by reference (pointer) - matches design principles

### ❌ Function Return Values (UDT)

**Status**: NOT STARTED

Returning UDTs from functions:

```basic
FUNCTION CreatePoint(x, y) AS Point
    DIM P AS Point
    P.X = x
    P.Y = y
    CreatePoint = P
END FUNCTION
```

**Design Decision**: Use hidden out-parameter approach (common in C compilers)

### ❌ Function-Local UDT Arrays

**Status**: PLANNED (two-tier strategy)

**Design**:
- Small arrays (< 1KB): Stack allocation with `alloc8`
- Large arrays (≥ 1KB): Heap allocation with defer-on-exit cleanup list
- Ensures no memory leaks while keeping small arrays fast

---

## Known Issues

### Issue 1: TYPE Declarations in Output

**Problem**: TYPE...END TYPE statements appear as "TODO: Unhandled statement type 42"

**Impact**: Cosmetic only - doesn't affect functionality

**Fix**: Add case for `STMT_TYPE` in `emitStatement()` (just emit comment, no code needed)

### Issue 2: Arrays Still Created

**Problem**: Parser creates 0-dimensional arrays even for scalars

**Impact**: Extra array symbols created (e.g., `%arr_P =l copy 0`)

**Fix**: These are ignored, but could be cleaned up in semantic phase

### Issue 3: No Type Validation Yet

**Problem**: No runtime or compile-time checks for type compatibility

**Impact**: Assigning wrong types might succeed incorrectly

**Fix**: Add type checking in member assignment

---

## Test Results

### Test Program: `test_udt_basic.bas`

```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

DIM P AS Point
P.X = 10.5
P.Y = 20.5
PRINT "Point: ("; P.X; ", "; P.Y; ")"
```

**Result**: ✅ **PASSES** - Correct output, correct QBE IL generated

### Test Program: Nested Types

```basic
TYPE Entity
    Name AS STRING
    Position AS Vector2D
    Health AS INTEGER
END TYPE

DIM Player AS Entity
Player.Position.X = 100
```

**Result**: ✅ **PASSES** - Nested member access works correctly

---

## Performance Characteristics

### Memory Layout
- **Aligned to 8 bytes** - optimal for modern CPUs
- **Packed structs** - no wasted space within fields
- **Stack allocation** - fast (no heap overhead)

### Code Generation
- **Offset calculation at compile time** - no runtime overhead
- **Cached type sizes** - O(1) lookup after first calculation
- **Direct memory access** - pointer arithmetic, no indirection

### Type Size Examples
```
Point (X, Y doubles):           16 bytes
Entity (string, Point, int):    32 bytes (with padding)
Sprite (2 doubles, int, string): 32 bytes
```

---

## Design Decisions

### 1. Stack Allocation (not Heap)

**Decision**: UDTs allocated on stack with `alloc8`

**Rationale**:
- Simple and fast
- No garbage collection needed
- Matches C struct semantics
- QBE stack allocation is efficient

**Limitation**: Large structs might overflow stack (acceptable for typical use)

### 2. Pass-by-Pointer (Future)

**Decision**: Functions will receive UDTs as pointers

**Rationale**:
- Avoids copying entire structs
- Matches C convention
- Efficient for large types
- Allows modifications in functions

### 3. 8-byte Alignment

**Decision**: All fields aligned to 8 bytes

**Rationale**:
- Simplifies offset calculation
- Optimal for 64-bit architecture
- QBE `alloc8` requires 8-byte alignment
- Small memory overhead acceptable

### 4. No QBE Aggregate Types

**Decision**: Don't use QBE's `type :name = { ... }` feature

**Rationale**:
- Raw memory + pointer arithmetic is simpler
- More control over layout
- Easier to implement
- QBE aggregate types have limitations

---

## Integration with Type System

### Type Coercion at Member Assignment

Member assignments respect the three-point coercion strategy:

```basic
Player.Health = 100.0    ' DOUBLE → INT conversion
```

**Generated**:
```qbe
%t0 =d copy d_100.0      # literal is DOUBLE
%t1 =w dtosi %t0         # convert to INT (Health field is INT)
%t2 =l add %var_Player, 24
storew %t1, %t2          # store as word (4 bytes)
```

### Type Inference

Member access expressions properly infer types:

```basic
x = Player.Position.X    ' x inferred as DOUBLE
i = Player.Health        ' i inferred as INTEGER
```

---

## Next Steps

### Priority 1: Arrays of UDTs ✅ DONE
- ✅ Implemented Phase 5 for global 1D arrays
- ✅ Tested with `test_udt_arrays.bas`
- ✅ Verified array element pointer access
- ✅ Member read/write on array elements working
- ✅ FOR loops with array elements working
- ✅ Automatic memory cleanup implemented

### Priority 2: Clean Up Issues
- Handle TYPE declarations gracefully (emit comment)
- Remove redundant array symbols
- Add type validation

### Priority 3: Function Support
- Pass UDTs to functions (by pointer)
- Return UDTs from functions
- Test with DEF FN and FUNCTION/SUB

### Priority 4: Advanced Array Features
- Multi-dimensional UDT arrays
- Function-local UDT arrays (with defer cleanup)
- Dynamic (runtime-sized) UDT arrays
- Array initialization syntax

### Priority 5: Additional Features
- Array operations on UDT arrays
- Type initialization
- Field initializers
- BYVAL for explicit copy semantics

---

## Summary

**Core UDT functionality is WORKING!**

✅ Memory layout calculation  
✅ Variable declaration (scalars)  
✅ Member access (read)  
✅ Member assignment (write)  
✅ Nested types  
✅ Type coercion  
✅ **Arrays of UDTs (1D, global, constant dimensions)**  
✅ **Array element member access**  
✅ **Automatic heap allocation and cleanup**  

The implementation provides a solid foundation for composite types in FasterBASIC. The generated QBE code is clean, efficient, and follows modern compiler practices.

**Test Coverage**:
- `test_udt_basic.bas` - Scalar UDTs ✅
- `test_udt_arrays.bas` - Arrays of UDTs ✅

**Next steps**: Multi-dimensional arrays, function-local arrays with defer cleanup, and function parameter passing!