' Basic SPLIT$/JOIN$ integration test
' Expect: a-b-c
PRINT JOIN$(SPLIT$("a,b,c", ","), "-")

' Expect: single element when delimiter empty
PRINT JOIN$(SPLIT$("solo", ""), ";")

' Expect: handles leading/trailing delimiters
PRINT JOIN$(SPLIT$(",x,,y,", ","), "+")

' Expect: empty input -> empty element
PRINT JOIN$(SPLIT$("", ","), ":")
