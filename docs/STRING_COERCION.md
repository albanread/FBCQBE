# String Type Coercion in FasterBASIC

## Overview

FasterBASIC now supports automatic string type coercion when mixing ASCII and UNICODE strings. The `OPTION DETECTSTRING` mode is now the default, providing intelligent string type detection based on literal content.

## Default String Mode

**`OPTION DETECTSTRING` is now the default mode.**

Previous versions required explicit `OPTION UNICODE` or `OPTION ASCII` declarations. Now the compiler automatically detects string encoding based on the content of string literals:

- **ASCII strings**: Literals containing only characters with codepoints < 128 are stored as byte arrays (ASCII encoding)
- **UNICODE strings**: Literals containing any character with codepoint ≥ 128 are stored as UTF-32 arrays

## String Type Promotion Rules

When concatenating or comparing strings of different encodings, FasterBASIC automatically promotes types to ensure correctness:

### Concatenation (`+` operator)

| Left Type | Right Type | Result Type | Behavior |
|-----------|-----------|-------------|----------|
| ASCII     | ASCII     | ASCII       | No promotion, efficient byte concatenation |
| ASCII     | UNICODE   | UNICODE     | ASCII promoted to UTF-32 on-the-fly |
| UNICODE   | ASCII     | UNICODE     | ASCII promoted to UTF-32 on-the-fly |
| UNICODE   | UNICODE   | UNICODE     | Direct UTF-32 concatenation |

### Comparison Operators (`=`, `<>`, `<`, `>`, `<=`, `>=`)

String comparisons work seamlessly across ASCII and UNICODE types. The runtime library handles encoding differences automatically.

## Examples

### Basic Type Coercion

```basic
OPTION DETECTSTRING  ' This is now the default

DIM ascii_str$
DIM unicode_str$

ascii_str$ = "Hello "        ' ASCII (all chars < 128)
unicode_str$ = "世界"         ' UNICODE (contains non-ASCII)

' Concatenation automatically promotes ASCII to UNICODE
DIM result$
result$ = ascii_str$ + unicode_str$
PRINT result$  ' Output: Hello 世界
```

### Multiple Concatenations

```basic
' Mixed ASCII and UNICODE in a single expression
DIM mixed$
mixed$ = "Start " + "middle " + "文字 " + "end"
PRINT mixed$  ' Output: Start middle 文字 end
```

### String Comparison

```basic
DIM ascii$
DIM unicode$

ascii$ = "test"
unicode$ = "test"

IF ascii$ = unicode$ THEN
    PRINT "Strings are equal"  ' This works!
END IF
```

### Complex Example

```basic
OPTION DETECTSTRING

' Demonstrate type promotion in complex expressions
DIM part1$, part2$, part3$, final$

part1$ = "English"           ' ASCII
part2$ = " 中文 "             ' UNICODE (Chinese)
part3$ = "Español"           ' ASCII (even though Spanish, no accents here)

final$ = part1$ + part2$ + part3$
PRINT final$  ' Output: English 中文 Español
```

## Implementation Details

### Compiler Changes

1. **Default Mode**: `CompilerOptions::StringMode` now defaults to `DETECTSTRING` instead of `ASCII`
2. **Type Inference**: `inferBinaryExpressionTypeD()` promotes result to UNICODE when either operand is UNICODE
3. **Code Generation**: `emitBinaryOp()` handles all combinations of STRING and UNICODE types for concatenation and comparison

### Runtime Library

The runtime library (`string_utf32.c`) handles type promotion transparently:

- `string_concat()`: Automatically detects encoding of both operands
  - If both ASCII: Creates ASCII result (efficient)
  - If either UNICODE: Creates UTF-32 result, promoting ASCII operands during copy
  
- `string_compare()`: Works across encoding boundaries

### Memory and Performance

- **ASCII strings** use 1 byte per character (efficient for English text)
- **UNICODE strings** use 4 bytes per character (UTF-32 for O(1) indexing)
- **Promotion overhead**: ASCII→UNICODE conversion happens only when necessary during concatenation
- **No implicit conversions**: String variables maintain their encoding until explicitly concatenated with a different type

## Testing

Comprehensive tests validate string coercion behavior:

### Test Files

- `tests/types/test_string_coercion_simple.bas` - Basic ASCII/UNICODE mixing
- `tests/types/test_string_coercion.bas` - Comprehensive coercion tests
- `tests/types/test_detectstring.bas` - DETECTSTRING mode validation
- `tests/types/test_detectstring_simple.bas` - Simple DETECTSTRING test

### Test Results (All Passing ✓)

```
STRING TYPE COERCION TESTS (DETECTSTRING)
✓ test_detectstring_simple
✓ test_detectstring
✓ test_string_coercion_simple
  - Hello 世界
  - 世界Hello
  - Hello World
✓ test_string_coercion
  - ASCII+ASCII: Hello World
  - ASCII+UNICODE: Hello世界
  - UNICODE+ASCII: 世界Hello
  - UNICODE+UNICODE: 世界こんにちは
  - Mixed literals: Start middle 文字 end
  - ASCII==UNICODE comparison works
  - Auto UNICODE: Unicode: café
  - Complex: Hello 世界 World
```

## Migration Guide

### From Previous Versions

If your code explicitly used `OPTION ASCII` or `OPTION UNICODE`:

**Old way (still works):**
```basic
OPTION UNICODE
DIM str$
str$ = "Hello"  ' Forces UNICODE encoding even for ASCII text
```

**New way (recommended):**
```basic
OPTION DETECTSTRING  ' Or omit - it's the default
DIM str$
str$ = "Hello"      ' Automatically ASCII (efficient)
str$ = "Hello 世界" ' Automatically UNICODE (when needed)
```

### Backwards Compatibility

- Existing code with explicit `OPTION ASCII` or `OPTION UNICODE` continues to work
- The default behavior change only affects programs that didn't specify a string mode
- All string operations (concatenation, comparison, functions) work identically regardless of encoding

## Benefits

1. **Developer Convenience**: No need to choose between ASCII and UNICODE upfront
2. **Memory Efficiency**: ASCII text uses 1/4 the memory compared to forced UTF-32
3. **International Support**: Unicode text "just works" without special declarations
4. **Type Safety**: Compiler and runtime ensure correct handling of mixed encodings
5. **Performance**: ASCII operations remain fast; promotion only happens when necessary

## Future Enhancements

Potential future improvements:

- UTF-8 internal representation (memory efficient, but O(n) indexing)
- Explicit literal type hints: `"text"u` for UNICODE, `"text"a` for ASCII
- Automatic ASCII→UNICODE promotion for variables (currently only for expressions)
- Performance optimizations for common concatenation patterns

## See Also

- `fsh/FasterBASICT/src/fasterbasic_options.h` - Compiler options
- `fsh/FasterBASICT/runtime_c/string_utf32.c` - Runtime string implementation
- `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - Type inference and promotion
- `fsh/FasterBASICT/src/codegen/qbe_codegen_expressions.cpp` - Code generation