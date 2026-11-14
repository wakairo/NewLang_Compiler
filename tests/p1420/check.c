#include "../../src/copy_body_internal.h"
#include "../../src/h_origin_internal.h"
#include "../../src/syntax_internal.h"
#include "../../src/unchecked_ref_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include "newlang/if_field.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    CHECK(nl_source_create("main()", 6, "p1420-actual-call", &p->call_source) ==
          NL_SOURCE_OK);
    CHECK(nl_parser_create(p->call_source, &p->call_parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(p->call_parser, &p->call, NULL) ==
          NL_PARSE_OK);
    return true;
}
static bool define(Program *p)
{
    const NLSyntaxTree *units[] = {p->unit};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s =
        nl_semantic_register_function_unit(p->context, units, 1, &d);
    if (s != NL_CHECK_OK)
        fprintf(stderr, "definition=%d code=%s message=%s\n", (int)s,
                d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                             : "none",
                d.diagnostic.diagnostic.message
                    ? d.diagnostic.diagnostic.message
                    : "none");
    CHECK(s == NL_CHECK_OK);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    p->unit = NULL;
    p->parser = NULL;
    p->source = NULL;
    return true;
}
static bool actual(Program *p)
{
    NLCheckDiagnostic d = {0};
    NLCheckStatus s =
        nl_semantic_check_expression(p->context, p->call, &p->artifact, &d);
    if (s != NL_CHECK_OK)
        fprintf(stderr, "actual=%d code=%s message=%s\n", (int)s,
                d.diagnostic.code ? d.diagnostic.code : "none",
                d.diagnostic.message ? d.diagnostic.message : "none");
    CHECK(s == NL_CHECK_OK);
    return true;
}
static const NLCheckedFragment *find(const NLCheckedFragment *f,
                                     NLCheckedLocalAssumptionView *v)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        if (nl_checked_local_assumption_view(f, i, v))
            return f;
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->body_backed) {
            const NLCheckedFragment *child = nl_checked_call_body(f, i);
            const NLCheckedFragment *r = child ? find(child, v) : NULL;
            if (r)
                return r;
        }
        if (n->kind == NL_CHECKED_MATCH || n->kind == NL_CHECKED_IF)
            for (size_t a = 0; a < n->item_count; ++a) {
                const NLCheckedFragment *child =
                    n->kind == NL_CHECKED_IF ? nl_checked_if_arm(f, i, a)
                                             : nl_checked_match_arm(f, i, a);
                const NLCheckedFragment *r = child ? find(child, v) : NULL;
                if (r)
                    return r;
            }
    }
    return NULL;
}

static bool inject, hit, fail_malloc, fail_realloc, saw_qualified;
static size_t mode, observed_machine_rejects;
void *__real_malloc(size_t);
void *__wrap_malloc(size_t n)
{
    if (fail_malloc) {
        fail_malloc = false;
        hit = true;
        return NULL;
    }
    return __real_malloc(n);
}
void *__real_realloc(void *, size_t);
void *__wrap_realloc(void *p, size_t n)
{
    if (fail_realloc) {
        fail_realloc = false;
        hit = true;
        return NULL;
    }
    return __real_realloc(p, n);
}
NLCheckStatus __real_nl_h_unchecked_prepare(NLCheckedFragment *);
NLCheckStatus __wrap_nl_h_unchecked_prepare(NLCheckedFragment *f)
{
    if (inject && !hit && mode == 1 && !nl_h_unchecked_data(f))
        fail_malloc = true;
    NLCheckStatus s = __real_nl_h_unchecked_prepare(f);
    fail_malloc = false;
    return s;
}
NLCheckStatus __real_nl_sem_new_value(NLSemanticContext *, NLSemanticValueView,
                                      NLValueId *);
NLCheckStatus __wrap_nl_sem_new_value(NLSemanticContext *c,
                                      NLSemanticValueView v, NLValueId *out)
{
    const NLSemanticTypeView t = c->types[v.type - 1].view;
    const bool qualified = t.kind == NL_TYPE_REF && t.target &&
                           c->types[t.target - 1].recursive_header &&
                           v.reference.provenance == NL_PROVENANCE_UNKNOWN &&
                           v.reference.scope;
    if (inject && !hit &&
        ((mode == 2 && t.kind == NL_TYPE_PTR &&
          v.reference.provenance == NL_PROVENANCE_UNKNOWN) ||
         (mode == 4 && qualified) ||
         (mode == 5 && saw_qualified && t.kind == NL_TYPE_SUM && !v.variant)))
        fail_realloc = true;
    NLCheckStatus s = __real_nl_sem_new_value(c, v, out);
    if (s == NL_CHECK_OK && qualified)
        saw_qualified = true;
    fail_realloc = false;
    return s;
}
NLCheckStatus __real_nl_sem_copy_value(NLSemanticContext *, NLValueId,
                                       NLValueId *);
NLCheckStatus __wrap_nl_sem_copy_value(NLSemanticContext *c, NLValueId value,
                                       NLValueId *out)
{
    const NLSemanticValueView v = c->values[value - 1];
    if (inject && !hit && mode == 3 &&
        c->types[v.type - 1].view.kind == NL_TYPE_PTR &&
        v.reference.provenance == NL_PROVENANCE_UNKNOWN)
        fail_realloc = true;
    NLCheckStatus s = __real_nl_sem_copy_value(c, value, out);
    fail_realloc = false;
    return s;
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    if (inject && !hit && mode == 6 && world && nl_h_unchecked_present(child))
        fail_malloc = true;
    NLCheckStatus s = __real_nl_h_custody_seal_result(p, child, call, world);
    fail_malloc = false;
    return s;
}
bool __real_nl_h_unchecked_machine_only(const NLCheckedFragment *);
bool __wrap_nl_h_unchecked_machine_only(const NLCheckedFragment *f)
{
    const bool result = __real_nl_h_unchecked_machine_only(f);
    if (!result)
        ++observed_machine_rejects;
    return result;
}
static bool coarse;
static size_t coarse_hits;
NLCheckStatus __real_nl_unchecked_domain_target(const NLSemanticContext *,
                                                NLTypeId, NLDomainId,
                                                NLPlaceId *);
NLCheckStatus __wrap_nl_unchecked_domain_target(const NLSemanticContext *input,
                                                NLTypeId type, NLDomainId d,
                                                NLPlaceId *out)
{
    NLSemanticContext *c = (NLSemanticContext *)input;
    size_t index = c->place_count;
    NLDomainId saved = 0;
    if (coarse)
        for (size_t i = 0; i < c->place_count; ++i)
            if (c->places[i].live && c->places[i].independent_root &&
                c->places[i].type == type &&
                c->places[i].governing_domain != d) {
                index = i;
                saved = c->places[i].governing_domain;
                c->places[i].governing_domain = d;
                ++coarse_hits;
                break;
            }
    NLCheckStatus s = __real_nl_unchecked_domain_target(c, type, d, out);
    if (index < c->place_count)
        c->places[index].governing_domain = saved;
    return s;
}

static bool authentic(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p) && define(&p) && actual(&p));
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_AUTHOR_PREMISE);
    NLCheckedLocalAssumptionView v = {0};
    const NLCheckedFragment *f = find(p.artifact, &v);
    CHECK(f && v.owner == f &&
          v.assumed_conditions == NL_AUTHOR_PTR_TARGET_DOMAIN);
    CHECK(nl_checked_local_assumption_origin(f, v.operation) == NL_CHECK_OK);
    CHECK(v.stable_at_operation && v.original_live_at_operation &&
          v.loan_closed);
    NLSourceView text;
    CHECK(nl_source_view(v.source, v.span, &text) && text.length > 9 &&
          !memcmp(text.bytes, "unchecked", 9));
    const NLSemanticContext *c = nl_checked_context(f);
    NLSemanticValueView ptr, ref, result;
    CHECK(nl_semantic_value_view(c, v.pointer_value, &ptr) &&
          nl_semantic_value_view(c, v.reference_value, &ref) &&
          nl_semantic_value_view(c, v.read_value, &result));
    CHECK(ptr.reference.provenance == NL_PROVENANCE_UNKNOWN &&
          !ptr.reference.place && !ptr.reference.incarnation && !ptr.domain &&
          !ptr.allocation_region && !ptr.slot_place);
    CHECK(ref.reference.provenance == NL_PROVENANCE_UNKNOWN &&
          !ref.reference.place && ref.reference.scope == v.scope);
    CHECK(!result.scalar_known && !result.variant && !result.sum_payload);
    CHECK(!nl_checked_node_view(f, v.operation)->has_reference_result &&
          !nl_checked_node_view(f, v.pointer_argument)->has_reference_result &&
          !nl_checked_node_view(f, v.projection)->field.present);
    CHECK(!c->scopes[v.scope - 1].active && !c->domains[v.domain - 1].live &&
          !c->places[v.conditional_target - 1].live);
    const NLUncheckedProof *q = nl_h_unchecked_data(f);
    CHECK(q && q->has_pointer && q->copy_count &&
          q->pointer_result != v.pointer_value);
    const NLCheckedNodeView *condition = nl_checked_node_view(f, q->condition);
    NLSemanticValueView decision;
    CHECK(condition && condition->result_count == 1 &&
          nl_semantic_value_view(c, condition->results[0].value, &decision));
    printf("decision scalar_known=%d value=%zu; ptr correspondence remains "
           "UNKNOWN\n",
           decision.scalar_known, decision.scalar_value);
    observed_machine_rejects = 0;
    CHECK(nl_checked_copy_body_validate(f, v.operation, NULL) != NL_CHECK_OK);
    CHECK(observed_machine_rejects > 0);
    observed_machine_rejects = 0;
    CHECK(nl_checked_if_field_validate(f, v.pointer_if, NULL) != NL_CHECK_OK);
    CHECK(observed_machine_rejects > 0);
    char *out = NULL;
    size_t length = 98765;
    CHECK(nl_checked_c_node(p.artifact, &out, &length) ==
          NL_NODE_C_UNSUPPORTED);
    CHECK(!out && length == 98765);
    printf("AUTHOR_PREMISE actual ptr relation UNKNOWN; Copy edges=%zu; "
           "temporal loan closed, original ended; C17 NOT_RUN/unsupported\n",
           q->copy_count);
    destroy(&p);
    return true;
}
static bool compatibility(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p) && define(&p) && actual(&p));
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
    CHECK(nl_h_custody_result_valid(p.artifact, nl_checked_root(p.artifact)));
    destroy(&p);
    return true;
}
static bool negative(const char *path, const char *code)
{
    Program p = {0};
    CHECK(load(path, &p));
    const NLSyntaxTree *units[] = {p.unit};
    NLCopyCallAnchor anchor = {0};
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s =
        nl_semantic_register_function_unit(p.context, units, 1, &d);
    fprintf(stdout, "negative status=%d code=%s\n", (int)s,
            d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                         : "none");
    CHECK(s != NL_CHECK_OK && d.diagnostic.diagnostic.code &&
          !strcmp(d.diagnostic.diagnostic.code, code));
    CHECK(nl_copy_state_matches(p.context, &anchor));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
static bool syntax_negative(const char *path, const char *code)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLParseDiagnostic d = {0};
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &tree, &d) != NL_PARSE_OK);
    CHECK(!tree && d.diagnostic.code && !strcmp(d.diagnostic.code, code));
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
static bool oom(const char *path, size_t which)
{
    Program p = {0};
    CHECK(load(path, &p) && define(&p));
    NLCopyCallAnchor anchor = {0};
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
    mode = which;
    hit = saw_qualified = false;
    inject = true;
    NLCheckStatus s =
        nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL);
    inject = fail_malloc = fail_realloc = false;
    printf("OOM event=%zu hit=%d status=%d out=%p\n", mode, hit, (int)s,
           (void *)p.artifact);
    CHECK(hit && s == NL_CHECK_OUT_OF_MEMORY && !p.artifact);
    CHECK(nl_copy_state_matches(p.context, &anchor));
    nl_copy_state_destroy(&anchor);
    CHECK(actual(&p));
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_AUTHOR_PREMISE);
    destroy(&p);
    return true;
}
static bool coarse_event(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p) && define(&p));
    NLCopyCallAnchor anchor = {0};
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK);
    coarse = true;
    NLCheckDiagnostic d = {0};
    NLCheckStatus s =
        nl_semantic_check_expression(p.context, p.call, &p.artifact, &d);
    coarse = false;
    printf("coarse_hits=%zu status=%d expected_precision=%d out=%p\n",
           coarse_hits, (int)s, (int)NL_CHECK_ANALYSIS_PRECISION_LIMIT,
           (void *)p.artifact);
    CHECK(coarse_hits == 1 && s == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          !p.artifact);
    CHECK(d.diagnostic.code &&
          !strcmp(d.diagnostic.code, "UNCHECKED-DOMAIN-ROOT-PRECISION"));
    CHECK(nl_copy_state_matches(p.context, &anchor));
    nl_copy_state_destroy(&anchor);
    CHECK(actual(&p));
    destroy(&p);
    puts("real event coarse-D ambiguity rejected; no universal uniqueness law");
    return true;
}
static bool faults(const char *path)
{
    Program p = {0}, b = {0};
    CHECK(load(path, &p) && define(&p) && actual(&p));
    CHECK(load(path, &b) && define(&b) && actual(&b));
    NLCheckedLocalAssumptionView original = {0}, foreign = {0};
    NLCheckedFragment *f = (NLCheckedFragment *)find(p.artifact, &original);
    const NLCheckedFragment *g = find(b.artifact, &foreign);
    CHECK(f && g);
    NLUncheckedProof *q = (NLUncheckedProof *)nl_h_unchecked_data(f);
    const NLUncheckedProof *other = nl_h_unchecked_data(g);
#define DAMAGE(target, value, label)                                           \
    do {                                                                       \
        unsigned char saved[sizeof(target)];                                   \
        memcpy(saved, &(target), sizeof(target));                              \
        (target) = (value);                                                    \
        NLCheckedLocalAssumptionView got = {0};                                \
        CHECK(nl_checked_local_assumption_view(f, original.operation, &got));  \
        CHECK(!memcmp(&got, &original, sizeof(got)));                          \
        CHECK(nl_checked_local_assumption_origin(f, original.operation) !=     \
              NL_CHECK_OK);                                                    \
        CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_PROOF_INVALID); \
        memcpy(&(target), saved, sizeof(target));                              \
        CHECK(nl_checked_local_assumption_origin(f, original.operation) ==     \
              NL_CHECK_OK);                                                    \
        puts(label);                                                           \
    } while (0)
    DAMAGE(q->has_read, false, "missing marker/record rejected");
    DAMAGE(q->read.assumed_conditions, 0, "no assumption laundering");
    DAMAGE(q->read.span.start_byte, q->read.span.start_byte + 1,
           "span rejected");
    DAMAGE(q->read.operation, q->read.operation + 1, "operation rejected");
    DAMAGE(q->read.pointer_value, q->pointer_result,
           "other equal-Unknown package rejected");
    DAMAGE(q->read.owner, g, "foreign call owner rejected");
    DAMAGE(q->arms[0], other->arms[0], "foreign canonical arm recipe rejected");
    DAMAGE(q->copies[0].binding, q->copies[0].binding + 1,
           "Copy edge rejected");
    DAMAGE(f->context, g->context,
           "foreign world rejected, historical GET preserved");
    DAMAGE(f->source, g->source, "foreign source rejected");
    DAMAGE(f->arm_count, 0, "short arm view rejected");
    DAMAGE(f->arm_count, f->arm_count + 1, "long arm view rejected");
    NLSemanticContext *c = (NLSemanticContext *)f->context;
    NLFunctionEntry short_functions = {0};
    DAMAGE(c->functions, &short_functions,
           "short foreign signature storage rejected before indexing");
    DAMAGE(c->values[original.pointer_value - 1].reference.provenance,
           NL_PROVENANCE_VALID, "fake checked provenance rejected");
    NLSourceView text;
    CHECK(nl_source_view(original.source, original.span, &text));
    unsigned char *bytes = (unsigned char *)text.bytes;
    DAMAGE(bytes[0], (unsigned char)'x', "operation source bytes rejected");
    bool changed = false;
    for (NLSyntaxNode *n = f->body_owner ? f->body_owner->syntax->nodes : NULL;
         n; n = n->owned_next)
        if (n->unchecked_read) {
            DAMAGE(n->unchecked_read, false, "retained parser marker rejected");
            changed = true;
            break;
        }
    /* Arms borrow the body plan through the registered function table. */
    if (!changed)
        for (size_t i = 0; i < c->function_count && !changed; ++i) {
            NLFunctionBody *body = c->functions[i].body;
            for (NLSyntaxNode *n = body ? body->syntax->nodes : NULL; n;
                 n = n->owned_next)
                if (n->unchecked_read) {
                    DAMAGE(n->unchecked_read, false,
                           "retained function marker rejected");
                    changed = true;
                    break;
                }
        }
    CHECK(changed);
    NLCheckedFragment replay = *p.artifact;
    CHECK(nl_checked_proof_class(&replay) == NL_CHECKED_PROOF_INVALID);
    /* Do not restore the foreign display arm or recipe before A destruction.
     * Neither borrowed foreign B nor its record becomes A's owned child. */
    CHECK(f->arm_count && g->arm_count);
    f->arms[0] = g->arms[0];
    q->arms[0] = other->arms[0];
    destroy(&p);
    CHECK(nl_checked_proof_class(b.artifact) == NL_CHECKED_AUTHOR_PREMISE);
    CHECK(nl_checked_local_assumption_origin(g, foreign.operation) ==
          NL_CHECK_OK);
    destroy(&b);
    return true;
#undef DAMAGE
}
int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    bool ok = false;
    if (!strcmp(argv[1], "positive"))
        ok = authentic(argv[2]);
    else if (!strcmp(argv[1], "machine"))
        ok = compatibility(argv[2]);
    else if (!strcmp(argv[1], "faults"))
        ok = faults(argv[2]);
    else if (!strcmp(argv[1], "coarse"))
        ok = coarse_event(argv[2]);
    else if (!strcmp(argv[1], "negative") && argc == 4)
        ok = negative(argv[2], argv[3]);
    else if (!strcmp(argv[1], "syntax") && argc == 4)
        ok = syntax_negative(argv[2], argv[3]);
    else if (!strcmp(argv[1], "oom") && argc == 4)
        ok = oom(argv[2], (size_t)strtoul(argv[3], NULL, 10));
    return ok ? 0 : 1;
}
