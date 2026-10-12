#define main p338_unused_entry
#include "../p338/public_probe.c"
#undef main
#include "../../src/semantic_internal.h"
#include "probe.h"
#include <stdint.h>
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t allocation_count, failure = SIZE_MAX;
static unsigned failure_phase;
static bool phase_triggered;
static bool refuse(void)
{
    if (!injecting)
        return false;
    size_t index = allocation_count++;
    if (failure_phase && nl_test_p352_phase == failure_phase &&
        !phase_triggered) {
        phase_triggered = true;
        return true;
    }
    return index == failure;
}
void *__wrap_malloc(size_t n)
{
    return refuse() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return refuse() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return refuse() ? NULL : __real_realloc(p, n);
}

static NLCheckStatus public_validate(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    return nl_checked_captured_closure_validate(f, allocation_match(f));
}
static bool baseline(const NLCheckedFragment *entry,
                     const NLCapturedChange *const *tape, size_t length)
{
    (void)tape;
    (void)length;
    CHECK(public_validate(entry) == NL_CHECK_OK);
    return true;
}
static bool reject(const NLCheckedFragment *entry,
                   const NLCapturedChange *const *tape, size_t length,
                   const char *name, bool public_check)
{
    (void)tape;
    (void)length;
    (void)public_check;
    NLCheckStatus status = public_validate(entry);
    CHECK(status == NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    printf("{\"case\":\"%s\",\"public_validator_status\":%d,\"owned_ninth\":"
           "true}\n",
           name, status);
    return true;
}
static bool hostile(const NLCheckedFragment *entry, NLCheckedFragment *f,
                    const NLCapturedChange *const *original, size_t length)
{
    const NLCapturedChange *tape[9] = {0};
    memcpy(tape, original, length * sizeof(*tape));
    CHECK(length == 9);
    --f->field_change_count;
    CHECK(reject(entry, tape, length - 1, "missing_ninth_owned_record", true));
    ++f->field_change_count;
    CHECK(baseline(entry, original, length));
    NLCapturedChange *saved_last = f->field_changes[8];
    f->field_changes[8] = f->field_changes[7];
    CHECK(reject(entry, tape, length, "duplicate_ninth_owned_record", true));
    f->field_changes[8] = saved_last;
    CHECK(baseline(entry, original, length));
    NLCapturedChange *saved_previous = f->field_changes[7];
    f->field_changes[7] = saved_last;
    f->field_changes[8] = saved_previous;
    CHECK(reject(entry, tape, length, "reordered_ninth_owned_record", true));
    f->field_changes[7] = saved_previous;
    f->field_changes[8] = saved_last;
    CHECK(baseline(entry, original, length));
    NLCheckedNodeView *op = &f->nodes[tape[8]->node - 1];
    NLCheckedKind kind = op->kind;
    op->kind = NL_CHECKED_UNIT;
    CHECK(reject(entry, tape, length, "missing_checked_replace", true));
    op->kind = kind;
    CHECK(baseline(entry, original, length));
    NLSourceSpan span = op->span;
    op->span = f->nodes[tape[7]->node - 1].span;
    CHECK(reject(entry, tape, length, "spliced_source_span", false));
    op->span = span;
    NLCapturedChange *second = f->field_changes[8];
    NLCheckedNodeId node = second->node;
    second->node = original[0]->node;
    CHECK(reject(entry, tape, length, "duplicated_owned_event_node", true));
    second->node = node;
    CHECK(baseline(entry, original, length));
    const NLCheckedNodeView *first = nl_checked_node_view(f, tape[8]->node);
    NLCheckedNodeView *acquired = NULL;
    NLValueId root_ref =
        tape[8]->before->bindings[first->field.base - 1].view.value;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_REF_FROM_PTR &&
            f->nodes[i].result_count == 1 &&
            f->nodes[i].results[0].value == root_ref)
            acquired = &f->nodes[i];
    CHECK(acquired);
    NLDomainId saved_domain = acquired->lifetime_domain, other = 0;
    for (size_t i = 1; i <= tape[8]->before->domain_count; ++i)
        if (i != saved_domain && tape[8]->before->domains[i - 1].live) {
            other = i;
            break;
        }
    CHECK(other);
    acquired->lifetime_domain = other;
    CHECK(reject(entry, tape, length, "wrong_original_domain_proof", true));
    acquired->lifetime_domain = saved_domain;
    NLProvenance provenance = acquired->reference_result.provenance;
    acquired->reference_result.provenance = NL_PROVENANCE_UNKNOWN;
    CHECK(reject(entry, tape, length, "unknown_write_ref_provenance", true));
    acquired->reference_result.provenance = provenance;
    CHECK(baseline(entry, original, length));
    size_t old_event = length;
    for (size_t i = 0; i < length; ++i)
        if (f->nodes[tape[i]->node - 1].field.payload_occurrence) {
            old_event = i;
            break;
        }
    CHECK(old_event < length);
    NLOccurrenceId old_occ =
        f->nodes[tape[old_event]->node - 1].field.payload_occurrence;
    NLSemanticContext *after = f->field_changes[8]->after;
    bool live = after->occurrences[old_occ - 1].live;
    CHECK(!live);
    after->occurrences[old_occ - 1].live = true;
    CHECK(reject(entry, tape, length, "old_some_occurrence_revived", true));
    after->occurrences[old_occ - 1].live = live;
    CHECK(baseline(entry, original, length));
    NLCheckedNodeView *old_op = &f->nodes[tape[8]->node - 1];
    NLOccurrenceId post = old_op->field.post_payload_occurrence;
    old_op->field.post_payload_occurrence = old_occ;
    CHECK(reject(entry, tape, length, "nonfresh_post_occurrence_claim", true));
    old_op->field.post_payload_occurrence = post;
    NLPlaceId scalar = 0;
    for (size_t i = 1; i <= tape[8]->after->place_count; ++i) {
        const NLSemanticPlaceView p = tape[8]->after->places[i - 1];
        if (p.live && p.independent_root && !p.implicit_local &&
            p.placement.region && p.fixed_field_count == 4 &&
            i != first->field.parent) {
            scalar = p.fixed_fields[3];
        }
    }
    CHECK(scalar);
    NLValueFactId fact =
        f->field_changes[8]->after->places[scalar - 1].current_fact;
    f->field_changes[8]->after->places[scalar - 1].current_fact = 0;
    CHECK(reject(entry, tape, length, "unrelated_fourth_scalar_fact", true));
    f->field_changes[8]->after->places[scalar - 1].current_fact = fact;
    CHECK(baseline(entry, original, length));
    /* Counterexample: two individually VALID primitive snapshots with an
     * unrecorded Copy scalar change between them. Source-owned clones only;
     * no seed, no new Allocation/Domain and no published positive forgery. */
    NLSemanticContext *bad_before = NULL, *bad_after = NULL;
    CHECK(nl_sem_clone(second->before, &bad_before) == NL_CHECK_OK);
    NLValueId scalar_copy = 0;
    CHECK(nl_sem_copy_value(bad_before,
                            bad_before->places[scalar - 1].current_value,
                            &scalar_copy) == NL_CHECK_OK &&
          nl_fixed_change(bad_before, scalar, scalar_copy, false) ==
              NL_CHECK_OK &&
          nl_sem_validate(bad_before) == NL_CHECK_OK &&
          nl_sem_clone(bad_before, &bad_after) == NL_CHECK_OK &&
          nl_fixed_change(bad_after, op->field.child, op->field.new_value,
                          false) == NL_CHECK_OK &&
          nl_sem_validate(bad_after) == NL_CHECK_OK);
    NLCapturedChange saved = *second;
    NLCheckedField field = op->field;
    second->before = bad_before;
    second->before_origin = bad_before;
    second->after = bad_after;
    second->after_origin = bad_after;
    op->field.child_post_fact =
        bad_after->places[op->field.child - 1].current_fact;
    op->field.parent_post_fact =
        bad_after->places[op->field.parent - 1].current_fact;
    op->field.post_payload_occurrence =
        bad_after->places[op->field.child - 1].payload_occurrence;
    NLCheckStatus one = nl_test_p352_primitive(f, second);
    CHECK(one == NL_CHECK_OK);
    printf("{\"case\":\"disconnected_valid_primitive_pair\",\"individual_"
           "primitive_status\":%d,\"scalar_value_same_but_fact_fresh\":true}\n",
           one);
    CHECK(
        reject(entry, tape, length, "disconnected_valid_primitive_pair", true));
    *second = saved;
    op->field = field;
    nl_semantic_destroy(bad_before);
    nl_semantic_destroy(bad_after);
    CHECK(baseline(entry, original, length));
    /* An unknown side effect in the source gap must not be treated as Copy. */
    NLCheckedNodeView *between = NULL;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].span.start_byte >=
                nl_checked_node_view(f, tape[7]->node)->span.end_byte &&
            f->nodes[i].span.start_byte < op->span.start_byte) {
            between = &f->nodes[i];
            break;
        }
    CHECK(between);
    kind = between->kind;
    between->kind = NL_CHECKED_TAKE;
    CHECK(reject(entry, tape, length, "unmodelled_intermediate_effect", false));
    between->kind = kind;
    CHECK(baseline(entry, original, length));
    return true;
}
static bool oom(const NLCheckedFragment *entry,
                const NLCapturedChange *const *tape, size_t length)
{
    allocation_count = 0;
    injecting = true;
    NLCheckStatus status = public_validate(entry);
    injecting = false;
    CHECK(status == NL_CHECK_OK);
    size_t total = allocation_count;
    size_t samples[] = {0, 1, 2, 3, 7, 15, total / 2, total - 1};
    for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
        allocation_count = 0;
        failure = samples[i];
        injecting = true;
        status = public_validate(entry);
        injecting = false;
        failure = SIZE_MAX;
        CHECK(status == NL_CHECK_OUT_OF_MEMORY);
        CHECK(baseline(entry, tape, length));
        printf("{\"case\":\"public_validator_OOM\",\"fail_at\":%zu,\"total\":%"
               "zu,\"status\":%d,\"retry\":true}\n",
               samples[i], total, status);
    }
    return true;
}

static bool registration_oom(const NLSyntaxTree *t)
{
    const NLSyntaxTree *units[] = {t};
    size_t total = 0;
    for (size_t attempt = 0; attempt < 12; ++attempt) {
        NLSemanticContext *c = NULL;
        NLSemanticSnapshot before, after;
        NLFunctionUnitDiagnostic d = {0};
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK &&
              nl_semantic_snapshot(c, &before));
        nl_test_p352_reset_faults();
        allocation_count = 0;
        failure = SIZE_MAX;
        failure_phase = 0;
        phase_triggered = false;
        if (attempt >= 1 && attempt <= 3)
            failure_phase = (unsigned)attempt;
        if (attempt >= 4) {
            size_t samples[] = {0, 1, 2, 3, 7, 15, total / 2, total - 1};
            failure = samples[attempt - 4];
        }
        injecting = true;
        NLCheckStatus status =
            nl_semantic_register_function_unit(c, units, 1, &d);
        injecting = false;
        CHECK(nl_semantic_snapshot(c, &after));
        if (attempt == 0) {
            CHECK(status == NL_CHECK_OK);
            total = allocation_count;
        } else {
            if (status != NL_CHECK_OUT_OF_MEMORY ||
                memcmp(&before, &after, sizeof(before)))
                fprintf(stderr,
                        "P352_REG_OOM attempt=%zu status=%d unchanged=%d "
                        "phase=%u triggered=%d allocations=%zu\n",
                        attempt, status,
                        !memcmp(&before, &after, sizeof(before)), failure_phase,
                        phase_triggered, allocation_count);
            CHECK(status == NL_CHECK_OUT_OF_MEMORY &&
                  !memcmp(&before, &after, sizeof(before)));
            if (failure_phase)
                CHECK(phase_triggered);
            failure = SIZE_MAX;
            failure_phase = 0;
            nl_test_p352_reset_faults();
            CHECK(nl_semantic_register_function_unit(c, units, 1, &d) ==
                  NL_CHECK_OK);
            printf("{\"case\":\"registration_OOM\",\"attempt\":%zu,\"status\":%"
                   "d,\"ninth_phase\":%zu,\"snapshot_unchanged\":true,\"no_"
                   "owned_exposed\":true,\"same_context_retry\":true,\"total_"
                   "allocations\":%zu}\n",
                   attempt, status, attempt <= 3 ? attempt : 0, total);
        }
        failure = SIZE_MAX;
        failure_phase = 0;
        nl_semantic_destroy(c);
    }
    return true;
}

static bool entry_oom(NLSemanticContext *c, const NLSyntaxTree *t)
{
    size_t total = 0;
    for (size_t attempt = 0; attempt < 12; ++attempt) {
        NLSemanticSnapshot before, after;
        NLCheckedFragment *owned = NULL;
        NLCheckDiagnostic d = {0};
        CHECK(nl_semantic_snapshot(c, &before));
        nl_test_p352_reset_faults();
        allocation_count = 0;
        failure = SIZE_MAX;
        failure_phase = 0;
        phase_triggered = false;
        if (attempt >= 1 && attempt <= 3)
            failure_phase = (unsigned)attempt;
        if (attempt >= 4) {
            size_t samples[] = {0, 1, 2, 3, 7, 15, total / 2, total - 1};
            failure = samples[attempt - 4];
        }
        injecting = true;
        NLCheckStatus status = nl_semantic_check_expression(c, t, &owned, &d);
        injecting = false;
        CHECK(nl_semantic_snapshot(c, &after));
        if (attempt == 0) {
            CHECK(status == NL_CHECK_OK && owned &&
                  public_validate(owned) == NL_CHECK_OK);
            /* Warmup may populate an existing function-body cache. Count the
             * next same-context attempt, rather than inject past its end. */
            nl_checked_destroy(owned);
            owned = NULL;
            nl_test_p352_reset_faults();
            allocation_count = 0;
            injecting = true;
            status = nl_semantic_check_expression(c, t, &owned, &d);
            injecting = false;
            CHECK(status == NL_CHECK_OK && owned &&
                  public_validate(owned) == NL_CHECK_OK);
            total = allocation_count;
        } else {
            if (status != NL_CHECK_OUT_OF_MEMORY || owned != NULL ||
                memcmp(&before, &after, sizeof(before)))
                fprintf(stderr,
                        "P352_ENTRY_OOM attempt=%zu status=%d owned=%d "
                        "unchanged=%d allocations=%zu fail_at=%zu code=%s\n",
                        attempt, status, owned != NULL,
                        !memcmp(&before, &after, sizeof(before)),
                        allocation_count, failure,
                        d.diagnostic.code ? d.diagnostic.code : "");
            CHECK(status == NL_CHECK_OUT_OF_MEMORY && owned == NULL &&
                  !memcmp(&before, &after, sizeof(before)));
            if (failure_phase)
                CHECK(phase_triggered);
            failure = SIZE_MAX;
            failure_phase = 0;
            nl_test_p352_reset_faults();
            CHECK(nl_semantic_check_expression(c, t, &owned, &d) ==
                      NL_CHECK_OK &&
                  owned && public_validate(owned) == NL_CHECK_OK);
            const NLCheckedFragment *leaf =
                nl_checked_call_body(owned, nl_checked_root(owned));
            for (size_t k = 0; k < 5; ++k)
                leaf = nl_checked_match_arm(leaf, allocation_match(leaf), 1);
            CHECK(leaf && nl_checked_captured_change_count(leaf) == 9);
            printf("{\"case\":\"entry_OOM\",\"attempt\":%zu,\"status\":%d,"
                   "\"ninth_phase\":%zu,\"snapshot_unchanged\":true,\"failed_"
                   "owned_null\":true,\"same_context_owned_nine_retry\":true,"
                   "\"total_allocations\":%zu}\n",
                   attempt, status, attempt <= 3 ? attempt : 0, total);
        }
        failure = SIZE_MAX;
        failure_phase = 0;
        nl_checked_destroy(owned);
    }
    return true;
}

static bool test(const char *path, const char *negative)
{
    NLSource *s = NULL, *es = NULL;
    NLParser *p = NULL, *ep = NULL;
    NLSyntaxTree *t = NULL, *et = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *entry = NULL;
    CHECK(nl_source_load(path, &s) == NL_SOURCE_OK &&
          nl_parser_create(s, &p) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(p, &t, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(refusal_rollback(c, negative) && registration_oom(t));
    const NLSyntaxTree *units[] = {t};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckDiagnostic e = {0};
    CHECK(nl_semantic_register_function_unit(c, units, 1, &d) == NL_CHECK_OK &&
          nl_source_create("main()", 6, "<P352-entry>", &es) == NL_SOURCE_OK &&
          nl_parser_create(es, &ep) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(ep, &et, NULL) == NL_PARSE_OK &&
          entry_oom(c, et) &&
          nl_semantic_check_expression(c, et, &entry, &e) == NL_CHECK_OK &&
          entry);
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    for (size_t i = 0; i < 5; ++i) {
        NLCheckedNodeId m = allocation_match(f);
        CHECK(m);
        f = nl_checked_match_arm(f, m, 1);
        CHECK(f);
    }
    const NLCapturedChange *tape[9] = {0};
    for (size_t i = 0; i < f->field_change_count; ++i)
        tape[i] = f->field_changes[i];
    CHECK(f->field_change_count == 9 && inspect(entry, 9, s));
    CHECK(hostile(entry, (NLCheckedFragment *)f, tape, 9) &&
          oom(entry, tape, 9));
    NLCheckStatus status = NL_CHECK_OK;
    nl_checked_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(et);
    nl_parser_destroy(ep);
    nl_source_destroy(es);
    nl_syntax_tree_destroy(t);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    return status == NL_CHECK_OK;
}
int main(int argc, char **argv)
{
    return argc == 3 && test(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
