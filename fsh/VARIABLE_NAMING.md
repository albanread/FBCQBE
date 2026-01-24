# Variable Name Mangling in FasterBASIC QBE Code Generator

## Overview

The FasterBASIC QBE code generator uses **type-mangled variable names** throughout the generated QBE IL. This document explains the mangling convention and why it's important.

## Why Name Mangling?

Variable name mangling serves several purposes:

1. **Self-Documenting Code**: Types are immediately visible in the QBE IL
2. **No Name Collisions**: `X%` (integer) and `X#` (double) become distinct names
3. **Easier Debugging**: Can understand variable types without symbol table
4. **Type Safety**: QBE IL is explicit about types at every use

## Where Mangling Happens

**Important:** Name mangling is performed by the **semantic analyzer** (phase 3), NOT the code generator (phase 5).

```
Phase 1: Lexer       → Tokens (including type suffixes)
Phase 2: Parser      → AST (preserves type suffixes)
Phase 3: Semantic    → Symbol Table (MANGLES NAMES HERE)
Phase 4: CFG Builder → Uses mangled names
Phase 5: QBE Codegen → Uses mangled names from symbol table
```

## Mangling Rules

### BASIC Type Suffixes

| Suffix | BASIC Type | C Type     | QBE Type | Description              |
|--------|------------|------------|----------|--------------------------|
| `%`    | INTEGER    | `int32_t`  | `w`      | 32-bit signed integer    |
| `!`    | SINGLE     | `float`    | `s`      | 32-bit floating point    |
| `#`    | DOUBLE     | `double`   | `d`      | 64-bit floating point    |
| `$`    | STRING     | `char*`    | `l`      | Pointer (64-bit)         |
| `&`    | LONG       | `int64_t`  | `l`      | 64-bit signed integer    |
| (none) | SINGLE     | `float`    | `s`      | Default type in BASIC    |

### Mangled Name Format

The semantic analyzer transforms BASIC variable names into mangled names:

```
<BASENAME>_<TYPE>
```

Examples:

| BASIC Variable | Base Name | Type    | Mangled Name | QBE Variable     |
|----------------|-----------|---------|--------------|------------------|
| `X%`           | X         | INT     | `X_INT`      | `%var_X_INT`     |
| `Y#`           | Y         | DOUBLE  | `Y_DOUBLE`   | `%var_Y_DOUBLE`  |
| `S$`           | S         | STRING  | `S_STRING`   | `%var_S_STRING`  |
| `TOTAL!`       | TOTAL     | FLOAT   | `TOTAL_FLOAT`| `%var_TOTAL_FLOAT` |
| `COUNT`        | COUNT     | FLOAT   | `COUNT_FLOAT`| `%var_COUNT_FLOAT` |

**Note:** Variables without a suffix default to SINGLE (FLOAT) type in BASIC.

## QBE IL Variable Names

The code generator adds a prefix to create QBE SSA variable names:

### Variables
```
%var_<MANGLED_NAME>
```

Examples:
- `%var_X_INT` - Integer variable from `X%`
- `%var_Y_DOUBLE` - Double variable from `Y#`
- `%var_S_STRING` - String variable from `S$`

### Arrays
```
%arr_<MANGLED_NAME>
```

Examples:
- `%arr_A_INT` - Integer array from `A%()`
- `%arr_B_DOUBLE` - Double array from `B#()`

### Loop Variables
```
%step_<MANGLED_NAME>    - Loop step value
%end_<MANGLED_NAME>     - Loop end value
```

Examples:
- `%step_I_INT` - Step value for `FOR I% = ...`
- `%end_I_INT` - End value for `FOR I% = ...`

### Temporaries
```
%t<N>    - Temporary SSA values (untyped)
```

Examples:
- `%t0`, `%t1`, `%t2` - Sequential temporaries

## Example: Complete Transformation

### BASIC Source
```basic
10 LET X% = 42
20 LET Y# = 3.14
30 LET S$ = "Hello"
40 PRINT X%; Y#; S$
```

### Symbol Table (After Semantic Analysis)
```
Variables:
  X_INT    (type: INTEGER, declared: line 10)
  Y_DOUBLE (type: DOUBLE,  declared: line 20)
  S_STRING (type: STRING,  declared: line 30)
```

### Generated QBE IL
```qbe
export function w $main() {
@start
    call $basic_init()

    # Variable declarations
    %var_X_INT =w copy 0                    ← INTEGER variable
    %var_Y_DOUBLE =d copy d_0.0             ← DOUBLE variable
    %var_S_STRING =l call $str_alloc(w 0)   ← STRING variable

@block_0
    # Line 10: LET X% = 42
    %t0 =w copy 42
    %var_X_INT =w copy %t0

    # Line 20: LET Y# = 3.14
    %t1 =d copy d_3.14
    %var_Y_DOUBLE =d copy %t1

    # Line 30: LET S$ = "Hello"
    %t2 =l copy $str.0
    %var_S_STRING =l copy %t2

    # Line 40: PRINT X%; Y#; S$
    call $basic_print_int(w %var_X_INT)
    call $basic_print_double(d %var_Y_DOUBLE)
    call $basic_print_string(l %var_S_STRING)
    call $basic_print_newline()

@exit
    call $basic_cleanup()
    ret 0
}

data $str.0 = { b "Hello", b 0 }
```

## Benefits of This Approach

### 1. Type Safety
Every variable use shows its type explicitly:
```qbe
%var_X_INT =w copy 42       # Clearly an integer
%var_Y_DOUBLE =d copy d_3.14  # Clearly a double
```

### 2. No Name Collisions
Different types can use the same base name:
```basic
10 DIM X%       ' Integer X
20 DIM X#       ' Double X (different variable!)
30 DIM X$       ' String X (also different!)
```

Becomes:
```qbe
%var_X_INT =w copy 0
%var_X_DOUBLE =d copy d_0.0
%var_X_STRING =l call $str_alloc(w 0)
```

### 3. Readable QBE IL
You can understand the code without the symbol table:
```qbe
%var_TOTAL_INT =w add %var_COUNT_INT, %var_VALUE_INT
call $basic_print_int(w %var_TOTAL_INT)
```

### 4. Debugging
When debugging generated QBE IL, type errors are immediately obvious:
```qbe
# ERROR: Type mismatch is visible
%var_X_INT =d copy d_3.14    # Wrong! INT can't hold DOUBLE
```

## Implementation Details

### Code Generator (qbe_codegen_main.cpp)
```cpp
// Variable names are ALREADY mangled when we get them
for (const auto& [name, varSym] : m_symbols->variables) {
    // 'name' is already "X_INT", "Y_DOUBLE", etc.
    std::string varRef = "%var_" + name;  // Creates %var_X_INT
    // ...
}
```

### Helper Function (qbe_codegen_helpers.cpp)
```cpp
std::string QBECodeGenerator::getVariableRef(const std::string& varName) {
    // varName is already mangled: "X_INT", "Y_DOUBLE", etc.
    return "%var_" + varName;
}
```

## Common Pitfalls

### ❌ Don't try to parse type from suffix
```cpp
// WRONG - type suffix already removed by parser
if (varName.back() == '%') {
    type = INTEGER;
}
```

### ✅ Use the symbol table type
```cpp
// CORRECT - get type from symbol table
auto it = m_symbols->variables.find(varName);
VariableType type = it->second.type;
```

### ❌ Don't strip the mangled suffix
```cpp
// WRONG - name is already mangled
std::string baseName = varName.substr(0, varName.find('_'));
```

### ✅ Use the mangled name as-is
```cpp
// CORRECT - use the full mangled name
std::string qbeVar = "%var_" + varName;  // varName = "X_INT"
```

## Future Considerations

### Potential Enhancements
1. **Shorter suffixes**: `_I`, `_D`, `_S` instead of `_INT`, `_DOUBLE`, `_STRING`
2. **Uniform case**: Force uppercase or lowercase for consistency
3. **Namespace prefixes**: `basic_var_X_INT` to avoid global namespace pollution

### Compatibility
- QBE IL is internal - name format can change without breaking user code
- Runtime functions use type-specific interfaces (not name-dependent)
- Generated executables don't expose QBE variable names

## Conclusion

Variable name mangling is a key feature that makes the QBE code generator:
- **Self-documenting**: Types visible at every use
- **Safe**: No name collisions between types
- **Debuggable**: Easy to understand generated IL
- **Maintainable**: Clear separation between semantic analysis and code generation

The mangling happens in the semantic analyzer and is transparent to the code generator, which simply uses the mangled names from the symbol table.