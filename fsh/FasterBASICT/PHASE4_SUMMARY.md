# Phase 4 Implementation Summary: Parser & Suffix Support

## Overview
Phase 4 adds lexer and parser support for the new type system introduced in Phases 1-3, including new type suffixes (`@` for BYTE, `^` for SHORT) and `AS` type keywords for all integer sizes and unsigned variants.

## Changes Made

### 1. Token Definitions (`fasterbasic_token.h`)

#### New Token Types Added:
- **Type Keywords:**
  - `KEYWORD_BYTE` - BYTE type keyword
  - `KEYWORD_SHORT` - SHORT type keyword
  - `KEYWORD_UBYTE` - UBYTE (unsigned byte) type keyword
  - `KEYWORD_USHORT` - USHORT (unsigned short) type keyword
  - `KEYWORD_UINTEGER` - UINTEGER (unsigned integer) type keyword
  - `KEYWORD_ULONG` - ULONG (unsigned long) type keyword

- **Type Suffixes:**
  - `TYPE_BYTE` - `@` suffix for BYTE variables
  - `TYPE_SHORT` - `^` suffix for SHORT variables

#### Token String Mappings:
Updated `tokenTypeToString()` to include string representations for all new tokens.

---

### 2. Lexer Updates (`fasterbasic_lexer.cpp`)

#### Keyword Table:
Added keyword mappings in `initializeKeywords()`:
```cpp
s_keywords["BYTE"] = TokenType::KEYWORD_BYTE;
s_keywords["SHORT"] = TokenType::KEYWORD_SHORT;
s_keywords["UBYTE"] = TokenType::KEYWORD_UBYTE;
s_keywords["USHORT"] = TokenType::KEYWORD_USHORT;
s_keywords["UINTEGER"] = TokenType::KEYWORD_UINTEGER;
s_keywords["UINT"] = TokenType::KEYWORD_UINTEGER;  // Alias
s_keywords["ULONG"] = TokenType::KEYWORD_ULONG;
```

#### Suffix Recognition:
Updated `scanIdentifierOrKeyword()` to recognize `@` and `^` as type suffixes:
```cpp
// Check for type suffix (%, !, #, $, @, ^)
char suffix = currentChar();
if (suffix == '%' || suffix == '!' || suffix == '#' || suffix == '$' || 
    suffix == '@' || suffix == '^') {
    text += advance();
}
```

**Note:** The `^` character is handled as POWER operator when standalone, but as a type suffix when following an identifier. The lexer flow correctly handles this: identifiers with suffixes are scanned first in `scanIdentifierOrKeyword()`, and only standalone operators go through `scanOperator()`.

---

### 3. Parser Updates (`fasterbasic_parser.cpp`)

#### Helper Functions:

**`isTypeKeyword()`** - Updated to recognize new type keywords:
```cpp
bool Parser::isTypeKeyword(TokenType type) const {
    return type == TokenType::KEYWORD_INTEGER || type == TokenType::KEYWORD_DOUBLE ||
           type == TokenType::KEYWORD_SINGLE || type == TokenType::KEYWORD_STRING ||
           type == TokenType::KEYWORD_LONG || type == TokenType::KEYWORD_BYTE ||
           type == TokenType::KEYWORD_SHORT || type == TokenType::KEYWORD_UBYTE ||
           type == TokenType::KEYWORD_USHORT || type == TokenType::KEYWORD_UINTEGER ||
           type == TokenType::KEYWORD_ULONG;
}
```

**`asTypeToSuffix()`** - Maps AS type keywords to token suffixes:
```cpp
case TokenType::KEYWORD_BYTE:    return TokenType::TYPE_BYTE;
case TokenType::KEYWORD_SHORT:   return TokenType::TYPE_SHORT;
case TokenType::KEYWORD_UBYTE:   return TokenType::TYPE_BYTE;   // Same suffix, unsigned tracked in TypeDescriptor
case TokenType::KEYWORD_USHORT:  return TokenType::TYPE_SHORT;  // Same suffix, unsigned tracked in TypeDescriptor
case TokenType::KEYWORD_UINTEGER: return TokenType::TYPE_INT;   // Same suffix, unsigned tracked in TypeDescriptor
case TokenType::KEYWORD_ULONG:   return TokenType::TYPE_INT;    // Same suffix, unsigned tracked in TypeDescriptor
```

**`parseVariableName()`** - Updated to handle `@` and `^` suffixes:
- Added `@` and `^` to suffix detection check
- Added cases for `TYPE_BYTE` and `TYPE_SHORT` in suffix token check
- Maps `@` → `_BYTE` name mangling
- Maps `^` → `_SHORT` name mangling

#### Statement Parsing:
Updated all places that check for type keywords to include new types:
- `parseFunctionStatement()` - function name and parameter type checks
- `parseSubStatement()` - subroutine parameter type checks
- `parsePrimary()` - identifier/expression primary checks
- `isAssignment()` - lookahead for assignment detection

---

### 4. Semantic Analysis Updates (`fasterbasic_semantic.h`)

#### New Helper Functions:

**`tokenSuffixToDescriptor()`** - Converts token suffix to TypeDescriptor:
```cpp
inline TypeDescriptor tokenSuffixToDescriptor(TokenType suffix, bool isUnsigned = false) {
    switch (suffix) {
        case TokenType::TYPE_INT:
            return TypeDescriptor(isUnsigned ? BaseType::UINTEGER : BaseType::INTEGER);
        case TokenType::TYPE_BYTE:
            return TypeDescriptor(isUnsigned ? BaseType::UBYTE : BaseType::BYTE);
        case TokenType::TYPE_SHORT:
            return TypeDescriptor(isUnsigned ? BaseType::USHORT : BaseType::SHORT);
        // ... etc
    }
}
```

**`keywordToDescriptor()`** - Converts AS type keyword to TypeDescriptor:
```cpp
inline TypeDescriptor keywordToDescriptor(TokenType keyword) {
    switch (keyword) {
        case TokenType::KEYWORD_BYTE:    return TypeDescriptor(BaseType::BYTE);
        case TokenType::KEYWORD_SHORT:   return TypeDescriptor(BaseType::SHORT);
        case TokenType::KEYWORD_UBYTE:   return TypeDescriptor(BaseType::UBYTE);
        case TokenType::KEYWORD_USHORT:  return TypeDescriptor(BaseType::USHORT);
        case TokenType::KEYWORD_UINTEGER: return TypeDescriptor(BaseType::UINTEGER);
        case TokenType::KEYWORD_ULONG:   return TypeDescriptor(BaseType::ULONG);
        // ... etc
    }
}
```

These helpers bridge the gap between parser tokens and the type system, enabling full TypeDescriptor creation from syntax.

---

## Syntax Support

### Type Suffixes
- `@` - BYTE suffix (e.g., `counter@`, `value@`)
- `^` - SHORT suffix (e.g., `index^`, `temp^`)
- Existing: `%` (INTEGER), `&` (LONG), `!` (SINGLE), `#` (DOUBLE), `$` (STRING)

### AS Type Keywords
All of these can be used in `AS` declarations:
- `AS BYTE` / `AS UBYTE`
- `AS SHORT` / `AS USHORT`
- `AS INTEGER` / `AS UINTEGER` / `AS INT` / `AS UINT`
- `AS LONG` / `AS ULONG`
- `AS SINGLE` / `AS FLOAT`
- `AS DOUBLE`
- `AS STRING`

### Examples

```basic
REM Variable declarations with suffixes
DIM byteVal@
DIM shortVal^
DIM intVal%
DIM longVal&

REM Variable declarations with AS keyword
DIM explicitByte AS BYTE
DIM explicitShort AS SHORT
DIM explicitUnsigned AS UINTEGER

REM Function with typed parameters
FUNCTION AddBytes@(a@ AS BYTE, b@ AS BYTE) AS BYTE
    AddBytes@ = a@ + b@
END FUNCTION

REM Arrays with typed elements
DIM byteArray@(100)
DIM shortArray^(50)
DIM typedArray(10) AS UBYTE
```

---

## Design Notes

### Unsigned Type Handling
- Unsigned types (`UBYTE`, `USHORT`, `UINTEGER`, `ULONG`) use the same token suffixes as their signed counterparts
- The unsigned flag is tracked in the `TypeDescriptor` attributes (`TYPE_ATTR_UNSIGNED`)
- Parser's `asTypeToSuffix()` maps unsigned keywords to signed suffixes because the suffix alone doesn't distinguish signedness
- Full type information (signed/unsigned) is preserved in `TypeDescriptor` for semantic analysis and code generation

### Suffix vs. AS Declaration
Both forms are supported:
```basic
DIM x@ AS BYTE      REM Redundant but allowed (suffix takes precedence)
DIM y AS BYTE       REM AS clause required when no suffix
DIM z@              REM Suffix alone is sufficient
```

### Name Mangling
Variables with type suffixes are internally mangled:
- `counter@` → `counter_BYTE`
- `index^` → `index_SHORT`
- `value%` → `value_INT`
- `price#` → `price_DOUBLE`

This prevents naming conflicts and makes the internal type clear.

### Power Operator vs. SHORT Suffix
The `^` character serves dual purpose:
- **As operator:** `2 ^ 8` (power/exponentiation)
- **As suffix:** `value^` (SHORT type)

This is disambiguated by context:
- After an identifier → type suffix (handled in `scanIdentifierOrKeyword()`)
- Standalone between expressions → power operator (handled in `scanOperator()`)

---

## Compilation Status

All files compile successfully with only pre-existing warnings (unrelated to Phase 4 changes):
- `fasterbasic_token.h` - No new warnings
- `fasterbasic_lexer.cpp` - 1 pre-existing warning (incomplete switch in `tokenTypeToString`)
- `fasterbasic_parser.cpp` - 4 pre-existing warnings (incomplete switches in AST, unrelated)
- `fasterbasic_semantic.cpp` - 2 pre-existing warnings (incomplete switches, unrelated)

---

## Testing

A comprehensive test file (`phase4_test.bas`) has been created covering:
1. BYTE suffix (`@`) usage
2. SHORT suffix (`^`) usage
3. AS BYTE/SHORT/LONG declarations
4. Unsigned type declarations (UBYTE, USHORT, UINTEGER, ULONG)
5. Mixed suffix and AS clause usage
6. Functions with new type suffixes
7. Arrays with new types
8. Type coercion chains

---

## Next Steps (Phase 5)

Phase 5 will integrate the new type system into code generation:
1. Update QBE IR emission to use `TypeDescriptor::toQBEType()` and `TypeDescriptor::toQBEMemOp()`
2. Implement correct sign/zero extension for byte/short loads (sb/ub/sh/uh)
3. Generate array and string descriptors with proper type information
4. Implement loop index promotion to LOOP_INDEX (LONG) semantics
5. Add runtime conversion functions (CBYTE, CSHORT, etc.)
6. Ensure all codegen paths use TypeDescriptor instead of legacy VariableType

---

## Summary

Phase 4 successfully extends the FasterBASIC parser and lexer to support:
- ✅ New type suffixes: `@` (BYTE) and `^` (SHORT)
- ✅ New AS type keywords: BYTE, SHORT, UBYTE, USHORT, UINTEGER, ULONG
- ✅ Helper functions to convert tokens to TypeDescriptor
- ✅ Full integration with existing parser infrastructure
- ✅ Backward compatibility with existing code
- ✅ Clean compilation with no new errors or warnings

The compiler frontend is now fully equipped to parse and recognize the complete QBE-aligned type system. The next phase will complete the implementation by updating code generation to leverage this type information.