# ON GOTO/GOSUB vs SELECT CASE: Developer's Guide

**Quick Start:** When should you use ON GOTO/GOSUB vs SELECT CASE?

## TL;DR

```basic
' For simple numeric dispatch (1, 2, 3...):
ON menuChoice GOTO Option1, Option2, Option3    ' ✓ Fast, simple

' For complex conditions:
SELECT CASE menuChoice                          ' ✓ Flexible, readable
  CASE 1: PRINT "First"
  CASE 2 TO 5: PRINT "Range"
  CASE 10, 20, 30: PRINT "Multiple"
END SELECT
```

**Rule of thumb:** Use SELECT CASE unless you need the extra 20% speed in a hot loop.

---

## Feature Comparison

| Feature | ON GOTO/GOSUB | SELECT CASE | Winner |
|---------|---------------|-------------|--------|
| **Performance** | ⚡ Faster (20-30%) | 🐌 Slower | ON GOTO |
| **Readability** | 😐 Okay | 😊 Better | SELECT CASE |
| **Flexibility** | ⚠️ Limited | ✅ Full-featured | SELECT CASE |
| **Code size** | 📉 Smaller | 📈 Larger | ON GOTO |
| **Sequential dispatch** | ✅ Perfect | ✅ Works | Tie |
| **Range support** | ❌ No | ✅ Yes | SELECT CASE |
| **Multiple values/case** | ❌ No | ✅ Yes | SELECT CASE |
| **String matching** | ❌ No | ✅ Yes | SELECT CASE |
| **CASE ELSE** | ⚠️ Fallthrough | ✅ Explicit | SELECT CASE |

---

## When to Use ON GOTO

### ✅ Perfect For:

1. **Menu Systems**
```basic
PRINT "1) New Game"
PRINT "2) Load Game"
PRINT "3) Options"
PRINT "4) Quit"
INPUT choice
ON choice GOTO NewGame, LoadGame, Options, Quit
```

2. **State Machines**
```basic
' Game state dispatcher
ON gameState GOTO StateMenu, StatePlay, StatePause, StateGameOver
```

3. **Simple Numeric Dispatch (1, 2, 3...)**
```basic
' Execute command by index
ON cmdIndex GOTO Cmd1, Cmd2, Cmd3, Cmd4, Cmd5
```

4. **Performance-Critical Loops**
```basic
FOR i = 1 TO 1000000
  LET action = ComputeAction()
  ON action GOSUB Handler1, Handler2, Handler3
NEXT i
```

### ❌ Don't Use For:

- **Non-sequential values** (1, 5, 10, 100) - wastes "slots"
- **Complex conditions** - hard to express
- **String matching** - not supported
- **Range checks** (CASE 1 TO 10) - not supported

---

## When to Use SELECT CASE

### ✅ Perfect For:

1. **Range Checks**
```basic
SELECT CASE score
  CASE 90 TO 100: grade$ = "A"
  CASE 80 TO 89: grade$ = "B"
  CASE 70 TO 79: grade$ = "C"
  CASE ELSE: grade$ = "F"
END SELECT
```

2. **Multiple Values per Case**
```basic
SELECT CASE dayOfWeek
  CASE 1, 7: PRINT "Weekend!"
  CASE 2 TO 6: PRINT "Weekday"
END SELECT
```

3. **String Matching**
```basic
SELECT CASE command$
  CASE "QUIT", "EXIT": END
  CASE "HELP": ShowHelp
  CASE "LOAD": LoadFile
END SELECT
```

4. **Complex Conditions**
```basic
SELECT CASE temperature
  CASE IS < 0: PRINT "Freezing"
  CASE 0 TO 32: PRINT "Cold"
  CASE 32 TO 70: PRINT "Cool"
  CASE 70 TO 85: PRINT "Warm"
  CASE IS > 85: PRINT "Hot"
END SELECT
```

5. **Code Maintainability**
```basic
' Easy to add, remove, reorder cases
SELECT CASE userRole
  CASE "admin": ShowAdminPanel
  CASE "moderator": ShowModPanel
  CASE "user": ShowUserPanel
  CASE ELSE: ShowGuestPanel
END SELECT
```

### ❌ Don't Use For:

- **Ultra-tight loops** where every nanosecond counts (rare)
- **Simple sequential dispatch** where ON GOTO is clearer

---

## Performance Guide

### Small Switches (2-5 cases)

**Performance difference:** < 5 nanoseconds per dispatch

```basic
' Either is fine - choose for clarity
ON x GOTO A, B, C           ' If x is 1, 2, 3
SELECT CASE x               ' If x needs ranges or special handling
```

**Recommendation:** Use whichever is clearer. Performance is negligible.

### Medium Switches (6-15 cases)

**Performance difference:** ~10-20 nanoseconds per dispatch (~30% faster with ON GOTO)

```basic
' ON GOTO is measurably faster but rarely matters
FOR i = 1 TO 1000           ' 1000 iterations
  ON action GOSUB H1, H2, H3, H4, H5, H6, H7, H8
NEXT i
' vs SELECT CASE: +30% time (still only ~20 microseconds total)
```

**Recommendation:** Use SELECT CASE unless profiling shows this is a bottleneck.

### Large Switches (16+ cases)

**Performance difference:** Both are slow with current linear search

```basic
' Both implementations are O(N) - need optimization
ON x GOTO L1, L2, L3, ..., L50    ' ~50 comparisons worst case
SELECT CASE x                      ' ~50 comparisons worst case
  CASE 1: ...
  ' ... 50 cases
END SELECT
```

**Recommendation:** 
- Avoid if possible (refactor into table-driven design)
- If unavoidable, use ON GOTO (easier to optimize with jump tables later)
- Consider IF-ELSEIF chains with most common cases first

---

## Code Examples

### Example 1: Menu System (Use ON GOTO)

```basic
100 REM Main menu
110 CLS
120 PRINT "╔═══════════════════════════╗"
130 PRINT "║     MAIN MENU            ║"
140 PRINT "╠═══════════════════════════╣"
150 PRINT "║ 1) Start New Game        ║"
160 PRINT "║ 2) Continue Game         ║"
170 PRINT "║ 3) High Scores           ║"
180 PRINT "║ 4) Options               ║"
190 PRINT "║ 5) Exit                  ║"
200 PRINT "╚═══════════════════════════╝"
210 INPUT "Select option: ", choice
220 IF choice < 1 OR choice > 5 THEN GOTO 100
230 ON choice GOTO NewGame, Continue, HighScores, Options, ExitGame
```

**Why ON GOTO:** Sequential choices (1-5), simple dispatch, performance is good.

### Example 2: Grade Calculator (Use SELECT CASE)

```basic
100 INPUT "Enter score (0-100): ", score
110 SELECT CASE score
120   CASE 90 TO 100
130     grade$ = "A"
140     PRINT "Excellent!"
150   CASE 80 TO 89
160     grade$ = "B"
170     PRINT "Good job!"
180   CASE 70 TO 79
190     grade$ = "C"
200     PRINT "Satisfactory"
210   CASE 60 TO 69
220     grade$ = "D"
230     PRINT "Needs improvement"
240   CASE 0 TO 59
250     grade$ = "F"
260     PRINT "Failed"
270   CASE ELSE
280     PRINT "Invalid score!"
290     GOTO 100
300 END SELECT
310 PRINT "Grade: "; grade$
```

**Why SELECT CASE:** Ranges are easy, clear intent, CASE ELSE for validation.

### Example 3: Command Parser (Use SELECT CASE)

```basic
100 INPUT "Command: ", cmd$
110 LET cmd$ = UCASE$(cmd$)
120 SELECT CASE cmd$
130   CASE "QUIT", "EXIT", "Q"
140     PRINT "Goodbye!"
150     END
160   CASE "HELP", "?"
170     GOSUB ShowHelp
180   CASE "LOAD"
190     INPUT "Filename: ", file$
200     GOSUB LoadFile
210   CASE "SAVE"
220     INPUT "Filename: ", file$
230     GOSUB SaveFile
240   CASE "LIST", "DIR"
250     GOSUB ListFiles
260   CASE ELSE
270     PRINT "Unknown command. Type HELP for help."
280 END SELECT
290 GOTO 100
```

**Why SELECT CASE:** String matching, multiple alternatives per case, extensible.

### Example 4: State Machine (Use ON GOTO)

```basic
100 REM Game state machine
110 LET STATE_MENU = 1
120 LET STATE_PLAY = 2
130 LET STATE_PAUSE = 3
140 LET STATE_GAMEOVER = 4
150 LET currentState = STATE_MENU
160
170 REM Main game loop
180 ON currentState GOTO StateMenu, StatePlay, StatePause, StateGameOver
190
200 StateMenu:
210   GOSUB DrawMenu
220   GOSUB HandleMenuInput
230   GOTO 180
240
250 StatePlay:
260   GOSUB UpdateGame
270   GOSUB DrawGame
280   GOSUB HandleGameInput
290   GOTO 180
300
310 StatePause:
320   GOSUB DrawPauseScreen
330   GOSUB HandlePauseInput
340   GOTO 180
350
360 StateGameOver:
370   GOSUB ShowGameOver
380   GOSUB WaitForRestart
390   GOTO 180
```

**Why ON GOTO:** Fast state dispatch, clear state transitions, tight loop.

---

## Performance Tuning Tips

### Tip 1: Put Common Cases First (SELECT CASE only)

```basic
' BAD: Rare case first
SELECT CASE errorCode
  CASE 999: PRINT "Rare error"
  CASE 1: PRINT "Common error"     ' Checked second!
  ...
END SELECT

' GOOD: Common case first
SELECT CASE errorCode
  CASE 1: PRINT "Common error"     ' Fast path
  CASE 999: PRINT "Rare error"
  ...
END SELECT
```

**Note:** Doesn't apply to ON GOTO (fixed order by index).

### Tip 2: Use ON GOTO for Hot Loops

```basic
' SLOW: SELECT CASE in tight loop
FOR i = 1 TO 1000000
  LET opcode = GetNextOpcode()
  SELECT CASE opcode
    CASE 1: GOSUB OpAdd
    CASE 2: GOSUB OpSub
    ' ... 20 cases
  END SELECT
NEXT i

' FAST: ON GOSUB in tight loop (~30% faster)
FOR i = 1 TO 1000000
  LET opcode = GetNextOpcode()
  ON opcode GOSUB OpAdd, OpSub, OpMul, ' ... 20 handlers
NEXT i
```

### Tip 3: Cache Computed Selectors

```basic
' BAD: Recomputes every case check
SELECT CASE ComputeIndex(x, y, z)
  CASE 1: ...
  CASE 2: ...
END SELECT

' GOOD: Compute once
LET idx = ComputeIndex(x, y, z)
SELECT CASE idx
  CASE 1: ...
  CASE 2: ...
END SELECT
```

---

## Migration Guide

### Converting SELECT CASE to ON GOTO

**Before:**
```basic
SELECT CASE choice
  CASE 1: GOSUB Option1
  CASE 2: GOSUB Option2
  CASE 3: GOSUB Option3
END SELECT
```

**After:**
```basic
IF choice < 1 OR choice > 3 THEN GOTO InvalidChoice
ON choice GOSUB Option1, Option2, Option3
InvalidChoice:
  PRINT "Invalid choice!"
```

**Watch out for:**
- Need explicit bounds checking (ON x falls through if out of range)
- No CASE ELSE equivalent (must handle explicitly)
- Cases must be sequential (1, 2, 3..., not 1, 5, 10)

### Converting ON GOTO to SELECT CASE

**Before:**
```basic
ON choice GOTO Menu1, Menu2, Menu3
Menu1: PRINT "First": GOTO MenuDone
Menu2: PRINT "Second": GOTO MenuDone
Menu3: PRINT "Third": GOTO MenuDone
MenuDone:
```

**After:**
```basic
SELECT CASE choice
  CASE 1: PRINT "First"
  CASE 2: PRINT "Second"
  CASE 3: PRINT "Third"
END SELECT
```

**Benefits:**
- No need for labels and GOTO chains
- Automatic fallthrough to next statement
- Easy to add CASE ELSE

---

## Best Practices

### ✅ DO:

1. **Use SELECT CASE as default choice** - it's more maintainable
2. **Use ON GOTO for simple menus** (1, 2, 3, 4 style)
3. **Profile before optimizing** - measure actual impact
4. **Add bounds checking** for ON GOTO/GOSUB
5. **Document why you chose** ON GOTO if performance-critical

### ❌ DON'T:

1. **Don't use ON GOTO for sparse values** (1, 10, 100, 1000)
2. **Don't optimize prematurely** - readability first
3. **Don't mix ON GOTO and complex logic** - hard to follow
4. **Don't forget fallthrough cases** - ON x falls through if out of range
5. **Don't sacrifice clarity for 10 nanoseconds**

---

## Summary

| Your Situation | Recommendation |
|----------------|----------------|
| **Default choice** | SELECT CASE (more flexible) |
| **Menu system (1, 2, 3...)** | ON GOTO (simpler) |
| **State machine** | ON GOTO (faster dispatch) |
| **Range checks** | SELECT CASE (only option) |
| **String matching** | SELECT CASE (only option) |
| **Hot loop with many cases** | ON GOSUB (20-30% faster) |
| **Code readability matters** | SELECT CASE (clearer) |
| **Adding cases frequently** | SELECT CASE (easier to modify) |
| **Every nanosecond counts** | ON GOTO (but measure first!) |
| **Complex conditions** | SELECT CASE (more expressive) |

**Final advice:** Start with SELECT CASE. If profiling shows it's a bottleneck AND you have sequential numeric cases, consider ON GOTO. Otherwise, don't worry about it.

---

**Last Updated:** February 4, 2025  
**FasterBASIC Compiler:** v2 (CFG-aware codegen)