# Code Generator V2 - Design Plan

**Date:** February 2025  
**Status:** Planning Phase  
**Purpose:** Design new code generator for CFG v2 and refactored AST

---

## Executive Summary

We have successfully refactored the CFG (Control Flow Graph) to use a single-pass recursive builder (CFG v2). The old code generator was designed for the old CFG structure and needs to be replaced with a new design that:

1. Works with CFG v2's explicit edge-based architecture
2. Leverages the refactored AST structure
3. Is cleaner, more maintainable, and better organized
4. Learns from the mistakes and limitations of the old implementation
5. Is split into modular files for ease of editing and maintenance

---

## Current State Assessment

### Old Code Generator Statistics

**Size:** ~8,691 lines across 6 files
- `qbe_codegen_main.cpp` - 55KB (1,300+ lines)
- `qbe_codegen_expressions.cpp` - 105KB (2,500+ lines)
- `qbe_codegen_statements.cpp` - 112KB (2,800+ lines)
- `qbe_codegen_runtime.cpp` - 13KB (300+ lines)
- `qbe_codegen_helpers.cpp` - 54KB (1,300+ lines)
- `fasterbasic_qbe_codegen.h` - 20KB (500+ lines)

### Old Code Generator Architecture

**Strengths:**
- ✅ Modular file organization
- ✅ Separation of concerns (expressions, statements, runtime)
- ✅ Comprehensive runtime library integration
- ✅ Good documentation in README.md

**Weaknesses:**
- ❌ Assumes sequential block numbering
- ❌ Mixed CFG/AST traversal (unclear ownership)
- ❌ Limited use of CFG edge information
- ❌ Hardcoded assumptions about block structure
- ❌ Complex state management (loop stacks, GOSUB stacks)
- ❌ Type inference duplicates semantic analyzer work
- ❌ Variable naming conventions spread across multiple places
- ❌ Limited optimization opportunities

### CFG v2 Changes

**Key Improvements:**
- ✅ Single-pass recursive construction
- ✅ Explicit edges with types (Sequential, Conditional, Jump, Call/Return)
- ✅ Exit block is always block 1 (predictable)
- ✅ Proper handling of terminators (END, RETURN, GOTO, etc.)
- ✅ Correct unreachable block detection
- ✅ Loop structures with header/body/exit blocks
- ✅ SELECT CASE with When/Check/Exit blocks

**Implications for Code Generator:**
- Must iterate through edges, not assume sequential flow
- Can leverage edge types for optimization
- Must handle unreachable blocks (for GOSUB/ON GOTO targets)
- Can use structured loop metadata
- Exit block is known target for all terminators

---

## Phase 1: Preservation & Analysis

### Step 1.1: Archive Old Code Generator

**Action:** ✅ COMPLETE - Archive of current code generator created as reference only

```bash
mkdir -p fsh/FasterBASICT/src/oldcodegen
cp fsh/FasterBASICT/src/codegen/* fsh/FasterBASICT/src/oldcodegen/
cp fsh/FasterBASICT/src/fasterbasic_qbe_codegen.h fsh/FasterBASICT/src/oldcodegen/
```

**Important:** Old code generator will NOT be maintained or accessible in production. It exists purely as reference material during new implementation.

**Documentation:** ✅ COMPLETE
- README_ARCHIVE.md explaining why it's archived
- What worked well documented
- What needs improvement documented
- Cross-reference to CFG v2 migration included

### Step 1.2: Analyze Old Code Generator

**Tasks:**
1. Inventory all statement types handled
2. Inventory all expression types handled
3. Map runtime library functions used
4. Document state management patterns
5. Identify optimization opportunities missed
6. List assumptions that broke with CFG v2

**Deliverables:**
- `OLDCODEGEN_ANALYSIS.md` - Comprehensive analysis
- `STATEMENT_INVENTORY.md` - All statements and how they're handled
- `EXPRESSION_INVENTORY.md` - All expressions and how they're handled
- `RUNTIME_API.md` - Runtime library function catalog

---

## Phase 2: New Architecture Design

### Design Principles

1. **CFG-First**: Code generator driven by CFG structure, not AST traversal
2. **Edge-Aware**: Use edge types to generate optimal code
3. **Single Responsibility**: Each component has one clear job
4. **Explicit Flow**: No assumptions about block ordering
5. **Minimal State**: Reduce mutable state, increase immutability
6. **Type Safety**: Leverage semantic analyzer's type information
7. **Optimization-Ready**: Design for easy addition of optimization passes

### Architectural Layers

```
┌────────────────────────────────────────────────────────┐
│              Code Generator V2 Architecture            │
├────────────────────────────────────────────────────────┤
│                                                        │
│  ┌──────────────────────────────────────────────┐    │
│  │  QBECodeGeneratorV2 (Main Orchestrator)     │    │
│  │  - generate()                                │    │
│  │  - Entry point, drives generation            │    │
│  └────────────────┬─────────────────────────────┘    │
│                   │                                    │
│  ┌────────────────┴─────────────────────────────┐    │
│  │  Module Layer                                │    │
│  ├──────────────────────────────────────────────┤    │
│  │                                              │    │
│  │  ┌──────────────┐  ┌──────────────┐        │    │
│  │  │ CFG Emitter  │  │ AST Emitter  │        │    │
│  │  │ - Blocks     │  │ - Statements │        │    │
│  │  │ - Edges      │  │ - Expressions│        │    │
│  │  │ - Flow       │  │              │        │    │
│  │  └──────────────┘  └──────────────┘        │    │
│  │                                              │    │
│  │  ┌──────────────┐  ┌──────────────┐        │    │
│  │  │ Type Manager │  │ Symbol Map   │        │    │
│  │  │ - Type conv. │  │ - Var lookup │        │    │
│  │  │ - Promotion  │  │ - Array map  │        │    │
│  │  └──────────────┘  └──────────────┘        │    │
│  │                                              │    │
│  │  ┌──────────────┐  ┌──────────────┐        │    │
│  │  │ Runtime Lib  │  │ QBE Builder  │        │    │
│  │  │ - Call wrap  │  │ - IL emit    │        │    │
│  │  │ - Signatures │  │ - Temp alloc │        │    │
│  │  └──────────────┘  └──────────────┘        │    │
│  │                                              │    │
│  └──────────────────────────────────────────────┘    │
│                                                        │
│  ┌──────────────────────────────────────────────┐    │
│  │  Optimization Layer (Optional)               │    │
│  ├──────────────────────────────────────────────┤    │
│  │  - Dead code elimination                     │    │
│  │  - Constant folding                          │    │
│  │  - Redundant load/store elimination          │    │
│  │  - Jump optimization                         │    │
│  └──────────────────────────────────────────────┘    │
│                                                        │
└────────────────────────────────────────────────────────┘
```

### Core Components

#### 1. QBECodeGeneratorV2 (Main Class)

**Responsibilities:**
- Orchestrate entire code generation process
- Manage output stream
- Coordinate between modules
- Handle main() and function generation

**Key Methods:**
```cpp
class QBECodeGeneratorV2 {
public:
    std::string generate(const ProgramCFG& programCFG,
                        const SymbolTable& symbols,
                        const CompilerOptions& options);
    
private:
    void emitHeader();
    void emitDataSection();
    void emitFunctions();
    void emitMainFunction();
    
    // Module instances
    CFGEmitter m_cfgEmitter;
    ASTEmitter m_astEmitter;
    TypeManager m_typeManager;
    SymbolMapper m_symbolMapper;
    RuntimeLibrary m_runtime;
    QBEBuilder m_qbe;
};
```

#### 2. CFGEmitter (CFG-to-QBE)

**Responsibilities:**
- Emit QBE IL for CFG blocks
- Handle control flow edges
- Generate jumps, branches, and calls
- Use edge information to optimize flow

**Key Methods:**
```cpp
class CFGEmitter {
public:
    void emitFunction(const ControlFlowGraph& cfg);
    void emitBlock(const BasicBlock& block);
    void emitEdges(const BasicBlock& block);
    void emitConditionalBranch(const CFGEdge& edge);
    void emitUnconditionalJump(const CFGEdge& edge);
    
private:
    void optimizeFallthrough(const BasicBlock& block);
    bool canFallthrough(int fromBlock, int toBlock) const;
    std::string getEdgeLabel(const CFGEdge& edge) const;
};
```

**Key Features:**
- Edge-driven control flow (not assumption-based)
- Fallthrough optimization (omit unnecessary jumps)
- Handles unreachable blocks (for GOSUB/ON GOTO)
- Respects edge types (Sequential, Conditional, Jump)

#### 3. ASTEmitter (AST-to-QBE)

**Responsibilities:**
- Emit QBE IL for statements
- Emit QBE IL for expressions
- Delegate to runtime library for complex operations

**Key Methods:**
```cpp
class ASTEmitter {
public:
    void emitStatement(const Statement* stmt);
    std::string emitExpression(const Expression* expr);
    
private:
    // Statement handlers
    void emitPrint(const PrintStatement* stmt);
    void emitLet(const LetStatement* stmt);
    void emitDim(const DimStatement* stmt);
    void emitInput(const InputStatement* stmt);
    // ... etc
    
    // Expression handlers
    std::string emitBinaryOp(const BinaryExpression* expr);
    std::string emitUnaryOp(const UnaryExpression* expr);
    std::string emitVariable(const VariableExpression* expr);
    std::string emitLiteral(const NumberExpression* expr);
    // ... etc
};
```

**Key Features:**
- Pure AST traversal (no CFG mixing)
- Returns SSA temps for expressions
- Delegates complex ops to runtime
- No type inference (uses semantic analyzer results)

#### 4. TypeManager

**Responsibilities:**
- Map BASIC types to QBE types
- Handle type conversions and promotions
- Provide type information from semantic analyzer

**Key Methods:**
```cpp
class TypeManager {
public:
    std::string getQBEType(VariableType basicType) const;
    std::string emitConversion(const std::string& temp,
                              VariableType from,
                              VariableType to);
    VariableType getExpressionType(const Expression* expr) const;
    
    bool needsConversion(VariableType from, VariableType to) const;
    VariableType promoteTypes(VariableType t1, VariableType t2) const;
};
```

**Type Mapping:**
```
BASIC       QBE      Size    Notes
--------------------------------------
INTEGER (%) → w      32-bit  Word (signed)
LONG (&)    → l      64-bit  Long (signed)
SINGLE (!)  → s      32-bit  Single float
DOUBLE (#)  → d      64-bit  Double float
STRING ($)  → l      64-bit  Pointer
BOOLEAN     → w      32-bit  0=false, 1=true
```

#### 5. SymbolMapper

**Responsibilities:**
- Map BASIC symbols to QBE names
- Handle variable/array name mangling
- Track allocations and lifetimes

**Key Methods:**
```cpp
class SymbolMapper {
public:
    void registerSymbolTable(const SymbolTable& symbols);
    std::string getVariableRef(const std::string& name) const;
    std::string getArrayRef(const std::string& name) const;
    std::string getLabelRef(const std::string& label) const;
    
    bool isGlobal(const std::string& name) const;
    VariableType getType(const std::string& name) const;
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
```

#### 6. RuntimeLibrary

**Responsibilities:**
- Wrap runtime library function calls
- Generate call sequences
- Handle parameter marshalling

**Key Methods:**
```cpp
class RuntimeLibrary {
public:
    std::string emitPrintInt(const std::string& value);
    std::string emitPrintDouble(const std::string& value);
    std::string emitPrintString(const std::string& value);
    std::string emitInputInt();
    std::string emitInputDouble();
    std::string emitInputString();
    
    std::string emitStringConcat(const std::string& s1, const std::string& s2);
    std::string emitArrayCreate(const std::vector<int>& dimensions);
    std::string emitMathFunction(const std::string& name, 
                                 const std::vector<std::string>& args);
};
```

**Runtime API:**
```c
// Basic I/O
void basic_print_int(int32_t);
void basic_print_double(double);
void basic_print_string(const char*);
int32_t basic_input_int();
double basic_input_double();
char* basic_input_string();

// String operations
char* str_concat(const char*, const char*);
int32_t str_compare(const char*, const char*);
int32_t str_len(const char*);
char* str_substr(const char*, int32_t, int32_t);

// Array operations
void* array_create_1d(int32_t size);
void* array_create_2d(int32_t rows, int32_t cols);
int32_t array_get(void* arr, ...);
void array_set(void* arr, ..., int32_t value);

// Math functions
double basic_sin(double);
double basic_cos(double);
double basic_sqrt(double);
double basic_rnd();
```

#### 7. QBEBuilder

**Responsibilities:**
- Low-level QBE IL emission
- SSA temporary allocation
- Label generation
- Instruction formatting

**Key Methods:**
```cpp
class QBEBuilder {
public:
    void emit(const std::string& text);
    void emitLine(const std::string& line);
    void emitComment(const std::string& comment);
    void emitLabel(const std::string& label);
    
    std::string allocTemp(const std::string& qbeType);
    std::string makeLabel(const std::string& prefix);
    
    std::string emitAdd(const std::string& t1, const std::string& t2, const std::string& type);
    std::string emitSub(const std::string& t1, const std::string& t2, const std::string& type);
    std::string emitMul(const std::string& t1, const std::string& t2, const std::string& type);
    std::string emitDiv(const std::string& t1, const std::string& t2, const std::string& type);
    std::string emitCompare(const std::string& op, const std::string& t1, 
                           const std::string& t2, const std::string& type);
    
    std::string getOutput() const;
};
```

---

## Phase 3: Implementation Strategy

### Step 3.1: Create Skeleton

**Goal:** Set up new directory structure and empty implementations

**Files to Create:**
```
fsh/FasterBASICT/src/codegen_v2/
├── README.md                          - Architecture overview
├── qbe_codegen_v2.h                   - Main header
├── qbe_codegen_v2.cpp                 - Main orchestrator
├── cfg_emitter.h / .cpp               - CFG-to-QBE
├── ast_emitter.h / .cpp               - AST-to-QBE
├── type_manager.h / .cpp              - Type system
├── symbol_mapper.h / .cpp             - Symbol mapping
├── runtime_library.h / .cpp           - Runtime wrappers
└── qbe_builder.h / .cpp               - QBE IL builder
```

### Step 3.2: Implement Core (Bottom-Up)

**Order of Implementation:**

1. **QBEBuilder** (lowest level)
   - Emit functions
   - Temp allocation
   - Label generation
   - Basic instructions

2. **TypeManager**
   - Type mapping
   - Conversions
   - Promotions

3. **SymbolMapper**
   - Symbol registration
   - Name mangling
   - Reference generation

4. **RuntimeLibrary**
   - Function signatures
   - Call emission
   - Parameter marshalling

5. **ASTEmitter**
   - Expression emission (start with simple: literals, variables)
   - Binary operators
   - Function calls
   - Statement emission (start with simple: PRINT, LET)
   - Complex statements (IF, FOR, WHILE)

6. **CFGEmitter**
   - Block emission
   - Edge traversal
   - Control flow (jumps, branches)
   - Fallthrough optimization

7. **QBECodeGeneratorV2**
   - Header/data emission
   - Function coordination
   - Main() generation

### Step 3.3: Test Incrementally with IL Review

**Test Strategy:**

1. **Unit Tests**
   - Test each component in isolation
   - Mock dependencies
   - Verify QBE IL correctness

2. **Integration Tests with IL Review**
   - Simple programs first (10 PRINT "Hello")
   - Use -i flag to generate and inspect IL output
   - Review generated IL for correctness
   - Arithmetic (variables, expressions)
   - Control flow (IF, FOR, WHILE)
   - Subroutines (GOSUB/RETURN)
   - Arrays
   - Strings
   - Complex programs

3. **Regression Tests**
   - Use existing test suite (125 tests)
   - Generate IL with -i flag for each test
   - Verify IL correctness and completeness
   - Compare behavior (not necessarily identical IL)

**Test Programs (in order):**
```basic
# Test 1: Hello World
10 PRINT "Hello, World!"
20 END

# Test 2: Variables
10 LET X% = 42
20 PRINT X%
30 END

# Test 3: Expressions
10 LET A% = 10
20 LET B% = 20
30 LET C% = A% + B%
40 PRINT C%
50 END

# Test 4: IF statement
10 LET X% = 10
20 IF X% > 5 THEN PRINT "Big" ELSE PRINT "Small"
30 END

# Test 5: FOR loop
10 FOR I% = 1 TO 10
20   PRINT I%
30 NEXT I%
40 END

# Test 6: GOSUB
10 GOSUB 1000
20 PRINT "Done"
30 END
1000 PRINT "In subroutine"
1010 RETURN

# Test 7: Arrays
10 DIM A%(10)
20 LET A%(5) = 42
30 PRINT A%(5)
40 END
```

---

## Phase 4: CFG v2 Integration

### Key Integration Points

#### 4.1: Block Iteration

**Old Way (BROKEN):**
```cpp
// Assumes blocks are numbered sequentially
for (size_t i = 0; i < cfg->blocks.size(); i++) {
    emitBlock(cfg->blocks[i]);
}
```

**New Way (CORRECT):**
```cpp
// Use edge information to determine flow
std::set<int> visited;
std::queue<int> workList;
workList.push(cfg->entryBlock);

while (!workList.empty()) {
    int blockId = workList.front();
    workList.pop();
    
    if (visited.count(blockId)) continue;
    visited.insert(blockId);
    
    const BasicBlock* block = cfg->getBlock(blockId);
    emitBlock(block);
    
    // Add successors to work list
    for (const auto& edge : cfg->edges) {
        if (edge.from == blockId) {
            workList.push(edge.to);
        }
    }
}

// Also emit unreachable blocks (for GOSUB/ON GOTO)
for (const auto& block : cfg->blocks) {
    if (!visited.count(block->id)) {
        emitBlock(block.get());
    }
}
```

#### 4.2: Edge-Based Control Flow

**Old Way (BROKEN):**
```cpp
// Assumes next block is sequential
if (block->id + 1 < cfg->blocks.size()) {
    emit("jmp @block_" + std::to_string(block->id + 1) + "\n");
}
```

**New Way (CORRECT):**
```cpp
// Use CFG edge information
std::vector<CFGEdge> outEdges = cfg->getOutEdges(block->id);

if (outEdges.empty()) {
    // No successors - jump to exit
    emit("jmp @block_" + std::to_string(cfg->exitBlock) + "\n");
} else if (outEdges.size() == 1) {
    // Single successor
    const CFGEdge& edge = outEdges[0];
    if (canFallthrough(block->id, edge.to)) {
        // Omit jump if next block is sequential
    } else {
        emit("jmp @" + getBlockLabel(edge.to) + "\n");
    }
} else if (outEdges.size() == 2) {
    // Conditional branch (IF, WHILE)
    // Evaluate condition and branch
    emitConditionalBranch(outEdges);
} else {
    // Multi-way branch (SELECT CASE, ON GOTO)
    emitMultiwayBranch(outEdges);
}
```

#### 4.3: Exit Block Handling

**Key Insight:** Exit block is always block 1 in CFG v2

```cpp
void CFGEmitter::emitBlock(const BasicBlock* block) {
    // Special handling for exit block
    if (block->id == m_cfg->exitBlock) {
        emitLabel(getBlockLabel(block->id));
        emit("    call $basic_cleanup()\n");
        emit("    ret 0\n");
        return;
    }
    
    // Regular block
    emitLabel(getBlockLabel(block->id));
    for (const auto& stmt : block->statements) {
        m_astEmitter.emitStatement(stmt.get());
    }
    emitEdges(block);
}
```

#### 4.4: Unreachable Block Handling

**Important:** Unreachable blocks must still be emitted (for GOSUB/ON GOTO)

```cpp
void QBECodeGeneratorV2::emitMainFunction() {
    // Emit reachable blocks in CFG order
    m_cfgEmitter.emitReachableBlocks(m_cfg);
    
    // Emit unreachable blocks (GOSUB targets, ON GOTO targets)
    m_cfgEmitter.emitUnreachableBlocks(m_cfg);
}
```

#### 4.5: Loop Structure Metadata

**Leverage CFG v2 loop information:**

```cpp
// CFG v2 tracks loop structure
struct LoopStructure {
    int headerBlockId;
    int bodyBlockId;
    int exitBlockId;
    std::string loopVariable;  // For FOR loops
};

// Use this in code generation
void CFGEmitter::emitForLoop(const LoopStructure& loop) {
    // Header block has loop initialization
    // Body block has loop statements
    // Exit block is where loop exits go
    // Can optimize CONTINUE (goto header) and EXIT (goto exit)
}
```

---

## Phase 5: Optimization Opportunities

### Optimization Passes (Optional, Future Work)

#### 5.1: Dead Code Elimination
- Remove unreachable blocks that aren't GOSUB/ON GOTO targets
- Remove unused variables
- Remove redundant assignments

#### 5.2: Constant Folding
- Evaluate constant expressions at compile time
- `LET X% = 10 + 20` → `LET X% = 30`
- Propagate constants through basic blocks

#### 5.3: Jump Optimization
- Remove unnecessary jumps
- Optimize fallthrough when possible
- Combine multiple jumps

#### 5.4: Register Allocation
- Use QBE locals efficiently
- Minimize memory operations
- Leverage SSA form

#### 5.5: Type-Specific Optimization
- Use integer arithmetic when possible
- Avoid unnecessary type conversions
- Specialize runtime calls by type

---

## Phase 6: Migration & Validation

### Step 6.1: IL Generation and Review

**Strategy:** Generate IL with -i flag and review output for correctness

```bash
# Generate IL for test program
./fbc_qbe -i tests/arithmetic/test_simple.bas > output.ssa

# Review generated IL
cat output.ssa

# Verify:
# - All blocks present
# - Correct control flow edges
# - Proper variable handling
# - Runtime calls correct
# - Types correct
```

**Focus:** IL correctness, not comparison with old generator

### Step 6.2: Test Suite Validation

**Goal:** All 125 tests pass with new generator

**Process:**
1. Run test suite with new generator
2. Generate IL with -i flag for each test
3. Review IL for correctness
4. Verify program behavior (execution correctness)
5. Fix any issues found
6. Document IL generation patterns

### Step 6.3: Performance Benchmarking

**Metrics to Track:**
- Compilation time
- Generated code size
- Runtime performance
- Memory usage

**Benchmarks:**
- Simple programs (< 100 lines)
- Medium programs (100-1000 lines)
- Large programs (> 1000 lines)
- Compute-heavy programs (loops, math)
- I/O-heavy programs (PRINT, INPUT)

---

## Phase 7: Integration & Cleanup

### Step 7.1: Update Build System

**Changes:**
- Remove old codegen from build entirely
- Integrate codegen_v2 into compiler pipeline
- Update -i flag to output IL to file or stdout

```bash
# Default: compile to executable
./fbc_qbe program.bas -o program

# Generate and review IL
./fbc_qbe -i program.bas > program.ssa

# Or save IL to file
./fbc_qbe -i program.bas -o program.ssa
```

### Step 7.2: Documentation Updates

**Documents to Update:**
- README.md - Update compilation instructions
- ARCHITECTURE.md - Document new code gen architecture
- CONTRIBUTING.md - Code gen development guidelines
- API documentation for new components
- IL generation guide (-i flag usage)

### Step 7.3: Old Code Generator

**Status:** Reference only, not in build system
- Kept in `oldcodegen/` directory
- Documented for reference
- Not compiled or linked
- Used only for understanding old patterns

---

## Success Criteria

### Must Have (Phase 1-4)
- ✅ New code generator compiles successfully
- ✅ Generates valid QBE IL
- ✅ Handles all statement types
- ✅ Handles all expression types
- ✅ Passes 100% of test suite (125 tests)
- ✅ Works with CFG v2 edge structure
- ✅ Handles unreachable blocks correctly
- ✅ IL can be reviewed with -i flag
- ✅ Split into modular files for maintainability

### Should Have (Phase 5)
- ✅ Code is cleaner and more maintainable
- ✅ Documentation is comprehensive
- ✅ Performance is equivalent or better
- ✅ Generated IL is correct and efficient
- ✅ Fallthrough optimization working
- ✅ Easy to review and debug IL output

### Nice to Have (Phase 6)
- ✅ Optimization passes implemented
- ✅ Benchmarks show improvements
- ✅ Code size reduction
- ✅ Runtime performance improvements

---

## Risk Assessment

### High Risk
- **Compatibility:** Generated code must be compatible with runtime library
  - *Mitigation:* Extensive testing, runtime library versioning
- **Completeness:** Must handle all BASIC features
  - *Mitigation:* Reference old generator, comprehensive test suite

### Medium Risk
- **Performance:** New generator should not be slower
  - *Mitigation:* Profiling, optimization, benchmarking
- **Bugs:** New code may introduce new bugs
  - *Mitigation:* Incremental implementation, thorough testing

### Low Risk
- **Documentation:** Risk of inadequate documentation
  - *Mitigation:* Write docs alongside code
- **Migration:** Risk of breaking existing users
  - *Mitigation:* Parallel execution, gradual migration

---

## Timeline Estimate

### Optimistic (2 weeks)
- Week 1: Phase 1-3 (Archive, Design, Core Implementation)
- Week 2: Phase 4-6 (Integration, Testing, Validation)

### Realistic (4 weeks)
- Week 1: Phase 1-2 (Archive, Design)
- Week 2: Phase 3 (Core Implementation)
- Week 3: Phase 4 (CFG v2 Integration)
- Week 4: Phase 5-6 (Testing, Optimization, Validation)

### Pessimistic (8 weeks)
- Weeks 1-2: Phase 1-2 (Archive, Analysis, Design)
- Weeks 3-5: Phase 3-4 (Implementation, Integration)
- Weeks 6-7: Phase 5 (Optimization, Bug Fixes)
- Week 8: Phase 6-7 (Validation, Documentation, Deprecation)

---

## Next Steps

### Immediate Actions

1. **Archive Old Generator**
   ```bash
   mkdir -p fsh/FasterBASICT/src/oldcodegen
   cp fsh/FasterBASICT/src/codegen/* fsh/FasterBASICT/src/oldcodegen/
   ```

2. **Create Analysis Documents**
   - OLDCODEGEN_ANALYSIS.md
   - STATEMENT_INVENTORY.md
   - EXPRESSION_INVENTORY.md

3. **Set Up New Directory**
   ```bash
   mkdir -p fsh/FasterBASICT/src/codegen_v2
   ```

4. **Create Skeleton Files**
   - qbe_codegen_v2.h
   - qbe_codegen_v2.cpp
   - (and all component headers/implementations)

5. **Write Tests**
   - Unit tests for each component
   - Integration tests for simple programs

### Decision Points

**Before Starting Implementation:**
- [ ] Review and approve this plan
- [ ] Decide on optimization scope (phase 5)
- [ ] Confirm timeline expectations
- [ ] Assign resources

**Key Decisions:**
- Use incremental implementation (yes/no)?
- Include optimization passes (yes/no/later)?
- Parallel execution during migration (yes/no)?
- Keep old generator long-term (yes/no)?

---

## Conclusion

This plan provides a structured approach to replacing the old code generator with a new design that leverages CFG v2's improved architecture. The new generator will be:

- **Cleaner:** Better separation of concerns
- **More Maintainable:** Clearer component boundaries
- **CFG v2 Compatible:** Works with explicit edges
- **Correct:** Handles unreachable blocks properly
- **Optimizable:** Designed for future improvements

**Recommended Approach:** Start with Phase 1 (archival and analysis - COMPLETE), then proceed incrementally through implementation with continuous IL review. Old generator kept as reference only, never compiled or executed.

**Key Success Factor:** Incremental implementation with continuous IL review (-i flag) ensures we catch issues early and generate correct, efficient code from day one.

**Modular Structure:** Split implementation into focused files (CFGEmitter, ASTEmitter, TypeManager, etc.) for ease of editing and maintenance.

---

**Status:** ✅ Plan Complete and Updated - Phase 1 COMPLETE

**Next:** Create codegen_v2 directory structure and begin implementation with QBEBuilder component.

**Key Changes from Original Plan:**
1. ✅ Old generator is reference only (not available via flag)
2. ✅ No comparison testing needed (verify correctness, not equivalence)
3. ✅ Focus on IL review with -i flag
4. ✅ Modular file structure for maintainability
5. ✅ Old generator never compiled into production