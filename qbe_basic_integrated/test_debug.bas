10 REM Minimal test for array of UDT
20 TYPE Point
30   X AS INTEGER
40 END TYPE
50 DIM Points(2) AS Point
60 Points(0).X = 42
70 PRINT Points(0).X
80 END
