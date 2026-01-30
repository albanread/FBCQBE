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

# 2. Calculate offset (slot * 8 bytes)
%offset =l mul <slot>, 8

# 3. Calculate address
%addr =l add %base, %offset

# 4. Load value into cache variable
%cache =l loadl %addr
```

**Write Operation:**
```qbe
# 1. Load global vector base address
%base =l call $basic_global_base()

# 2. Calculate offset
%offset =l mul <slot>, 8

# 3. Calculate address
%addr =l add %base, %offset

# 4. Perform operations on cache variable
%result =l add %cache, 5

# 5. Write back to global
storel %result, %addr
```

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
                    varSym.globalOffset = nextOffset++;  // Assign slot number
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
            
            // 2. Calculate byte offset
            std::string offset = allocTemp("l");
            emit("    " + offset + " =l mul " + std::to_string(slot) + ", 8\n");
            
            // 3. Calculate address
            std::string addr = allocTemp("l");
            emit("    " + addr + " =l add " + base + ", " + offset + "\n");
            
            // 4. Load into cache variable
            std::string cache = allocTemp(getQBEType(type));
            if (type == VariableType::DOUBLE) {
                emit("    " + cache + " =d loadd " + addr + "\n");
            } else {
                emit("    " + cache + " =l loadl " + addr + "\n");
            }
            
            m_stats.instructionsGenerated += 4;
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
            
            // 2. Calculate byte offset
            std::string offset = allocTemp("l");
            emit("    " + offset + " =l mul " + std::to_string(slot) + ", 8\n");
            
            // 3. Calculate address
            std::string addr = allocTemp("l");
            emit("    " + addr + " =l add " + base + ", " + offset + "\n");
            
            // 4. Store value
            if (type == VariableType::DOUBLE) {
                emit("    stored " + valueTemp + ", " + addr + "\n");
            } else {
                emit("    storel " + valueTemp + ", " + addr + "\n");
            }
            
            m_stats.instructionsGenerated += 4;
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
    
    # x% = 10 (slot 0)
    %t1 =l call $basic_global_base()
    %t2 =l mul 0, 8
    %t3 =l add %t1, %t2
    storel 10, %t3
    
    # y# = 3.14 (slot 1)
    %t4 =l call $basic_global_base()
    %t5 =l mul 1, 8
    %t6 =l add %t4, %t5
    stored d_3.14, %t6
    
    # CALL Test
    call $Test()
    
    # PRINT x%
    %t7 =l call $basic_global_base()
    %t8 =l mul 0, 8
    %t9 =l add %t7, %t8
    %t10 =l loadl %t9
    call $basic_print_int(l %t10)
    
    # PRINT y#
    %t11 =l call $basic_global_base()
    %t12 =l mul 1, 8
    %t13 =l add %t11, %t12
    %t14 =d loadd %t13
    call $basic_print_double(d %t14)
    
    call $basic_print_newline()
    jmp @exit
    
@exit
    call $basic_global_cleanup()
    call $basic_runtime_cleanup()
    ret 0
}

export function w $Test() {
@start
    # SHARED x% - read (slot 0)
    %t1 =l call $basic_global_base()
    %t2 =l mul 0, 8
    %t3 =l add %t1, %t2
    %cache_x =l loadl %t3
    
    # x% + 5
    %t4 =l add %cache_x, 5
    
    # Write back
    %t5 =l call $basic_global_base()
    %t6 =l mul 0, 8
    %t7 =l add %t5, %t6
    storel %t4, %t7
    
    # SHARED y# - read (slot 1)
    %t8 =l call $basic_global_base()
    %t9 =l mul 1, 8
    %t10 =l add %t8, %t9
    %cache_y =d loadd %t10
    
    # y# * 2.0
    %t11 =d mul %cache_y, d_2.0
    
    # Write back
    %t12 =l call $basic_global_base()
    %t13 =l mul 1, 8
    %t14 =l add %t12, %t13
    stored %t11, %t14
    
    ret 0
}
```

## Optimization: Cache Base Pointer

For multiple global accesses in a row, cache the base pointer:

```qbe
# Cache base at start of function
%global_base =l call $basic_global_base()

# Access global 0
%offset0 =l mul 0, 8
%addr0 =l add %global_base, %offset0
%val0 =l loadl %addr0

# Access global 1
%offset1 =l mul 1, 8
%addr1 =l add %global_base, %offset1
%val1 =d loadd %addr1
```

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
3. **Efficient** - 4 QBE instructions per access (base, offset, addr, load/store)
4. **Type-safe** - Use `loadl`/`storel` for integers, `loadd`/`stored` for doubles
5. **Works across scopes** - Same code pattern in main and functions
6. **Cache-friendly** - Can cache base pointer for multiple accesses

## Summary

GLOBALS use **efficient pointer arithmetic** to access a runtime vector:
1. Get base pointer: `%base =l call $basic_global_base()`
2. Calculate offset: `%off =l mul slot, 8`
3. Calculate address: `%addr =l add %base, %off`
4. Load/store: `%val =l loadl %addr` or `storel %val, %addr`

This generates efficient machine code with minimal overhead.