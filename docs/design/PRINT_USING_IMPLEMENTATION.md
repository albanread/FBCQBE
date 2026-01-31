# PRINT USING Implementation

## Status: ✅ IMPLEMENTED

The PRINT USING command has been successfully implemented with basic placeholder substitution.

## Syntax

```basic
PRINT USING "format string"; value1; value2; ...
```

## Current Implementation

### Features

1. **Placeholder Characters**
   - `@` - String/numeric placeholder
   - `#` - String/numeric placeholder
   - Consecutive placeholders (`@@@@@` or `#####`) are treated as a single field

2. **Type Conversion**
   - Numeric values (INT, DOUBLE) are automatically converted to strings
   - String variables are passed through directly

3. **Multiple Arguments**
   - Supports multiple semicolon-separated values
   - Each placeholder group consumes one argument

### Examples

```basic
DIM name$ AS STRING
name$ = "Alice"
PRINT USING "Name: @@@@@"; name$
' Output: Name: Alice

PRINT USING "Value: #"; 42
' Output: Value: 42

PRINT USING "Pi = #"; 3.14159
' Output: Pi = 3.14159

PRINT USING "@@@@@ @@@@@"; "John"; "Smith"
' Output: John Smith

PRINT USING "Name: @@@@, Age: #"; name$; 25
' Output: Name: Alice, Age: 25
```

## Implementation Details

### Architecture

**Parser Support** (Already existed):
- `PrintStatement::hasUsing` - Flag for PRINT USING
- `PrintStatement::formatExpr` - Format string expression
- `PrintStatement::usingValues` - Vector of argument expressions

**Runtime Function**:
- `basic_print_using(format, count, args)` in `io_ops_format.c`
- Takes format StringDescriptor, argument count, and array of StringDescriptor pointers
- **Note**: Uses array-based arguments instead of varargs to avoid ARM64 ABI issues

**Code Generation**:
- Evaluates format string and arguments
- Converts non-string types using `string_from_int`/`string_from_double`
- Builds stack-allocated array of StringDescriptor pointers
- Calls `basic_print_using` with array
- Releases temporary string descriptors after call

### Key Technical Decisions

1. **Array-Based Arguments**: Changed from variadic function to array-based to avoid ARM64/QBE ABI complications with varargs.

2. **Simplified Formatting**: The current implementation treats all placeholder runs as simple substitution points. It doesn't implement:
   - Field width padding
   - Decimal point alignment
   - Thousands separators
   - Currency symbols (`$$`)
   - Scientific notation (`^^^^`)
   - Sign forcing (`+`)
   - String truncation/padding (`!`, `\ \`, `&`)

3. **UTF-8 Extraction**: The runtime immediately converts all StringDescriptor arguments to UTF-8 strings and copies them, protecting against descriptor releases.

## Future Enhancements

To implement full BASIC PRINT USING formatting:

1. **Numeric Field Widths**
   - Count `#` characters to determine field width
   - Right-align numbers within field
   - Pad with spaces

2. **Decimal Alignment**
   - Parse `###.##` patterns
   - Format numbers with fixed decimal places
   - Align decimal points vertically in tables

3. **String Formatting**
   - `!` - First character only
   - `\ n spaces \` - Fixed width (n+2 characters)
   - `&` - Variable length (full string)

4. **Special Formatting**
   - `$$` - Floating dollar sign
   - `,` - Thousands separator
   - `+` - Force sign display
   - `^^^^` - Scientific notation

## Files Modified

- `FasterBASICT/runtime_c/basic_runtime.h` - Function declaration
- `FasterBASICT/runtime_c/io_ops_format.c` - Runtime implementation (NEW FILE)
- `FasterBASICT/src/codegen/qbe_codegen_statements.cpp` - Codegen for PRINT USING
- `FasterBASICT/src/codegen/qbe_codegen_runtime.cpp` - Fixed string conversion functions
- `FasterBASICT/runtime_c/string_utf32.c` - Added `string_from_int`/`string_from_double` declarations
- `run_basic.sh` - Added `io_ops_format.c` to link step

## Testing

Test files:
- `test_print_using.bas` - Basic functionality
- `test_print_using_full.bas` - Comprehensive tests

All tests pass successfully.
