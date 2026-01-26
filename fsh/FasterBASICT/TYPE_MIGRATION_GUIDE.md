# FasterBASIC Type System Migration Guide

This guide helps you migrate existing FasterBASIC code to take advantage of the new QBE-aligned type system with BYTE, SHORT, and unsigned integer support.

---

## Quick Start

**Good news:** Your existing code will continue to work without any changes! The new type system is 100% backward compatible.

This guide is for developers who want to:
- Optimize memory usage with smaller types
- Use unsigned integers for bit manipulation
- Take advantage of QBE's native type system
- Write more explicit, type-safe code

---

## What's New?

### New Type Suffixes
- `@` - BYTE (8-bit signed: -128 to 127)
- `^` - SHORT (16-bit signed: -32,768 to 32,767)

### New AS Keywords
- `AS BYTE` / `AS UBYTE`
- `AS SHORT` / `AS USHORT`
- `AS UINTEGER` / `AS UINT`
- `AS ULONG`

### Improved Type System
- Better memory efficiency (use 1 byte instead of 4 for small values)
- Unsigned integer support (0 to 255 for UBYTE, etc.)
- More precise type tracking for optimization
- Better alignment with QBE IR backend

---

## Migration Scenarios

### Scenario 1: Counter Variables

**Before:**
```basic
DIM counter%
FOR counter% = 0 TO 100
    PRINT counter%
NEXT counter%
```

**After (optimized):**
```basic
DIM counter@ AS BYTE
FOR counter@ = 0 TO 100
    PRINT counter@
NEXT counter@
```

**Why?** BYTE uses 1 byte instead of 4, reducing memory usage by 75%.

---

### Scenario 2: Array Indices

**Before:**
```basic
DIM array(1000) AS INTEGER
DIM i%
FOR i% = 0 TO 999
    array(i%) = i% * 2
NEXT i%
```

**After (optimized):**
```basic
DIM array(1000) AS INTEGER
DIM i^ AS SHORT
FOR i^ = 0 TO 999
    array(i^) = i^ * 2
NEXT i^
```

**Why?** SHORT uses 2 bytes instead of 4, sufficient for index up to 32,767.

---

### Scenario 3: Flags and Booleans

**Before:**
```basic
DIM isActive%
DIM hasError%
isActive% = 1
hasError% = 0
```

**After (optimized):**
```basic
DIM isActive@ AS BYTE
DIM hasError@ AS BYTE
isActive@ = 1
hasError@ = 0
```

**Even better (unsigned):**
```basic
DIM isActive AS UBYTE
DIM hasError AS UBYTE
isActive = 1
hasError = 0
```

**Why?** BYTE/UBYTE uses 1 byte instead of 4 for boolean flags.

---

### Scenario 4: Bit Manipulation

**Before:**
```basic
DIM flags%
flags% = 0
flags% = flags% OR 1     REM Set bit 0
flags% = flags% OR 4     REM Set bit 2
IF (flags% AND 1) THEN PRINT "Bit 0 is set"
```

**After (improved):**
```basic
DIM flags AS UBYTE
flags = 0
flags = flags OR 1       REM Set bit 0
flags = flags OR 4       REM Set bit 2
IF (flags AND 1) THEN PRINT "Bit 0 is set"
```

**Why?** UBYTE is unsigned (0-255) and more appropriate for bit flags.

---

### Scenario 5: Color Values

**Before:**
```basic
DIM red%, green%, blue%
red% = 255
green% = 128
blue% = 64
```

**After (optimized):**
```basic
DIM red AS UBYTE, green AS UBYTE, blue AS UBYTE
red = 255
green = 128
blue = 64
```

**Why?** Color components are 0-255, perfect for UBYTE.

---

### Scenario 6: Large Arrays

**Before:**
```basic
DIM buffer%(10000)
DIM i%
FOR i% = 0 TO 9999
    buffer%(i%) = i% MOD 256
NEXT i%
```

**After (optimized):**
```basic
DIM buffer@(10000) AS BYTE
DIM i^ AS SHORT
FOR i^ = 0 TO 9999
    buffer@(i^) = i^ MOD 256
NEXT i^
```

**Savings:** 40,000 bytes (10,000 × 4) → 10,000 bytes (10,000 × 1) = 75% reduction!

---

### Scenario 7: Function Return Types

**Before:**
```basic
FUNCTION GetStatus%()
    GetStatus% = 1
END FUNCTION
```

**After (explicit):**
```basic
FUNCTION GetStatus() AS BYTE
    GetStatus = 1
END FUNCTION
```

**Even better (with suffix):**
```basic
FUNCTION GetStatus@() AS BYTE
    GetStatus@ = 1
END FUNCTION
```

**Why?** Explicit return type makes function signature clearer.

---

### Scenario 8: Function Parameters

**Before:**
```basic
FUNCTION Calculate%(x%, y%)
    Calculate% = x% + y%
END FUNCTION
```

**After (explicit):**
```basic
FUNCTION Calculate(x AS INTEGER, y AS INTEGER) AS INTEGER
    Calculate = x + y
END FUNCTION
```

**Why?** AS clauses make parameter types explicit and improve readability.

---

## When to Use Each Type

### Use BYTE / UBYTE when:
- Values range from -128 to 127 (BYTE)
- Values range from 0 to 255 (UBYTE)
- Storing small counters, flags, or status codes
- Working with single bytes of data
- Memory is a concern (e.g., large arrays)

**Examples:**
- Loop counters up to 100
- Boolean flags (0/1)
- Color components (0-255)
- Status codes
- Character codes

### Use SHORT / USHORT when:
- Values range from -32,768 to 32,767 (SHORT)
- Values range from 0 to 65,535 (USHORT)
- Array indices for medium-sized arrays
- Coordinate values for moderate-resolution graphics
- Port numbers or similar identifiers

**Examples:**
- Array indices up to 30,000
- Screen coordinates (up to 32K pixels)
- Network port numbers (0-65535)
- Medium-range counters

### Use INTEGER / UINTEGER when:
- Values range from -2.1B to 2.1B (INTEGER)
- Values range from 0 to 4.2B (UINTEGER)
- Standard integer arithmetic
- Most general-purpose counters
- Default for most numeric variables

**Examples:**
- General calculations
- Large array indices
- Standard numeric variables
- Most function parameters

### Use LONG / ULONG when:
- Values exceed INTEGER range
- 64-bit arithmetic required
- Pointer arithmetic
- Very large counters or calculations
- File sizes or timestamps

**Examples:**
- File sizes in bytes
- Millisecond timestamps
- Very large counters (billions)
- Memory addresses

---

## Migration Strategy

### Step 1: Identify Opportunities
Review your code for:
- Small-range counters (0-100, etc.)
- Boolean flags
- Color values
- Bit flags
- Large arrays of small values

### Step 2: Choose Appropriate Types
- Use smallest type that fits the value range
- Use unsigned for values that are never negative
- Use BYTE for 0-255 range
- Use SHORT for 0-32K range

### Step 3: Update Declarations
```basic
REM Before
DIM counter%, flag%, color%

REM After
DIM counter@ AS BYTE
DIM flag AS UBYTE
DIM color AS UBYTE
```

### Step 4: Update Function Signatures
```basic
REM Before
FUNCTION Process%(input%)
    Process% = input% * 2
END FUNCTION

REM After
FUNCTION Process(input AS BYTE) AS SHORT
    Process = input * 2
END FUNCTION
```

### Step 5: Test Thoroughly
- Verify value ranges don't exceed new type limits
- Check for overflow in calculations
- Test edge cases (min/max values)

---

## Common Pitfalls and Solutions

### Pitfall 1: Value Overflow

**Problem:**
```basic
DIM counter@ AS BYTE
counter@ = 200
counter@ = counter@ + 100  REM Overflows! (300 > 127)
```

**Solution:**
```basic
DIM counter^ AS SHORT      REM Use larger type
counter^ = 200
counter^ = counter^ + 100  REM OK (300 < 32767)
```

Or use unsigned:
```basic
DIM counter AS UBYTE       REM 0-255 range
counter = 200
counter = counter + 55     REM OK (255 max)
```

### Pitfall 2: Negative Values with Unsigned

**Problem:**
```basic
DIM value AS UBYTE
value = -1                 REM Error or wraps to 255
```

**Solution:**
```basic
DIM value AS BYTE          REM Use signed type
value = -1                 REM OK
```

### Pitfall 3: Array Index Out of Range

**Problem:**
```basic
DIM array(100000) AS INTEGER
DIM index^ AS SHORT
FOR index^ = 0 TO 99999    REM 99999 > 32767 (overflow!)
    array(index^) = 0
NEXT index^
```

**Solution:**
```basic
DIM array(100000) AS INTEGER
DIM index AS INTEGER       REM Use larger type
FOR index = 0 TO 99999     REM OK
    array(index) = 0
NEXT index
```

### Pitfall 4: Mixed Type Arithmetic

**Problem:**
```basic
DIM small@ AS BYTE
DIM large& AS LONG
small@ = 100
large& = small@ * 1000000  REM May overflow during calculation
```

**Solution:**
```basic
DIM small@ AS BYTE
DIM large& AS LONG
small@ = 100
large& = CLNG(small@) * 1000000  REM Explicit conversion
```

---

## Type Conversion Functions

Use these when explicit conversion is needed:

```basic
CBYTE(expr)    REM Convert to BYTE (-128 to 127)
CSHORT(expr)   REM Convert to SHORT (-32768 to 32767)
CINT(expr)     REM Convert to INTEGER
CLNG(expr)     REM Convert to LONG
CSNG(expr)     REM Convert to SINGLE (float)
CDBL(expr)     REM Convert to DOUBLE
```

**Example:**
```basic
DIM f! AS SINGLE
DIM b@ AS BYTE

f! = 123.456
b@ = CBYTE(f!)             REM Converts 123.456 to 123
```

---

## Best Practices

### 1. Be Explicit
```basic
REM Good: Clear intent
FUNCTION Calculate(x AS INTEGER, y AS INTEGER) AS INTEGER
    Calculate = x + y
END FUNCTION

REM Avoid: Implicit types
FUNCTION Calculate(x, y)
    Calculate = x + y
END FUNCTION
```

### 2. Use Smallest Appropriate Type
```basic
REM Good: Memory efficient
DIM buffer@(10000) AS BYTE

REM Wasteful: Uses 4x memory
DIM buffer%(10000)
```

### 3. Document Range Assumptions
```basic
REM Score is 0-100, using BYTE for efficiency
DIM playerScore@ AS BYTE
```

### 4. Validate Input Ranges
```basic
FUNCTION SetLevel(level AS BYTE) AS INTEGER
    IF level < 1 OR level > 100 THEN
        PRINT "Error: Level must be 1-100"
        SetLevel = -1
        EXIT FUNCTION
    END IF
    REM Process level...
    SetLevel = 0
END FUNCTION
```

### 5. Use Unsigned for Bit Operations
```basic
REM Good: Unsigned for bit flags
DIM permissions AS USHORT
permissions = permissions OR &H0001  REM Set bit 0

REM Avoid: Signed for bit operations
DIM permissions% AS INTEGER
```

---

## Quick Reference Table

| Old Type | New Type | When to Use | Memory Savings |
|----------|----------|-------------|----------------|
| `INTEGER` (32-bit) | `BYTE` (8-bit) | Values -128 to 127 | 75% |
| `INTEGER` (32-bit) | `UBYTE` (8-bit) | Values 0 to 255 | 75% |
| `INTEGER` (32-bit) | `SHORT` (16-bit) | Values -32K to 32K | 50% |
| `INTEGER` (32-bit) | `USHORT` (16-bit) | Values 0 to 65K | 50% |
| `INTEGER` (32-bit) | `UINTEGER` (32-bit) | Values 0 to 4.2B | 0% (but clearer) |
| `LONG` (64-bit) | `ULONG` (64-bit) | Values 0 to 18E18 | 0% (but clearer) |

---

## Gradual Migration Approach

You don't have to migrate all at once! Here's a gradual approach:

### Phase 1: New Code
- Use new types in all new code
- Get comfortable with syntax

### Phase 2: High-Impact Areas
- Migrate large arrays
- Optimize memory-intensive sections
- Update frequently-called functions

### Phase 3: Opportunistic Updates
- Update code as you touch it
- Modernize one module at a time

### Phase 4: Complete Migration
- Systematic review of all code
- Update remaining INTEGER/LONG usage
- Full type system leverage

---

## Testing Checklist

After migration, verify:

- [ ] All variables are within their type's range
- [ ] Loop counters don't overflow
- [ ] Array indices are valid
- [ ] Calculations don't overflow intermediate results
- [ ] Function parameters accept valid ranges
- [ ] Return values fit in declared types
- [ ] Bit operations work correctly with unsigned types
- [ ] Type conversions are explicit where needed

---

## Summary

The new type system provides:
- ✅ Better memory efficiency
- ✅ More precise type declarations
- ✅ Unsigned integer support
- ✅ Improved optimization opportunities
- ✅ 100% backward compatibility

**Start migrating today** to take advantage of these improvements!

---

## Need Help?

- Review `TYPE_SYNTAX_REFERENCE.md` for syntax examples
- Check `PHASE4_SUMMARY.md` for technical details
- Test with `phase4_test.bas` for usage patterns
- Remember: Migration is optional and gradual!