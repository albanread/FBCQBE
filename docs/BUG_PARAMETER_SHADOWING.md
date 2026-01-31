# Bug: Function Parameters Shadowed by Global Variables

## Status
**IDENTIFIED** - Not yet fixed

## Severity
**MEDIUM** - Causes incorrect code generation when function parameters have the same name as global variables

## Description

When a function parameter has the same name as a global variable, the compiler incorrectly uses the global variable inside the function instead of the parameter. This causes QBE IL type errors and incorrect behavior.

## Example

```basic
DIM m AS INTEGER
DIM n AS INTEGER

FOR m = 0 TO 3
    FOR n = 0 TO 3
        result = TestFunc(m, n)
        PRINT result
    NEXT n
NEXT m

FUNCTION TestFunc(m AS INTEGER, n AS INTEGER) AS INTEGER
    IF m = 0 THEN           ' ERROR: Uses global m, not parameter m
        TestFunc = n + 1
    ELSE
        TestFunc = m + n
    END IF
END FUNCTION
```

### Error Output
```
qbe:test.bas:192: invalid type for first operand %var_m in ceql
```

## Root Cause

In `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp`, the `getVariableRef()` function checks for global variables **before** checking for function parameters:

```cpp
std::string QBECodeGenerator::getVariableRef(const std::string& varName) {
    // Check if this is a GLOBAL variable first  <-- WRONG ORDER
    if (m_symbols) {
        auto it = m_symbols->variables.find(varName);
        if (it != m_symbols->variables.end() && it->second.isGlobal) {
            // ... returns global variable reference
            return cache;  // Returns global, not parameter!
        }
    }
    
    // ... much later ...
    
    // Check if we're in a function and if this is a parameter
    if (m_inFunction && m_cfg) {
        for (const auto& param : m_cfg->parameters) {
            if (param == varName ...) {
                return "%" + param;  // Never reached if global exists
            }
        }
    }
}
```

The global lookup happens at line 467-489, while the parameter check doesn't happen until lines 540-559. This means globals always win.

## Workaround

Use different names for function parameters than global variables:

```basic
DIM m AS INTEGER
DIM n AS INTEGER

' ... globals used here ...

FUNCTION TestFunc(mm AS INTEGER, nn AS INTEGER) AS INTEGER
    ' mm and nn work correctly because they don't shadow globals
    IF mm = 0 THEN
        TestFunc = nn + 1
    ELSE
        TestFunc = mm + nn
    END IF
END FUNCTION
```

## Fix Required

Reorder the checks in `getVariableRef()` to follow proper scoping rules:

1. **First**: Check if in a function and variable is a parameter → use parameter
2. **Second**: Check if in a function and variable is declared LOCAL → use local
3. **Third**: Check if variable is SHARED or global → use global
4. **Last**: Default behavior

The correct precedence should be:
- Most local (parameters, then locals)
- Then shared/global
- Then defaults

## Files Affected

- `fsh/FasterBASICT/src/codegen/qbe_codegen_helpers.cpp` - `getVariableRef()` function (line ~464)

## Test Cases

### Failing Test (Before Fix)
```basic
DIM m AS INTEGER, n AS INTEGER
m = 5: n = 10

FUNCTION Test(m AS INTEGER, n AS INTEGER) AS INTEGER
    Test = m + n
END FUNCTION

PRINT Test(1, 2)  ' Should print 3, but fails with IL error
```

### Passing Test (After Fix)
Same code should compile and print `3`.

### Edge Cases to Test
1. Parameter shadowing global of same type
2. Parameter shadowing global of different type (should catch type mismatch)
3. Multiple levels: parameter shadows LOCAL shadows global
4. Parameter with type suffix (m%) shadowing global without suffix (m)

## Impact

- **Current**: Must avoid using common variable names like `i`, `j`, `m`, `n` as parameters if they're also used as globals
- **After fix**: Normal scoping behavior - parameters shadow globals as expected in most programming languages

## Related Issues

None - this is a newly discovered bug during Ackermann function implementation.

## References

- Discovered during Rosetta Code implementation: `tests/rosetta/ackermann.bas`
- Commit: fe4904f - "Add Ackermann function Rosetta Code challenge"
- Original failing code used `m` and `n` as both globals and parameters
- Workaround: renamed parameters to `mm` and `nn`
