#include "../../src/copy_body_internal.h"
#include "../../src/h_origin_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    NLSource *source, *call_source;
    NLParser *parser, *call_parser;
    NLSyntaxTree *unit, *call;
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} Program;

static unsigned fault;
static bool hit, in_clone;
void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t size)
{
    if (!hit && ((fault == 2 && in_clone) || (fault == 3 && size == 131072))) {
        hit = true;
        return NULL;
    }
    return __real_malloc(size);
}
void *__wrap_realloc(void *p, size_t size)
{
    if (!hit && fault == 1) {
        hit = true;
        return NULL;
    }
    return __real_realloc(p, size);
}
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    bool state = false;
    if (fault == 2 && !hit && c->region_count == 2)
        for (size_t i = 0; i < c->binding_count; ++i)
            state |= c->bindings[i].name && !c->bindings[i].hidden &&
                     !strcmp(c->bindings[i].name, "state") &&
                     c->bindings[i].view.availability == NL_AVAILABLE;
    const bool old = in_clone;
    in_clone = state;
    NLCheckStatus status = __real_nl_sem_clone(c, out);
    in_clone = old;
    return status;
}

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
    CHECK(nl_source_create("main()", 6, "p1435-actual", &p->call_source) ==
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
static bool ready(const char *path, Program *p)
{
    CHECK(load(path, p));
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = define(p, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "definition status=%d code=%s\n", (int)status,
                d.diagnostic.diagnostic.code);
    CHECK(status == NL_CHECK_OK);
    return true;
}
static bool actual(Program *p)
{
    NLCheckDiagnostic d = {0};
    NLCheckStatus status =
        nl_semantic_check_expression(p->context, p->call, &p->artifact, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "actual status=%d code=%s\n", (int)status,
                d.diagnostic.code);
    CHECK(status == NL_CHECK_OK && p->artifact);
    CHECK(nl_checked_proof_class(p->artifact) == NL_CHECKED_MACHINE_PROVED);
    CHECK(nl_h_custody_result_valid(p->artifact, nl_checked_root(p->artifact)));
    return true;
}

typedef struct {
    size_t bool_matches, copies, loans, payloads;
    NLSemanticValueView *decision;
    NLCheckedFragment *if_owner;
    NLCheckedNodeId if_id;
} Trace;
static bool inspect(NLCheckedFragment *f, Trace *trace, size_t depth)
{
    CHECK(f && depth < 16);
    const NLSemanticContext *c = nl_checked_context(f);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        CHECK(n);
        if (n->kind == NL_CHECKED_MATCH &&
            n->type == nl_semantic_core_type(c, NL_TYPE_BOOL)) {
            NLSemanticValueView v;
            CHECK(n->result_count == 1 && n->normal_arms == 2 &&
                  nl_semantic_value_view(c, n->results[0].value, &v));
            CHECK(!v.scalar_known && v.dependencies == NL_DEPENDENCY_FREE &&
                  !v.value_dependency_count && !v.reference_count &&
                  !v.owner_place && !v.domain);
            trace->decision =
                &((NLSemanticContext *)c)->values[n->results[0].value - 1];
            ++trace->bool_matches;
        }
        if (n->kind == NL_CHECKED_IF) {
            trace->if_owner = f;
            trace->if_id = id;
        }
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(n->loan.body_nonescape_proved &&
                  n->loan.normal_result_forwarded && n->loan.scope &&
                  n->loan.scope <= c->scope_count &&
                  !c->scopes[n->loan.scope - 1].active);
            ++trace->loans;
        }
        if (n->kind == NL_CHECKED_LINK_READ &&
            n->type == nl_semantic_core_type(c, NL_TYPE_U8)) {
            const NLCheckedNodeView *projection =
                nl_checked_node_view(f, n->first_argument);
            printf("payload field=%zu parent=%zu incarnation=%zu "
                   "projection-domain=%zu\n",
                   n->field.index, n->field.parent, n->field.parent_incarnation,
                   projection ? projection->lifetime_domain : 0);
            CHECK(n->field.present && n->field.index == 1 &&
                  n->field.access == NL_ACCESS_READ && projection &&
                  projection->kind == NL_CHECKED_FIELD_REF &&
                  projection->field.present &&
                  projection->field.index == n->field.index &&
                  projection->field.parent == n->field.parent &&
                  projection->field.child == n->field.child &&
                  projection->lifetime_domain &&
                  projection->lifetime_incarnation ==
                      n->field.parent_incarnation);
            ++trace->payloads;
        }
        NLCopyBodyView body;
        if (nl_checked_copy_body_view(f, id, &body)) {
            NLCopyBodyParameters p;
            NLCopyBodyEventView event;
            CHECK(nl_checked_copy_body_validate(f, id, &body) == NL_CHECK_OK &&
                  nl_checked_copy_body_parameters(f, id, &p) &&
                  nl_checked_copy_body_parameters_validate(f, id, &p) ==
                      NL_CHECK_OK &&
                  body.event_count == 1 &&
                  nl_checked_copy_body_event(f, id, 0, &event) &&
                  event.kind == NL_COPY_BODY_CHANGE && event.field.index == 0);
            NLSemanticValueView old, next;
            CHECK(nl_semantic_value_view(event.before, event.field.old_value,
                                         &old) &&
                  nl_semantic_value_view(event.after, event.field.new_value,
                                         &next));
            CHECK((!old.scalar_known || old.scalar_value == 31) &&
                  (!next.scalar_known || next.scalar_value == 197));
            ++trace->copies;
        }
        const NLCheckedFragment *child = nl_checked_call_body(f, id);
        if (child)
            CHECK(inspect((NLCheckedFragment *)child, trace, depth + 1));
        if (n->kind == NL_CHECKED_MATCH || n->kind == NL_CHECKED_IF)
            for (size_t a = 0; a < n->item_count; ++a) {
                child = n->kind == NL_CHECKED_MATCH
                            ? nl_checked_match_arm(f, id, a)
                            : nl_checked_if_arm(f, id, a);
                CHECK(inspect((NLCheckedFragment *)child, trace, depth + 1));
            }
    }
    return true;
}
static bool output(Program *p, NLNodeCStatus expected)
{
    char *text = NULL;
    size_t length = 98765;
    NLNodeCStatus status = nl_checked_c_node(p->artifact, &text, &length);
    printf("emit status=%d out=%s length=%zu\n", (int)status,
           text ? "text" : "NULL", length);
    CHECK(status == expected);
    if (expected == NL_NODE_C_OK)
        CHECK(text && length == strlen(text));
    else
        CHECK(!text && length == 98765);
    free(text);
    return true;
}
static bool positive(int argc, char **argv, bool emit)
{
    for (int i = 2; i < argc; ++i) {
        Program p = {0};
        Trace t = {0};
        CHECK(ready(argv[i], &p) && actual(&p) && inspect(p.artifact, &t, 0));
        CHECK(t.copies == 1 && t.payloads == 1 && t.loans > 0);
        CHECK(t.bool_matches == (i == 2 ? 0u : 3u));
        printf("%s MACHINE_PROVED Bool=%zu Copy=%zu payload=%zu "
               "closed-loans=%zu\n",
               argv[i], t.bool_matches, t.copies, t.payloads, t.loans);
        if (emit)
            CHECK(output(&p, NL_NODE_C_OK));
        destroy(&p);
    }
    return true;
}
static bool tamper(const char *a, const char *b)
{
    Program p = {0}, other = {0};
    Trace t = {0}, foreign = {0};
    CHECK(ready(a, &p) && actual(&p) && inspect(p.artifact, &t, 0));
    CHECK(ready(b, &other) && actual(&other) &&
          inspect(other.artifact, &foreign, 0));
    CHECK(t.decision && t.if_owner && foreign.if_owner);
    t.decision->scalar_known = true;
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_PROOF_INVALID);
    CHECK(output(&p, NL_NODE_C_UNSUPPORTED));
    t.decision->scalar_known = false;
    size_t edge = 0, other_edge = 0;
    while (edge < t.if_owner->arm_count &&
           t.if_owner->arms[edge].match != t.if_id)
        ++edge;
    while (other_edge < foreign.if_owner->arm_count &&
           foreign.if_owner->arms[other_edge].match != foreign.if_id)
        ++other_edge;
    CHECK(edge < t.if_owner->arm_count &&
          other_edge < foreign.if_owner->arm_count);
    const NLCheckedArm saved = t.if_owner->arms[edge];
    t.if_owner->arms[edge] = foreign.if_owner->arms[other_edge];
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_PROOF_INVALID);
    CHECK(output(&p, NL_NODE_C_UNSUPPORTED));
    t.if_owner->arms[edge] = saved;
    CHECK(nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
    destroy(&other);
    destroy(&p);
    return true;
}
static bool negative(const char *path, const char *control)
{
    Program p = {0}, retry = {0};
    CHECK(load(path, &p) && load(control, &retry));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status = define(&p, &d);
    printf("negative status=%d diagnostic=%s\n", (int)status,
           d.diagnostic.diagnostic.code);
    CHECK(status != NL_CHECK_OK && nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)));
    nl_copy_state_destroy(&anchor);
    const NLSyntaxTree *units[] = {retry.unit};
    CHECK(nl_semantic_register_function_unit(p.context, units, 1, &d) ==
          NL_CHECK_OK);
    CHECK(actual(&p));
    destroy(&p);
    destroy(&retry);
    return true;
}
static bool oom(const char *path, unsigned point)
{
    Program p = {0};
    CHECK(ready(path, &p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    hit = false;
    if (point == 3) {
        CHECK(actual(&p));
        fault = point;
        bool ok = output(&p, NL_NODE_C_OUT_OF_MEMORY);
        fault = 0;
        CHECK(ok && hit && output(&p, NL_NODE_C_OK));
    } else {
        fault = point;
        NLCheckStatus status =
            nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL);
        fault = 0;
        printf("CHECK OOM point=%u hit=%d status=%d out=%p\n", point, hit,
               (int)status, (void *)p.artifact);
        CHECK(hit && status == NL_CHECK_OUT_OF_MEMORY && !p.artifact &&
              nl_copy_state_matches(p.context, &anchor) &&
              nl_semantic_snapshot(p.context, &after) &&
              !memcmp(&before, &after, sizeof(before)));
        CHECK(actual(&p));
    }
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    bool ok = false;
    if (!strcmp(argv[1], "owned"))
        ok = positive(argc, argv, false);
    else if (!strcmp(argv[1], "emit"))
        ok = positive(argc, argv, true);
    else if (!strcmp(argv[1], "tamper") && argc == 4)
        ok = tamper(argv[2], argv[3]);
    else if (!strcmp(argv[1], "negative") && argc == 4)
        ok = negative(argv[2], argv[3]);
    else if (!strcmp(argv[1], "oom1"))
        ok = oom(argv[2], 1);
    else if (!strcmp(argv[1], "oom2"))
        ok = oom(argv[2], 2);
    else if (!strcmp(argv[1], "oom3"))
        ok = oom(argv[2], 3);
    return ok ? 0 : 1;
}
