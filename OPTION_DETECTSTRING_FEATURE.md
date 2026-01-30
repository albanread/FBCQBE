# OPTION DETECTSTRING Feature - Implementation Summary

**Date:** 2024
**Status:** ✅ COMPLETE - All tests passing (69/69)

---

## Overview

Added a new string mode `OPTION DETECTSTRING` that automatically detects whether string literals should be treated as ASCII or Unicode based on their content, rather than forcing a single mode for the entire program.

### Previous Behavior

- `OPTION UNICODE` - All strings treated as Unicode (codepoint arrays)
- `OPTION ASCII` (implicit default) - All strings treated as byte sequences, non-ASCII characters cause errors
- **Problem:** Program was "stuck" in one mode - couldn't mix ASCII and Unicode strings

### New Behavior

- `OPTION DETECTSTRING` (new default) - Each string literal automatically detected:
  - ASCII if all bytes < 128 → `BaseType::STRING`
  - Unicode if any byte ≥ 128 → `BaseType::UNICODE`
- `OPTION ASCII` - Explicit ASCII-only mode (non-ASCII is error)
- `OPTION UNICODE` - Explicit Unicode-only mode (all strings are Unicode)
- Programs can now naturally mix ASCII and Unicode strings without declaring a mode

---

## Implementation Details

### 1. New String Mode Enum

**File:** `fsh/FasterBASICT/src/fasterbasic_options.h`

Changed from boolean `unicodeMode` to enum `stringMode`:

```cpp
enum class StringMode {
    ASCII,         // All strings are byte sequences, non-ASCII is error
    UNICODE,       // All strings are Unicode codepoint arrays
    DETECTSTRING   // Auto-detect per-literal (default)
};

StringMode stringMode = StringMode::DETECTSTRING;  // Default
```

### 2. AST Updates

**File:** `fsh/FasterBASICT/src/fasterbasic_ast.h`

- Added `ASCII` and `DETECTSTRING` to `OptionStatement::OptionType` enum
- Added `hasNonASCII` field to `StringExpression` class to track literal content

```cpp
class StringExpression : public Expression {
public:
    std::string value;
    bool hasNonASCII;  // Track if string contains non-ASCII characters
    
    explicit StringExpression(const std::string& v, bool nonASCII = false);
};
```

### 3. Token System

**File:** `fsh/FasterBASICT/src/fasterbasic_token.h`

- Added `ASCII` and `DETECTSTRING` token types
- Token struct already had `hasNonASCII` field for tracking string content

### 4. Lexer Updates

**File:** `fsh/FasterBASICT/src/fasterbasic_lexer.cpp`

- Added `ASCII` and `DETECTSTRING` as recognized keywords

### 5. Parser Updates

**File:** `fsh/FasterBASICT/src/fasterbasic_parser.cpp`

- `collectOptionsFromTokens()`: Handle `OPTION ASCII` and `OPTION DETECTSTRING`
- `parseOptionStatement()`: Parse new option types
- `validateStringLiterals()`: Allow non-ASCII in DETECTSTRING and UNICODE modes
- `parsePrimary()`: Pass `hasNonASCII` flag to `StringExpression` constructor

### 6. Semantic Analyzer Updates

**File:** `fsh/FasterBASICT/src/fasterbasic_semantic.h`

Changed `SymbolTable::unicodeMode` (bool) to `stringMode` (enum):

```cpp
CompilerOptions::StringMode stringMode = CompilerOptions::StringMode::DETECTSTRING;
```

Added helper method:

```cpp
BaseType getStringTypeForLiteral(bool hasNonASCII) const {
    switch (stringMode) {
        case StringMode::ASCII:
            return BaseType::STRING;
        case StringMode::UNICODE:
            return BaseType::UNICODE;
        case StringMode::DETECTSTRING:
            return hasNonASCII ? BaseType::UNICODE : BaseType::STRING;
    }
}
```

**File:** `fsh/FasterBASICT/src/fasterbasic_semantic.cpp`

Updated all ~18 locations that used `unicodeMode`:
- String literal type inference: Use `getStringTypeForLiteral()` with per-literal detection
- Variable/field declarations: Use global mode (UNICODE if mode is UNICODE, else STRING)
- Type conversions: Use appropriate mode based on context

---

## Key Design Decisions

### 1. When to Detect vs. Use Global Mode

**Per-Literal Detection (DETECTSTRING mode only):**
- String literal expressions (`"Hello"` vs `"Hello 🌍"`)
- Each literal independently typed based on content

**Global Mode (all modes):**
- Variable declarations: `DIM A$` - uses mode setting
- Array declarations: `DIM Arr$(10)` - uses mode setting
- UDT field declarations: `TYPE T : Name AS STRING : END TYPE` - uses mode setting
- Function return types - uses mode setting

**Rationale:** Declarations need consistent types across the program. Only literals can vary per-instance.

### 2. Default Mode

Changed default from ASCII to **DETECTSTRING** because:
- More user-friendly (no errors on Unicode by default)
- Allows natural mixing of ASCII and Unicode
- Backwards compatible (ASCII strings work exactly as before)
- Performance: ASCII strings are still byte sequences (no Unicode overhead unless needed)

### 3. Backwards Compatibility

- Programs without OPTION statement: Now default to DETECTSTRING (was ASCII)
  - **Impact:** Programs with Unicode literals now work instead of erroring
  - **Benefit:** More permissive, no breaking changes
- Programs with `OPTION UNICODE`: Behavior unchanged
- Programs needing explicit ASCII-only: Can add `OPTION ASCII`

---

## Testing

### Test 1: Simple DETECTSTRING

**File:** `tests/types/test_detectstring_simple.bas`

```basic
10 REM Test: OPTION DETECTSTRING - simple test
20 OPTION DETECTSTRING
30 LET A$ = "Hello"
40 PRINT A$
50 PRINT "PASS"
60 END
```

**Result:** ✅ PASS
- ASCII string handled correctly
- No Unicode overhead for pure ASCII

### Test 2: Mixed ASCII and Unicode

**File:** `tests/types/test_detectstring.bas`

```basic
10 REM Test: OPTION DETECTSTRING - automatic detection
20 OPTION DETECTSTRING
30 LET A$ = "Hello World"        ' ASCII
40 LET B$ = "Hello 🌍"           ' Unicode (emoji)
50 LET C$ = "Testing 123"        ' ASCII
60 PRINT "ASCII: "; A$
70 PRINT "Unicode: "; B$
80 PRINT "ASCII: "; C$
90 IF LEN(A$) > 0 AND LEN(B$) > 0 AND LEN(C$) > 0 THEN PRINT "PASS"
```

**Result:** ✅ PASS
```
ASCII: Hello World
Unicode: Hello 🌍
ASCII: Testing 123
PASS
```

### Full Test Suite

**Result:** ✅ **69/69 tests passing** (no regressions)

---

## Performance Implications

### Memory

- **ASCII strings:** Remain as byte sequences (1 byte per character)
- **Unicode strings:** Stored as codepoint arrays (4 bytes per character)
- **DETECTSTRING mode:** Each string uses only what it needs

### Runtime

- **ASCII strings:** No change - same performance as before
- **Unicode strings:** Same as OPTION UNICODE mode
- **Detection overhead:** Zero - detection happens at parse time (compile-time)

### Optimization

Programs that mix ASCII and Unicode get the best of both:
- English text: Fast byte operations
- Emoji/Unicode: Full Unicode support
- No performance penalty for ASCII-only strings in a mixed program

---

## Usage Examples

### Example 1: Automatic Detection (Recommended)

```basic
OPTION DETECTSTRING  ' Default - can be omitted

' ASCII strings (byte sequences)
LET greeting$ = "Hello"
LET message$ = "Error code: 404"

' Unicode strings (codepoint arrays)
LET emoji$ = "👍"
LET multilang$ = "Hello/Bonjour/こんにちは"

PRINT greeting$    ' Fast byte operations
PRINT emoji$       ' Unicode support
```

### Example 2: Explicit ASCII Mode

```basic
OPTION ASCII

LET name$ = "John"
LET code$ = "ABC123"
' LET invalid$ = "🚫"  ' Compile error: non-ASCII not allowed
```

### Example 3: Explicit Unicode Mode

```basic
OPTION UNICODE

' All strings are Unicode, even if ASCII
LET ascii$ = "Hello"     ' Stored as Unicode
LET emoji$ = "🎉"        ' Also Unicode
```

---

## Migration Guide

### For New Programs

Just use DETECTSTRING (default):
```basic
' No OPTION needed - DETECTSTRING is default
LET s1$ = "Hello"
LET s2$ = "Hello 🌍"
```

### For Existing Programs

#### Programs with Unicode Content

**Before:**
```basic
' Error: Non-ASCII characters not allowed
LET greeting$ = "Hello 🌍"
```

**After (two options):**
```basic
' Option 1: Use DETECTSTRING (new default)
OPTION DETECTSTRING
LET greeting$ = "Hello 🌍"

' Option 2: Use UNICODE mode
OPTION UNICODE
LET greeting$ = "Hello 🌍"
```

#### Programs Requiring ASCII-Only

**Before:**
```basic
' Implicit ASCII mode
LET data$ = "ABCD"
```

**After:**
```basic
' Explicit ASCII mode (if you want to enforce no Unicode)
OPTION ASCII
LET data$ = "ABCD"
' Non-ASCII characters will be compile errors
```

---

## Technical Notes

### String Type Resolution Flow

1. **Lexer:** Tokenizes string literal, sets `token.hasNonASCII` flag
2. **Parser:** Creates `StringExpression` with `hasNonASCII` flag
3. **Semantic Analyzer:** 
   - If DETECTSTRING mode: calls `getStringTypeForLiteral(hasNonASCII)`
   - Returns `STRING` or `UNICODE` based on content
   - If ASCII/UNICODE mode: uses fixed type
4. **Codegen:** Generates appropriate runtime calls based on type

### Type System Consistency

- **Variable declarations:** Type determined at declaration time by mode
- **Literals:** Type determined per-literal in DETECTSTRING mode
- **Assignments:** Type coercion handled by semantic analyzer
- **Function parameters:** Type matches formal parameter declaration

---

## Files Modified

### Core Implementation (11 files)

1. `fsh/FasterBASICT/src/fasterbasic_options.h` - String mode enum
2. `fsh/FasterBASICT/src/fasterbasic_ast.h` - OptionType enum, StringExpression
3. `fsh/FasterBASICT/src/fasterbasic_token.h` - New tokens
4. `fsh/FasterBASICT/src/fasterbasic_lexer.cpp` - Keyword recognition
5. `fsh/FasterBASICT/src/fasterbasic_parser.cpp` - Option parsing, string creation
6. `fsh/FasterBASICT/src/fasterbasic_semantic.h` - SymbolTable stringMode
7. `fsh/FasterBASICT/src/fasterbasic_semantic.cpp` - Type inference updates

### Tests (2 new files)

8. `tests/types/test_detectstring_simple.bas` - Basic ASCII test
9. `tests/types/test_detectstring.bas` - Mixed ASCII/Unicode test

---

## Future Enhancements

### Potential Improvements

1. **String concatenation optimization:** Detect if all parts are ASCII to avoid Unicode conversion
2. **Automatic promotion:** Convert ASCII string to Unicode when concatenated with Unicode
3. **Type hints:** Optional syntax to force literal type: `"text"u` for Unicode, `"text"a` for ASCII
4. **Performance metrics:** Report string type distribution in compilation stats

### Not Implemented (By Design)

1. **Runtime detection:** All detection is compile-time (no runtime overhead)
2. **Dynamic type changing:** String variables have fixed type after declaration
3. **Implicit conversions:** ASCII ↔ Unicode conversions are explicit in type system

---

## Conclusion

✅ **OPTION DETECTSTRING successfully implemented**
✅ **All 69 tests passing - zero regressions**
✅ **Backwards compatible with improved defaults**
✅ **Zero runtime overhead for detection**
✅ **Natural mixing of ASCII and Unicode strings**

The feature provides a better user experience while maintaining performance and compatibility. Programs can now naturally include Unicode content without special configuration, while still benefiting from efficient ASCII string handling for pure-ASCII data.

---

## Quick Reference

| Option | Behavior | Use Case |
|--------|----------|----------|
| `OPTION DETECTSTRING` | Auto-detect per literal | **Recommended default** - mix ASCII and Unicode naturally |
| `OPTION UNICODE` | All strings are Unicode | Programs with heavy Unicode use |
| `OPTION ASCII` | ASCII only, non-ASCII errors | Strict ASCII-only requirements |

**Default:** `DETECTSTRING` (if no OPTION statement)