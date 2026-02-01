# CFG Recovery and Redesign Plan

**Date:** February 1, 2026  
**Status:** RECOVERY MODE - Learning from setback  
**Incident:** Hours of CFG v2 work may have been lost due to accidental file overwrite

---

## What Happened

1. We created a new CFG v2 implementation in `fsh/FasterBASICT/src/cfg_v2/`
2. The new code was NEVER integrated into the build system
3. The old CFG code in `fsh/FasterBASICT/src/fasterbasic_cfg.cpp` remained in use
4. At some point, `fasterbasic_cfg.cpp` may have been overwritten/restored from backup
5. We discovered we have been working on isolated code that was never compiled or tested

## Current State

### Backed Up
- ✅ `archive/cfg_v2_backup_20260201_120212/cfg_v2/` - All cfg_v2 work preserved
- ✅ `fsh/FasterBASICT/src/oldcfg/` - Original old CFG code
- ✅ Git commit `4d27847` - Full project state preserved

### Active Files
- `fsh/FasterBASICT/src/fasterbasic_cfg.cpp` - OLD CODE (in build, being compiled)
- `fsh/FasterBASICT/src/fasterbasic_cfg.h` - OLD CODE (in build, being compiled)
- `fsh/FasterBASICT/src/cfg_v2/*` - NEW CODE (not in build, never tested)

### Build System
- `qbe_basic_integrated/build_qbe_basic.sh` compiles the OLD cfg code only
- cfg_v2 has NEVER been compiled or integrated

---

## Lessons Learned

1. **Never work on isolated code for extended periods without integration testing**
2. **Commit frequently when making structural changes**
3. **Test builds early and often - if it doesn't compile, it doesn't count**
4. **Make build integration part of the initial implementation, not an afterthought**
5. **Be clear about what code is "active" vs "experimental"**

---

## Recovery Strategy

### Phase 1: Assessment (CURRENT)
- [x] Back up all cfg_v2 work to archive
- [x] Identify what's salvageable from cfg_v2
- [ ] Review cfg_v2 design docs and extract good ideas
- [ ] Review cfg_v2 code for reusable patterns

### Phase 2: Design (NEXT)
- [ ] Create CFG Modular Design document
- [ ] Plan logical file split (not monolithic)
- [ ] Design incremental migration path
- [ ] Plan build integration from day 1

### Phase 3: Implementation
- [ ] Split OLD cfg code into logical modules FIRST
- [ ] Verify build works after each split
- [ ] Commit after each successful split
- [ ] Then apply new design patterns incrementally

### Phase 4: Testing
- [ ] Test after EVERY change
- [ ] Run nested loop test suite
- [ ] Compare outputs before/after each change
- [ ] Maintain backward compatibility during migration

---

## Salvageable Ideas from CFG v2

### Good Design Concepts (from cfg_v2 headers/docs)
1. **Single-pass recursive construction** - eliminate two-phase build
2. **Context passing** - LoopContext, SelectContext, TryContext structs
3. **Explicit edge wiring** - no implicit fallthrough assumptions
4. **Jump target pre-scan** - Phase 0 to identify landing zones
5. **Deferred edge resolution** - handle forward GOTO/GOSUB references
6. **Black-box abstraction** - builders return entry/exit blocks only

### Code Patterns to Reuse
- Context structures for nested control flow
- Jump target collection logic
- Deferred edge tracking
- CFG dump/debug functionality
- Block and edge management helpers

### What NOT to Repeat
- Working in isolation without build integration
- Large monolithic files (split from the start)
- Incomplete error handling
- Missing the `</parameter>` corruption bugs (review process failure)

---

## New Modular CFG Design Plan

### File Structure (Proposed)

```
fsh/FasterBASICT/src/cfg/
├── cfg_builder.h              # Main public API and class declaration
├── cfg_builder_core.cpp       # Constructor, main entry points
├── cfg_builder_blocks.cpp     # Block/edge creation and management
├── cfg_builder_context.cpp    # Context structures and helpers
├── cfg_builder_statements.cpp # Statement range builder (core loop)
├── cfg_builder_conditional.cpp # IF/ELSEIF/ELSE, SELECT CASE
├── cfg_builder_loops.cpp      # WHILE, FOR, REPEAT, DO
├── cfg_builder_jumps.cpp      # GOTO, GOSUB, RETURN, ON GOTO/GOSUB
├── cfg_builder_exception.cpp  # TRY/CATCH/FINALLY, THROW
├── cfg_builder_subroutine.cpp # SUB/FUNCTION CFG building
└── cfg_builder_dump.cpp       # Debug dump and visualization
```

### Build Integration First

```bash
# In build_qbe_basic.sh, compile ALL cfg files:
clang++ -std=c++17 -O2 -I"$FASTERBASIC_SRC" -c \
    "$FASTERBASIC_SRC/cfg/cfg_builder_core.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_blocks.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_context.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_statements.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_conditional.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_loops.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_jumps.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_exception.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_subroutine.cpp" \
    "$FASTERBASIC_SRC/cfg/cfg_builder_dump.cpp"
```

### Migration Path (Incremental)

**Step 1: Split without changing behavior**
- Take OLD working cfg code
- Split into multiple files (same logic)
- Update build to compile all files
- TEST - ensure no behavior changes
- COMMIT immediately

**Step 2: Add context structures**
- Add LoopContext, SelectContext, TryContext
- Refactor to pass contexts instead of using stacks
- TEST after each structure added
- COMMIT after each successful change

**Step 3: Eliminate two-phase build**
- Convert to single-pass recursive
- Wire edges immediately in builders
- TEST extensively
- COMMIT

**Step 4: Add jump target pre-scan**
- Implement Phase 0 jump collection
- Create landing zones at target lines
- TEST
- COMMIT

**Step 5: Fix nested loop bugs**
- Use new architecture to eliminate fallthrough assumptions
- TEST with nested control flow suite
- COMMIT

---

## Implementation Checklist

### Before Starting
- [ ] Review all salvaged cfg_v2 code
- [ ] Extract design patterns to reuse
- [ ] Create modular design document
- [ ] Plan file split structure
- [ ] Update build script template

### For Each Module
- [ ] Write the code
- [ ] Update build script
- [ ] Compile successfully
- [ ] Run basic smoke test
- [ ] Run full test suite
- [ ] Commit with clear message
- [ ] Update progress document

### Testing Strategy
- [ ] Maintain test_programs/ directory
- [ ] Run after EVERY change
- [ ] Compare CFG dumps before/after
- [ ] Test nested control flows
- [ ] Test GOTO/GOSUB edge cases
- [ ] Test exception handling

---

## Recovery Timeline

### Immediate (Today)
1. Review backed-up cfg_v2 code
2. Extract salvageable design patterns
3. Create modular design document
4. Plan first split (core + blocks)

### Next Session
1. Split old CFG into cfg_builder_core.cpp + cfg_builder_blocks.cpp
2. Update build script
3. Compile and test
4. Commit immediately
5. Continue with next module

### This Week
1. Complete file split (all modules)
2. Add context structures
3. Begin single-pass conversion
4. Test with nested loop suite

---

## Success Criteria

✅ **Every change compiles**  
✅ **Every change is tested**  
✅ **Every change is committed**  
✅ **All tests pass after each commit**  
✅ **Code is modular and maintainable**  
✅ **Nested loops work correctly**  
✅ **No regression in existing functionality**

---

## Reference Materials

### Backed Up Code
- `archive/cfg_v2_backup_20260201_120212/cfg_v2/`
- `fsh/FasterBASICT/src/cfg_v2/README.md` - Design rationale
- `fsh/FasterBASICT/src/cfg_v2/cfg_builder_v2.h` - API design

### Documentation
- `docs/CFG_REFACTORING_PLAN.md` - Original v2 plan
- `docs/CFG_V2_PROGRESS.md` - What we attempted
- `CFG_V2_STATUS.md` - Status before incident

### Test Suite
- `tests/loops/test_nested_*.bas` - Nested control flow tests
- `test_programs/test_cfg_v2_simple.bas` - Simple CFG test

---

## Notes

- **No blame** - accidents happen, we learn and move forward
- **Stay positive** - we have good designs and patterns to reuse
- **Be systematic** - test and commit frequently
- **Stay focused** - one module at a time
- **Build confidence** - every successful compile/test is progress

---

## Next Steps

1. **NOW:** Review backed-up cfg_v2 code and extract patterns
2. **NEXT:** Create detailed modular design document  
3. **THEN:** Begin splitting old CFG with build integration
4. **FINALLY:** Apply new patterns incrementally with tests

---

*"In software engineering, recovery is a skill. We have the designs, we have the backups, we have the knowledge. Let's rebuild it right this time."*