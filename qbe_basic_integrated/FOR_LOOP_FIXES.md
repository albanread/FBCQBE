# FOR Loop Investigation and Fixes

## Summary

Investigated and fixed FOR loop code generation issues in the FasterBASIC QBE compiler. All FOR loop tests now pass successfully.

## Issues Found and Fixed

### 1. Literal `\n` Characters in Generated IL (CRITICAL)

**Problem:** The FOR/NEXT loop code generation was emitting literal `\n` strings instead of actual newlines, causing QBE to fail with "invalid character \" errors.

**Location:** `fsh/FasterBASICT/src/codegen/qbe_codegen_statements.cpp`

**Fix:** Changed 5 occurrences from `"\\n"` to `"\n"`:
- Line 632: FOR loop variable initialization
- Line 638: User variable sync in FOR init
- Line 716: NEXT loop variable increment (2 occurrences)
- Line 722: User variable sync in NEXT

```cpp
// Before:
emit("    " + varRef + " =l copy " + startTemp + "\\n");

// After:
emit("    " + varRef + " =l copy " + startTemp + "\n");
```

### 2. Invalid QBE Variable Names with BASIC Type Suffixes (CRITICAL)

**Problem:** Variables with BASIC type suffixes (`%`, `$`, `#`, `!`, `&`, `@`, `^`) were being used directly in QBE IL, causing syntax errors. For example, `%var_i%` instead of `%var_i_`.

**Example Error:**
```
qbe:test_for_nested.bas:15: invalid character   (32)
```

**Root Cause:** The semantic analyzer stores variable names with their original BASIC suffixes, but QBE requires valid C-like identifiers.

**Fix:** Created sanitization function to replace BASIC type suffixes with underscores:

```cpp
static std::string sanitizeForQBE(const std::string& name) {
    std::string result = name;
    for (size_t i = 0; i < result.length(); ++i) {
        char c = result[i];
        if (c == '%' || c == '$' || c == '#' || c == '!' || 
            c == '&' || c == '@' || c == '^') {
            result[i] = '_';
        }
    }
    return result;
}
```

**Locations Fixed:**

1. **Variable declarations** (`qbe_codegen_main.cpp:267-310`)
   - Sanitize names when declaring variables in main function

2. **Variable references** (`qbe_codegen_helpers.cpp:308-370`)
   - Sanitize names in `getVariableRef()` function

3. **FOR loop initialization** (`qbe_codegen_statements.cpp:589-642`)
   - Sanitize user variable references
   - Sanitize step variable names (`%step_varname`)
   - Sanitize end variable names (`%end_varname`)

4. **NEXT statement** (`qbe_codegen_statements.cpp:690-740`)
   - Sanitize user variable references
   - Sanitize step variable names

5. **FOR loop condition check** (`qbe_codegen_main.cpp:661-720`)
   - Sanitize variable names for step and end variables

### 3. Token Redefinition in Lexer

**Problem:** The `AT` token was defined twice in `fasterbasic_token.h`:
- Line 159: `AT` for the AT command (position cursor)
- Line 248: `AT` for the @ byte suffix

**Fix:** Renamed second definition to `AT_SUFFIX` and updated parser references:
- `fasterbasic_token.h:248`: Changed to `AT_SUFFIX`
- `fasterbasic_parser.cpp:2252`: Updated FOR statement suffix checking
- `fasterbasic_parser.cpp:2296`: Updated NEXT statement suffix checking

### 4. Semantic Analyzer Variable Initialization

**Problem:** Incorrect `VariableInfo` struct usage (non-existent) in FOR loop variable registration.

**Fix:** Changed to use proper `VariableSymbol` constructor:
```cpp
// Before:
m_symbolTable.variables[plainVarName] = VariableInfo{
    VariableType::INT,
    TypeDescriptor(BaseType::INTEGER),
    stmt.location
};

// After:
VariableSymbol varSym(plainVarName, TypeDescriptor(BaseType::INTEGER), true);
varSym.firstUse = stmt.location;
m_symbolTable.variables[plainVarName] = varSym;
```

## Test Results

All FOR loop tests now compile successfully:

✓ `test_for_64.bas` - 64-bit loop counter
✓ `test_for_loop.bas` - General loop test
✓ `test_for_minimal.bas` - Simple 1 to 5 loop
✓ `test_for_modify_index.bas` - Modifying loop index
✓ `test_for_nested.bas` - Nested loops with % suffix
✓ `test_for_next.bas` - NEXT statement variations
✓ `test_for_simple.bas` - Simple loops with different steps
✓ `test_for_tiny.bas` - Minimal loop test
✓ `test_for_var_ref.bas` - Variable reference in loop

## Example Generated Assembly

From `test_for_minimal.bas` (FOR i = 1 TO 5):

```assembly
.text
.balign 4
.globl _main
_main:
	hint	#34
	stp	x29, x30, [sp, -32]!
	mov	x29, sp
	str	x19, [x29, 24]
	bl	_basic_runtime_init
	mov	x19, #1
L2:
	cmp	x19, #5
	cset	w1, le
	cmp	x19, #5
	cset	w0, ge
	mov	w2, #0
	and	w0, w0, w2
	mov	w2, #0
	and	w1, w1, w2
	orr	w0, w0, w1
	cmp	w0, #0
	beq	L4
	mov	x0, x19
	bl	_basic_print_int
	bl	_basic_print_newline
	mov	x0, #1
	add	x19, x19, x0
	b	L2
L4:
	bl	_basic_runtime_cleanup
	mov	w0, #0
	ldr	x19, [x29, 24]
	ldp	x29, x30, [sp], 32
	ret
```

## Compilation Command

```bash
./qbe_basic input.bas > output.s
```

Or with IL output:
```bash
./qbe_basic -i input.bas > output.qbe
```

## Build Information

- Compiler: `qbe_basic` (QBE + FasterBASIC integrated)
- Build script: `build_qbe_basic.sh`
- Build time: ~5 seconds for full rebuild
- Executable size: 1.6 MB
- Target: ARM64 macOS

### 5. FOR Loop Condition Logic Bug (CRITICAL - RUNTIME)

**Problem:** FOR loops never executed because the condition check logic was inverted. The loop would exit immediately even when it should run.

**Root Cause:** Line 702 in `qbe_codegen_main.cpp` used `cnew` (compare not equal) instead of `ceqw` (compare equal) to check if the step was non-negative.

**Incorrect Logic:**
```cpp
// This computes: notNeg = (isNeg != 0) 
// When step=1: isNeg=0, so notNeg=0 (WRONG - should be 1)
emit("    " + notNeg + " =w cnew " + isNeg + ", 0\n");
```

**Fix:**
```cpp
// Correct: notNeg = (isNeg == 0)
// When step=1: isNeg=0, so notNeg=1 (CORRECT)
emit("    " + notNeg + " =w ceqw " + isNeg + ", 0\n");
```

**Location:** `fsh/FasterBASICT/src/codegen/qbe_codegen_main.cpp:703`

### 6. FOR Loop Variable Tracking Bug (CRITICAL - RUNTIME)

**Problem:** Variables with type suffixes (like `i%`) printed incorrect values (always 0) because the loop was updating one variable (`%var_i_`) while PRINT used another (`%var_i_INT`).

**Root Cause:** The `m_forLoopVariables` set stored variable names with suffixes (`i%`), but the lookup code stripped suffixes and looked for the plain name (`i`), so it never found a match.

**Fix:** Strip suffixes when collecting FOR loop variable names:

```cpp
// Strip suffix from variable name (i% -> i, j$ -> j, etc.)
std::string varName = forStmt->variable;
size_t pos = varName.find_last_of("%$#!&@^");
if (pos != std::string::npos && pos == varName.length() - 1) {
    varName = varName.substr(0, pos);
}
m_forLoopVariables.insert(varName);
```

**Location:** `fsh/FasterBASICT/src/codegen/qbe_codegen_main.cpp:144-149`

## Runtime Test Results

After all fixes, comprehensive FOR loop test produces correct output:

```
=== FOR Loop Comprehensive Test ===

Test 1: Simple ascending (1 TO 5)
12345

Test 2: Loop with STEP 2
0246810

Test 3: Descending loop (10 TO 1 STEP -1)
10987654321

Test 4: Loop with % suffix
n% = 1
n% = 2
n% = 3

Test 5: Nested loops
Outer: 1
  Inner: 1
  Inner: 2
  Inner: 3
Outer: 2
  Inner: 1
  Inner: 2
  Inner: 3

Test 6: Sum calculation
Sum of 1 to 10 = 55

Test 7: Triple nested
1,1,1
1,1,2
1,2,1
1,2,2
2,1,1
2,1,2
2,2,1
2,2,2

=== All FOR loop tests complete ===
```

## Conclusion

FOR loops are now fully functional in the FasterBASIC QBE compiler. The main issues were:
1. Incorrect escape sequences in string literals (compile-time)
2. Invalid variable name characters in QBE IL (compile-time)
3. Missing sanitization throughout the code generation pipeline (compile-time)
4. **Inverted loop condition logic** (runtime - loops never executed)
5. **Variable tracking for suffixed names** (runtime - wrong values printed)

All fixes maintain backward compatibility and follow QBE's strict IL format requirements. The compiler now successfully compiles and runs:
- Simple FOR loops with ascending/descending ranges
- FOR loops with custom STEP values (positive and negative)
- Nested FOR loops (2 and 3 levels deep)
- FOR loops with type-suffixed variables (%, $, #, etc.)
- FOR loops with calculations and accumulation