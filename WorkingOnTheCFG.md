# Working on the CFG Builder - Quick Reference

**Last Updated:** February 1, 2025  
**Status:** 🚧 Foundation Complete - Loop Implementations In Progress  
**Priority:** HIGH - Fixes critical nested control flow bugs

---

## 📍 What Is This?

We're rewriting the Control Flow Graph (CFG) builder from scratch to fix critical bugs where nested loops (REPEAT, DO) inside IF statements cause infinite loops.

**The Problem:** Old builder uses two-phase construction (build blocks → add edges). This breaks with nested structures.  
**The Solution:** New single-pass recursive builder that creates complete structures immediately.

---

## 📊 Quick Status

| Component | Status | Next Action |
|-----------|--------|-------------|
| Foundation | ✅ Complete | - |
| IF Statement | ✅ Complete | - |
| WHILE Loop | 📋 TODO | **Implement next** |
| FOR Loop | 📋 TODO | Implement next |
| REPEAT Loop | 📋 TODO | **CRITICAL - fixes infinite loop** |
| DO Loop | 📋 TODO | **CRITICAL - fixes infinite loop** |

**Current:** 25% complete  
**Test Results:** 3 of 6 nested tests pass (50%)  
**Target:** 6 of 6 nested tests pass (100%)

---

## 📂 Essential Documents (Read These!)

### 1. **Design & Architecture**
📄 `docs/CFG_REFACTORING_PLAN.md` (1,152 lines)
- Why we're doing this (old builder is broken)
- Complete architectural design
- Implementation examples for all control structures
- Timeline and migration strategy

**Read this first to understand the "why" and "how"**

### 2. **Implementation Guide**
📄 `fsh/FasterBASICT/src/cfg_v2/README.md` (478 lines)
- Overview of new architecture
- File structure
- Implementation status
- Code patterns and examples
- Testing strategy

**Read this to start coding**

### 3. **Progress Tracking**
📄 `docs/CFG_V2_PROGRESS.md` (454 lines)
- Daily progress log
- Milestone tracking
- Test results
- Issues and blockers
- Next steps

**Update this as you work**

### 4. **Test Results That Proved The Bug**
📄 `docs/session_notes/NESTED_TEST_RESULTS_2025_02_01.md` (587 lines)
- Detailed test failures
- CFG trace analysis
- Root cause identification

**Read this to understand what we're fixing**

---

## 🚀 Getting Started (New Team Member)

### Day 1: Understand the Problem
1. Read the test results: `docs/session_notes/NESTED_TEST_RESULTS_2025_02_01.md`
2. Look at failing tests: `tests/loops/test_nested_repeat_if.bas`
3. See them fail: `./qbe_basic -o /tmp/test tests/loops/test_nested_repeat_if.bas` (infinite loop!)

### Day 2: Understand the Solution
1. Read the plan: `docs/CFG_REFACTORING_PLAN.md` (focus on sections 1-4)
2. Read the implementation guide: `fsh/FasterBASICT/src/cfg_v2/README.md`
3. Study the IF implementation: `fsh/FasterBASICT/src/cfg_v2/cfg_builder_v2.cpp` (lines 280-385)

### Day 3: Start Contributing
1. Pick a control structure (WHILE is easiest, REPEAT is most critical)
2. Use IF as a template
3. Test with simple cases first
4. Update `docs/CFG_V2_PROGRESS.md` with your work

---

## 💻 Code Location

**New Implementation (Work Here):**
```
fsh/FasterBASICT/src/cfg_v2/
├── cfg_builder_v2.h          ← Interface & context structures
├── cfg_builder_v2.cpp        ← Core + IF (✅ done)
├── cfg_while.cpp             ← WHILE (📋 todo - do this next!)
├── cfg_for.cpp               ← FOR (📋 todo)
├── cfg_repeat.cpp            ← REPEAT (📋 todo - CRITICAL!)
├── cfg_do.cpp                ← DO (📋 todo - CRITICAL!)
└── README.md                 ← Implementation guide
```

**Old Implementation (Don't Modify):**
```
fsh/FasterBASICT/src/fasterbasic_cfg.{h,cpp}  ← Keep for reference
```

---

## 🧪 Testing

**Nested Control Flow Tests (The Proof):**
```bash
# Run all nested tests (currently 3/6 pass)
./scripts/test_nested_control_flow.sh

# Run individual test
./qbe_basic -o /tmp/test tests/loops/test_nested_repeat_if.bas
timeout 5 /tmp/test  # Will timeout - infinite loop bug!
```

**Test Files:**
- `tests/loops/test_nested_while_if.bas` ✅ (passes)
- `tests/loops/test_nested_for_if.bas` ✅ (passes)
- `tests/loops/test_nested_repeat_if.bas` ❌ (infinite loop)
- `tests/loops/test_nested_do_if.bas` ❌ (infinite loop)
- `tests/loops/test_nested_mixed_controls.bas` ❌ (infinite loop)

---

## 🎯 What Needs Doing (Priority Order)

### Week 1 (Current)
1. **WHILE loop** - Use as template for others
2. **FOR loop** - Similar to WHILE
3. Test both with basic tests

### Week 2 (Next)
1. **REPEAT loop** - Fixes infinite loop bug! 🔥
2. **DO loop** - Fixes infinite loop bug! 🔥
3. Run nested test suite → expect 100% pass rate

### Week 3-4
1. SELECT CASE, TRY/CATCH
2. Integration with feature flag
3. Full regression testing

---

## 📝 Key Design Principles (Follow These!)

### ✅ DO:
- Create blocks and edges together (single pass)
- Pass context as function parameters
- Wire every edge explicitly
- Return exit block from each builder
- Test each structure individually first

### ❌ DON'T:
- Use global stacks (use context parameters)
- Assume block N flows to block N+1
- Scan forward for loop ends
- Add edges in a separate phase
- Skip testing

---

## 🆘 Need Help?

**Architecture Questions:**
- Read: `docs/CFG_REFACTORING_PLAN.md` sections 1-4

**Implementation Questions:**
- Read: `fsh/FasterBASICT/src/cfg_v2/README.md`
- Study: IF implementation in `cfg_builder_v2.cpp`

**"Why are we doing this?"**
- Read: `docs/session_notes/NESTED_TEST_RESULTS_2025_02_01.md`
- Run: `./scripts/test_nested_control_flow.sh` to see failures

**Testing Questions:**
- Read: `tests/loops/README_NESTED_TESTS.md`

---

## 📅 Timeline

- **Week 1 (Current):** Foundation + WHILE/FOR
- **Week 2:** REPEAT/DO (fixes bugs!)
- **Week 3:** Testing & integration
- **Week 4:** Production deployment

---

## ✅ Success Criteria

- [ ] All 6 nested control flow tests pass (currently 3/6)
- [ ] No infinite loop bugs
- [ ] Zero regressions in existing tests
- [ ] Faster compilation (O(n) vs O(n²))
- [ ] Cleaner, maintainable code

---

## 🔄 Daily Workflow

1. **Update progress:** Edit `docs/CFG_V2_PROGRESS.md`
2. **Write code:** In `fsh/FasterBASICT/src/cfg_v2/`
3. **Test it:** Run relevant tests
4. **Document:** Update progress tracking
5. **Commit:** Clear commit messages

---

## 📌 Remember

**This is critical infrastructure work.** The CFG builder is the foundation of the compiler. Bugs here cause infinite loops, crashes, and incorrect code generation.

**Test everything.** Every control structure, every nesting pattern.

**Follow the architecture.** Don't fall back to old patterns. The whole point is to fix the architectural flaws.

**Document your work.** Update `docs/CFG_V2_PROGRESS.md` daily.

---

**Questions?** Read the docs listed above. Everything is documented!

**Ready to code?** Start with `fsh/FasterBASICT/src/cfg_v2/README.md` and the IF implementation example.