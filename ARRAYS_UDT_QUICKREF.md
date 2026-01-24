# Arrays of UDTs - Quick Reference

## ✅ What Works Now

```basic
TYPE Point
    x AS DOUBLE
    y AS DOUBLE
END TYPE

' Declare array
DIM points(10) AS Point

' Write to elements
points(0).x = 1.5
points(0).y = 2.5

' Read from elements
PRINT points(0).x

' Use in loops
FOR i% = 0 TO 10
    points(i%).x = i% * 10
NEXT i%
```

## 📋 Status

**Working**: 1D arrays, global scope, constant dimensions  
**Not Yet**: Multi-dim, function-local, dynamic sizing

## 🔧 How It Works

- **Allocation**: Heap (`malloc`) at program start
- **Access**: Pointer arithmetic: `base + (index × elementSize)`
- **Cleanup**: Automatic `free()` in exit block
- **Performance**: Fast, no overhead, contiguous memory

## 📝 Limitations

1. Only 1D arrays: `DIM arr(10) AS Point` ✅, `DIM arr(10, 10) AS Point` ❌
2. Constant size only: `DIM arr(10)` ✅, `DIM arr(n%)` ❌
3. Global scope only: Function-local arrays coming soon

## 📚 Documentation

- `UDT_ARRAYS_IMPLEMENTATION.md` - Full technical details
- `SESSION_ARRAYS_OF_UDTS.md` - Implementation session summary
- `UDT_IMPLEMENTATION_STATUS.md` - Overall UDT status

## 🧪 Test

Run: `./fsh/fbc_qbe test_udt_arrays.bas`
