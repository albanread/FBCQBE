# Numerics and Math Support

This project exposes a broad set of math, statistical, and finance helpers through the FasterBASIC runtime. All functions operate on doubles unless noted. Domain errors emit a runtime error message and return 0.0.

## Core Math
- Trig: SIN, COS, TAN, ATAN/ATN, ATAN2, ASIN/ASN, ACOS/ACS
- Hyperbolic: SINH, COSH, TANH, ASINH, ACOSH, ATANH
- Exponentials/Logs: EXP, EXP2, EXPM1, LOG/LN, LOG10, LOG1P, POW, SQRT/SQR, CBRT, HYPOT
- Remainders: FMOD, REMAINDER
- Rounding: FLOOR, CEIL, TRUNC, ROUND, COPYSIGN
- Absolute: ABS
- Angle conversion: DEG (rad→deg), RAD (deg→rad)
- Linear interpolation: LERP(a, b, t)

## Probability and Statistics
- Logistic: SIGMOID(x), LOGIT(p)
- Normal distribution: NORMPDF(z), NORMCDF(z)
- Error functions: ERF(x), ERFC(x)
- Gamma: TGAMMA(x), LGAMMA(x)

## Combinatorics
- FACT / FACTORIAL(n) via gamma
- COMB(n, k) combinations
- PERM(n, k) permutations

## Utilities
- CLAMP(x, min, max)
- NEXTAFTER(x, y) next representable value toward y
- FMAX/FMIN(a, b)
- FMA(x, y, z) fused multiply-add

## Finance Helpers
- PMT(rate, nper, pv): payment for a loan/annuity
- PV(rate, nper, pmt): present value given payment
- FV(rate, nper, pmt): future value given payment

## Randomness
- RND(): uniform [0,1]
- RND_INT(min, max), RAND(n): integer variants
- RANDOMIZE(seed): seed control

## Integer/Sign Helpers
- INT (floor), FIX (truncate), CINT (round-to-int), SGN (sign)

## Notes
- All functions are mapped through the codegen to the C runtime in `runtime_c/math_ops.c`.
- Aliases like LN, ATN, SQR, ASN, ACS are supported where noted.
- Most functions return double; string functions return StringDescriptor*; LEN/INSTR/ASC return integer types.

## Usage Examples

```basic
' Trig and angles
PRINT "deg(pi) = "; DEG(3.141592653589793)
PRINT "rad(180) = "; RAD(180)
PRINT "lerp 0→10 @0.25 = "; LERP(0, 10, 0.25)

' Logs/exp and power
PRINT "log1p(0.5) = "; LOG1P(0.5)
PRINT "exp2(3) = "; EXP2(3)
PRINT "pow(9, 0.5) = "; POW(9, 0.5)

' Normal distribution
PRINT "normpdf(0) = "; NORMPDF(0)
PRINT "normcdf(0) = "; NORMCDF(0)

' Combinatorics
PRINT "fact(5) = "; FACT(5)
PRINT "comb(5, 2) = "; COMB(5, 2)
PRINT "perm(5, 2) = "; PERM(5, 2)

' Rounding and remainder
PRINT "floor(-2.7) = "; FLOOR(-2.7)
PRINT "fmod(5.5, 2) = "; FMOD(5.5, 2)

' Clamp
PRINT "clamp(10, 0, 5) = "; CLAMP(10, 0, 5)

' Finance
DIM pmt AS DOUBLE
pmt = PMT(0.05, 12, 1000)
PRINT "pmt = "; pmt
PRINT "pv = "; PV(0.05, 12, -pmt)
PRINT "fv = "; FV(0.05, 12, -pmt)
```
