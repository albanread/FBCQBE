# Code Generator V2 - Start Here 🚀

**Welcome!** This guide will get you started on the Code Generator V2 implementation.

---

## 📍 Where Are We?

✅ **CFG v2 is complete** - Single-pass recursive builder, 123/125 tests passing  
✅ **Old code generator archived** - Located in `oldcodegen/` (reference only)  
🚧 **New code generator needed** - That's what we're building!

---

## 🎯 What We're Building

A new QBE IL code generator with **7 modular components**:

1. **QBEBuilder** - Low-level IL emission (temps, labels, instructions)
2. **TypeManager** - BASIC types → QBE types (w, l, s, d)
3. **SymbolMapper** - BASIC names → QBE names (%var_X_INT)
4. **RuntimeLibrary** - Wrap runtime calls (I/O, strings, arrays, math)
5. **ASTEmitter** - Statements and expressions → IL
6. **CFGEmitter** - CFG blocks and edges → control flow
7. **QBECodeGeneratorV2** - Orchestrate everything

---

## 🚀 Quick Start (5 minutes)

### Step 1: Create Directory
```bash
cd fsh/FasterBASICT/src
mkdir codegen_v2
cd codegen_v2
```

### Step 2: Create Skeleton Files
```bash
touch README.md \
      qbe_builder.h qbe_builder.cpp \
      type_manager.h type_manager.cpp \
      symbol_mapper.h symbol_mapper.cpp \
      runtime_library.h runtime_library.cpp \
      ast_emitter.h ast_emitter.cpp \
      cfg_emitter.h cfg_emitter.cpp \
      qbe_codegen_v2.h qbe_codegen_v2.cpp
```

### Step 3: Read the Design
```bash
# Start with the action plan
cat ../../../docs/CODEGEN_V2_ACTION_PLAN.md

# Then read the full design
cat ../../../docs/CODEGEN_V2_DESIGN_PLAN.md
```

---

## 📚 Essential Reading (in order)

### 1. This Document (you are here)
Quick overview and links to everything

### 2. [CODEGEN_V2_ACTION_PLAN.md](CODEGEN_V2_ACTION_PLAN.md) (720 lines)
- **Read this first after START_HERE**
- Step-by-step implementation guide
- Component interfaces with code examples
- Daily breakdown of 3-week plan
- Test programs to use

### 3. [CODEGEN_V2_DESIGN_PLAN.md](CODEGEN_V2_DESIGN_PLAN.md) (942 lines)
- **Read for deep understanding**
- Complete architectural design
- 7 implementation phases
- CFG v2 integration details
- Optimization opportunities

### 4. [CODEGEN_REFACTOR_SUMMARY.md](CODEGEN_REFACTOR_SUMMARY.md) (367 lines)
- Executive summary
- Key decisions documented
- Timeline and risks

---

## 🔑 Key Principles

### ✅ Do:
- Use **CFG edge information** for control flow (not assumptions)
- Split code into **modular files** (7 components)
- Implement **bottom-up** (QBEBuilder first)
- Test **incrementally** (simple programs first)
- Review **generated IL** with `-i` flag
- Leverage **semantic analyzer** type information
- Emit **ALL blocks** (including unreachable for GOSUB/ON GOTO)

### ❌ Don't:
- Use old generator except as reference
- Assume sequential block numbering
- Assume block N flows to N+1
- Mix CFG and AST traversal
- Duplicate type inference
- Skip unreachable blocks

---

## 📋 Implementation Order

**Bottom-up implementation (3 weeks):**

### Week 1: Foundation
- **Days 1-2:** QBEBuilder (emit IL, allocate temps, labels)
- **Days 2-3:** TypeManager (BASIC→QBE type mapping)
- **Days 3-4:** SymbolMapper (name mangling)
- **Days 4-5:** RuntimeLibrary (runtime call wrappers)

### Week 2: Emission
- **Days 5-8:** ASTEmitter (expressions, then statements)
- **Days 8-10:** CFGEmitter (blocks and edge-based flow)
- **Days 10-11:** QBECodeGeneratorV2 (orchestration)

### Week 3: Integration
- **Days 11-12:** Update compiler pipeline, add `-i` flag
- **Days 12-14:** Test simple programs with IL review
- **Days 14-18:** Test complex features
- **Days 18-21:** Full test suite (125 tests)

---

## 🧪 Testing Strategy

For every program:
```bash
# 1. Generate IL
./fbc_qbe -i test.bas > test.ssa

# 2. Review IL manually
cat test.ssa

# 3. Verify checklist:
#    ✓ All blocks present?
#    ✓ Control flow edges correct?
#    ✓ Variables properly named?
#    ✓ Runtime calls correct?
#    ✓ Types consistent?
```

Start with simple programs:
```basic
' Test 1: Hello World
10 PRINT "Hello, World!"
20 END

' Test 2: Variables  
10 LET X% = 42
20 PRINT X%
30 END

' Test 3: Arithmetic
10 LET A% = 10 + 20
20 PRINT A%
30 END
```

---

## 📊 Component Interfaces (Quick Reference)

### QBEBuilder
```cpp
std::string allocTemp(const std::string& qbeType);  // %t0, %t1, ...
std::string makeLabel(const std::string& prefix);   // @label_0, @label_1, ...
void emit(const std::string& text);                 // Raw IL output
std::string emitAdd(t1, t2, type);                  // Arithmetic ops
void emitJump(const std::string& label);            // Control flow
```

### TypeManager
```cpp
std::string getQBEType(VariableType basicType);     // INT→w, DOUBLE→d
std::string emitConversion(temp, from, to);         // Type conversions
bool needsConversion(from, to);                     // Check if needed
```

### SymbolMapper
```cpp
std::string getVariableRef(const std::string& name); // X% → %var_X_INT
std::string getArrayRef(const std::string& name);    // A% → %arr_A_INT
std::string getBlockLabel(int blockId);              // 5 → @block_5
```

### RuntimeLibrary
```cpp
void emitPrintInt(const std::string& value);         // call $basic_print_int
std::string emitInputInt();                          // call $basic_input_int
std::string emitStringConcat(s1, s2);                // call $str_concat
```

---

## 🔗 Critical CFG v2 Integration

**Old Way (BROKEN):**
```cpp
// Assumes sequential blocks
for (int i = 0; i < blocks.size(); i++) {
    emitBlock(blocks[i]);
    if (i + 1 < blocks.size()) {
        emit("jmp @block_" + std::to_string(i + 1));  // WRONG!
    }
}
```

**New Way (CORRECT):**
```cpp
// Use CFG edges
std::vector<CFGEdge> outEdges = cfg->getOutEdges(block->id);
if (outEdges.empty()) {
    emit("jmp @block_" + std::to_string(cfg->exitBlock));
} else if (outEdges.size() == 1) {
    emit("jmp @" + getBlockLabel(outEdges[0].to));
} else {
    emitConditionalBranch(outEdges);  // IF, WHILE, etc.
}
```

---

## 📂 File Structure

```
fsh/FasterBASICT/src/
├── codegen_v2/                          ← CREATE THIS
│   ├── README.md
│   ├── qbe_builder.{h,cpp}             ← Start here (Day 1)
│   ├── type_manager.{h,cpp}            ← Then this (Day 2)
│   ├── symbol_mapper.{h,cpp}           ← Then this (Day 3)
│   ├── runtime_library.{h,cpp}         ← Then this (Day 4)
│   ├── ast_emitter.{h,cpp}             ← Then this (Day 5)
│   ├── cfg_emitter.{h,cpp}             ← Then this (Day 8)
│   └── qbe_codegen_v2.{h,cpp}          ← Finally (Day 10)
├── oldcodegen/                          ← Reference only
│   ├── README_ARCHIVE.md
│   └── [old code - do not use]
└── [other compiler components]
```

---

## 📖 All Documentation Links

### Getting Started
- **[STATUS_AND_NEXT_STEPS.md](STATUS_AND_NEXT_STEPS.md)** - Current status overview
- **[CODEGEN_V2_KICKOFF.md](CODEGEN_V2_KICKOFF.md)** - Quick kickoff guide

### Implementation Guides
- **[CODEGEN_V2_ACTION_PLAN.md](CODEGEN_V2_ACTION_PLAN.md)** ⭐ **READ FIRST**
- **[CODEGEN_V2_DESIGN_PLAN.md](CODEGEN_V2_DESIGN_PLAN.md)** - Complete design
- **[CODEGEN_REFACTOR_SUMMARY.md](CODEGEN_REFACTOR_SUMMARY.md)** - Executive summary

### CFG v2 Documentation
- **[CFG_V2_COMPLETION_STATUS.md](CFG_V2_COMPLETION_STATUS.md)** - Final CFG v2 status
- **[CFG_V2_STATUS.md](CFG_V2_STATUS.md)** - Detailed status
- **[CFG_TEST_RESULTS_2026_02_01.md](CFG_TEST_RESULTS_2026_02_01.md)** - Test results

### Unreachable Code Analysis
- **[unreachable_code_analysis.md](unreachable_code_analysis.md)** - Complete (18KB, 588 lines)
- **[unreachable_warnings_summary.md](unreachable_warnings_summary.md)** - Quick ref
- **[unreachable_patterns_diagram.md](unreachable_patterns_diagram.md)** - Visual
- **[unreachable_trace_examples.md](unreachable_trace_examples.md)** - Traces

### Reference
- **[oldcodegen/README_ARCHIVE.md](../fsh/FasterBASICT/src/oldcodegen/README_ARCHIVE.md)** - Why old generator was archived

---

## ✅ Success Criteria

- [ ] All 7 components implemented
- [ ] Generates valid QBE IL
- [ ] Works with CFG v2 explicit edges
- [ ] Passes all 125 tests
- [ ] IL reviewable with `-i` flag
- [ ] Code is modular and maintainable

---

## 🎯 Your First Task

```bash
# 1. Create the directory
cd fsh/FasterBASICT/src
mkdir codegen_v2
cd codegen_v2

# 2. Create qbe_builder.h
# Start with class declaration:
# - Constructor/destructor
# - emit() methods
# - allocTemp() for SSA temps
# - makeLabel() for labels
# - emitAdd/Sub/Mul/Div() for ops

# 3. Create qbe_builder.cpp
# Implement:
# - Output stream management
# - Temp counter (%t0, %t1, ...)
# - Label counter (@label_0, @label_1, ...)
# - Basic arithmetic emission

# 4. Test with simple program
# Generate: %t0 =w add %t1, %t2
```

See **[CODEGEN_V2_ACTION_PLAN.md](CODEGEN_V2_ACTION_PLAN.md)** for complete interface definitions and examples.

---

## 💡 Quick Tips

1. **Start small** - QBEBuilder is just string emission with counters
2. **Test immediately** - Don't wait to test, verify each component
3. **Use old generator as reference** - But don't copy its assumptions
4. **Review IL constantly** - Use `-i` flag to see what you generate
5. **Ask questions** - Documentation is comprehensive, use it!

---

## 📞 Need Help?

1. **Read CODEGEN_V2_ACTION_PLAN.md** - Has detailed interfaces
2. **Read CODEGEN_V2_DESIGN_PLAN.md** - Has architectural decisions
3. **Check oldcodegen/** - For reference (what NOT to do with CFG v2)
4. **Review test programs** - In `tests/` directory

---

## 🚀 Ready to Start?

```bash
# Your first command:
cd fsh/FasterBASICT/src/codegen_v2
vim qbe_builder.h  # or your favorite editor

# Start with this skeleton:
```

```cpp
#ifndef QBE_BUILDER_H
#define QBE_BUILDER_H

#include <string>
#include <sstream>

namespace FasterBASIC {

class QBEBuilder {
public:
    QBEBuilder();
    
    // Output management
    void emit(const std::string& text);
    void emitLine(const std::string& line);
    void emitComment(const std::string& comment);
    void emitLabel(const std::string& label);
    
    // Temporary allocation
    std::string allocTemp(const std::string& qbeType);
    std::string makeLabel(const std::string& prefix);
    
    // Get output
    std::string getOutput() const;
    void clear();
    
private:
    std::ostringstream m_output;
    int m_tempCounter;
    int m_labelCounter;
};

} // namespace FasterBASIC

#endif // QBE_BUILDER_H
```

---

**Let's build it!** 🎯

**Next:** Read [CODEGEN_V2_ACTION_PLAN.md](CODEGEN_V2_ACTION_PLAN.md) and start implementing QBEBuilder.