# P917 — private owned same-definition CALL witness

Fixed producer: P905 `6e8be7c313d19b41ca2d45ae608b4dabb01b22e7`,
tree `509c5954aac3c2a65af8e86af8ba8ace6625281e`. Canonical Draft17.34.
This is P-owned construction/calibrated evidence, NOT independent R or F.
M895 is a proposal, F66's circular Call.body shortcut remains rejected,
M916's cooperative Stage0 contract is preserved, and R912 remains overall
INCONCLUSIVE. No frozen P/R source, oracle, CI, or verdict is edited.

Read-only Gate A was posted before branch/code. New two-nominal-family
source/oracle/SHA freeze and original CHECK3 measurements are in this directory.
No R harness or oracle is used. The ten inputs remain byte-identical to freeze.

The constructor is a separate private session, not a Stage0 observer:

1. Capture its own semantic PRE immediately after actual evaluation and before
   any formal binding. Keep exact source-order arguments and registered body.
2. Attach its own POST after the real callee RETURN/local cleanup. These are
   data, not assumed valid effects or cached verification results.
3. On definition failure, transfer the ORIGINAL complete two-arm owner subtree
   and definition context before normal destruction. No graph is fabricated,
   re-numbered or cloned from an observer. Both arms retain their distinct
   worlds and nested callee graph namespaces. The enclosing definition is still
   FAILED/PARTIAL: this is not a successful public checked artifact.
4. Close the session, destroy input/parser/registration owners, then independently
   replay from original IF entry/source AST through ordered actual/formal,
   active loan, primitive Replace, typed RETURN-old and loan exit. Every verify
   call executes the replay; constructor does not invoke it or cache a PASS.
5. Destroy PRE/POST retained plans first, then original graph/nested targets and
   contexts, root context last. Existing body reference ownership keeps source
   bytes/syntax alive. No pointer from NLDefinitionCallCapture/View is retained.

The shared replay kernel is the existing P905 independent source interpreter,
factored from the Stage0 wrapper. Stage0's borrowed candidate identity and
callback/free order remain unchanged. The owned validator first checks its own
session and actual original-tree membership, then passes a temporary stack
descriptor to that kernel. Descriptor construction is not observer authorization.
The session is TLS/explicit, single definition, nonreentrant and nonpublishing;
open-session validation or nested open is refused. It is not a general host API,
thread transfer, ownership registry, immutable public schema, or public issuer.

Four additive CTests:

- `p917_owned_lifetime.unit`: four source cases/eight arms; Stage0 callbacks end,
  all original input/context owners are destroyed, then genuine owned PRE,
  old-Copy provenance, fresh field effects/sibling and loan closure replay.
- `p917_owned_integrity.unit`: successful positives first, metadata controls,
  real equal-number foreign graph and separate-arm edge substitution,
  actual graph/field/fact/scope/RETURN/source-owner mutations with restoration.
  RETURN-incoming and equal-number literal-old require successful PRE before
  refusal; missing/duplicate calls yield no committed private record.
- `p917_owned_oom.unit`: actual new pending calloc, PRE clone's first context
  malloc, POST clone's first context malloc each reached once; no partial record,
  original public-state rollback and SAME-context recovery/replay. This is not
  verifier scratch OOM, R's harness, or guest allocation failure. Other sites
  and arbitrary multi-definition/reentry behavior are NOT claimed tested.
- `p917_cli_unchanged.integration`: all frozen sources CHECK3/P6; zero C17 and
  existing named output preserved. Additional archived comparison covers 51
  ordinary CLI source/mode pairs against fixed P905, identical byte triplets.

Memory cost: private session + per-call descriptor/PRE/POST clones, plus the
original failed definition owner tree kept alive until explicit destroy. Source
plans are retained through existing body ownership; there is no deep graph copy.
No memory/performance optimization or zero-cost claim. Source/checker bounds
still apply, and proof construction supports one exact two-NORMAL-arm instance.

No public IF event, Change grant, Unknown join, CHECK0, backend or new native
result is issued. Public IF validator/body_count guard remain unchanged. New
host tests/sanitizers are distinct from absent M8b guest C17/native evidence.
No canonical/source/core/Allocation/Domain/authority change, no general CFG.
Author + ordinary-safe caller UB-prevention audit locality versus well-factored
C is UNMEASURED; Product GO PAUSED and H1 HOLD remain.

Gate A and first logs are preserved under `evidence/`. Initial read-only probe
missed parser.h (host compile only, corrected outside repo), and one local test
command lacked the activated ctest PATH (exit127, no tests executed). These are
preparation failures, not source/proof failures; neither was discarded or used
as a successful run. No unexpected semantic/proof mismatch occurred in executed
local tests. CI is at most one natural FULL5+PARTIAL pair, attempt1; any first
CI failure means STOP, no manual retry, repair push, rebase or merge.
