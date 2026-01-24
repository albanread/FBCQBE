# FasterBASIC User-Defined Types - Design Principles

## Core Philosophy

**Keep it simple. Keep it BASIC.**

User-defined types in FasterBASIC follow classic BASIC conventions with modern efficiency:
- Stack allocation (no `NEW`/`malloc` needed)
- Pass by reference (fast, like QB45)
- Arrays for collections (no heap management)

---

## 1. Stack Allocation by Default

### The Rule

**All UDT variables are allocated on the stack.**

```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

DIM P AS Point        ' Allocated on stack (16 bytes)
DIM Q AS Point        ' Another stack allocation
```

**Generated QBE**:
```qbe
%var_P =l alloc8 16
%var_Q =l alloc8 16
```

### Why Stack?

- ✅ **Fast** - Single instruction allocation
- ✅ **Simple** - No memory management
- ✅ **Safe** - Automatic cleanup when function returns
- ✅ **BASIC-like** - Simple variables, simple semantics

### No NEW keyword

Unlike Java or modern OOP languages, we **don't** do this:

```basic
' NOT SUPPORTED (Java-style)
P = NEW Point
DELETE P
```

**Rationale**: 
- BASIC programmers don't think about heap vs stack
- Memory management is a C/C++ concern, not BASIC
- Stack allocation is sufficient for 99% of BASIC programs

### When You Need Dynamic Storage

**Use arrays** (the BASIC way):

```basic
' Want many points? Use an array!
DIM Points(100) AS Point

FOR i = 1 TO 100
    Points(i).X = i * 10
    Points(i).Y = i * 20
NEXT i
```

Arrays already handle dynamic sizing in FasterBASIC. No need for heap primitives.

---

## 2. Pass by Reference (Pointer)

### The Rule

**UDT parameters are always passed by reference (pointer).**

```basic
SUB MovePoint(P AS Point, DX AS DOUBLE, DY AS DOUBLE)
    P.X = P.X + DX
    P.Y = P.Y + DY
END SUB

DIM MyPoint AS Point
MyPoint.X = 10
MyPoint.Y = 20

CALL MovePoint(MyPoint, 5, 10)
PRINT MyPoint.X, MyPoint.Y    ' Prints 15, 30 (modified!)
```

**Generated QBE**:
```qbe
# Pass pointer to P, not a copy
function w $MovePoint(l %ptr_P, d %DX, d %DY) {
@start
    # Access P.X via pointer
    %x_ptr =l add %ptr_P, 0
    %x =d loadd %x_ptr
    
    # Modify it
    %new_x =d add %x, %DX
    stored %new_x, %x_ptr
    
    # Same for P.Y...
    ret 0
}
```

### Why Pass by Reference?

- ✅ **Fast** - Just pass 8-byte pointer
- ✅ **Efficient** - No copying 16/32/64 byte structs
- ✅ **Matches QB45** - Classic BASIC behavior
- ✅ **Matches C** - Industry standard for structs
- ✅ **Works for any size** - 16 bytes or 1024 bytes, same cost

### Behavior is Explicit

The function signature makes it clear:

```basic
SUB ModifyEntity(E AS Entity)    ' E is a reference (can modify)
    E.Health = 100                ' Caller's entity is changed
END SUB
```

This is **expected** in BASIC. Compare with scalars:

```basic
SUB ModifyValue(X)               ' X is BYREF by default in BASIC
    X = 999                      ' Caller's variable is changed
END SUB
```

UDTs simply follow the same convention.

---

## 3. Future: BYVAL for Explicit Copies

### When BYVAL is Needed

Sometimes you want to modify a parameter **without** affecting the caller:

```basic
FUNCTION SimulateDamage(BYVAL E AS Entity, Damage AS INTEGER) AS INTEGER
    E.Health = E.Health - Damage    ' Modify local copy
    IF E.Health < 0 THEN E.Health = 0
    SimulateDamage = E.Health       ' Return result, caller's E unchanged
END FUNCTION

DIM Player AS Entity
Player.Health = 100

NewHealth = SimulateDamage(Player, 30)
PRINT Player.Health    ' Still 100 (not modified)
PRINT NewHealth        ' 70 (simulated result)
```

### Implementation (Future)

When `BYVAL` is specified:
1. Allocate temporary copy on stack
2. Copy original struct to temporary
3. Pass pointer to temporary
4. Temporary is destroyed after call

**Generated QBE** (conceptual):
```qbe
# Caller
%var_Player =l alloc8 32

# BYVAL: create copy
%temp_copy =l alloc8 32
call $memcpy(l %temp_copy, l %var_Player, l 32)

# Pass pointer to copy
%result =w call $SimulateDamage(l %temp_copy, w 30)

# Original %var_Player is unchanged
```

### Default is Still BYREF

To keep things simple:
- **Default**: BYREF (pointer) - fast, standard
- **Explicit**: BYVAL (copy) - when you need it

Most functions don't modify their parameters, so the distinction rarely matters.

---

## 4. Arrays are the Collection Type

### The BASIC Way

Want multiple entities? Use arrays:

```basic
TYPE Enemy
    Name AS STRING
    Health AS INTEGER
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

' This is the BASIC way!
DIM Enemies(100) AS Enemy

FOR i = 1 TO 10
    Enemies(i).Name = "Goblin" + STR$(i)
    Enemies(i).Health = 30
    Enemies(i).X = RND * 800
    Enemies(i).Y = RND * 600
NEXT i
```

### Why Arrays?

- ✅ **Familiar** - BASIC programmers understand arrays
- ✅ **Simple** - No pointers, no malloc/free
- ✅ **Dynamic** - FasterBASIC arrays can resize
- ✅ **Sufficient** - Covers 99% of collection needs

### Not This (Java/C# style)

```basic
' NOT SUPPORTED - too complex for BASIC
DIM Enemies AS List(OF Enemy)
Enemies.Add(New Enemy)
Enemies.Remove(5)
```

**Rationale**:
- Generic collections are OOP complexity
- BASIC is about simplicity
- Arrays + some helper SUBs achieve the same thing

---

## 5. No Complex OOP Features

### What We DON'T Support

**No Methods**:
```basic
' NOT SUPPORTED
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
    
    FUNCTION Distance() AS DOUBLE    ' Methods not in BASIC
        Distance = SQR(X*X + Y*Y)
    END FUNCTION
END TYPE
```

**No Constructors**:
```basic
' NOT SUPPORTED  
TYPE Point(X AS DOUBLE, Y AS DOUBLE)    ' No constructor syntax
    ' ...
END TYPE
```

**No Inheritance**:
```basic
' NOT SUPPORTED
TYPE Circle EXTENDS Shape    ' No inheritance
    Radius AS DOUBLE
END TYPE
```

**No Interfaces**:
```basic
' NOT SUPPORTED
TYPE Drawable AS INTERFACE    ' No interfaces
    SUB Draw()
END TYPE
```

### Why Not?

These are **OOP features**, not BASIC features:
- BASIC is procedural, not object-oriented
- Complexity defeats the purpose of BASIC
- If you want OOP, use C++/Java/C#

### What We DO Support

**Composition** (the BASIC way):
```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

TYPE Circle
    Center AS Point        ' Composition (nested type)
    Radius AS DOUBLE
END TYPE

' Helper functions (instead of methods)
FUNCTION PointDistance(P AS Point) AS DOUBLE
    PointDistance = SQR(P.X * P.X + P.Y * P.Y)
END FUNCTION

FUNCTION CircleArea(C AS Circle) AS DOUBLE
    CircleArea = 3.14159 * C.Radius * C.Radius
END FUNCTION
```

This is **sufficient** and **simple**.

---

## 6. Keep Memory Management Simple

### The Philosophy

**Memory management is the enemy of simplicity.**

In FasterBASIC:
- **Local variables** → Stack (automatic)
- **Collections** → Arrays (managed by runtime)
- **No manual management** → No malloc/free/delete

### Comparison with Other Languages

**C** (manual):
```c
Point* p = malloc(sizeof(Point));    // Manual allocation
p->x = 10;
free(p);                             // Manual deallocation
```

**Java** (garbage collected):
```java
Point p = new Point();    // Heap allocation
p.x = 10;
// GC collects later
```

**FasterBASIC** (automatic):
```basic
DIM P AS Point    ' Stack allocation
P.X = 10
' Automatic cleanup
```

**Winner**: FasterBASIC! Simplest of all. 🏆

---

## 7. Design Constraints

### What Limits Stack Allocation?

**Stack size**: Typically 1-8 MB per thread

**This is plenty** for:
- ✅ Game sprites (32-64 bytes each)
- ✅ Player entities (64-128 bytes)
- ✅ UI elements (16-32 bytes)
- ✅ Coordinate pairs (16 bytes)
- ✅ Configuration structs (dozens of bytes)

**Not enough** for:
- ❌ Huge scene graphs (thousands of objects)
- ❌ Large datasets (megabytes of data)
- ❌ Deeply recursive algorithms with big structs

### Solution for Large Data

**Use arrays** (which use heap internally):

```basic
' Small local scratch space (stack)
DIM CurrentEnemy AS Enemy

' Large collection (heap-backed array)
DIM AllEnemies(1000) AS Enemy

FOR i = 1 TO 1000
    CurrentEnemy = AllEnemies(i)    ' Copy to local for processing
    ' ... work with CurrentEnemy ...
    AllEnemies(i) = CurrentEnemy    ' Copy back if modified
NEXT i
```

This gives you:
- Fast local access (stack-allocated `CurrentEnemy`)
- Large storage (heap-backed `AllEnemies` array)
- Simple semantics (no pointer juggling)

---

## 8. Return Values (Future Design)

### The Problem

```basic
FUNCTION CreatePoint(X AS DOUBLE, Y AS DOUBLE) AS Point
    DIM P AS Point
    P.X = X
    P.Y = Y
    CreatePoint = P    ' How does this work?
END FUNCTION
```

### Solution: Hidden Out-Parameter

Compiler rewrites internally as:

```qbe
# Original BASIC:
# FUNCTION CreatePoint(X, Y) AS Point

# Rewritten as:
function w $CreatePoint(l %result_ptr, d %X, d %Y) {
@start
    %result_ptr    # Pointer to caller's result storage
    
    # Fill it
    stored %X, %result_ptr           # result.X = X
    %y_ptr =l add %result_ptr, 8
    stored %Y, %y_ptr                # result.Y = Y
    
    ret 0
}
```

**At call site**:
```basic
P = CreatePoint(10, 20)
```

**Generated**:
```qbe
# Allocate space for result
%result_space =l alloc8 16

# Call with hidden result pointer
call $CreatePoint(l %result_space, d 10.0, d 20.0)

# Copy result to P
%var_P =l copy %result_space
```

**Benefits**:
- No heap allocation
- No dangling pointers
- Efficient (stack-to-stack copy)
- Hidden from programmer (just works)

---

## 9. Integration with Existing Type System

### UDTs Respect Type Coercion

Member assignments follow the three-point coercion strategy:

```basic
TYPE Entity
    Health AS INTEGER
    Score AS DOUBLE
END TYPE

DIM Player AS Entity
Player.Health = 100.7    ' DOUBLE → INT (truncates to 100)
Player.Score = 50        ' INT → DOUBLE (widens to 50.0)
```

### Type Safety

Field types are **enforced**:

```basic
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

DIM P AS Point
P.X = "Hello"    ' ERROR: Cannot assign STRING to DOUBLE field
```

### Works with All Types

UDTs can contain:
- ✅ INTEGER, FLOAT, DOUBLE
- ✅ STRING
- ✅ Other UDTs (nested)
- ✅ (Future) Arrays as fields

---

## 10. Documentation and Expectations

### What Programmers Should Know

**Documentation should clearly state**:

1. **UDT variables are stack-allocated**
   - Fast and automatic
   - Limited by stack size (non-issue for typical programs)

2. **Function parameters are pass-by-reference**
   - Modifications are visible to caller
   - Use BYVAL if you need a copy (future)

3. **Use arrays for collections**
   - Not individual `NEW` allocations
   - Arrays are the BASIC way

4. **No OOP features**
   - No methods, inheritance, interfaces
   - Use functions and composition instead

### Example Documentation

```basic
' ============================================
' User-Defined Types in FasterBASIC
' ============================================
'
' Defining a type:
TYPE Point
    X AS DOUBLE
    Y AS DOUBLE
END TYPE

' Creating variables (stack-allocated):
DIM P AS Point
P.X = 10
P.Y = 20

' Passing to functions (by reference):
SUB MovePoint(P AS Point, DX, DY)
    P.X = P.X + DX    ' Modifies caller's point
    P.Y = P.Y + DY
END SUB

CALL MovePoint(P, 5, 10)
PRINT P.X, P.Y    ' 15, 30

' Collections use arrays:
DIM Points(100) AS Point
FOR i = 1 TO 100
    Points(i).X = i
    Points(i).Y = i * 2
NEXT i
```

---

## Summary: The FasterBASIC Way

### Design Principles

1. ✅ **Stack allocation** - simple, fast, automatic
2. ✅ **Pass by reference** - efficient, matches BASIC conventions
3. ✅ **Arrays for collections** - familiar, sufficient
4. ✅ **No complex OOP** - keep it procedural and simple
5. ✅ **No manual memory management** - let the compiler handle it

### Philosophy

> **"If it's not simple, it's not BASIC."**

User-defined types should feel like a **natural extension** of BASIC, not a bolted-on OOP system. They're just a way to group related data, nothing more.

### Result

A UDT system that is:
- 🎯 **Simple** - easy to understand and use
- ⚡ **Fast** - stack allocation, pointer passing
- 🛡️ **Safe** - automatic cleanup, no leaks
- 📚 **Familiar** - matches classic BASIC expectations
- 🚀 **Sufficient** - handles real-world use cases

**This is the BASIC way.** 🎉

---

## Appendix: What Other Languages Do

### QB45 / QuickBASIC
- UDTs pass by reference (pointer)
- Stack allocation
- **We match this** ✅

### FreeBASIC
- UDTs pass by reference by default
- Supports BYVAL/BYREF keywords
- **Similar approach** ✅

### C
- Structs pass by value (copy) by default
- Manual pointer passing for efficiency
- **We differ here** - always pass by reference for simplicity

### Java / C#
- Objects always on heap
- Pass references to heap objects
- **We differ here** - stack allocation, no GC

### Rust
- Ownership system (complex)
- Small types by value, large by reference
- **Too complex for BASIC** - we keep it simple

### Python
- Everything is a reference to heap object
- **Too dynamic for BASIC** - we're statically typed

---

**FasterBASIC UDTs: Simple, fast, and predictably BASIC.** 🎉