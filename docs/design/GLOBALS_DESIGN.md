# GLOBALS Design and Implementation Plan

## Core Principle

**GLOBALS ARE NOT SSA VALUES**

Global variables in FasterBASIC are stored in a runtime-allocated vector (array) and accessed via **direct memory operations** using pointer arithmetic. They are NOT QBE SSA temporaries.

## Architecture Overview

### 1. Global Variable Vector

All GLOBAL variables are stored in a single runtime-allocated vector:

```c
// In runtime (basic_runtime.c)
static int64_t* g_global_vector = NULL;
static size_t g_global_vector_size = 0;

void basic_global_init(int64_t count) {
    g_global_vector_size = count;
    g_global_vector = (int64_t*)calloc(count, sizeof(int64_t));
}

int64_t* basic_global_base(void) {
    return g_global_vector;
}

void basic_global_cleanup(void) {
    if (g_global_vector) {
        free(g_global_vector);
        g_global_vector = NULL;
    }
}
```

### 2. Variable Storage

All variables stored as 64-bit values (8 bytes each):

- **INTEGER** (`%`): `int64_t`
- **DOUBLE** (`#`): `double` (64 bits)
- **FLOAT** (`!`): `float` (stored in 64-bit slot)
- **STRING** (`$`): `StringDescriptor*` pointer

### 3. Efficient Memory Access Pattern

**Read Operation:**
```qbe
# 1. Load global vector base address
%base =l call $basic_global_base()

# 2. Calculate address (offset pre-calculated at compile time: slot * 8)
%addr =l add %base, <BYTE_OFFSET>

# 3. Load value into cache variable
%cache =l loadl %addr
```

**Write Operation:**
```qbe
# 1. Load global vector base address
%base =l call $basic_global_base()

# 2. Calculate address (offset pre-calculated at compile time: slot * 8)
%addr =l add %base, <BYTE_OFFSET>

# 3. Perform operations on cache variable
%result =l add %cache, 5

# 4. Write back to global
storel %result, %addr
```

**Optimization Note:** The byte offset (slot * 8) is calculated at **compile time** as a constant, eliminating the runtime multiply instruction. This reduces each access from 4 instructions to 3 instructions.

**For DOUBLE type:**
```qbe
%cache =d loadd %addr   # Load double
stored %result, %addr   # Store double
```

## Code Generation Flow

### Phase 1: Initialize Global Vector

At start of `main()`:

```qbe
export function w $main() {
@start
    call $basic_runtime_init()
    
    # Allocate global vector (5 slots for 5 globals)
    call $basic_global_init(l 5)
    
    # ... program code
}
```

### Phase 2: Assign Offsets in Semantic Analyzer

```cpp
void SemanticAnalyzer::collectGlobalStatements(Program& program) {
    int nextOffset = 0;
    
    for (const auto& line : program.lines) {
        for (const auto& stmt : line->statements) {
            if (stmt->getType() == ASTNodeType::STMT_GLOBAL) {
                const GlobalStatement& globalStmt = static_cast<const GlobalStatement&>(*stmt);
                
                for (const auto& var : globalStmt.variables) {
                    // Determine type
                    VariableType varType = /* ... */;
                    
                    // Create symbol
                    VariableSymbol varSym(var.name, legacyTypeToDescriptor(varType), true);
                    varSym.type = varType;
                    varSym.isGlobal = true;
                    varSym.globalOffset = nextOffset++;  // Assign slot number (used for compile-time offset calc)
                    varSym.functionScope = "";
                    
                    m_symbolTable.variables[var.name] = varSym;
                }
            }
        }
    }
    
    m_symbolTable.globalVariableCount = nextOffset;
}
```

### Phase 3: Generate Read Access

In `getVariableRef()`:

```cpp
std::string QBECodeGenerator::getVariableRef(const std::string& varName) {
    // Check if GLOBAL variable
    if (m_symbols) {
        auto it = m_symbols->variables.find(varName);
        if (it != m_symbols->variables.end() && it->second.isGlobal) {
            // Generate efficient accessor code
            int slot = it->second.globalOffset;
            VariableType type = it->second.type;
            
            // 1. Get base address
            std::string base = allocTemp("l");
            emit("    " + base + " =l call $basic_global_base()\n");
            
            // 2. Calculate address (pre-calculate byte offset at compile time)
            int byteOffset = slot * 8;
            std::string addr = allocTemp("l");
            emit("    " + addr + " =l add " + base + ", " + std::to_string(byteOffset) + "\n");
            
            // 3. Load into cache variable
            std::string cache = allocTemp(getQBEType(type));
            if (type == VariableType::DOUBLE) {
                emit("    " + cache + " =d loadd " + addr + "\n");
            } else {
                emit("    " + cache + " =l loadl " + addr + "\n");
            }
            
            m_stats.instructionsGenerated += 3;
            return cache;  // Return cache temp
        }
    }
    
    // ... rest of logic for local variables ...
}
```

### Phase 4: Generate Write Access

In `emitLet()`:

```cpp
void QBECodeGenerator::emitLet(const LetStatement* stmt) {
    // Evaluate RHS
    std::string valueTemp = emitExpression(stmt->value.get());
    
    // Check if LHS is GLOBAL
    if (m_symbols) {
        auto it = m_symbols->variables.find(stmt->variable);
        if (it != m_symbols->variables.end() && it->second.isGlobal) {
            int slot = it->second.globalOffset;
            VariableType type = it->second.type;
            
            // 1. Get base address
            std::string base = allocTemp("l");
            emit("    " + base + " =l call $basic_global_base()\n");
            
            // 2. Calculate address (pre-calculate byte offset at compile time)
            int byteOffset = slot * 8;
            std::string addr = allocTemp("l");
            emit("    " + addr + " =l add " + base + ", " + std::to_string(byteOffset) + "\n");
            
            // 3. Store value
            if (type == VariableType::DOUBLE) {
                emit("    stored " + valueTemp + ", " + addr + "\n");
            } else {
                emit("    storel " + valueTemp + ", " + addr + "\n");
            }
            
            m_stats.instructionsGenerated += 3;
            return;
        }
    }
    
    // ... rest of logic for local variables ...
}
```

## Complete Example

### BASIC Code

```basic
GLOBAL x%, y#

x% = 10
y# = 3.14

SUB Test()
    SHARED x%, y#
    x% = x% + 5
    y# = y# * 2.0
END SUB

CALL Test
PRINT x%, y#
END
```

### Generated QBE IL

```qbe
export function w $main() {
@start
    # Initialize runtime
    call $basic_runtime_init()
    
    # Allocate global vector (2 slots)
    call $basic_global_init(l 2)
    
    # x% = 10 (slot 0, offset 0)
    %t1 =l call $basic_global_base()
    %t2 =l add %t1, 0
    storel 10, %t2
    
    # y# = 3.14 (slot 1, offset 8)
    %t3 =l call $basic_global_base()
    %t4 =l add %t3, 8
    stored d_3.14, %t4
    
    # CALL Test
    call $Test()
    
    # PRINT x%
    %t5 =l call $basic_global_base()
    %t6 =l add %t5, 0
    %t7 =l loadl %t6
    call $basic_print_int(l %t7)
    
    # PRINT y#
    %t8 =l call $basic_global_base()
    %t9 =l add %t8, 8
    %t10 =d loadd %t9
    call $basic_print_double(d %t10)
    
    call $basic_print_newline()
    jmp @exit
    
@exit
    call $basic_global_cleanup()
    call $basic_runtime_cleanup()
    ret 0
}

export function w $Test() {
@start
    # SHARED x% - read (slot 0, offset 0)
    %t1 =l call $basic_global_base()
    %t2 =l add %t1, 0
    %cache_x =l loadl %t2
    
    # x% + 5
    %t3 =l add %cache_x, 5
    
    # Write back
    %t4 =l call $basic_global_base()
    %t5 =l add %t4, 0
    storel %t3, %t5
    
    # SHARED y# - read (slot 1, offset 8)
    %t6 =l call $basic_global_base()
    %t7 =l add %t6, 8
    %cache_y =d loadd %t7
    
    # y# * 2.0
    %t8 =d mul %cache_y, d_2.0
    
    # Write back
    %t9 =l call $basic_global_base()
    %t10 =l add %t9, 8
    stored %t8, %t10
    
    ret 0
}
```

## Optimization: Cache Base Pointer

For multiple global accesses in a row, cache the base pointer:

```qbe
# Cache base at start of function
%global_base =l call $basic_global_base()

# Access global 0 (offset 0)
%addr0 =l add %global_base, 0
%val0 =l loadl %addr0

# Access global 1 (offset 8)
%addr1 =l add %global_base, 8
%val1 =d loadd %addr1
```

**Note:** Offsets are compile-time constants, so no multiply instruction is needed.

## Implementation Checklist

### Runtime (basic_runtime.c)
- [ ] Add `g_global_vector` and `g_global_vector_size`
- [ ] Implement `basic_global_init(int64_t count)`
- [ ] Implement `basic_global_base(void)` - returns base pointer
- [ ] Implement `basic_global_cleanup(void)`
- [ ] Add declarations to `basic_runtime.h`

### Semantic Analyzer (fasterbasic_semantic.h/cpp)
- [ ] Add `globalOffset` field to `VariableSymbol`
- [ ] Add `globalVariableCount` to `SymbolTable`
- [ ] Assign offsets in `collectGlobalStatements()`

### Code Generator (qbe_codegen_*.cpp)
- [ ] Emit `basic_global_init()` in `emitMainFunction()`
- [ ] Modify `getVariableRef()` to generate load sequence for globals
- [ ] Modify `emitLet()` to generate store sequence for globals
- [ ] Handle DOUBLE type with `loadd`/`stored`
- [ ] Emit `basic_global_cleanup()` in exit block

### Testing
- [ ] Simple GLOBAL in main
- [ ] GLOBAL with SHARED in SUB
- [ ] GLOBAL with SHARED in FUNCTION
- [ ] Multiple GLOBALs
- [ ] Different types (INT, DOUBLE, STRING)
- [ ] Read-modify-write in function
- [ ] Multiple functions accessing same global

## Key Points

1. **Direct memory access** - Load base, add offset, load/store
2. **No function call overhead** - Just pointer arithmetic and memory ops
3. **Efficient** - 3 QBE instructions per access (base, addr, load/store)
4. **Static offsets** - Byte offset calculated at compile time (slot * 8)
5. **Type-safe** - Use `loadl`/`storel` for integers, `loadd`/`stored` for doubles
6. **Works across scopes** - Same code pattern in main and functions
7. **Cache-friendly** - Can cache base pointer for multiple accesses

## Summary

GLOBALS use **efficient pointer arithmetic** to access a runtime vector:
1. Get base pointer: `%base =l call $basic_global_base()`
2. Calculate address: `%addr =l add %base, BYTE_OFFSET` (offset = slot * 8, pre-calculated)
3. Load/store: `%val =l loadl %addr` or `storel %val, %addr`

This generates efficient machine code with minimal overhead (3 instructions per access).