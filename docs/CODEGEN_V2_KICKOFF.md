# Code Generator V2 - Ready to Begin! 🚀

**Date:** February 2025  
**Status:** ✅ Planning Complete - Ready to Implement

---

## What's Done ✅

1. **Old code generator archived** → `oldcodegen/` (reference only)
2. **Comprehensive design plan** → `docs/CODEGEN_V2_DESIGN_PLAN.md` (942 lines)
3. **Action plan created** → `docs/CODEGEN_V2_ACTION_PLAN.md` (720 lines)
4. **Architecture documented** with 7 modular components
5. **Unreachable code safety verified** (blocks ARE compiled)

---

## Key Corrections Applied ✓

1. ❌ Old generator will NOT be available via flag
   ✅ It's reference only, never compiled
   
2. ❌ No comparison testing with old generator
   ✅ Focus on IL correctness via -i flag review
   
3. ❌ No parallel execution
   ✅ New generator only, validate via IL inspection
   
4. ✅ Modular file structure for maintainability
5. ✅ Bottom-up implementation with continuous testing

---

## Next Steps (Start NOW)

### 1. Create Directory
```bash
cd fsh/FasterBASICT/src
mkdir codegen_v2
```

### 2. Create Skeleton Files (14 files)
```
codegen_v2/
├── README.md
├── qbe_codegen_v2.h / .cpp          (main orchestrator)
├── qbe_builder.h / .cpp             (low-level IL)
├── type_manager.h / .cpp            (type conversions)
├── symbol_mapper.h / .cpp           (name mapping)
├── runtime_library.h / .cpp         (runtime wrappers)
├── ast_emitter.h / .cpp             (statements/expressions)
└── cfg_emitter.h / .cpp             (blocks/edges)
```

### 3. Implement Bottom-Up (3 weeks)
- **Days 1-2:** QBEBuilder (foundation)
- **Days 2-3:** TypeManager
- **Days 3-4:** SymbolMapper  
- **Days 4-5:** RuntimeLibrary
- **Days 5-8:** ASTEmitter (incremental)
- **Days 8-10:** CFGEmitter
- **Days 10-11:** QBECodeGeneratorV2
- **Days 11-12:** Pipeline integration
- **Days 12-21:** Testing with IL review

---

## Test Strategy

For each test program:
```bash
# 1. Generate IL
./fbc_qbe -i test.bas > test.ssa

# 2. Review IL
cat test.ssa

# 3. Verify correctness
# - All blocks present?
# - Control flow correct?
# - Variables named properly?
# - Runtime calls correct?
# - Types consistent?
```

---

## 7 Components to Build

1. **QBEBuilder** - Emit low-level IL (temps, labels, instructions)
2. **TypeManager** - BASIC types → QBE types (w/l/s/d)
3. **SymbolMapper** - BASIC names → QBE names (%var_X_INT)
4. **RuntimeLibrary** - Wrap runtime calls (I/O, strings, arrays, math)
5. **ASTEmitter** - Statements/expressions → IL
6. **CFGEmitter** - Blocks/edges → control flow
7. **QBECodeGeneratorV2** - Orchestrate everything

---

## Critical Design Principles

✅ **CFG-First** - Use edges, not assumptions  
✅ **Edge-Aware** - Leverage edge types  
✅ **Modular** - Clear component boundaries  
✅ **Explicit** - No implicit flow assumptions  
✅ **Minimal State** - Immutable where possible  
✅ **Type Safe** - Use semantic analyzer results  

---

## Success = 125/125 Tests Pass ✓

Current CFG v2 status: 123/125 valid CFGs (98.4%)  
Target: 125/125 programs compile and run correctly

---

## First Command to Run

```bash
cd FBCQBE/fsh/FasterBASICT/src
mkdir codegen_v2
cd codegen_v2
touch README.md qbe_builder.{h,cpp} type_manager.{h,cpp} \
      symbol_mapper.{h,cpp} runtime_library.{h,cpp} \
      ast_emitter.{h,cpp} cfg_emitter.{h,cpp} \
      qbe_codegen_v2.{h,cpp}
```

---

**Status:** ✅ READY TO BEGIN IMPLEMENTATION  
**Timeline:** 3 weeks (realistic)  
**First Component:** QBEBuilder  

**LET'S BUILD IT! 🎯**
