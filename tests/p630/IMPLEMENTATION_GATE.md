# Before Production edits
The fixed-main arm has six source-issued nodes: BLOCK, literal/outer Copy,
BINDING, RECEIVER, copied tail, IF_ARM. It retains one consumed+hidden lexical
u8 local, its ended place, and its loose Copy tail. Existing IF lineage/source
identity and immutable original-prefix tables suffice for read-only consumption;
no public issuer or semantic policy change is proposed. Current first Pair
refusal: entry binding_count5 != expected4 (place_count7 !=6); pure control
similarly count4 !=3. Later three-node/no-block-items guard and ternary-only
emission are the same missing arm-local consumer, not new CFG semantics.

Plan: one bounded read-only arm-local consumer, retain all prefix comparisons
but delimit original prefix from ended local; fresh caller field facts must
use branch_fact_prefix, NEVER arm-local last_value_fact. C emits the genuine
local in its own if/else scope into a separate IF-result temporary, then the
original caller binding. Do not fold host-known branch truth or initializer.
Estimate <=250 relevant main.c lines plus tests; stop if new issuer, lifetime
rule, generic CFG or another independent mechanism is necessary. Original root,
field, loan admission is unchanged. Strict proof controls will cover local
membership/name/value/consumed state and cross-arm identity after AST teardown.
