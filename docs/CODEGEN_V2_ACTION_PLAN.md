# Code Generator V2 - Action Plan

**Date:** February 2025  
**Status:** Ready to Begin Implementation  
**Current Phase:** Phase 3 - Implementation  

---

## Overview

We are building a new code generator (codegen_v2) that works with CFG v2's explicit edge-based architecture. The old code generator has been archived as reference only and will NOT be used in production.

**Key Principles:**
1. Old generator is **reference only** (never compiled or executed)
2. New generator is **modular** (split into focused files)
3. Implementation is **incremental** (bottom-up with continuous testing)
4. Validation uses **IL review** (via -i flag, not comparison)

---

## Current Status

### ✅ COMPLETE
- Phase 1: Old code generator archived to `oldcodegen/`
- Phase 2: Architecture designed and documented
- Documentation: 3 comprehensive docs created (~1,600 lines)
- Critical finding: Unreachable blocks ARE compiled (safe)

### 🚧 IN PROGRESS
- Phase 3: Implementation (NEXT)

---

## Immediate Next Steps

### Step 1: Create Directory Structure (TODAY)

**Action:**
```bash
# Create main directory
mkdir -p fsh/FasterBASICT/src/codegen_v2

# Create directory structure
cd fsh/FasterBASICT/src/codegen_v2
```

**Files to create:**
```
codegen_v2/
├── README.md                    - Module overview
├── qbe_codegen_v2.h            - Main header
├── qbe_codegen_v2.cpp          - Main orchestrator
├── qbe_builder.h               - Low-level IL builder
├── qbe_builder.cpp
├── type_manager.h              - Type conversions
├── type_manager.cpp
├── symbol_mapper.h             - Symbol name mapping
├── symbol_mapper.cpp
├── runtime_library.h           - Runtime call wrappers
├── runtime_library.cpp
├── ast_emitter.h               - Statements/expressions
├── ast_emitter.cpp
├── cfg_emitter.h               - Blocks/edges
└── cfg_emitter.cpp
```

---

### Step 2: Implement QBEBuilder (Day 1-2)

**Purpose:** Low-level QBE IL emission

**Priority:** Highest (foundation for everything else)

**Interface:**
```cpp
class QBEBuilder {
public:
    // Output management
    void emit(const std::string& text);
    void emitLine(const std::string& line);
    void emitComment(const std::string& comment);
    void emitLabel(const std::string& label);
    
    // Temporary allocation
    std::string allocTemp(const std::string& qbeType);  // w, l, s, d
    std::string makeLabel(const std::string& prefix);
    
    // Basic instructions
    std::string emitAdd(const std::string& t1, const std::string& t2, 
                       const std::string& type);
    std::string emitSub(const std::string& t1, const std::string& t2,
                       const std::string& type);
    std::string emitMul(const std::string& t1, const std::string& t2,
                       const std::string& type);
    std::string emitDiv(const std::string& t1, const std::string& t2,
                       const std::string& type);
    
    // Comparisons
    std::string emitCompare(const std::string& op,      // ceqw, cltw, etc.
                           const std::string& t1,
                           const std::string& t2,
                           const std::string& type);
    
    // Control flow
    void emitJump(const std::string& label);
    void emitCondBranch(const std::string& cond,
                       const std::string& trueLabel,
                       const std::string& falseLabel);
    
    // Function calls
    std::string emitCall(const std::string& funcName,
                        const std::vector<std::pair<std::string, std::string>>& args);
    
    // Memory operations
    std::string emitLoad(const std::string& addr, const std::string& type);
    void emitStore(const std::string& value, const std::string& addr,
                  const std::string& type);
    
    // Type conversions
    std::string emitIntToDouble(const std::string& temp);
    std::string emitDoubleToInt(const std::string& temp);
    
    // Output retrieval
    std::string getOutput() const;
    void clear();
    
private:
    std::ostringstream m_output;
    int m_tempCounter = 0;
    int m_labelCounter = 0;
};
```

**Test with:**
```cpp
// Test basic emission
QBEBuilder builder;
builder.emitComment("Test addition");
std::string t1 = builder.allocTemp("w");
std::string t2 = builder.allocTemp("w");
builder.emit("    " + t1 + " =w copy 10\n");
builder.emit("    " + t2 + " =w copy 20\n");
std::string result = builder.emitAdd(t1, t2, "w");
// Should produce: %t2 =w add %t0, %t1
```

---

### Step 3: Implement TypeManager (Day 2-3)

**Purpose:** Map BASIC types to QBE types, handle conversions

**Priority:** High (needed by all emitters)

**Interface:**
```cpp
class TypeManager {
public:
    // Type mapping
    std::string getQBEType(VariableType basicType) const;
    VariableType getVariableType(const std::string& varName) const;
    
    // Type conversion
    std::string emitConversion(QBEBuilder& builder,
                              const std::string& temp,
                              VariableType from,
                              VariableType to);
    
    // Type checking
    bool needsConversion(VariableType from, VariableType to) const;
    VariableType promoteTypes(VariableType t1, VariableType t2) const;
    
    // Type queries
    bool isNumeric(VariableType type) const;
    bool isInteger(VariableType type) const;
    bool isFloat(VariableType type) const;
    bool isString(VariableType type) const;
    
    // Register symbol table for type lookup
    void setSymbolTable(const SymbolTable* symbols);
    
private:
    const SymbolTable* m_symbols = nullptr;
};
```

**Type Mapping Table:**
```
BASIC           QBE     Size    Notes
----------------------------------------
INTEGER (%)  →  w       32-bit  Word (signed)
LONG (&)     →  l       64-bit  Long (signed)
SINGLE (!)   →  s       32-bit  Single float
DOUBLE (#)   →  d       64-bit  Double float
STRING ($)   →  l       64-bit  Pointer
```

**Test with:**
```cpp
TypeManager types;
assert(types.getQBEType(VariableType::INT) == "w");
assert(types.getQBEType(VariableType::DOUBLE) == "d");
assert(types.needsConversion(VariableType::INT, VariableType::DOUBLE) == true);
```

---

### Step 4: Implement SymbolMapper (Day 3-4)

**Purpose:** Map BASIC symbols to QBE names

**Priority:** High (needed by AST emitter)

**Interface:**
```cpp
class SymbolMapper {
public:
    // Registration
    void registerSymbolTable(const SymbolTable* symbols);
    
    // Variable references
    std::string getVariableRef(const std::string& name) const;
    std::string getArrayRef(const std::string& name) const;
    std::string getLabelRef(const std::string& label) const;
    std::string getBlockLabel(int blockId) const;
    
    // Symbol queries
    bool isVariable(const std::string& name) const;
    bool isArray(const std::string& name) const;
    bool isLabel(const std::string& name) const;
    VariableType getType(const std::string& name) const;
    
private:
    const SymbolTable* m_symbols = nullptr;
};
```

**Naming Convention:**
```
BASIC           Mangled          QBE Variable
------------------------------------------------
X%              X_INT            %var_X_INT
Y#              Y_DOUBLE         %var_Y_DOUBLE
S$              S_STRING         %var_S_STRING
A%(10)          A_INT            %arr_A_INT
MyLabel:        MyLabel          @MyLabel
Block 5         block_5          @block_5
```

**Test with:**
```cpp
SymbolMapper mapper;
mapper.registerSymbolTable(&symbols);
assert(mapper.getVariableRef("X%") == "%var_X_INT");
assert(mapper.getArrayRef("A%") == "%arr_A_INT");
assert(mapper.getBlockLabel(5) == "@block_5");
```

---

### Step 5: Implement RuntimeLibrary (Day 4-5)

**Purpose:** Wrap runtime library function calls

**Priority:** Medium (needed for complex operations)

**Interface:**
```cpp
class RuntimeLibrary {
public:
    RuntimeLibrary(QBEBuilder& builder, TypeManager& types)
        : m_builder(builder), m_types(types) {}
    
    // I/O
    void emitPrintInt(const std::string& value);
    void emitPrintDouble(const std::string& value);
    void emitPrintString(const std::string& value);
    void emitPrintNewline();
    std::string emitInputInt();
    std::string emitInputDouble();
    std::string emitInputString();
    
    // Strings
    std::string emitStringConcat(const std::string& s1, const std::string& s2);
    std::string emitStringCompare(const std::string& s1, const std::string& s2);
    std::string emitStringLength(const std::string& s);
    std::string emitStringSubstr(const std::string& s, const std::string& start,
                                 const std::string& len);
    
    // Arrays
    std::string emitArrayCreate(const std::vector<std::string>& dimensions,
                                VariableType elemType);
    std::string emitArrayGet(const std::string& arr,
                            const std::vector<std::string>& indices);
    void emitArraySet(const std::string& arr,
                     const std::vector<std::string>& indices,
                     const std::string& value);
    
    // Math
    std::string emitAbs(const std::string& x, VariableType type);
    std::string emitSqrt(const std::string& x);
    std::string emitSin(const std::string& x);
    std::string emitCos(const std::string& x);
    std::string emitPow(const std::string& base, const std::string& exp);
    std::string emitRnd();
    
private:
    QBEBuilder& m_builder;
    TypeManager& m_types;
};
```

**Runtime Function Signatures:**
```c
// I/O
void basic_print_int(int32_t);
void basic_print_double(double);
void basic_print_string(const char*);
void basic_print_newline();
int32_t basic_input_int();
double basic_input_double();
char* basic_input_string();

// Strings
char* str_concat(const char*, const char*);
int32_t str_compare(const char*, const char*);
int32_t str_len(const char*);
char* str_substr(const char*, int32_t, int32_t);

// Arrays
void* array_create_1d(int32_t);
void* array_create_2d(int32_t, int32_t);
int32_t array_get_int(void*, ...);
void array_set_int(void*, ..., int32_t);

// Math
double basic_abs(double);
double basic_sqrt(double);
double basic_sin(double);
double basic_rnd();
```

---

### Step 6: Implement ASTEmitter (Day 5-8)

**Purpose:** Emit QBE IL for statements and expressions

**Priority:** High (core functionality)

**Implementation Order:**
1. Simple expressions (literals, variables)
2. Binary operators (+, -, *, /, <, >, etc.)
3. Unary operators (-, NOT)
4. Function calls
5. Array access
6. Simple statements (PRINT, LET)
7. Control flow (IF, FOR, WHILE)
8. Advanced statements (GOSUB, SELECT CASE)

**Interface:**
```cpp
class ASTEmitter {
public:
    ASTEmitter(QBEBuilder& builder, 
               TypeManager& types,
               SymbolMapper& symbols,
               RuntimeLibrary& runtime)
        : m_builder(builder), m_types(types), 
          m_symbols(symbols), m_runtime(runtime) {}
    
    // Statement emission
    void emitStatement(const Statement* stmt);
    
    // Expression emission (returns SSA temp holding result)
    std::string emitExpression(const Expression* expr, 
                               VariableType* outType = nullptr);
    
private:
    // Statement handlers
    void emitPrint(const PrintStatement* stmt);
    void emitLet(const LetStatement* stmt);
    void emitDim(const DimStatement* stmt);
    void emitInput(const InputStatement* stmt);
    void emitGoto(const GotoStatement* stmt);
    void emitGosub(const GosubStatement* stmt);
    void emitReturn(const ReturnStatement* stmt);
    void emitEnd(const EndStatement* stmt);
    void emitLabel(const LabelStatement* stmt);
    // ... etc for all statement types
    
    // Expression handlers
    std::string emitBinaryOp(const BinaryExpression* expr, VariableType* outType);
    std::string emitUnaryOp(const UnaryExpression* expr, VariableType* outType);
    std::string emitVariable(const VariableExpression* expr, VariableType* outType);
    std::string emitNumber(const NumberExpression* expr, VariableType* outType);
    std::string emitString(const StringExpression* expr, VariableType* outType);
    std::string emitFunctionCall(const FunctionCallExpression* expr, VariableType* outType);
    std::string emitArrayAccess(const ArrayAccessExpression* expr, VariableType* outType);
    
    QBEBuilder& m_builder;
    TypeManager& m_types;
    SymbolMapper& m_symbols;
    RuntimeLibrary& m_runtime;
};
```

**Test Programs (in order):**
```basic
# Test 1: Hello World
10 PRINT "Hello, World!"
20 END

# Test 2: Variables
10 LET X% = 42
20 PRINT X%
30 END

# Test 3: Arithmetic
10 LET A% = 10
20 LET B% = 20
30 LET C% = A% + B%
40 PRINT C%
50 END
```

---

### Step 7: Implement CFGEmitter (Day 8-10)

**Purpose:** Emit QBE IL for CFG blocks and edges

**Priority:** High (CFG v2 integration)

**Interface:**
```cpp
class CFGEmitter {
public:
    CFGEmitter(QBEBuilder& builder,
               ASTEmitter& astEmitter,
               SymbolMapper& symbols)
        : m_builder(builder), m_astEmitter(astEmitter), 
          m_symbols(symbols) {}
    
    // Function emission
    void emitFunction(const ControlFlowGraph* cfg);
    
    // Block emission
    void emitBlock(const BasicBlock* block);
    
    // Edge-based control flow
    void emitEdges(const BasicBlock* block,
                  const std::vector<CFGEdge>& outEdges);
    
private:
    void emitReachableBlocks();
    void emitUnreachableBlocks();  // For GOSUB/ON GOTO targets
    
    void emitConditionalBranch(const std::vector<CFGEdge>& edges);
    void emitMultiwayBranch(const std::vector<CFGEdge>& edges);
    
    bool canFallthrough(int fromBlock, int toBlock) const;
    void optimizeFallthrough(const BasicBlock* block);
    
    QBEBuilder& m_builder;
    ASTEmitter& m_astEmitter;
    SymbolMapper& m_symbols;
    const ControlFlowGraph* m_cfg = nullptr;
};
```

**Key:** Use CFG edge information, NOT block ordering assumptions

---

### Step 8: Implement QBECodeGeneratorV2 (Day 10-11)

**Purpose:** Main orchestrator

**Priority:** High (ties everything together)

**Interface:**
```cpp
class QBECodeGeneratorV2 {
public:
    QBECodeGeneratorV2();
    
    // Main API
    std::string generate(const ProgramCFG& programCFG,
                        const SymbolTable& symbols,
                        const CompilerOptions& options);
    
private:
    void emitHeader();
    void emitDataSection();
    void emitFunctions();
    void emitMainFunction();
    
    // Components
    QBEBuilder m_builder;
    TypeManager m_typeManager;
    SymbolMapper m_symbolMapper;
    RuntimeLibrary m_runtime;
    ASTEmitter m_astEmitter;
    CFGEmitter m_cfgEmitter;
    
    // State
    const ProgramCFG* m_programCFG = nullptr;
    const SymbolTable* m_symbols = nullptr;
    CompilerOptions m_options;
};
```

---

### Step 9: Update Compiler Pipeline (Day 11-12)

**Action:** Integrate new code generator into fbc_qbe

**Changes to make:**
```cpp
// In main compilation pipeline (fbc_qbe.cpp or similar)

// OLD (remove):
// #include "fasterbasic_qbe_codegen.h"
// QBECodeGenerator codegen;

// NEW (add):
#include "codegen_v2/qbe_codegen_v2.h"
QBECodeGeneratorV2 codegen;

// Same API call:
std::string qbeIL = codegen.generate(programCFG, symbols, options);

// Add -i flag handling:
if (options.outputIL) {
    std::cout << qbeIL;
    return 0;
}
```

**Update build system:**
```bash
# In CMakeLists.txt or build script
# Remove: codegen/*.cpp
# Add: codegen_v2/*.cpp
```

---

### Step 10: Test with IL Review (Day 12-14)

**Process:**
```bash
# 1. Compile a test program with -i flag
./fbc_qbe -i tests/hello_world.bas > hello.ssa

# 2. Review generated IL
cat hello.ssa

# 3. Verify:
#    - All blocks present
#    - Control flow correct
#    - Variables properly named
#    - Runtime calls correct
#    - Types correct

# 4. Compile IL to binary (if QBE is available)
qbe hello.ssa -o hello.s
as hello.s -o hello.o
gcc hello.o runtime.a -o hello

# 5. Run and verify output
./hello
```

**IL Review Checklist:**
- [ ] Function header correct (export function w $main())
- [ ] Basic blocks labeled (@block_0, @block_1, etc.)
- [ ] Variables properly named (%var_X_INT, etc.)
- [ ] Control flow edges correct (jmp, jnz)
- [ ] Runtime calls present (call $basic_print_int, etc.)
- [ ] Types consistent (w, l, s, d used correctly)
- [ ] Exit block reached (jmp @block_1, ret 0)

---

## Test Suite Integration

### Phase 1: Simple Programs (Day 13)
- Hello World
- Variable assignment
- Arithmetic operations
- PRINT statements

### Phase 2: Control Flow (Day 14)
- IF/THEN/ELSE
- FOR loops
- WHILE loops
- GOTO

### Phase 3: Subroutines (Day 15)
- GOSUB/RETURN
- ON GOSUB
- ON GOTO

### Phase 4: Complex Features (Day 16-18)
- Arrays (DIM, array access)
- Strings (concatenation, functions)
- SELECT CASE
- Exception handling

### Phase 5: Full Test Suite (Day 19-21)
- Run all 125 tests
- Generate IL for each with -i
- Review and verify correctness
- Fix any issues found

---

## Success Criteria

### Must Have ✓
- [ ] All components implemented and tested
- [ ] Compiles successfully
- [ ] Generates valid QBE IL
- [ ] Works with CFG v2 edge structure
- [ ] Passes all 125 tests
- [ ] IL can be reviewed with -i flag
- [ ] Code is modular and maintainable

### Verification Process ✓
- [ ] Generate IL for each test program
- [ ] Review IL manually for correctness
- [ ] Compile IL to binary (if possible)
- [ ] Verify program behavior matches expected
- [ ] Document any IL patterns or issues

---

## Timeline

### Optimistic (2 weeks)
- Days 1-7: Core components (QBEBuilder through ASTEmitter)
- Days 8-10: CFG integration
- Days 11-14: Pipeline integration and basic testing

### Realistic (3 weeks) ⭐ RECOMMENDED
- Days 1-8: Core components with thorough testing
- Days 9-12: CFG/AST emitters
- Days 13-15: Pipeline integration
- Days 16-21: Full test suite validation

### Pessimistic (4 weeks)
- Week 1: QBEBuilder, TypeManager, SymbolMapper
- Week 2: RuntimeLibrary, ASTEmitter (expressions)
- Week 3: ASTEmitter (statements), CFGEmitter
- Week 4: Integration, testing, fixes

---

## Key Reminders

### ❌ Don't:
- Use old code generator in any way except reference
- Assume sequential block numbering
- Assume blocks are in execution order
- Mix CFG and AST traversal logic
- Duplicate type inference from semantic analyzer

### ✅ Do:
- Use CFG edge information for control flow
- Split code into focused, modular files
- Test each component incrementally
- Review generated IL with -i flag
- Leverage semantic analyzer's type information
- Emit ALL blocks (including unreachable for GOSUB/ON GOTO)

---

## Next Immediate Actions

### RIGHT NOW:
```bash
cd FBCQBE/fsh/FasterBASICT/src
mkdir codegen_v2
cd codegen_v2
```

### TODAY:
1. Create skeleton files (all .h and .cpp files)
2. Add header guards and namespace
3. Write class declarations
4. Start implementing QBEBuilder
5. Write unit tests for QBEBuilder

### THIS WEEK:
1. Complete QBEBuilder (Day 1-2)
2. Complete TypeManager (Day 2-3)
3. Complete SymbolMapper (Day 3-4)
4. Complete RuntimeLibrary (Day 4-5)
5. Begin ASTEmitter (Day 5+)

---

## Status Tracking

**Current Phase:** Phase 3 - Implementation  
**Current Step:** Step 1 - Create directory structure  
**Days Elapsed:** 0  
**Components Complete:** 0/7  
**Tests Passing:** 0/125  

**Updated:** February 2025  
**Next Review:** After QBEBuilder complete  

---

**Let's begin! First action: Create the codegen_v2 directory and skeleton files.**