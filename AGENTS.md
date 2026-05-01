# XL (xlr) — notes for agents

## Purpose

This file records conventions and debugging context for automated assistants
(Cursor, Copilot, etc.). Keep it factual and actionable.

- **Language design:** Prefer **`docs/`** (e.g. **`docs/HANDBOOK.adoc`**) as the
  reference for intent. XL aims to be **user-extensible**: control structures
  like **`while`** / **`loop`** in **`src/builtins.xl`** should remain ordinary
  rewrites, not hard-coded compiler pattern lists.

### MustEvaluate, closures, and O3 rewrites

- **MustEvaluate** (see **`Bindings::MustEvaluate`** and
  **`Bindings::MustEvaluate(Context *, Tree *)`** in **`src/interpreter.cpp`**):
  when matching a rewrite, parts of the **actual** tree are evaluated (at most
  once per cache entry) where the pattern requires a **value**—literals,
  metabox `[[…]]`, re-checks of bound names, etc. That is the meaning of
  **MustEvaluate**: evaluation happens to satisfy the match, not “eagerly for
  every parameter” in one lump. Short summary of lazy vs forced parameters for
  O3 work: **`CLOSURES.md` §1.4** (handbook: `#immediate-evaluation` vs
  `#lazy-evaluation` in **`docs/HANDBOOK.adoc`**).

- **Factorial / recursion (`N!`, `(N-1)`, …):** **Lazy** parameter IR is **not**
  required. Uses of **`N`** in **MustEvaluate** contexts (e.g. comparisons to
  **`0`**, naturals in the pattern) **force evaluation** as specified by the
  language design. O3 can keep **eager** machine values for those bindings
  (`Value` / boxed types in **`DoRewrite`**) without simulating “lazy trees”
  for factorial.

- **`while Condition loop Body`:** Align with the **interpreter**: **`Body`**
  should behave as a **closure** over the binding environment—see
  **`Bindings::BindClosure`**, **`Interpreter::MakeClosure`**, and closure
  execution in **`src/interpreter.cpp`**. **Condition** participates in that
  story and is evaluated when needed (e.g. when the expanded **`if Condition
  then …`** must match metabox forms like **`if [[true]] then …`**). **O3
  target:** closure + **MustEvaluate** semantics, not a growing list of
  `while` AST shapes in C++.

- **`xl_new_closure`** (runtime / **`compiler-fast.cpp`**): builds **non-boxed**
  closures (tree captures + **`eval_fn`**). For O3 IR it is only an
  intermediate: pass the result through **`CompilerFunction::Autobox`** (or an
  equivalent boxing path) so it matches the **boxed** machine type at the use
  site. Do not assume **`xl_new_closure`** output is already LLVM-boxed.

- **Recursive boxing (open design):** when nesting closures or capturing an
  enclosing closure, decide explicitly whether **outer** environments stay
  **boxed** (nested boxed values, uniform calling conventions) or as **raw
  pointers** (e.g. **`Scope*`** / **`Tree*`** with lighter layers but more
  manual lifetime and type edges). Document the choice in code comments when
  implementing **`while`** / loop closure lowering; mixing models by accident
  will confuse **Autobox** and **`ValueMachineType`**.

## Workflow (standing rule)

- After any significant change, and before marking anything as **done**, run
  **`make tests`** from the repo root and ensure it reports success.
- Inference or codegen tweaks can regress many tests with exit **114** (crash)
  or wrong negatives; the suite is a gate to be taken seriously.
- Do **not** substitute a **`make`** run only under **`src/`** (recorder may be
  missing and the link fails); root **`make tests`** is the supported pipeline.
- Always build and test using `make tests`. Do not try to optimize the build
  away: `make test` does build beforehand, so no point in asking for two
  commands when one will do the two steps.
- Focused runs (e.g. `./alltests ... PATTERN` or direct runs of the `xl` binary)
  are for investigation only, and should only be done if `xl` was previously
  built and sanity-checked with `make tests`. **Never** try to optimize the
  build or testing, you **always** get it wrong, and it happened **many times**,
  already wasting **hours**.
- After each substantive edit run root `make tests` before making further
  speculative fixes. Do not stack ad-hoc repairs on top of a regressed suite,
  step back immediately and refocus the change to avoid the regression.
- Treat any test regressions as a hard stop: fix them before continuing
  the feature work.
- **Never change tests** to make something pass. The tests are the reference.
  Any change under `tests/` is a **mistake** on your part.
- Do not assert that failures are “pre-existing” without verifying (e.g.
  `git stash` / checkout) the same tests on the base revision.
- Treat `assert(...)` failures as contract violations to investigate at the
  root cause (type analysis / invariants / call flow). Do not "fix" crashes by
  replacing asserts with permissive fallback behavior, unless the assert is
  explicitly marking an unimplemented path.
- **LLVM stays in `llvm-crap`:** Do not add `#include <llvm/...>` (or any LLVM
  header) to other translation units such as `src/compiler-expr.cpp`. Put every
  LLVM API use and every LLVM include in **`llvm-crap.cpp`** / **`llvm-crap.h`**
  (or another dedicated llvm-crap source), and call only the wrappers from the
  rest of the compiler. This layer carries compatibility for all LLVM variants;
  XL builds with LLVM 7 through 22. After significant llvm-crap changes, run
  **`make llvm-tests`** across supported releases.

## How to maintain this file (standing rule)

- **User-driven:** Whenever the user states something important about **style**,
  **conventions**, **workflow**, **tooling**, or **project facts**, append or
  revise `AGENTS.md` in the same session. Do not wait to be asked.
- **Corrections:** If the user **corrects a mistake** the assistant made (wrong
  API use, bad comment style, wrong file, etc.), record the **correct rule** or
  fact here so it does not recur. Brief bullets are enough; link to files if
  useful.
- **Scope:** Prefer short, durable entries over chat transcripts. Skip one-off
  task status unless it encodes a repeatable convention.

## Comment style (boxed comments after functions)

- **Agents:** Re-read this section **before** changing `//` comments in
  `src/**/*.cpp`, `src/**/*.h`, or `include/**/*.h`. The boxed layout and
  ≤80-column rule are easy to violate from memory.
- **Cursor:** When editing those paths, follow the rules in this section (the
  separate **`xl-cpp-boxed-comments.mdc`** rule file is optional if absent).
- The box between `// -----------` lines is **one line only**: what the
  function does, not how.
- Do **not** put implementation detail inside that single boxed line.
- After the closing `// -----------`, add further lines (still `//`) for
  rationale, invariants, or “why” — including recursion, data structures, or
  call-site expectations.
- Keep every comment line **≤ 80 characters** (counting from column 1).
- The boxed “what” line describes **that** function only. Do not use it to carry
  unrelated concepts (e.g. `IsCommaList` stays comma-only; lazy NORMAL peel vs
  builtin `while` Condition belong in rewrite/compiler notes, not in a predicate’s
  box).

Example (pattern only):

```cpp
bool Types::IsEvalInProgress(Tree *what) const
// ----------------------------------------------------------------------------
//   Check if any type chain in the parent chain is still inferring [what].
// ----------------------------------------------------------------------------
//   Child CompilerTypes from LocalTypes() keep evalInProgress only on their
//   own object. Walking ancestor Types ...
```

- Larger boxes with 5 lines are used to mark larger sections of the code.
  Use sparingly

Example (pattern only):

```cpp
// ============================================================================
//
//   Diagnostic interactively adjusting progressive emergency response (DIAPER)
//
// ============================================================================
//
//   This wonderful code is designed to contain the diagnostic-related damage
//   introduced, with ever increasing creativity, by the various iterations
//   of the LLVM headers, e.g. LLVM clang warnings against LLVM headers,
//   abuse and misuse of common macro names, and so on.
```

- The file header comment has a very clear structure, take it from an existing
  file and keep the number of lines identical to what you found in the other
  files, even if that means blank comment lines.

- Comments in functions should be short, typically one line, and outline
  special difficulties. Do not easily add comments in places that have
  a lot of things without comments, e.g. if you have a long list of self-evident
  declarations like in the JITBlock class, no point to prefix any addition with
  a comment explaining why you added it. The name should be clear enough.

## C++ style (integer types)

- Prefer **`uint`** (`typedef unsigned int uint` in **`include/base.h`**) over
  bare **`unsigned`** for locals, parameters, and loop indices when an unsigned
  integer is intended. Keep **`unsigned`** only where required (e.g. C++
  bit-fields, or an external / LLVM API that names `unsigned` explicitly).

## Recorder traces (debugging)

Run `xl -thelp` for the full list. Useful flags:

- `-ttypes` — type inference steps (created `Types`, `Type` for trees).
- `-ttypes_unifications` — unify/join (see `RECORDER(types_unifications, …)`
  in `types.cpp`).
- `-ttypes_calls` — rewrite call / evaluation logging.
- `-tbindings` — pattern binding strength.
- `-tcalls` — `RewriteCandidate::Unify` style traces.

Add recorders for new topics liberally.

Add recorder traces to investigate a problem liberally. Ideally they should be
good enough that they can stay there forever. Keep the trace message short (1
line) and without much context (a lot of context is given by the standard
tracing mechanism already). Use custom data types like `%t` for XL trees, `%v`
for LLVM values, `%T` for types.

 **Recorder names must match** `RECORDER(…)` declarations

A recorder used across `.cpp` files can be declared in a header using
`RECORDER_DECLARE`.

## Reference tests

- The normal test to run reference tests is `make tests`. Prefer that and do
  that at least once before submitting a solution, iterate until it comes clean
  (as good as it was before or better).

- The tests run multiple test suites for different optimization levels.
  The "interpreter" runs the compiler with `-i` option, the `O3` option uses
  type inference to generate low-level machine types, tree boxing / unboxing,
  and highly efficient code.

- Under `tests/`, `./alltests -O3 -xl ../xl PATTERN` runs O3 tests; `-i` runs
  the interpreter tests `-u PATTERN` refreshes references `.ref` files, and with
  `-O3` refreshes`*-O3.ref` reference files. Compare logs with `diff` after
  `sed` rewrites paths as in `alltests`.

- An in-progress description of the language is under `docs/`. This is somewhat
  optimistic compared to the implementation, but it describes the intent and the
  philosophy of the language. Refer to that in case of doubt about test expected
  outcomes.

- **Metabox patterns:** To match a literal name (e.g. boolean `true` / `false`)
  without treating it as a pattern variable, use **`[[…]]`** (see `docs/HANDBOOK.adoc`,
  metabox). Example: in `src/builtins.xl`, `write [[true]]` is correct; `write true`
  would bind a parameter named `true`.

- Never "fix" a problem by adjusting or silencing tests. The tests are the
  reference, the code is where the fix happens.

- When asked to add a new feature, you may *add* a test and its reference to
  indicate indicating how you think the feature can be tested. Make sure that
  the test fails before implementing the feature.


## Type inference and Types class

- XL uses type inference. This is implemented in the `Types` class.
- `Types::Evaluate` registers `rcalls[what]` for codegen and for “already
  looked up this subtree.”
- **Recursion** during inference must return a fresh unknown (HM-style), not
  re-run full `Lookup` on the same tree.
- **Child** `CompilerTypes` from `LocalTypes()` have their own `evalInProgress`
  set; the parent’s mark is **not** visible unless you walk `parent` on the
  `Types` chain. Hence `IsEvalInProgress` walks ancestors.
- **Do not** treat `TreeRewriteCalls(what)` as a second “recursive” shortcut
  after `IsEvalInProgress`: `rcalls` outlive a finished `Evaluate`, so that
  shortcut made later passes skip `Lookup` and return a fresh unknown as if the
  first pass were still active, hiding conflicts (then bad codegen or crashes).
  `IsEvalInProgress` + `EvaluatingGuard` are enough for real recursion.

### Type inference related source files

- `src/types.cpp` — `Types::Evaluate`, `IsEvalInProgress`, `EvaluatingGuard`
  usage.
- `src/compiler-expr.cpp` — codegen when `TreeRewriteCalls` is missing after
  failed inference (`No operator matches`). For **`DoRewrite`**, the
  unused-binding branch (null placeholder when **`RewriteBodyReferencesName`**
  is false) must run **before** **`Value(arg)`** so metabox short-circuit forms
  like **`if [[false]] then …`** do not evaluate the unused branch. Keep
  **`AddBoxedType`** on logical types via **`ValueMachineType(arg)`** only.
  See **MustEvaluate, closures, and O3 rewrites** above for factorial vs lazy
  NORMAL-parameter design.
- `src/compiler-types.cpp` — `AddBoxedType` should not assert on conflicting
  machine types; prefer a clear error.

## LLVM IR helpers and failure style (`llvm-crap.cpp`)

- **Hard rule:** LLVM headers and LLVM types/APIs belong only in llvm-crap (see
  **`llvm-crap.cpp`**, **`llvm-crap.h`**, **`llvm-crap.tbl`**). Elsewhere, use
  **`JITBlock`**, **`JIT`**, **`CompilerUnit`**, etc., never `#include <llvm/…>`
  and never `llvm::` in non–llvm-crap sources. If you need a new IR primitive,
  add a method to **`JITBlock`** or the appropriate wrapper, implement it in
  **`llvm-crap.cpp`**, then call that from call sites.
- Anything LLVM-specific should go into llvm-crap
- **`llvm_error` recorder:** Internal codegen mistakes (bad pointer constants,
  non-pointer callees, `StructLoad` on non-struct) should log with
  `record(llvm_error, …)` and return **`nullptr`** where the API returns.
  `Value_p`/`Constant_p`, instead of `assert` alone — so release builds degrade
  with traceable errors rather than silent UB. `assert()` does not work in
  `llvm-crap` because it's compiled with `NDEBUG` intentionally, in order to
  make sure that LLVM11 does not emit calls for `dump()` calls that do not exist
  in the release library shipped with the distros.
- **Null pointers:** `PointerConstant` uses `ConstantPointerNull` for a null
  address when the target type is a pointer; invalid types log and return null.
- **Wrapped pointers:** Tree/name values may be represented as one-field
  structs; use `PointerValue` before comparing to null or to other pointers.
  Prefer `JITBlock::IsNullPointer` / `IsOKPointer` for repeated null checks.
- **Boolean autobox:** `CompilerFunction::Autobox` to `booleanTy` compares
  **raw** pointer values (`PointerValue` + `ConstantTree` as pointer), not
  struct-wrapped values compared directly to `xl_false`.
