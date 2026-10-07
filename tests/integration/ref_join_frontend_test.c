#include "../support/ref_join.h"

/* PW1: exact R3-01 source, with both concrete current-state variants. */
static bool payload_result(bool some, bool reversed)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, some));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(f.sem.context, &before));
    CHECK(join_ok(
        &f, reversed ? "let chosen=match r {None=>{fallback},Some(v)=>{v},};"
                     : "let chosen=match r {Some(v)=>{v},None=>{fallback},};"));
    CHECK(join_ok(&f, "observe(chosen);"));
    NLSemanticValueView chosen;
    CHECK(join_value(&f, "chosen", &chosen));
    CHECK(chosen.reference_count == (some ? 2 : 1) &&
          chosen.reference.place == 0);
    NLSemanticPlaceView root;
    CHECK(nl_semantic_place_view(f.sem.context, f.root, &root));
    bool found_payload = false, found_fallback = false;
    for (size_t i = 0; i < chosen.reference_count; ++i) {
        const NLReferenceFacts fact = chosen.references[i];
        CHECK(fact.scope <= before.scopes && fact.place <= before.places &&
              fact.occurrence_dependency <= before.occurrences &&
              fact.provenance == NL_PROVENANCE_VALID);
        found_fallback |= fact.place == f.a_place && fact.scope == f.a_scope;
        if (fact.occurrence_dependency != 0) {
            NLSemanticOccurrenceView occurrence;
            CHECK(nl_semantic_occurrence_view(
                f.sem.context, fact.occurrence_dependency, &occurrence));
            CHECK(fact.occurrence_dependency == root.payload_occurrence &&
                  fact.place == occurrence.payload_place &&
                  fact.scope == f.parent_scope && occurrence.root == f.root);
            found_payload = true;
        }
    }
    CHECK(found_fallback && found_payload == some);
    CHECK(nl_semantic_snapshot(f.sem.context, &after));
    CHECK(after.occurrences == before.occurrences &&
          after.scopes == before.scopes);
    /* Copy and call flow retain occurrence/provenance rather than laundering.
     */
    CHECK(join_ok(&f, "let copied=chosen;"));
    CHECK(join_ok(&f, "observe(copied);"));
    NLSemanticValueView copied;
    CHECK(join_value(&f, "copied", &copied) &&
          join_facts_equal(chosen, copied));
    if (some) {
        CHECK(join_reject(&f, "store(rw,Option::None)", NL_CHECK_SEMANTIC_ERROR,
                          "P6-OCCURRENCE-CONFLICT"));
        CHECK(join_reject(&f, "replace(rw,Option::None)",
                          NL_CHECK_SEMANTIC_ERROR, "P6-OCCURRENCE-CONFLICT"));
    } else {
        CHECK(join_ok(&f, "store(rw,Option::Some(x))"));
        CHECK(join_ok(&f, "observe(copied)"));
    }
    if (some) {
        /* Ending the fallback scope must not hide the still-live payload
         * alternative from a whole-sum conflict check. */
        CHECK(nl_semantic_end_scope(f.sem.context, f.a_scope) == NL_CHECK_OK);
        CHECK(join_reject(&f, "store(rw,Option::None)", NL_CHECK_SEMANTIC_ERROR,
                          "P6-OCCURRENCE-CONFLICT"));
    } else {
        CHECK(nl_semantic_end_scope(f.sem.context, f.parent_scope) ==
              NL_CHECK_OK);
        CHECK(join_ok(&f, "observe(copied)"));
    }
    nl_semantic_destroy(f.sem.context);
    return true;
}

/* PW2: two external refs; canonical alternatives do not depend on arm order.
 * PW4: either real incoming scope ending makes safe continuation illegal. */
static bool different_refs(void)
{
    for (size_t dead = 0; dead < 2; ++dead) {
        JoinFixture f = {0};
        CHECK(join_create(&f, true));
        CHECK(join_ok(
            &f, "let first=match r {Some(_)=>{fallback},None=>{other}};"));
        CHECK(join_ok(
            &f, "let reversed=match r {None=>{other},Some(_)=>{fallback}};"));
        NLSemanticValueView first, reversed;
        CHECK(join_value(&f, "first", &first) &&
              join_value(&f, "reversed", &reversed));
        CHECK(first.reference_count == 2 && join_facts_equal(first, reversed));
        CHECK(first.references[0].place != first.references[1].place);
        CHECK(join_ok(&f, "{observe(first);observe(reversed);}"));
        CHECK(nl_semantic_end_scope(f.sem.context,
                                    dead == 0 ? f.a_scope : f.b_scope) ==
              NL_CHECK_OK);
        CHECK(join_reject(&f, "observe(first)", NL_CHECK_SEMANTIC_ERROR,
                          "P3-DEAD-SCOPE"));
        CHECK(join_reject(&f, "observe(reversed)", NL_CHECK_SEMANTIC_ERROR,
                          "P3-DEAD-SCOPE"));
        nl_semantic_destroy(f.sem.context);
    }
    return true;
}

/* PW5: ignored payload creates no dependency in the escaping ref result. */
static bool wildcard(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    CHECK(join_ok(&f, "let chosen=match r "
                      "{Some(_)=>{fallback},None=>{other}};"));
    NLSemanticValueView chosen;
    CHECK(join_value(&f, "chosen", &chosen) && chosen.reference_count == 2);
    for (size_t i = 0; i < chosen.reference_count; ++i)
        CHECK(chosen.references[i].occurrence_dependency == 0);
    CHECK(join_ok(&f, "{store(rw,Option::None);observe(chosen);}"));
    /* Discarding an ordinary ref result must release its dependency packages.
     */
    CHECK(join_ok(&f, "store(rw,Option::Some(x));"));
    CHECK(join_ok(
        &f,
        "{let local=match r {Some(v)=>{v},None=>{fallback}};observe(local);}"));
    CHECK(join_ok(&f, "store(rw,Option::None);"));
    nl_semantic_destroy(f.sem.context);
    return true;
}

static bool failures(void)
{
    JoinFixture f = {0};
    CHECK(join_create(&f, true));
    CHECK(join_reject(&f, "let broken=match r {Some(v)=>{v},None=>{missing}};",
                      NL_CHECK_SEMANTIC_ERROR, "P3-UNKNOWN-BINDING"));
    CHECK(join_reject(
        &f,
        "match r "
        "{Some(_)=>{ptr_from_ref(fallback)},None=>{ptr_from_ref(other)}}",
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P6-JOIN-PRECISION"));
    CHECK(join_reject(&f, "let broken=match r {Some(v)=>{v},None=>{rw}};",
                      NL_CHECK_SEMANTIC_ERROR, "P6-RESULT-JOIN"));
    CHECK(join_reject(&f,
                      "match r {Some(_)=>{match r "
                      "{Some(_)=>{fallback},None=>{other}}},None=>{fallback}}",
                      NL_CHECK_SEMANTIC_UNSUPPORTED, "P6-NESTED-MATCH"));
    CHECK(join_ok(&f, "let chosen=match r {Some(v)=>{v},None=>{fallback}};"));
    CHECK(join_reject(&f, "ptr_from_ref(chosen)",
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                      "P7-SINGULAR-REF-PRECISION"));
    NLTypeId write_type;
    CHECK(nl_semantic_compound_type(f.sem.context, NL_TYPE_REF, f.sem.copy,
                                    NL_ACCESS_WRITE, false,
                                    &write_type) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.sem.context, "write", &write_type, 1,
                                        nl_semantic_unit_type(f.sem.context),
                                        false, false) == NL_CHECK_OK);
    CHECK(join_reject(&f, "write(chosen)", NL_CHECK_SEMANTIC_ERROR,
                      "P3-TYPE-MISMATCH"));
    nl_semantic_destroy(f.sem.context);
    f = (JoinFixture){0};
    CHECK(join_create(&f, true));
    CHECK(nl_semantic_compound_type(f.sem.context, NL_TYPE_REF, f.sem.copy,
                                    NL_ACCESS_WRITE, false,
                                    &write_type) == NL_CHECK_OK);
    CHECK(nl_semantic_register_function(f.sem.context, "write", &write_type, 1,
                                        nl_semantic_unit_type(f.sem.context),
                                        false, false) == NL_CHECK_OK);
    NLSymbolId symbol;
    CHECK(test_reference(f.sem.context, "wa", f.a_place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &symbol, NULL));
    CHECK(test_reference(f.sem.context, "wb", f.b_place, NL_TYPE_REF,
                         NL_ACCESS_WRITE, false, &symbol, NULL));
    CHECK(join_ok(&f, "let writable=match r {Some(_)=>{wa},None=>{wb}};"));
    CHECK(join_ok(&f, "observe(writable)"));
    CHECK(join_reject(&f, "store(writable,x)",
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                      "P7-SINGULAR-REF-PRECISION"));
    CHECK(join_reject(&f, "write(writable)", NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                      "P7-CALL-PRECISION"));
    CHECK(join_reject(
        &f, "let broken=match rw {Some(v)=>{store(v,x);v},None=>{wa}};",
        NL_CHECK_ANALYSIS_PRECISION_LIMIT, "P7-REF-JOIN-PRECISION"));
    nl_semantic_destroy(f.sem.context);
    return true;
}
int main(void)
{
    return payload_result(true, false) && payload_result(false, false) &&
                   payload_result(true, true) && payload_result(false, true) &&
                   different_refs() && wildcard() && failures()
               ? 0
               : 1;
}
