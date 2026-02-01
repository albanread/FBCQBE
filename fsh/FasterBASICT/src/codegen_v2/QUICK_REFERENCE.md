# QBE Code Generator V2 - Quick Reference

## Component Overview

```
QBECodeGeneratorV2
├── QBEBuilder       → Low-level IL emission
├── TypeManager      → Type mapping (BASIC ↔ QBE)
├── SymbolMapper     → Name mangling
├── RuntimeLibrary   → Runtime function wrappers
├── ASTEmitter       → Statements/expressions
└── CFGEmitter       → Control flow (CFG-aware)
```

---

## Quick Start

### Generate IL for a Program

```cpp
#include "codegen_v2/qbe_codegen_v2.h"

// After parsing and semantic analysis:
fbc::QBECodeGeneratorV2 codegen(semanticAnalyzer);
std::string il = codegen.generateProgram(program.get(), cfg.get());
std::cout << il;  // Or write to file
```

### Enable Verbose Mode

```cpp
codegen.setVerbose(true);  // Extra comments in IL
```

---

## Type Mappings

| BASIC Type | QBE Type | Size | Example |
|------------|----------|------|---------|
| BYTE, UBYTE | `w` | 1 | `123` |
| SHORT, USHORT | `w` | 2 | `1234` |
| INTEGER (%) | `w` | 4 | `12345` |
| LONG (&) | `l` | 8 | `1234567890` |
| SINGLE (!) | `s` | 4 | `s_3.14` |
| DOUBLE (#) | `d` | 8 | `d_3.14159` |
| STRING ($) | `l` | 8 | (pointer) |
| VOID | (empty) | 0 | (no return) |

---

## Name Mangling

| BASIC Name | Mangled (Local) | Mangled (Global) |
|------------|-----------------|------------------|
| `x` | `%var_x` | `$var_x` |
| `counter%` | `%var_counter_int` | `$var_counter_int` |
| `name$` | `%var_name_str` | `$var_name_str` |
| `pi#` | `%var_pi_dbl` | `$var_pi_dbl` |
| `value!` | `%var_value_sng` | `$var_value_sng` |
| `MyArray()` | `%arr_MyArray` | `$arr_MyArray` |
| `MyFunc` | — | `$func_MyFunc` |
| `MySub` | — | `$sub_MySub` |
| Line 100 | — | `line_100` |
| `MyLabel` | — | `label_MyLabel` |
| Block 5 | — | `block_5` |

---

## Common Patterns

### Emit a Function

```cpp
builder.emitFunctionStart("main", "w", "");
builder.emitLabel("start");
// ... function body ...
builder.emitReturn("0");
builder.emitFunctionEnd();
```

### Emit an Expression

```cpp
std::string result = astEmitter.emitExpression(expr);
// Returns temporary holding result (e.g., "%t.0")
```

### Emit a Binary Operation

```cpp
std::string t0 = builder.newTemp();  // "%t.0"
builder.emitBinary(t0, "w", "add", "10", "20");
// Generates: %t.0 =w add 10, 20
```

### Emit a Comparison

```cpp
std::string t0 = builder.newTemp();
builder.emitCompare(t0, "w", "sgt", "x", "10");
// Generates: %t.0 =w csgtw x, 10
```

### Emit a Conditional Branch

```cpp
builder.emitBranch(condition, "then_block", "else_block");
// Generates: jnz %condition, @then_block, @else_block
```

### Emit an Unconditional Jump

```cpp
builder.emitJump("target_block");
// Generates: jmp @target_block
```

### Load a Variable

```cpp
std::string value = astEmitter.loadVariable("counter%");
// Returns temporary with variable value
```

### Store to a Variable

```cpp
astEmitter.storeVariable("counter%", value);
// Stores value to variable
```

### Call a Runtime Function

```cpp
runtime.emitPrintInt("42");
// Generates: call $fb_print_int(w 42)

std::string result = runtime.emitStringLen(strPtr);
// Generates: %t.X =w call $fb_string_len(l %strPtr)
```

---

## Edge Types (CFG)

| Edge Type | QBE IL | Use Case |
|-----------|--------|----------|
| FALLTHROUGH | `jmp @target` | Sequential flow, GOTO |
| CONDITIONAL | `jnz %c, @t, @f` | IF, WHILE, FOR |
| MULTIWAY | (switch-like) | ON GOTO, SELECT CASE |
| RETURN | `ret` or `ret %val` | Function return, END |

---

## QBE Operations

### Arithmetic

```
add   sub   mul   div   rem   (signed)
udiv  urem              (unsigned)
neg                     (negation)
```

### Comparison

```
ceqw  cnew  csltw  cslew  csgtw  csgew  (signed int)
ceql  cnel  csltl  cslel  csgtl  csgel  (signed long)
ceqs  cnes  cslts  csles  csgts  csges  (float)
ceqd  cned  csltd  csled  csgtd  csged  (double)
```

### Logical

```
and   or    xor   (bitwise)
```

### Conversions

```
extsw  extuw  extsh  extuh  extsb  extub  (extend)
swtof  uwtof  sltof  ultof              (int → float)
stosi  stoui  dtosi  dtoui              (float → int)
exts   truncd                           (float ↔ double)
```

### Memory

```
loadw   loadl   loads   loadd   (load)
storew  storel  stores  stored  (store)
alloc4  alloc8  alloc16         (allocate)
```

---

## Common Errors and Fixes

### ERROR: null expression

**Fix**: Check AST construction, ensure all expression pointers are valid

### WARNING: conditional without IF statement

**Fix**: Block with CONDITIONAL edges should have IF statement in its statements list

### ERROR: array not found

**Fix**: Ensure DIM statement is processed during semantic analysis

### Segmentation fault

**Fix**: Add null checks, run with debugger:
```bash
gdb ./fbc_qbe
run -i test.bas
bt
```

---

## Testing Commands

### Generate IL Only

```bash
./fbc_qbe -i test.bas > test.ssa
cat test.ssa
```

### Compile with QBE (if installed)

```bash
./fbc_qbe -i test.bas > test.ssa
qbe -o test.o test.ssa
gcc -o test test.o -L../runtime_c -lfb_runtime -lm
./test
```

### Run Test Suite

```bash
cd tests
for test in *.bas; do
    echo "Testing $test..."
    ../fbc_qbe -i "$test" > "${test%.bas}.ssa"
done
```

---

## Architecture Rules

### ✅ DO

- Use explicit edges for control flow
- Emit ALL blocks (including UNREACHABLE)
- Use QBEBuilder for all IL emission
- Use TypeManager for type conversions
- Use SymbolMapper for all name mangling
- Add comments for complex logic
- Check for null pointers

### ❌ DON'T

- Assume block N+1 follows block N
- Skip UNREACHABLE blocks
- Emit raw IL (use QBEBuilder)
- Hardcode type conversions
- Hardcode name mangling
- Assume sequential block numbering

---

## File Locations

```
src/codegen_v2/
├── qbe_builder.{h,cpp}        (273 lines)
├── type_manager.{h,cpp}       (293 lines)
├── symbol_mapper.{h,cpp}      (342 lines)
├── runtime_library.{h,cpp}    (240 lines)
├── ast_emitter.{h,cpp}        (639 lines)
├── cfg_emitter.{h,cpp}        (436 lines)
├── qbe_codegen_v2.{h,cpp}     (352 lines)
├── README.md                  (overview)
├── INTEGRATION_GUIDE.md       (integration steps)
└── QUICK_REFERENCE.md         (this file)
```

---

## Key Functions

### QBEBuilder

```cpp
emitFunctionStart(name, returnType, params)
emitFunctionEnd()
emitLabel(label)
newTemp() → "%t.N"
emitBinary(dest, type, op, lhs, rhs)
emitCompare(dest, type, op, lhs, rhs)
emitJump(target)
emitBranch(cond, trueLabel, falseLabel)
emitReturn(value)
emitCall(dest, returnType, funcName, args)
```

### TypeManager

```cpp
getQBEType(basicType) → "w"|"l"|"s"|"d"
getTypeSize(basicType) → 1|2|4|8
isNumeric(basicType) → bool
isFloatingPoint(basicType) → bool
getConversionOp(from, to) → "swtof"|"dtosi"|...
getPromotedType(type1, type2) → promotedType
```

### SymbolMapper

```cpp
mangleVariableName(name, isGlobal) → "%var_x"|"$var_x"
mangleFunctionName(name) → "$func_MyFunc"
mangleSubName(name) → "$sub_MySub"
getBlockLabel(blockId) → "block_N"
getUniqueLabel(prefix) → "prefix_N"
```

### RuntimeLibrary

```cpp
emitPrintInt(value)
emitPrintString(strPtr)
emitStringConcat(left, right) → result
emitStringLen(strPtr) → length
emitChr(code) → strPtr
emitAbs(value, type) → result
emitSqr(value, type) → result
emitRnd() → randomFloat
```

### ASTEmitter

```cpp
emitExpression(expr) → result
emitExpressionAs(expr, expectedType) → result
emitStatement(stmt)
emitLetStatement(stmt)
emitPrintStatement(stmt)
loadVariable(name) → value
storeVariable(name, value)
```

### CFGEmitter

```cpp
emitCFG(cfg, functionName)
emitBlock(block, cfg)
emitBlockTerminator(block, cfg)
emitFallthrough(targetBlockId)
emitConditional(condition, trueId, falseId)
emitMultiway(selector, targets, defaultId)
```

### QBECodeGeneratorV2

```cpp
generateProgram(program, cfg) → IL
generateFunction(funcSymbol, cfg) → IL
generateSub(subSymbol, cfg) → IL
emitGlobalVariables()
emitGlobalArrays()
getIL() → IL string
```

---

## Example: Complete Flow

```cpp
// 1. Parse and analyze
Parser parser(tokens);
auto program = parser.parseProgram();

SemanticAnalyzer semantic;
semantic.analyze(program.get());

CFGBuilder cfgBuilder;
auto cfg = cfgBuilder.buildProgramCFG(program.get());

// 2. Generate code
fbc::QBECodeGeneratorV2 codegen(semantic);
std::string il = codegen.generateProgram(program.get(), cfg.get());

// 3. Output IL
std::cout << il;
```

---

## Status

✅ **COMPLETE** - All components implemented and ready for testing

**Total**: ~2,575 lines across 7 components

**Next**: Integration and test suite validation

---

For more details, see:
- `README.md` - Full component overview
- `INTEGRATION_GUIDE.md` - Step-by-step integration
- `docs/CODEGEN_V2_COMPLETE.md` - Complete documentation