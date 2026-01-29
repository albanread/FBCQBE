# Exception Handling Implementation - Decision Checklist

## Overview

Before implementing TRY/CATCH/FINALLY exception handling, we need to make several design decisions. This document captures those decisions and their rationale.

## Critical Decisions

### 1. Implementation Approach

**Options:**
- **A) Full TRY/CATCH/FINALLY** - Modern structured exception handling
- **B) ON ERROR GOTO first** - Classic BASIC error handling, then upgrade to TRY/CATCH
- **C) Hybrid** - Both mechanisms supported

**Recommendation:** **Option A (Full TRY/CATCH/FINALLY)**

**Rationale:**
- More structured and maintainable
- Better fits modern programming practices
- ON ERROR GOTO can be added later if needed for legacy code compatibility
- Clean implementation without backward compatibility baggage

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 2. Error Object Design

**Options:**
- **A) Simple structure** - Just CODE, MESSAGE, LINE
- **B) Rich object** - Add DESCRIPTION, SOURCE, STACK_TRACE, INNER_ERROR
- **C) UDT-based** - Define as actual BASIC UDT that users can extend

**Recommendation:** **Option A initially, upgrade to B later**

**Rationale:**
- Start simple, add fields as needed
- Easier to implement and test
- Can add rich fields in v2.0

**Proposed structure:**
```basic
ERR.CODE AS INTEGER      ' Error code (5, 9, 11, 13, etc.)
ERR.MESSAGE AS STRING    ' Human-readable message
ERR.LINE AS INTEGER      ' Line number where error occurred
```

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 3. Error Variable Scoping

**Options:**
- **A) CATCH variable is local** - Only visible in CATCH block
- **B) CATCH variable is accessible outside** - Can use ERR after CATCH
- **C) Global ERR object** - Always available, updated on error

**Recommendation:** **Option A (CATCH variable is local)**

**Rationale:**
- Cleaner scoping rules
- Matches Python try/except behavior
- Prevents accidental use of stale error info

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 4. THROW Statement Support

**Options:**
- **A) Implement THROW immediately** - Users can throw custom exceptions
- **B) Defer THROW** - Only runtime can throw, add THROW later
- **C) No THROW** - Runtime errors only

**Recommendation:** **Option B (Defer THROW)**

**Rationale:**
- Focus on catching runtime errors first
- THROW adds complexity (user-defined error codes, etc.)
- Can add in phase 2 after basic TRY/CATCH works

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 5. Multiple CATCH Clauses

**Options:**
- **A) Single CATCH only** - One catch block handles all errors
- **B) Multiple CATCH with WHEN** - Filter by error code
  ```basic
  CATCH ERR WHEN ERR.CODE = 11
  CATCH ERR WHEN ERR.CODE = 9
  CATCH ERR
  ```
- **C) Typed CATCH** - Different syntax for different errors
  ```basic
  CATCH DivisionByZero
  CATCH ArrayBounds
  ```

**Recommendation:** **Option A initially, add B in phase 2**

**Rationale:**
- Single CATCH is simpler to implement
- Users can use IF inside CATCH to check error code
- Multiple CATCH is a nice-to-have, not essential

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 6. FINALLY Execution Guarantee

**Options:**
- **A) FINALLY always runs** - Even on GOTO out of TRY
- **B) FINALLY runs on normal/exception exit only** - GOTO bypasses
- **C) FINALLY is optional** - Only if explicitly coded

**Recommendation:** **Option A (FINALLY always runs)**

**Rationale:**
- Matches expectations from other languages
- Essential for resource cleanup
- More complex to implement but worth it

**Implementation note:** This requires wrapping TRY blocks in a way that catches all exits.

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 7. Nested TRY Blocks

**Options:**
- **A) Full nesting support** - TRY inside TRY works correctly
- **B) Single-level only** - No nesting allowed
- **C) Limited nesting** - Max depth of 10

**Recommendation:** **Option A (Full nesting support)**

**Rationale:**
- Natural consequence of exception stack design
- Users expect this to work
- No good reason to limit it

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 8. Exception Propagation

**Options:**
- **A) Uncaught exceptions terminate** - exit(1) if no handler
- **B) Uncaught exceptions print and continue** - Warning only
- **C) Configurable** - User can set behavior

**Recommendation:** **Option A (Uncaught exceptions terminate)**

**Rationale:**
- Matches current behavior (exit on error)
- Safer - prevents silent corruption
- Standard practice in most languages

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 9. Error Code Scheme

**Options:**
- **A) Classic BASIC codes** - 5, 9, 11, 13, 52, 53, etc.
- **B) Custom numbering** - 1000-1999 for runtime errors
- **C) Enum-like constants** - ERR_DIVZERO, ERR_BOUNDS, etc.

**Recommendation:** **Option A (Classic BASIC codes)**

**Rationale:**
- Familiar to BASIC programmers
- Well-documented
- Standard across dialects

**Standard codes:**
```
5  = Illegal function call
6  = Overflow
9  = Subscript out of range
11 = Division by zero
13 = Type mismatch
52 = Bad file number
53 = File not found
61 = Disk full
62 = Input past end
71 = Disk not ready
```

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 10. Implementation Technology

**Options:**
- **A) setjmp/longjmp** - C-style exception handling
- **B) Error code propagation** - Return codes checked everywhere
- **C) Table-based dispatch** - Jump tables for error handlers

**Recommendation:** **Option A (setjmp/longjmp)**

**Rationale:**
- Standard POSIX mechanism
- Works well with QBE IL
- Efficient for exception case (which should be rare)
- No need to modify every function to return error codes

**Performance impact:** Minimal when no exception, ~microsecond when exception thrown.

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 11. Backward Compatibility

**Question:** What happens to existing code that doesn't use TRY/CATCH?

**Options:**
- **A) Terminates on error** - Same as current behavior
- **B) Automatic CATCH** - All code wrapped in implicit TRY
- **C) Configurable** - Compiler flag to enable exceptions

**Recommendation:** **Option A (Terminates on error)**

**Rationale:**
- Zero breaking changes
- Users opt-in to exception handling
- Clear migration path

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 12. RESUME Statement

**Options:**
- **A) No RESUME** - Use TRY/CATCH only
- **B) RESUME NEXT** - Continue after error
- **C) Full RESUME support** - RESUME, RESUME NEXT, RESUME label

**Recommendation:** **Option A (No RESUME initially)**

**Rationale:**
- RESUME is complex and error-prone
- TRY/CATCH provides better structure
- Can add later if needed for ON ERROR GOTO compatibility

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 13. Resource Cleanup Guarantee

**Question:** How do we ensure FINALLY runs even if user uses GOTO?

**Options:**
- **A) Prohibit GOTO out of TRY** - Semantic error
- **B) Track TRY depth** - Execute FINALLY on GOTO
- **C) No guarantee** - Document as undefined behavior

**Recommendation:** **Option B (Track TRY depth)**

**Rationale:**
- Most robust solution
- Users expect FINALLY to always run
- Matches behavior of other languages

**Implementation:** Maintain a runtime stack of FINALLY blocks, execute on any exit.

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 14. Error Message Quality

**Options:**
- **A) Simple messages** - "Division by zero"
- **B) Rich messages** - "Division by zero: 10 / 0 at line 150"
- **C) Localized messages** - Support multiple languages

**Recommendation:** **Option A initially, upgrade to B**

**Rationale:**
- Start with clear, simple messages
- Add context as we gather experience
- Localization is overkill for now

**Decision:** [ ] Approved  [ ] Needs discussion

---

### 15. Testing Strategy

**Options:**
- **A) Unit tests + integration tests** - Test each component
- **B) End-to-end only** - Just test BASIC programs
- **C) Formal verification** - Prove correctness

**Recommendation:** **Option A (Unit + integration)**

**Rationale:**
- Test runtime exception code in C (unit tests)
- Test TRY/CATCH in BASIC programs (integration)
- Comprehensive coverage at all levels

**Test categories:**
1. Basic TRY/CATCH
2. TRY/FINALLY
3. TRY/CATCH/FINALLY
4. Nested TRY
5. All error types (div by zero, bounds, etc.)
6. Edge cases (empty blocks, etc.)

**Decision:** [ ] Approved  [ ] Needs discussion

---

## Implementation Order

Based on decisions above, proposed implementation order:

### Phase 0: Preparation (1 day)
- Review and approve all decisions
- Create detailed task breakdown
- Set up development branch

### Phase 1: Runtime Foundation (3-5 days)
- Implement exception context stack
- Add setjmp/longjmp wrappers
- Update basic_error* functions to use basic_throw()
- Unit test exception runtime in C

### Phase 2: Lexer & Parser (2-3 days)
- Add TRY/CATCH/FINALLY/END TRY tokens
- Implement parseTryStatement()
- Add AST nodes
- Test parser with sample programs

### Phase 3: Semantic Analysis (2-3 days)
- Validate TRY/CATCH/FINALLY structure
- Implement error variable scoping
- Add validation tests

### Phase 4: Code Generation (4-5 days)
- Generate QBE IL for TRY blocks
- Generate CATCH handler code
- Generate FINALLY cleanup code
- Test generated IL manually

### Phase 5: Integration Testing (3-4 days)
- Create comprehensive test suite
- Test all error scenarios
- Fix bugs discovered during testing

### Phase 6: Documentation (1-2 days)
- Update language reference
- Add examples to START_HERE.md
- Document error codes
- Write migration guide

**Total estimated time:** 15-22 days (3-4 weeks)

---

## Success Metrics

How do we know the implementation is complete and correct?

1. **Functionality:**
   - [ ] All runtime errors can be caught
   - [ ] FINALLY blocks always execute
   - [ ] Nested TRY blocks work
   - [ ] Error object contains correct information

2. **Quality:**
   - [ ] Test coverage > 90%
   - [ ] No memory leaks
   - [ ] No crashes in error handling code
   - [ ] Clear error messages

3. **Performance:**
   - [ ] < 5% overhead when no exceptions
   - [ ] Exception throw/catch < 10 microseconds

4. **Documentation:**
   - [ ] Complete language reference
   - [ ] 10+ example programs
   - [ ] Migration guide for existing code

5. **Compatibility:**
   - [ ] Zero breaking changes to existing code
   - [ ] All existing tests still pass

---

## Risk Assessment

### High Risk Items
1. **setjmp/longjmp complexity** - Can be tricky to get right
   - **Mitigation:** Extensive unit testing, code review
   
2. **FINALLY guarantees** - Ensuring FINALLY always runs
   - **Mitigation:** Track all exit paths, add runtime checks

3. **Memory management** - Exception context cleanup
   - **Mitigation:** Valgrind testing, careful resource tracking

### Medium Risk Items
1. **QBE IL generation** - Complex control flow
   - **Mitigation:** Manual IL review, comparison with working examples

2. **Nested TRY blocks** - Exception stack management
   - **Mitigation:** Comprehensive nested test cases

### Low Risk Items
1. **Parser changes** - Straightforward token handling
2. **Error codes** - Well-defined standard codes
3. **Documentation** - Clear examples exist in other languages

---

## Open Questions for Discussion

1. Should we support `THROW` in the initial implementation?
2. Do we need multiple `CATCH` clauses immediately?
3. Should error object be extensible (UDT)?
4. What's the migration story for `ON ERROR GOTO` compatibility?
5. Do we need `RESUME`/`RESUME NEXT`?

---

## Approval Checklist

- [ ] All critical decisions reviewed
- [ ] Implementation order agreed upon
- [ ] Success metrics defined
- [ ] Risk mitigation strategies in place
- [ ] Timeline is realistic
- [ ] Team has capacity for this work
- [ ] Dependencies identified (none currently)

---

**Next Steps:**
1. Review this document
2. Approve/modify decisions
3. Create GitHub issues for each phase
4. Begin Phase 0 (preparation)
5. Start implementation!

---

**Document Status:** Draft for review
**Last Updated:** 2024
**Owner:** FasterBASIC Team