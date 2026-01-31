# FasterBASIC ARM64 Intrinsics Reference

## Quick Reference

Hardware-accelerated functions for Apple Silicon (ARM64).

### Bit Manipulation

| Function | Description | Hardware | Speedup |
|----------|-------------|----------|---------|
| `BITREV(x)` | Reverse all bits | `rbit` | 15-20x |
| `BITCOUNT(x)` | Count set bits | `cnt` | 5x |
| `LEADZERO(x)` | Count leading zeros | `clz` | 12x |
| `TRAILZERO(x)` | Count trailing zeros | `rbit+clz` | 7.5x |

### Fast Math

| Function | Description | Hardware | Speedup |
|----------|-------------|----------|---------|
| `FMAX(a#, b#)` | Maximum (NaN-safe) | `fmaxnm` | 5x |
| `FMIN(a#, b#)` | Minimum (NaN-safe) | `fminnm` | 5x |

### Automatic Optimizations

These patterns are automatically optimized:

```basic
' Multiply-add (uses MADD/FMADD)
total = total + (price * quantity)

' Conditional (uses CSEL when beneficial)
result = IF a > b THEN a ELSE b
```

---

## Detailed Reference

### BITREV(x) - Bit Reversal

**Syntax:** `result = BITREV(value)`

**Description:** Reverses the order of all bits in the value.

**Example:**
```basic
x = BITREV(1)  ' Returns 9223372036854775808 (2^63)
```

**Use Cases:**
- Endian swapping
- FFT bit-reversal permutation
- Cryptographic operations

---

### BITCOUNT(x) - Population Count

**Syntax:** `result = BITCOUNT(value)`

**Description:** Counts the number of '1' bits.

**Example:**
```basic
x = BITCOUNT(15)  ' Returns 4 (binary: 1111)
```

**Use Cases:**
- Hamming weight calculations
- Set cardinality
- Chess bitboards

---

### LEADZERO(x) - Count Leading Zeros

**Syntax:** `result = LEADZERO(value)`

**Description:** Counts zeros from MSB until first '1' bit.

**Example:**
```basic
x = LEADZERO(1)   ' Returns 63
x = LEADZERO(256) ' Returns 55
```

**Derived Functions:**
```basic
' Integer log2
FUNCTION LOG2INT(x AS INTEGER) AS INTEGER
  RETURN 63 - LEADZERO(x)
END FUNCTION
```

**Use Cases:**
- Binary search trees
- Integer log₂ calculation
- Bit scanning

---

### TRAILZERO(x) - Count Trailing Zeros

**Syntax:** `result = TRAILZERO(value)`

**Description:** Counts zeros from LSB until first '1' bit.

**Example:**
```basic
x = TRAILZERO(8)  ' Returns 3 (8 = 1000 in binary)
```

**Use Cases:**
- Finding next set bit in bitmask
- Power-of-2 detection
- Alignment checking

---

## Status

**Current:** Planning phase  
**Target:** Implementation in Q1 2025  
**Documentation:** See `QBE_ARM64_OPTIMIZATION_PLAN.md`

