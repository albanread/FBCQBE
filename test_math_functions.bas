REM Test Math Functions
REM This program tests various mathematical functions

PRINT "Testing Math Functions"
PRINT

REM Test trigonometric functions
PRINT "SIN(0) = "; SIN(0)
PRINT "COS(0) = "; COS(0)
PRINT "TAN(0) = "; TAN(0)
PRINT "ATAN(1) = "; ATAN(1)
PRINT "ATAN2(1, 1) = "; ATAN2(1, 1)
PRINT

REM Additional trigonometric coverage
PRINT "SIN(pi/6) = "; SIN(3.141592653589793 / 6)
PRINT "COS(pi/3) = "; COS(3.141592653589793 / 3)
PRINT "TAN(pi/4) = "; TAN(3.141592653589793 / 4)
PRINT "ATAN2(-1, -1) = "; ATAN2(-1, -1)
PRINT "ASIN(1) = "; ASIN(1)
PRINT "ACOS(1) = "; ACOS(1)
PRINT

REM Test exponential and logarithmic functions
PRINT "EXP(1) = "; EXP(1)
PRINT "LOG(2.718281828) approx "; LOG(2.718281828)
PRINT

REM Additional exponential/logarithmic coverage
PRINT "EXP(0) = "; EXP(0)
PRINT "LOG(1) = "; LOG(1)
PRINT "EXP(LOG(5)) = "; EXP(LOG(5))
PRINT "LOG10(1000) = "; LOG10(1000)
PRINT "LOG1P(0.5) = "; LOG1P(0.5)
PRINT "EXP2(3) = "; EXP2(3)
PRINT "EXPM1(1) = "; EXPM1(1)
PRINT

REM Test power and square root
PRINT "SQRT(4) = "; SQRT(4)
PRINT "POW(2, 3) = "; POW(2, 3)
PRINT

REM Additional power and root coverage
PRINT "SQRT(0) = "; SQRT(0)
PRINT "POW(9, 0.5) = "; POW(9, 0.5)
PRINT "POW(2, -2) = "; POW(2, -2)
PRINT "CBRT(27) = "; CBRT(27)
PRINT "HYPOT(3, 4) = "; HYPOT(3, 4)
PRINT

REM Test absolute value
PRINT "ABS(-5) = "; ABS(-5)
PRINT "ABS(5) = "; ABS(5)
PRINT "ABS(-5.5) = "; ABS(-5.5)
PRINT "ABS(0) = "; ABS(0)
PRINT

REM Hyperbolic functions
PRINT "SINH(1) = "; SINH(1)
PRINT "COSH(1) = "; COSH(1)
PRINT "TANH(1) = "; TANH(1)
PRINT "ASINH(1) = "; ASINH(1)
PRINT "ACOSH(1) = "; ACOSH(1)
PRINT "ATANH(0.5) = "; ATANH(0.5)
PRINT

REM Remainder and modulo helpers
PRINT "FMOD(5.5, 2) = "; FMOD(5.5, 2)
PRINT "REMAINDER(5.5, 2) = "; REMAINDER(5.5, 2)
PRINT

REM Rounding helpers
PRINT "FLOOR(-2.7) = "; FLOOR(-2.7)
PRINT "CEIL(-2.7) = "; CEIL(-2.7)
PRINT "TRUNC(-2.7) = "; TRUNC(-2.7)
PRINT "ROUND(-2.7) = "; ROUND(-2.7)
PRINT "COPYSIGN(2.5, -1) = "; COPYSIGN(2.5, -1)
PRINT

REM Error/Gamma functions and nextafter
PRINT "ERF(1) = "; ERF(1)
PRINT "ERFC(1) = "; ERFC(1)
PRINT "TGAMMA(5) = "; TGAMMA(5)
PRINT "LGAMMA(5) = "; LGAMMA(5)
PRINT "NEXTAFTER(1, 2) = "; NEXTAFTER(1, 2)
PRINT

REM Min/Max/FMA
PRINT "FMAX(1, 2) = "; FMAX(1, 2)
PRINT "FMIN(1, 2) = "; FMIN(1, 2)
PRINT "FMA(2, 3, 4) = "; FMA(2, 3, 4)
PRINT

REM Angle conversions and logistic/normal helpers
PRINT "DEG(pi) = "; DEG(3.141592653589793)
PRINT "RAD(180) = "; RAD(180)
PRINT "SIGMOID(0) = "; SIGMOID(0)
PRINT "LOGIT(0.5) = "; LOGIT(0.5)
PRINT "NORMPDF(0) = "; NORMPDF(0)
PRINT "NORMCDF(0) = "; NORMCDF(0)
PRINT

REM Factorial / combinatorics / clamp
PRINT "FACT(5) = "; FACT(5)
PRINT "COMB(5, 2) = "; COMB(5, 2)
PRINT "PERM(5, 2) = "; PERM(5, 2)
PRINT "CLAMP(10, 0, 5) = "; CLAMP(10, 0, 5)
PRINT "CLAMP(-1, 0, 5) = "; CLAMP(-1, 0, 5)
PRINT "LERP(0, 10, 0.25) = "; LERP(0, 10, 0.25)
PRINT

REM Finance helpers
PRINT "PMT(0.05, 12, 1000) = "; PMT(0.05, 12, 1000)
PRINT "PV(0.05, 12, -PMT(0.05, 12, 1000)) = "; PV(0.05, 12, -PMT(0.05, 12, 1000))
PRINT "FV(0.05, 12, -PMT(0.05, 12, 1000)) = "; FV(0.05, 12, -PMT(0.05, 12, 1000))
PRINT

PRINT "Math function tests completed."