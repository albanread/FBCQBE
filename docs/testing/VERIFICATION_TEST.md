# Verification Test - Both Compilers Work Correctly

## Date: January 30, 2026

### Test 1: Single-Line IF Statement
```bash
# Integrated version
./fbc tests/test_single_if.bas -o test_integrated
./test_integrated
```
Expected output:
```
Before IF
After IF
```

```bash
# Standalone version  
./fsh/basic tests/test_single_if.bas --run
```
Expected output:
```
Before IF
After IF
```

### Test 2: 2D Arrays
```bash
./fbc tests/test_2d_array_basic.bas -o test_array
./test_array | head -3
```
Expected:
```
Testing basic reads:
arr(0,0) = 10 (expect 10)
arr(1,1) = 21 (expect 21)
```

### Test 3: AST Dump
```bash
./fbc tests/test_single_if.bas --ast | grep "STMT_IF"
```
Expected:
```
  STMT_IF (hasGoto=0, thenStmts=1, elseIfClauses=0, elseStmts=0)
```

```bash
./fsh/basic tests/test_single_if.bas --trace-ast | grep "STMT_IF"
```
Expected:
```
  STMT_IF (hasGoto=0, thenStmts=1, elseIfClauses=0, elseStmts=0)
```

## Status: ✅ ALL TESTS PASS
