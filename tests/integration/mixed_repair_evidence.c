/* P294: actual source transactions, public owned inspection and sampled OOM.
 * No private state access, seeded roots, or synthetic matched authority. */
#include "../support/test.h"
#include "newlang/captured_closure.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include <stdlib.h>
#include <string.h>

static size_t allocation_calls, fail_at;
static bool injecting, hit;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool fail_allocation(void)
{
    if (!injecting)
        return false;
    ++allocation_calls;
    if (allocation_calls != fail_at)
        return false;
    hit = true;
    return true;
}
void *__wrap_malloc(size_t n)
{
    return fail_allocation() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t size)
{
    return fail_allocation() ? NULL : __real_calloc(n, size);
}
void *__wrap_realloc(void *p, size_t n)
{
    return fail_allocation() ? NULL : __real_realloc(p, n);
}

static bool unique_authorities(const NLSemanticContext *c)
{
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(c, &snapshot));
    for (NLValueId i = 1; i <= snapshot.values; ++i) {
        NLSemanticValueView a;
        NLSemanticTypeView type;
        CHECK(nl_semantic_value_view(c, i, &a) &&
              nl_semantic_type_view(c, a.type, &type));
        if (a.carrier == NL_CARRIER_ENDED ||
            (type.kind != NL_TYPE_ALLOCATION && a.type != 2))
            continue;
        for (NLValueId j = 1; j < i; ++j) {
            NLSemanticValueView b;
            CHECK(nl_semantic_value_view(c, j, &b));
            if (b.carrier != NL_CARRIER_ENDED && b.type == a.type)
                CHECK(type.kind == NL_TYPE_ALLOCATION
                          ? a.allocation_region != b.allocation_region
                          : a.domain != b.domain);
        }
    }
    return true;
}

static bool inspect(const NLCheckedFragment *f, size_t depth, size_t *frees,
                    size_t *calls, size_t *certificates)
{
    CHECK(f && depth < 24);
    const NLSemanticContext *c = nl_checked_context(f);
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_DEALLOCATE) {
            const NLCheckedNodeView *a =
                nl_checked_node_view(f, n->first_argument);
            const NLCheckedNodeView *raw =
                a ? nl_checked_node_view(f, a->next_argument) : NULL;
            NLSemanticValueView av, rv;
            CHECK(a && raw && a->result_count == 1 && raw->result_count == 1 &&
                  nl_semantic_value_view(c, a->results[0].value, &av) &&
                  nl_semantic_value_view(c, raw->results[0].value, &rv) &&
                  av.allocation_region &&
                  av.allocation_region == rv.occupancy.region &&
                  rv.occupancy.start == 0 && rv.occupancy.length);
            ++*frees;
            printf("release A=%zu R=%zu raw=%zu\n", a->results[0].value,
                   av.allocation_region, raw->results[0].value);
        }
        NLWholeValueCallView w;
        if (nl_checked_whole_value_call_view(f, i, &w)) {
            ++*calls;
            CHECK(w.entry && w.returned && w.received && w.count == 1);
            CHECK(unique_authorities(w.entry) &&
                  unique_authorities(w.returned) &&
                  unique_authorities(w.received));
            NLSemanticBindingView donor, moved, formal, receiver;
            CHECK(nl_semantic_binding_view(w.entry, w.donors[0], &donor) &&
                  nl_semantic_binding_view(w.returned, w.donors[0], &moved) &&
                  nl_semantic_binding_view(w.returned, w.parameters[0],
                                           &formal) &&
                  nl_semantic_binding_view(w.received, w.receiver, &receiver) &&
                  donor.availability == NL_AVAILABLE &&
                  moved.availability == NL_CONSUMED &&
                  formal.availability == NL_CONSUMED &&
                  receiver.availability == NL_AVAILABLE &&
                  receiver.value == w.result && donor.place != formal.place &&
                  formal.place != receiver.place);
            NLSemanticSnapshot before, after;
            CHECK(nl_semantic_snapshot(w.entry, &before) &&
                  nl_semantic_snapshot(w.returned, &after));
            printf("whole input=%zu result=%zu donor=%zu formal=%zu "
                   "receiver=%zu\n",
                   w.inputs[0], w.result, w.donors[0], w.parameters[0],
                   w.receiver);
            for (NLValueId v = 1; v <= before.values; ++v) {
                NLSemanticValueView a, b, d;
                NLSemanticTypeView t;
                CHECK(nl_semantic_value_view(w.entry, v, &a) &&
                      nl_semantic_type_view(w.entry, a.type, &t));
                if (a.carrier == NL_CARRIER_ENDED ||
                    (t.kind != NL_TYPE_ALLOCATION && a.domain == 0))
                    continue;
                CHECK(v <= after.values &&
                      nl_semantic_value_view(w.returned, v, &b) &&
                      nl_semantic_value_view(w.received, v, &d) &&
                      b.carrier != NL_CARRIER_ENDED &&
                      d.carrier != NL_CARRIER_ENDED && a.type == b.type &&
                      a.type == d.type &&
                      a.allocation_region == b.allocation_region &&
                      a.allocation_region == d.allocation_region &&
                      a.domain == b.domain && a.domain == d.domain);
                printf("original value=%zu A.R=%zu D=%zu carriers=%d,%d,%d\n",
                       v, a.allocation_region, a.domain, a.carrier, b.carrier,
                       d.carrier);
            }
        }
        NLCapturedClosureView closure;
        if (nl_checked_captured_closure_view(f, i, &closure)) {
            CHECK(nl_checked_captured_closure_validate(f, i) == NL_CHECK_OK);
            ++*certificates;
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body)
            CHECK(inspect(body, depth + 1, frees, calls, certificates));
        for (size_t arm = 0; arm < 2; ++arm) {
            body = nl_checked_match_arm(f, i, arm);
            if (body)
                CHECK(inspect(body, depth + 1, frees, calls, certificates));
        }
    }
    return true;
}

static bool registration(NLSemanticContext *c, const NLSyntaxTree *unit,
                         NLCheckStatus expected, const char *code, size_t fault,
                         size_t *count)
{
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(c, &before));
    const NLSyntaxTree *units[] = {unit};
    NLFunctionUnitDiagnostic diagnostic = {0};
    allocation_calls = 0;
    fail_at = fault;
    hit = false;
    injecting = true;
    const NLCheckStatus status =
        nl_semantic_register_function_unit(c, units, 1, &diagnostic);
    injecting = false;
    *count = allocation_calls;
    CHECK(status == (hit ? NL_CHECK_OUT_OF_MEMORY : expected));
    if (!hit && code)
        CHECK(diagnostic.diagnostic.diagnostic.code &&
              strcmp(diagnostic.diagnostic.diagnostic.code, code) == 0);
    CHECK(nl_semantic_snapshot(c, &after));
    if (status != NL_CHECK_OK)
        CHECK(memcmp(&before, &after, sizeof(before)) == 0);
    return true;
}

static bool run(const char *path, const char *expected_code)
{
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *entry = NULL;
    NLSemanticContext *c = NULL;
    NLCheckedFragment *artifact = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    const bool accepted = strcmp(expected_code, "accepted") == 0;
    const NLCheckStatus expected =
        accepted ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    size_t total = 0;
    CHECK(registration(c, unit, expected, accepted ? NULL : expected_code, 0,
                       &total));
    nl_semantic_destroy(c);
    c = NULL;
    const size_t faults[] = {1, 2, 8, 64, total / 2, total - 1, total};
    size_t samples = 0;
    for (size_t i = 0; i < sizeof(faults) / sizeof(faults[0]); ++i) {
        CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
        size_t count = 0;
        CHECK(registration(c, unit, expected, accepted ? NULL : expected_code,
                           faults[i], &count) &&
              hit);
        ++samples;
        CHECK(registration(c, unit, expected, accepted ? NULL : expected_code,
                           0, &count));
        nl_semantic_destroy(c);
        c = NULL;
    }
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    CHECK(registration(c, unit, expected, accepted ? NULL : expected_code, 0,
                       &total));
    size_t frees = 0, calls = 0, certificates = 0;
    if (accepted) {
        CHECK(nl_source_create("main()", 6, "p294-entry", &entry_source) ==
                  NL_SOURCE_OK &&
              nl_parser_create(entry_source, &entry_parser) == NL_PARSE_OK &&
              nl_parser_parse_expression_fragment(entry_parser, &entry, NULL) ==
                  NL_PARSE_OK &&
              nl_semantic_check_expression(c, entry, &artifact, NULL) ==
                  NL_CHECK_OK &&
              artifact && inspect(artifact, 0, &frees, &calls, &certificates) &&
              frees && calls);
    }
    printf("summary accepted=%d owned=%d frees_all_semantic_paths=%zu "
           "whole_calls=%zu public_certificates=%zu oom_samples=%zu "
           "registration_allocations=%zu\n",
           accepted, artifact != NULL, frees, calls, certificates, samples,
           total);
    nl_checked_destroy(artifact);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(entry);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}

int main(int argc, char **argv)
{
    return argc == 3 && run(argv[1], argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
