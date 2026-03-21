# XL Test Triage — LLVM ORCv2 Migration

Summary of the 4 failing tests after the dynamic library rpath fix.

---

## 1. `00.Parser/option-nonexistent.xl` — **Output mismatch** (easy fix)

**Expected:** EXIT=1, usage text with relative path `../xl`

**Actual:** EXIT=1, usage text with absolute path `/Users/ddd/Work/tao3D/libxlr/xlr/xl`

**Root cause:** `make test` invokes alltests with `-xl "$(abspath ../xl)"`, so the usage line shows the absolute binary path. The `.ref` was created when XL was invoked with a relative path (`../xl`). The `sed` in alltests only rewrites `$TESTDIR` → `TESTS`; it does not normalize the XL executable path.

**Fix options:**
- **A:** Add a `sed` in alltests to replace `$XL` (or any path ending in `/xl`) with `../xl` before diffing.
- **B:** Change the Makefile to pass `-xl "../xl"` instead of abspath when running from the tests directory (may break when tests run from a different cwd).
- **C:** Update the `.ref` to accept a normalized path pattern (e.g. via a broader sed or a `.ref` that uses a generic path).

**Recommendation:** Option A — add path normalization for the XL binary in alltests.

---

## 2. `00.Parser/option-bool-variants.xl` — **Invalid error exit (SIGSEGV)** (critical)

**Expected:** EXIT=0, trace output ending with `6`

**Actual:** EXIT=114 (SIGSEGV), crash in `CompilerUnit::CompilerUnit` during compilation of the main program *after* builtins.xl has loaded.

**Stack trace (abbreviated):**
```
#5 XL::CompilerUnit::CompilerUnit(...)  si_addr=0x0
#6 XL::Compiler::Evaluate
#7 XL::Main::Run
```

**Root cause:** Null pointer dereference when creating the second `CompilerUnit` (for the user program `1+2+3`) after the first one (builtins) has finished. This indicates a state management bug in the ORCv2 migration:

- After builtins.xl is compiled and JIT’d, `Address()` calls `addIRModule()`, moving `module` and `context` into the JIT.
- `DeleteModule` then clears the module and creates a new `LLVMContext` for the next unit.
- When the next `CompilerUnit` is created, something is still null or invalid (e.g. `module`, `context`, or JIT internal state).

**Additional clue:** `option_value: Text builtins_path = 0x16f8fe34a (was 0x100a946d8)` — Text option values are printed as raw pointers instead of strings. That may be a separate bug or related to memory layout.

**Fix:** Debug the JIT/CompilerUnit lifecycle for ORCv2. Verify that:
1. `CreateModule` correctly creates a new module after `DeleteModule`.
2. No pointers are used after `std::move`.
3. `p.Module()` and related accessors return valid pointers when the second CompilerUnit runs.

---

## 3. `00.Parser/C-syntax.xl` — **Output mismatch** (parser/format change)

**Expected:** Parse tree with `(infix CR` as the top-level wrapper, then `(infix;` inside.

**Actual:** Parse tree with `(infix;` as the top-level wrapper, then `(infix CR` nested inside.

**Root cause:** AST structure for `int foo(...); int * z\n\nextern double bar(...)` differs. The ordering of `infix CR` (newline) vs `infix;` (semicolon) at the top level is reversed. This could be:

- A pre-existing parser behavioral difference across environments.
- A change in parse order or AST construction (e.g. different operator precedence or infix handling).
- A simple formatting/rendering difference.

**Fix:** Determine whether the new order is correct. If yes, update `C-syntax.ref`. If no, track down the parser change that caused the reordering.

---

## 4. `00.Parser/fault-injection-stack.xl` — **Exit code 137 (SIGKILL) vs expected 114** (environmental)

**Expected:** EXIT=114 (SIGSEGV caught by recorder), filtered trace matching `.ref`

**Actual:** When run manually with the same command: EXIT=114 and output matches the reference. Under `make test`: EXIT=137.

**Root cause:** 137 = 128 + 9 (SIGKILL). The process is killed externally during the full test run, not by the fault-injection logic. Possible causes:

- Timeout or resource limit in the test runner.
- Parent (e.g. make) killing the child.
- Sandbox or system OOM killer.
- Cross-talk from `option-bool-variants` (SIGSEGV) affecting sibling test processes.

**Fix:**
1. Run `fault-injection-stack` in isolation to confirm it passes.
2. If it passes in isolation, adjust test order or add isolation (e.g. separate process, longer timeout).
3. If it still fails under `make test`, add a short delay or a separate test target for fault-injection tests.

---

## Priority

| Priority | Test                      | Effort | Blocker |
|----------|---------------------------|--------|---------|
| P0       | option-bool-variants      | High   | Yes     |
| P1       | option-nonexistent        | Low    | No      |
| P1       | fault-injection-stack     | Low    | No      |
| P2       | C-syntax                  | Medium | No      |

The critical path is **option-bool-variants**: the CompilerUnit/ORCv2 lifecycle must be fixed before the JIT-based tests can pass reliably.
