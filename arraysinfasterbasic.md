# Arrays in FasterBASIC

## Current Status

✅ **Fully Implemented**:
- Global arrays (basic types and UDTs)
- Local arrays in functions (automatic cleanup)
- 1D arrays with constant dimensions
- Array element access and assignment
- REDIM and ERASE

---

## Quick Start

### Basic Arrays

```basic
' Declare and use arrays
DIM numbers(10) AS INTEGER
DIM names$(5) AS STRING

numbers(0) = 42
names$(1) = "Hello"

PRINT numbers(0)
PRINT names$(1)
```

### Arrays of User-Defined Types

```basic
TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

' Array of UDTs
DIM points(10) AS Point

points(0).x = 1.5
points(0).y = 2.5

PRINT points(0).x, points(0).y
```

### Local Arrays in Functions

```basic
FUNCTION Process() AS INTEGER
    DIM data(100) AS DOUBLE    ' Heap-allocated
    data(0) = 3.14             ' Use normally
    Process = 1
END FUNCTION                   ' Automatically freed!
```

---

## Memory Model

### Global Arrays
- **Allocated**: At program start (heap via `malloc`)
- **Lifetime**: Entire program execution
- **Freed**: Automatically in exit block

### Local Arrays
- **Allocated**: At DIM statement (heap via `malloc`)
- **Lifetime**: Function scope
- **Freed**: Automatically before function return
- **Safety**: All exit paths (END FUNCTION, EXIT FUNCTION, RETURN) trigger cleanup

### Element Access
Direct pointer arithmetic (no runtime overhead):
```
element_address = base_pointer + (index × element_size)
```

---

## Implementation Details

### Array Declaration

```basic
DIM arrayName(size) AS Type
```

Generated QBE for UDT array:
```qbe
# Calculate total size
%t0 =l copy 160                    # 10 elements × 16 bytes
%arr_points =l call $malloc(l %t0)
```

### Element Access

```basic
points(5).x = 10.0
```

Generated QBE:
```qbe
%t1 =l extsw %index               # Convert index to long
%t2 =l mul %t1, 16                # index × sizeof(Point)
%t3 =l add %arr_points, %t2       # base + offset
stored %value, %t3                # Store to field
```

### Cleanup

**Global arrays** (in main exit block):
```qbe
@exit
    call $free(l %arr_points)
    call $free(l %arr_colors)
    call $basic_cleanup()
    ret 0
```

**Local arrays** (function tidy_exit block):
```qbe
@tidy_exit_ProcessData
    call $free(l %arr_local_data)
    call $free(l %arr_local_temp)
@exit
    ret %return_value
```

---

## Features

### ✅ What Works

| Feature | Status | Example |
|---------|--------|---------|
| 1D arrays | ✅ | `DIM arr(10)` |
| Basic types | ✅ | `AS INTEGER`, `AS DOUBLE`, `AS STRING` |
| UDT arrays | ✅ | `DIM points(10) AS Point` |
| Global scope | ✅ | Module-level DIM |
| Function-local | ✅ | DIM inside FUNCTION/SUB |
| REDIM | ✅ | Resize arrays at runtime |
| ERASE | ✅ | Free array memory |
| Automatic cleanup | ✅ | No memory leaks |

### 🚧 Limitations

| Feature | Status | Notes |
|---------|--------|-------|
| Multi-dimensional | ❌ | `DIM grid(10, 10)` not supported |
| Dynamic size | ❌ | Size must be compile-time constant |
| Array bounds | ❌ | No runtime bounds checking |

---

## Type-Specific Behavior

### Integer Arrays
```basic
DIM values%(10)
values%(0) = 42        # 32-bit signed integers
```

### Double Arrays
```basic
DIM measurements(10)   # Default: DOUBLE
measurements(0) = 3.14
```

### String Arrays
```basic
DIM names$(10)
names$(0) = "Alice"
```

### UDT Arrays
```basic
TYPE Color
    r AS INTEGER
    g AS INTEGER
    b AS INTEGER
END TYPE

DIM palette(256) AS Color
palette(0).r = 255
```

---

## Memory Layout Example

For `DIM points(2) AS Point` where Point has 2 DOUBLEs:

```
Base: %arr_points

Offset  Element        Field
------  -------------  -----
  0     points(0).x    (8 bytes)
  8     points(0).y    (8 bytes)
 16     points(1).x    (8 bytes)
 24     points(1).y    (8 bytes)
 32     points(2).x    (8 bytes)
 40     points(2).y    (8 bytes)

Total: 48 bytes (3 × 16 bytes)
```

---

## Advanced: Function-Local Arrays

### Automatic Memory Management

```basic
FUNCTION TestArrays() AS INTEGER
    DIM temp(100) AS DOUBLE      ' malloc'd
    DIM points(50) AS Point      ' malloc'd
    
    ' Use arrays normally
    temp(0) = 3.14
    points(0).x = 1.0
    
    IF condition THEN
        EXIT FUNCTION            ' Cleanup happens here
    END IF
    
    TestArrays = 1
END FUNCTION                     ' And here
```

All exit paths → `tidy_exit` block → cleanup → return

### Multiple Arrays

```basic
FUNCTION Process() AS INTEGER
    DIM data1(100) AS DOUBLE
    DIM data2(50) AS INTEGER
    DIM results(25) AS Point
    
    ' All three freed automatically
    Process = 1
END FUNCTION
```

---

## Performance

### Allocation
- **malloc**: Fast for typical array sizes (<1MB)
- **Stack option**: Future optimization for small arrays

### Access
- **Zero overhead**: Direct pointer arithmetic
- **Cache friendly**: Contiguous memory layout
- **Predictable**: No hidden function calls

### Benchmarks
```basic
' Array access is equivalent to C:
FOR i% = 0 TO 9999
    arr(i%) = i% * 2
NEXT i%
```
Generated code: Simple loop with pointer arithmetic

---

## Testing

### Test Files
- [test_udt_arrays.bas](test_udt_arrays.bas) - UDT array basics
- [test_local_arrays_final.bas](test_local_arrays_final.bas) - Function-local arrays
- [test_erase_redim.bas](test_erase_redim.bas) - Dynamic operations

### Verification
```bash
./fsh/fbc_qbe test_udt_arrays.bas && ./test_udt_arrays
./fsh/fbc_qbe test_local_arrays_final.bas && ./test_local_arrays_final
```

---

## Technical Implementation

### Files Modified
- `fasterbasic_semantic.cpp` - Track array scope (global/local)
- `qbe_codegen_main.cpp` - Function context management
- `qbe_codegen_statements.cpp` - DIM, REDIM, ERASE emission
- `qbe_codegen_expressions.cpp` - Array element access
- `qbe_codegen_helpers.cpp` - Type size calculation

### Key Components
1. **Semantic Analysis**: Track array scope per function
2. **Function Context**: Stack of active functions with local array lists
3. **Tidy Exit**: Cleanup block before all function returns
4. **Type Sizes**: Compile-time calculation for UDT arrays

---

## Future Enhancements

### Planned
1. **Multi-dimensional arrays**: `DIM grid(10, 10)`
2. **Dynamic sizing**: `DIM arr(userInput%)`
3. **Bounds checking**: Optional runtime validation
4. **Stack allocation**: Small arrays (<1KB) on stack
5. **Array passing**: Functions receiving arrays as parameters

### Under Consideration
- Array initialization syntax: `DIM arr(3) = {1, 2, 3, 4}`
- Array slicing: `slice = arr(1:5)`
- PRESERVE keyword for REDIM

---

## Related Documentation

- [TYPE_SYSTEM.md](TYPE_SYSTEM.md) - Type definitions and UDTs
- [COERCION_STRATEGY.md](COERCION_STRATEGY.md) - Type conversions
- [README.md](README.md) - Project overview
