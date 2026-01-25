PRINT "=== Advanced Numeric Formatting ==="

' Basic integer with padding
PRINT USING "Value: #####"; 42

' Decimal formatting
PRINT USING "Amount: ###.##"; 123.456

' Comma separators
PRINT USING "Big number: ###,###,###"; 1234567

' Dollar sign
PRINT USING "Price: $$###.##"; 99.95

' Sign handling
PRINT USING "Positive: +###.##"; 42.5
PRINT USING "Negative: +###.##"; -42.5

' Exponential notation
PRINT USING "Scientific: #.##^^^^"; 12345.67

' String placeholder still works
PRINT USING "Name: @"; "Alice"
