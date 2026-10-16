# PROPOSED / UNMERGED — Draft17.30-based Option-backed Array4 SOURCE constructor candidate (M #530)

> **Candidate only. NOT canonical; NOT source-admitted in Production.**
>
> Fixed base: Compiler/main \`c6ae58512683090b2d98b09f51765143b1517345\`.
> [CURRENT_SPEC.md](CURRENT_SPEC.md) continues to select
> [NewLang_v0_spec_Draft17_30.md](NewLang_v0_spec_Draft17_30.md).
> This is a **uniquely named additive source-selection appendix** to that base,
> **not** Draft17.31: the unrelated [unmerged DRAFT PR #273](https://github.com/wakairo/NewLang_Compiler/pull/273)
> already owns a proposed Draft17.31. Nothing here assumes/adopts its contents.
> Independent Coordination Historical Gate E / merge / explicit canonical selection
> is required before this candidate becomes normative. This PR must remain DRAFT.

## 0. Scope and decision

**Select as the *candidate* one closed source construction family: explicit positional, exactly four separately authored typed \`None\` initializer expressions for one live original lexical \`Array<Option<u8>,4>\`.** This is the first prerequisite for the [V #528](https://github.com/wakairo/NewLang_Compiler/issues/528#issuecomment-6215314191) NewLang-native tagged Ring4 reference. It is **not** a selected general \`Array<T,N>\` implementation or an already supported \`Option<u8>\` spelling.

The candidate introduces only:
1. a **closed compiler built-in concrete type resolution** of \`Option<u8>\` and \`Array<Option<u8>,4>\`, preserving the structural built-in identities \`Option<T>\` and \`Array<T,N>\` (not registering a new nominal \`Array4OptU8\`);
2. the **qualified \`Option<u8>::None\` payloadless sum constructor** for this concrete \`Option<u8>\` under existing §26.3;
3. **one exact four-position whole-Array initializer** and lexical local placement, with a source-issued single original Array lifetime root and four independently current fixed child Option places, following §§3.8,4,10,14.4,25.1 and 26.

The additions are SOURCE type/constructor eligibility and their required compiler-issued evidence. They **do not** add a new lifetime, owner/Allocation/Domain grant, special raw Storage claim, implicit Drop, effect, generic type declaration, generic inference, traits, module/private visibility, explicit ref, or new control rule.

**Hard exclusion:** \`array_element\` or \`array_element_ptr\` source, \`i<4\` index/bounds validation, read/write loan to any Array child, \`consume_array\` source, \`Some\` payload transport in this profile, arbitrary T/N instantiation, indexed Ring operations, external \`OwnedFrame\` owner, complete nonCopy construction/refund/teardown, generic Result, safe Ring library/opacity, C ABI/FFI, concurrency, callbacks and Product PASS all remain **Deferred**. §25.2 and §25.2a stay conceptual. Do not use this appendix as a source authorization for them.

## 1. Chosen closed source form and exact resolution

**Proposed source (invalid on fixed main until independently adopted and implemented):**

\`\`\`newlang
fn main() -> unit {
    let slots = Array<Option<u8>,4>{
        Option<u8>::None,
        Option<u8>::None,
        Option<u8>::None,
        Option<u8>::None
    };
    unit
}
\`\`\`

The shown local end is valid for the \`u8\` instantiation **only** because its entire type is statically Discardable. This is **not** a general permission to drop \`Array<Option<nonDiscardable T>,4>\` at scope exit; when that future instance is admitted, it must be explicitly consumed/settled by then-current source rules.

The **exact candidate parser production** for this *closed positive profile* is:

\`\`\`text
array4_none_type      := 'Array' '<' 'Option' '<' 'u8' '>' ',' '4' '>'
option_u8_type        := 'Option' '<' 'u8' '>'
none_option_u8_expr   := option_u8_type '::' 'None'
array4_initializer    := array4_none_type '{'
                         none_option_u8_expr ','
                         none_option_u8_expr ','
                         none_option_u8_expr ','
                         none_option_u8_expr [','] '}'
\`\`\`

\`4\` is a **nonnegative compile-time usize element-count type argument**, not an arbitrary numeric expression or bare runtime value. The nested \`Option<u8>\` qualifier and \`::\` have exact §26.3 meaning: \`None\` is payloadless, with **no parentheses**, and binds to the qualified concrete sum type. \`None\` must be emitted as a typed sum value, **never** by assuming zero-filled bytes happen to mean None. Source evaluation is ordered 0,1,2,3 (ordinary left-to-right expression evaluation/value-use rule), with each value used exactly once at its target element.

**Semantic arity/type verification rather than parser-only refusal:** the parser must admit an **array constructor node with a count of parsed initializer expressions** even for otherwise well-formed 3/5 element lists and for a single differently typed source expression. The *semantic checker* rejects count≠4, mismatch against exact \`Option<u8>\`, wrong sum variant, and unknown constructor. These negative spellings must never be reinterpreted as a different named aggregate, function call, or four-field struct. This grammar exposition is the closed **successful** spelling, not a requirement to reject all arity/type negatives during lexical parsing. In particular, \`Option<u8>::Some\` without its mandatory payload is a §26.3 constructor error; \`u8(1)\` in one position is a static element-type error. Wrong arity is a checked semantic error **after** recognized Array source parsing.

**No-foreclosure principle:** The parser/AST/type checking scheme must represent the initializer as **(built-in Array type, concrete element type, compile-time count, ordered expression vector)**. This single closed admission is an eligibility predicate over that representation; it is **not** a distinct permanent \`Array4OptU8\` nominal or an invented four-field \`Pair\`. \`Option<T>\`/ \`Array<T,N>\` structural type identity and the layout property of real \`Array\` remain type-directed. Future independently selected source profiles may admit (a) other N, including N=0, without dummy elements, (b) other closed concrete T, including \`Option<OwnedFrame>\`, and (c) other fully typed initializer expressions, subject to their own nonCopy/constructor failure rules. **None is authorized by this profile today.**

This candidate does **not** choose the general source grammar for built-in type argument expressions, arbitrary \`Self<T>\`/user generic types, nominal inference, type-directed \`None\` shorthand, or \`T::Some(payload)\` beyond the existing authorized §26.3 subsets.

## 2. Static type and responsibility contract

- The resolved built-in sum \`Option<u8>\` has \`None\` and \`Some(u8)\` variants. The *static* \`Copy\` and \`Discardable\` properties derive from **all possible payload types**, not the current \`None\` variant (§26.4). \`Option<u8>\` and its four-element Array are Copy/Discardable under current \`u8\` properties. This does **not** justify treating \`Option<OwnedFrame>::None\` as Discardable, or cloning Some nonCopy payload.
- One syntactically unique lexical placement creates exactly **one live original Array lifetime root** \`R_array\` with one original object \`O_array\` / incarnation \`inc_array\`, and target-authorized actual local backing under an **implicit lexical governing domain**. This constructor creates **no user-visible heap BackingRegion/Allocation/Storage claim and no new \`LifetimeDomain\` value**.
- Its four distinct child element **Place/ProjectionIds** \`R_array.element[0..3]\` are fixed subobjects following the same \`R_array\` lifetime root/incarnation; they are **NOT independent original roots, D grants, or allocator owners**. They are all initialized and live **as \`Option<u8>\` objects** at constructor success; each has its own current typed \`None\` value/fact, and **no active \`Some\` payload occurrence**. Source/checked state does not publish an intermediate partial array, and no whole-Array "partially moved" binding becomes source-visible.
- The initializer conceptually evaluates each expression left-to-right and transfers its ordinary typed value package into the corresponding fixed element. The \`None\` case carries no \`u8\` payload, no nonCopy authority and no failure; construction is a **single checked whole-value initialization**, not repeated unchecked slot claims or four individual independent root starts.
- **Future nonCopy case — preservation obligation, NOT source selection:** If a concrete \`Option<OwnedFrame>\` initializer is ever permitted, its entire type remains nonCopy/nonDiscardable even in \`None\`; moving an actual \`Some(original OwnedFrame)\` transfers **the original frame value package** and all original resource identifiers exactly once, without inventing A/D, treating a copyable pointer as owner, or independently moving only the Array child's conditional payload. No source-visible partial-move Array, automatic destructor, silently dropped old Some/None, or branch-specific Copy relaxation. Any recoverable initializer failure after acquiring/consuming nonCopy values must **prove explicit exactly-once refund/settlement for every value**, or the candidate checker must **reject the program before effects**; it may not publish a half-built Array or assume RAII. The first all-None \`u8\` slice has no such fallible effects or owner packets, so does not require a new transaction/unwind protocol.
- A later whole-Array move/relocation does **not** preserve the original **placement** incarnations at the old address: §18.6 gives the destination fresh place/incarnation state. The constructor promises **one original active lexical placement**, not a general pin guarantee across all moves. The V528 Ring needs physical slot stability while operating on its active current backing and no exported persistent slot aliases; broader stable-address/opaque owner contract is separate.

## 3. Mandatory owned artifact / C17 mapping in a **future P** (not implemented here)

The source compiler must prove and issue an **authenticated owned Array-construction artifact**, independently inspectable after source AST teardown:

\`\`\`text
source constructor identity, AST order, concrete type = Array<Option<u8>,4>
element type = Option<u8>, compile-time count = 4
initializer[0..3] = source-authenticated Option<u8>::None typed values
one original lexical Array root R/O/inc + implicit local backing relation
element[0..3]: distinct indexed ProjectionIds, same parent R/O/inc,
               four live fixed Option places, four current None value facts,
               no active conditional Some payload occurrences
target-derived layout: contiguous element extent and sizeof(Option<u8>) stride
\`\`\`

A checker/independent public consumer must verify correspondence from the genuine source constructor through the whole original object, with unique child paths and no duplicated or host-injected independent root/D/Allocation grants. A \`nl_semantic_seed_root\` fixture, private JSON, a raw C array, a compiler-generated record of four standalone nominal fields, matching integers without common root provenance, and a byte-zeroing \`memset\` are **not** proof of this contract. If no public checked-owned issuer/validator can authenticate the producer→four children→one actual C carrier relation, **fail closed before C17**. The API and node names are **not chosen here**; a later P task must choose the smallest truthful bounded implementation, not a ceremonial \`is_valid\` flag.

**Future physical C17 oracle:** One actual source-authenticated local backing contains a **typed fixed array of four full \`Option<u8>\` object representations** (e.g. an internal \`OptU8 elements[4]\` with compiler-controlled opaque \`None\` tag representation). All four actual C cells are live and initialized to semantically None. The element-start addresses within that **one** true original array extent must be \`base + k * sizeof(OptU8)\` for k=0,1,2,3. Their physical identity is correlated with \`R_array.element[k]\` and the same emitted local. The backend may choose its own payload/tag representation and padding/alignment; \`sizeof(OptU8)\` is **not** \`sizeof(u8)\` by fiat, no serialized/stable C ABI, no tagged raw-owner slot with \`sizeof(OwnedFrame)\` stride claim. Source-generated array may be optimized away if unobserved; a future passive same-function original-object observer must make the real object live/observable **without writing/injecting its values** and demonstrate genuine physical positions in strict C17 GCC/Clang (appropriate O0/O2 and layout checks), not C test fixtures unrelated to checked-owned source. A source-level indexed getter is **not yet authorized** just to make this measurement convenient.

**Future smallest evidence gate:** Freeze one positive true source file as in §1, four initialized None and one original Array root; source/CHECK → public owned proof → target C17/native **each separately reported**. Same compiler must admit 3/5 expression counts under its Array AST but reject in semantic CHECK, reject \`u8(1)\` or malformed \`Option<u8>::Some\` type/variant, and reject a duplicated/foreign child parent/inc or forged root in a **test-only corrupted copy** under a public owned verifier. Failure/OOM paths must not mutate a previous artifact or leak obligations. \`index=4\` is a negative for a *later* distinct \`array_element\` SOURCE family, never for this constructor. Neither a successful \`u8\` None Array nor one native C array is proof of Ring4 or \`Option<OwnedFrame>\` transfer.

## 4. Historical Design-Intent Gate A–D and no-foreclosure

**Gate A:** fixed canonical is Draft17.30 §§3.8,4,10,14.4,17.4,18.6,21,23,25.1–2a,26.3–7; this candidate adds a **source-admission profile** only, not new ownership semantics.

**Gate B sources:** [Ledger](../NewLang_Design_Intent_Ledger.md) DI-001/002/005/006/007/008; older [Draft17.2 intent summarized in Draft17.4](NewLang_v0_spec_Draft17_4.md) (whole \`consume_array\` without partial move, representation is not authority); earlier non-normative SurfaceDraft1/identity-validation as indexed, not re-executed; [M499](https://github.com/wakairo/NewLang_Compiler/issues/499#issuecomment-6205260677), [M513](https://github.com/wakairo/NewLang_Compiler/issues/513#issuecomment-6208891023), [M529](https://github.com/wakairo/NewLang_Compiler/issues/529#issuecomment-6215633774), [Coordination M529](https://github.com/wakairo/NewLang_Compiler/issues/529#issuecomment-6215691762), [V528](https://github.com/wakairo/NewLang_Compiler/issues/528#issuecomment-6215314191) and older [M474](https://github.com/wakairo/NewLang_Compiler/issues/474#issuecomment-6190730734). This is a targeted audit, **not** proof the historical archive has no other proposals. Unmerged PR #273 belongs to another independent five-root owner-grant topic.

| Candidate / historical evidence class | Gate C disposition | Preservation / exclusion |
| --- | --- | --- |
| Draft17.30 §25.1 one Array root, contiguous indexed fixed children; §25.2a consuming decomposition / DI-008 \`consume_array\` | **KEEP** \`normative-current\` meaning and \`historical-intent\`; defer source \`consume_array\` | no partial move, no extra child lifetime roots; preserve N=0 possibility and future nonCopy terminal settlement |
| Draft17.30 §26.3 \`SumType::None\`, §26.4 properties, §§26.5–7 opaque variant/occurrence/whole replace; DI-001 | **KEEP** canonical; **INTENTIONALLY EXTEND source eligibility only**, proposed | add concrete \`Option<u8>::None\`, not generalized module qualification/inference or known zero-byte tag |
| DI-005 minimal generic semantics and deferred user \`Self<T>\`/generic nominal | **DEFER** general primary generic source; **use bounded built-in type elaboration only** | array/option type identity retains parameter T/N, no \`Array4OptU8\` magic nominal, no solver/traits/general monomorphizer |
| DI-002 representation privacy; DI-006/007 loan/projection surface | **DEFER** source/private/ref expansion; **KEEP** existing scoped and visibility laws | no safe Ring exported, no child index/ref grammar, no concealment assumed |
| V528 tagged Option Ring vs raw-vacant M474, fixed four-field record | **KEEP reference DESIGN ONLY**; **DEFER** raw claim and fixed-record as substitutes | head/len not rights; raw vacant/extra Storage issuer absent; record doesn't guarantee \`sizeof(Option<T>)\` stride |
| Four explicit values vs \`Array<Option<u8>,4>::all_none()\` intrinsic | **SELECT explicit positional candidate; DEFER all-None** | explicit evaluation/type/value-use/arity/provenance, reuse with later non-None and N; all-None could be cheaper for this one zero-owner case but needs special implicit repeated initialization and does not test typed item transport |

**Why not \`all_none\`:** even though \`None\` has no resource payload, an all-None intrinsic adds a special implicit whole-container initializer and hides four distinct typed value issuances. Its generalization to arbitrary T/Some values is poor, and a type-generic "fill" operator might silently Copy a nonCopy \`Some(T)\` or treat a nonDiscardable \`None\` as freely discarded. A specifically guarded \`all_none\` could be sound, but its smaller *syntactic* cost for Ring4 alone is not enough to prefer it over source-authored four exact typed values, with no evidence that implementation of issuer/layout becomes simpler. No prohibition of future specialized all-None helper is imposed.

**PR273 collision handling:** use this **Draft17.30-derived unique appendix filename**; do **not** create/modify \`NewLang_v0_spec_Draft17_31.md\` or PR273's branch/DI-015. Add a separate **DI-016 PROPOSED (unmerged)** ledger record. The candidate and PR273 can coexist as unmerged proposals; if either later changes canonical, independent Coordination must **rebase/re-evaluate** applicability, not implicitly merge the two languages. No \`CURRENT_SPEC\` edit or Draft17.30 in-place mutation.

**Gate D:** candidate appendix and DI-016 ledger index are the only documentation changes in this DRAFT PR. **Gate E pending** independent Coordination review; no source/semantic selection became canonical, no Production/P/R/F/CI authorization. **INTENTIONALLY REPLACE: NONE** of current canonical semantics; only candidate source admission proposed.

## 5. Investment and truthful boundary

**Qualitative cost** of a faithful implementation, not a measurement: bounded nested builtin type resolution plus exact positional source AST, four checked Option None value issuers, one original array lifecycle with four fixed element places/provenance, authenticated owned public view/validator, target-specific C sum/Array layout mapping and strict native/OOM/poison tests. Moderate–high total work; not "a parser alias" or a hardcoded C fixture. **First cost STOP:** if the first source compiler cannot create concrete \`Option<u8>\` and \`Array<Option<u8>,4>\` built-in identities, one Array original placement and child facts **without** general user generics/monomorphization/trait resolution, a new raw owner grant or a second indexed access family, **HOLD** and stop P before codegen. No code is implemented here.

**VALUE-ALIGNMENT-RISK (separate from design technical claim):** A true four-None constructor is a reusable CAPABILITY stepping stone, not proof of reduced human audit/re-audit scope or safe original nonCopy FULL-refund Ring operation. C/Zig can factor index/bounds behind library API and Rust \`[Option<OwnedFrame>;4]\` with private fields/moves/Drop is a strong matched physical competitor; NewLang's explicit nonDiscardable None settlement may cost more. Do not buy a generic solver/owner engine simply to report four initialized cells. [Product Strategy §§1.1–1.2](../NewLang_Product_Value_Strategy.md) is unchanged.

**Goal Alignment (§4.3/4.3.1):**
1. **Goal:** an authentic contiguous one-root Option Array constructor reused by future NewLang-native Ring4 and other fixed typed collections.
2. **Evidence:** Draft17.30 conceptual Array/sum and V528 reference; **NO** present source/CHECK/owned/C17/native for this constructor; no Ring, external frame identity or measured audit burden.
3. **Alternatives:** explicit typed positional (candidate), typed all-None intrinsic (defer), fixed four-field queue (different contiguity), no new Array feature/intrusive/arena pivot; compare C/Zig/Rust at matched workload.
4. **Stop/owner:** cost/authority/general generic/representation contradictions → **HOLD**; **Coordination** alone independently reviews and may approve future source rule/next P gate; this DRAFT has no authority.

No code, tests, parser, compiler, CI, additional docs, current canonical, CURRENT_SPEC, PR273, other Track or main merge changes are authorized by this appendix.
