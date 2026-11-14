#include "../../src/copy_body_internal.h"
#include "../../src/h_origin_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

typedef struct {
    NLSource *source, *call_source;
    NLParser *parser, *call_parser;
    NLSyntaxTree *unit, *call;
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} Program;
static void destroy(Program *p)
{
    nl_checked_destroy(p->artifact);
    nl_syntax_tree_destroy(p->call);
    nl_parser_destroy(p->call_parser);
    nl_source_destroy(p->call_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    *p = (Program){0};
}
static bool load(const char *path, Program *p)
{
    CHECK(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(p->parser, &p->unit, NULL) ==
          NL_PARSE_OK);
    CHECK(nl_semantic_create(&p->context) == NL_CHECK_OK);
    CHECK(nl_source_create("main()", 6, "p1408-actual-call", &p->call_source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(p->call_source, &p->call_parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(p->call_parser, &p->call, NULL) ==
          NL_PARSE_OK);
    return true;
}
static NLCheckStatus define(Program *p, NLFunctionUnitDiagnostic *d)
{
    const NLSyntaxTree *units[] = {p->unit};
    return nl_semantic_register_function_unit(p->context, units, 1, d);
}

/* Select the three real ptr Copy allocations at this source IF: true operand,
 * false operand, and the imported parent result. The same selector is used
 * at definition and actual replay. It only injects allocator failure, never
 * substitutes semantic facts/status or changes the original source. */
static bool counting, fail_next, injected;
static size_t selected, wanted;
void *__real_realloc(void *, size_t);
void *__wrap_realloc(void *p, size_t n)
{
    if (fail_next) {
        fail_next = false;
        injected = true;
        return NULL;
    }
    return __real_realloc(p, n);
}
NLCheckStatus __real_nl_sem_copy_value(NLSemanticContext *, NLValueId,
                                       NLValueId *);
NLCheckStatus __wrap_nl_sem_copy_value(NLSemanticContext *c, NLValueId value,
                                       NLValueId *out)
{
    if (counting && value && value <= c->value_count &&
        c->types[c->values[value - 1].type - 1].view.kind == NL_TYPE_PTR &&
        c->value_count &&
        c->values[c->value_count - 1].carrier == NL_CARRIER_ENDED &&
        c->types[c->values[c->value_count - 1].type - 1].view.kind ==
            NL_TYPE_BOOL) {
        ++selected;
        if (wanted == selected)
            fail_next = true;
    }
    return __real_nl_sem_copy_value(c, value, out);
}

static bool ptr_value(const NLSemanticContext *c, NLValueId id,
                      NLReferenceFacts expected)
{
    NLSemanticValueView v;
    NLSemanticTypeView t;
    CHECK(nl_semantic_value_view(c, id, &v));
    CHECK(nl_semantic_type_view(c, v.type, &t) && t.kind == NL_TYPE_PTR &&
          t.is_copy);
    CHECK(v.dependencies == NL_DEPENDENCY_FREE && v.reference_count == 0 &&
          v.reference.place == expected.place &&
          v.reference.incarnation == expected.incarnation &&
          v.reference.provenance == NL_PROVENANCE_VALID &&
          v.reference.scope == 0 && v.reference.occurrence_dependency == 0 &&
          v.reference.readable == expected.readable &&
          v.reference.writable == expected.writable && v.domain == 0 &&
          v.slot_place == 0 && v.allocation_region == 0 &&
          v.occupancy.region == 0 && v.field_count == 0 && v.sum_payload == 0 &&
          !v.scalar_known);
    return true;
}
static bool walk(const NLCheckedFragment *f, size_t *choices)
{
    NLReferenceFacts original = {0};
    NLDomainId domain = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_INITIALIZE) {
            original = v->reference_result;
            domain = v->lifetime_domain;
        }
        if (v->kind == NL_CHECKED_IF) {
            CHECK(original.place && domain && v->result_count == 1 &&
                  v->normal_arms == 2);
            CHECK(ptr_value(nl_checked_context(f), v->results[0].value,
                            original));
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm = nl_checked_if_arm(f, i, a);
                CHECK(arm && arm->if_parent == f && arm->if_parent_node == i &&
                      arm->if_arm_index == a);
                const NLCheckedNodeView *root =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                CHECK(root && root->result_count == 1 && !root->terminates);
                CHECK(ptr_value(nl_checked_context(arm), root->results[0].value,
                                original));
                NLSemanticPlaceView place;
                NLSemanticDomainView d;
                CHECK(nl_semantic_place_view(nl_checked_context(arm),
                                             original.place, &place) &&
                      place.live && place.independent_root &&
                      place.incarnation == original.incarnation &&
                      place.governing_domain == domain);
                CHECK(nl_semantic_domain_view(nl_checked_context(arm), domain,
                                              &d) &&
                      d.live);
            }
            ++*choices;
        }
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
                CHECK(arm && walk(arm, choices));
            }
    }
    return true;
}
static bool owned(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p));
    CHECK(define(&p, NULL) == NL_CHECK_OK);
    CHECK(nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL) ==
          NL_CHECK_OK);
    CHECK(nl_h_custody_validate(p.artifact) &&
          nl_h_origin_validate(p.artifact));
    const NLCheckedFragment *body =
        nl_checked_call_body(p.artifact, nl_checked_root(p.artifact));
    size_t choices = 0;
    CHECK(body && walk(body, &choices) && choices == 1);
    char *out = NULL;
    size_t length = 98765;
    CHECK(nl_checked_c_node(p.artifact, &out, &length) ==
              NL_NODE_C_UNSUPPORTED &&
          out == NULL && length == 98765);
    destroy(&p);
    puts("definition/actual CHECK; genuine canonical owned arms; same original "
         "ptr Copy; C17 UNSUPPORTED/no-clobber PASS (native NOT_RUN)");
    return true;
}
static bool negative(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p));
    NLFunctionUnitDiagnostic d = {0};
    NLCopyCallAnchor anchor = {0};
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
    CHECK(define(&p, &d) == NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(strcmp(d.diagnostic.diagnostic.code, "P13-JOIN-PRECISION") == 0 &&
          nl_copy_state_matches(p.context, &anchor));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    puts("different-original precision refusal; no fabricated root PASS");
    return true;
}
static bool oom(const char *path)
{
    for (size_t phase = 0; phase < 2; ++phase) {
        for (size_t point = 1; point <= 3; ++point) {
            Program p = {0};
            CHECK(load(path, &p));
            if (phase)
                CHECK(define(&p, NULL) == NL_CHECK_OK);
            NLCopyCallAnchor anchor = {0};
            CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
            selected = 0;
            wanted = point;
            injected = false;
            counting = true;
            NLCheckStatus s = phase ? nl_semantic_check_expression(
                                          p.context, p.call, &p.artifact, NULL)
                                    : define(&p, NULL);
            counting = false;
            CHECK(injected && !fail_next && selected == point &&
                  s == NL_CHECK_OUT_OF_MEMORY && p.artifact == NULL &&
                  nl_copy_state_matches(p.context, &anchor));
            nl_copy_state_destroy(&anchor);
            if (!phase)
                CHECK(define(&p, NULL) == NL_CHECK_OK);
            CHECK(nl_semantic_check_expression(p.context, p.call, &p.artifact,
                                               NULL) == NL_CHECK_OK);
            CHECK(nl_h_custody_validate(p.artifact) &&
                  nl_h_origin_validate(p.artifact));
            destroy(&p);
            printf("OOM phase=%zu ptr-copy=%zu: OUT_OF_MEMORY, unchanged "
                   "world/out, rollback/retry PASS\n",
                   phase, point);
        }
    }
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    bool ok = !strcmp(argv[1], "owned")      ? owned(argv[2])
              : !strcmp(argv[1], "negative") ? negative(argv[2])
              : !strcmp(argv[1], "oom")      ? oom(argv[2])
                                             : false;
    return ok ? 0 : 1;
}
