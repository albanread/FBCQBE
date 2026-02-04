# ON GOTO/GOSUB Codegen Plan

## Context
FasterBASIC's CFG-aware QBE backend now handles most control-flow constructs, but the ON…GOTO and ON…GOSUB statements still rely on legacy linearized codegen paths. The AST and semantic passes populate `OnGotoStatement` and `OnGosubStatement` nodes, and the modular CFG builder produces multi-edge terminators for them. The V2 `CFGEmitter`, however, does not yet recognise these terminators and falls back to a placeholder multiway dispatch, producing unusable IL. This document outlines the plan to complete the new implementation in a performant and maintainable way.

## Goals
- Evaluate the ON selector once, honour BASIC’s 1-based semantics, and branch to the appropriate target.
- Emit efficient IL by using QBE’s native `switch` instruction instead of long comparison chains.
- Integrate ON…GOTO and ON…GOSUB into the existing CFG emitter without regressing ordinary GOSUB handling.
- Share as much logic as possible between direct GOSUB paths and computed GOSUB targets (e.g., return-stack push code).
- Ensure that out-of-range selector values fall through correctly (do nothing for ON…GOTO, resume execution for ON…GOSUB).

## Current State
- **AST/Semantics:** `OnGotoStatement` / `OnGosubStatement` already carry selector expressions plus ordered target metadata. Semantic validation ensures targets exist.
- **CFG Builder:** `handleOnGoto()` / `handleOnGosub()` create one edge per case (`case_#` or `call_#`) and a default edge (fall-through block or return point). ON…GOSUB return blocks are recorded in `ControlFlowGraph::gosubReturnBlocks`.
- **Codegen V2:** `emitBlockStatements()` skips RETURN but still emits ON statements inline; `emitBlockTerminator()` treats the resulting edge set as an unknown multiway branch and inserts a placeholder `emitMultiway()` call with a fake selector.
- **Legacy Reference:** The old code generator contains working `emitOnGoto()` / `emitOnGosub()` implementations that evaluate the selector and issue comparison chains followed by jumps. These functions prove the runtime semantics but are tightly coupled to the non-CFG pipeline.

## Design Summary
1. **Skip ON statements during statement emission.** Treat them like RETURN: detect in `emitBlockStatements()` and leave them for the terminator stage.
2. **Detect ON statements when emitting terminators.** Scan the block’s statements inside `emitBlockTerminator()` to find an `OnGotoStatement` or `OnGosubStatement`. If found, hand control to dedicated helpers and return early.
3. **Selector normalisation helper.** Add `emitSelectorWord(const Expression*)` inside `CFGEmitter` to evaluate the selector via `astEmitter_`, then convert it to a 32-bit word using `typeManager_` so that downstream logic always works with integers.
4. **Use QBE `switch` for ON…GOTO.**
   - Build a dense case map by parsing outgoing edges with labels `case_#`.
   - Identify the default block from the edge labelled `default` (or the sole `JUMP` edge).
   - Subtract one from the selector to convert BASIC’s 1-based indices to zero-based case slots.
   - Call a new `QBEBuilder::emitSwitch()` helper that lowers to `switchw %selector, @default, @case0, …`.
5. **Construct trampolines for ON…GOSUB.**
   - Locate the fall-through return block (edge with `EdgeType::JUMP` or the default label).
   - Create a trampoline label per `call_#` edge. Each trampoline pushes the return block ID, then jumps to the actual subroutine target.
   - Feed the trampoline labels to the same `emitSwitch()` helper so default values fall through without touching the return stack.
6. **Refactor return-stack push logic.** Extract the existing push sequence from the direct GOSUB path into `emitPushReturnBlock(int returnBlockId)` and reuse it from both direct and computed GOSUB paths.
7. **Extend QBEBuilder.** Add `emitSwitch(type, selector, defaultLabel, caseLabels)` with corresponding declarations in the header and implementation in `qbe_builder.cpp`. The helper should write a single `switch` instruction and rely on callers to provide already-normalised selectors.
8. **Optional CFG tweak.** (Nice-to-have) Make `handleOnGosub()` label the fall-through edge explicitly as `"default"` for readability; the new codegen logic can handle either form.

## Implementation Steps
1. **CFGEmitter updates**
   - Skip ON statements in `emitBlockStatements()`.
   - Add selector normalisation, ON…GOTO, ON…GOSUB, and return-push helpers.
   - Modify `emitBlockTerminator()` to detect ON statements before processing other edge types.
2. **QBEBuilder updates**
   - Implement `emitSwitch()` and expose it in the header.
   - Ensure existing helpers (e.g., `emitBranch`) remain unaffected.
3. **Refactor GOSUB push**
   - Replace the inline return-stack push in the GOSUB branch with the new helper.
   - Reuse the helper inside ON…GOSUB trampolines.
4. **Regenerate integrated build sources**
   - Copy or sync the modified files into `qbe_basic_integrated/` or rerun the packaging script, then rebuild.
5. **Documentation/cleanup**
   - Update this plan if deviations occur.
   - Note the change in any relevant summary docs once merged.

## Testing Strategy
1. **Existing BASIC programs**
   - `test_on_goto`, `test_on_gosub`
   - `gosub_if_control_flow.bas`, `test_edge_cases.bas`
2. **New regression case**
   - Add a program covering selector = 0, negative values, >N, floats, and mixed label/line targets. Ensure ON…GOSUB includes paths that immediately RETURN.
3. **Compiler harness**
   - Run `./run_tests.sh` at repo root after rebuilding.
4. **Integrated build**
   - `cd qbe_basic_integrated && ./run_tests.sh` (or equivalent smoke tests).
5. **Manual inspection**
   - For a sample program, dump generated QBE to confirm the presence of `switchw` and proper trampolines.

## Follow-Ups
- Consider migrating SELECT CASE to the new `emitSwitch()` helper once ON statements are stable.
- Review `gosub_return_stack` sizing; add runtime guards if necessary.
- After landing the implementation, update any higher-level roadmap docs summarising CFG codegen readiness.

## Owners & Timeline
- **Primary engineer:** (assign when work begins)
- **Supporting reviewers:** CFG/Codegen maintainers
- **ETA:** Align with the next control-flow milestone (tentatively upcoming sprint)

Once this plan is executed, ON…GOTO and ON…GOSUB will be fully supported in the CFG-driven pipeline, eliminating the remaining dependency on legacy code paths and unlocking the remaining failing tests tied to computed jumps.