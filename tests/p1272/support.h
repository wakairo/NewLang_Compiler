#ifndef P1272_SUPPORT_H
#define P1272_SUPPORT_H
#define main p1272_cli_main
#include "main.c"
#undef main
#include "h_origin_internal.h"
#include <assert.h>

typedef struct {
    NLSource *source, *call;
    NLParser *parser, *call_parser;
    NLSyntaxTree *tree, *call_tree;
    NLSemanticContext *context;
    NLCheckedFragment *entry, *read_owner;
    NLCheckedNodeId read;
    size_t reads;
} PInput;

static void p_load(PInput *p, const char *path)
{
    NLParseDiagnostic d = {0};
    NLFunctionUnitDiagnostic u = {0};
    assert(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    assert(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    assert(nl_parser_parse_function_unit(p->parser, &p->tree, &d) ==
           NL_PARSE_OK);
    assert(nl_semantic_create(&p->context) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {p->tree};
    assert(nl_semantic_register_function_unit(p->context, inputs, 1, &u) ==
           NL_CHECK_OK);
    assert(nl_source_create("main()", 6, "P1272 genuine main", &p->call) ==
           NL_SOURCE_OK);
    assert(nl_parser_create(p->call, &p->call_parser) == NL_PARSE_OK);
    assert(nl_parser_parse_expression_fragment(p->call_parser, &p->call_tree,
                                               &d) == NL_PARSE_OK);
}
static NLCheckStatus p_check(PInput *p)
{
    NLCheckDiagnostic d = {0};
    return nl_semantic_check_expression(p->context, p->call_tree, &p->entry,
                                        &d);
}
static void p_dispose(PInput *p)
{
    nl_checked_destroy(p->entry);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->call_tree);
    nl_parser_destroy(p->call_parser);
    nl_source_destroy(p->call);
    nl_syntax_tree_destroy(p->tree);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
}
static void p_find(PInput *p, const NLCheckedFragment *f)
{
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    for (NLCheckedNodeId i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        assert(n);
        if (n->kind == NL_CHECKED_LINK_READ && n->type == u8) {
            assert(p->reads++ == 0);
            p->read_owner = (NLCheckedFragment *)f;
            p->read = i;
        }
        if (n->kind == NL_CHECKED_BINDING) {
            NLSemanticBindingView b;
            NLSourceView name;
            if (nl_semantic_binding_view(c, n->symbol, &b) && b.type == u8 &&
                nl_source_view(f->source, n->name, &name))
                printf("BIND %.*s %zu\n", (int)name.length,
                       (const char *)name.bytes, n->symbol);
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, i);
        if (body)
            p_find(p, body);
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
                assert(arm);
                p_find(p, arm);
            }
    }
}
static void p_validate(PInput *p, size_t expected)
{
    assert(nl_h_custody_validate(p->entry) && nl_h_origin_validate(p->entry));
    assert(nl_h_custody_result_valid(p->entry, nl_checked_root(p->entry)));
    if (!p->reads)
        p_find(p, p->entry);
    assert(p->reads == 1);
    const NLCheckedFragment *f = p->read_owner;
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *n = nl_checked_node_view(f, p->read);
    const NLCheckedNodeView *r =
        n ? nl_checked_node_view(f, n->first_argument) : NULL;
    NLSemanticValueView old, copy, ref;
    NLSemanticPlaceView root, child;
    NLSemanticTypeView type;
    assert(n && r && r->kind == NL_CHECKED_FIELD_REF && n->field.present &&
           n->field.index == 1 && n->field.access == NL_ACCESS_READ &&
           n->field.parent_fact && n->field.child_fact &&
           n->field.parent == r->lifetime_place &&
           n->field.parent_incarnation == r->lifetime_incarnation &&
           n->value_use == NL_VALUE_COPIED && n->result_count == 1 &&
           r->has_reference_result &&
           nl_semantic_type_view(c, r->type, &type) &&
           type.kind == NL_TYPE_REF && !type.is_exclusive &&
           type.access == NL_ACCESS_READ &&
           type.target == nl_semantic_core_type(c, NL_TYPE_U8) &&
           nl_semantic_value_view(c, n->field.old_value, &old) &&
           nl_semantic_value_view(c, n->results[0].value, &copy) &&
           nl_semantic_value_view(c, r->results[0].value, &ref) &&
           old.type == type.target && copy.type == type.target &&
           old.scalar_known && copy.scalar_known &&
           old.scalar_value == expected && copy.scalar_value == expected &&
           old.dependencies == NL_DEPENDENCY_FREE &&
           copy.dependencies == NL_DEPENDENCY_FREE &&
           n->field.old_value != n->results[0].value && ref.reference.scope &&
           ref.reference.place == n->field.child && ref.reference.readable &&
           !ref.reference.writable &&
           nl_semantic_place_view(c, n->field.parent, &root) &&
           nl_semantic_place_view(c, n->field.child, &child) &&
           root.incarnation == n->field.parent_incarnation &&
           child.incarnation == n->field.child_incarnation && !root.live &&
           !child.live && !root.fixed_field_count);
    bool loan_found = false;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *loan = nl_checked_node_view(f, id);
        const NLCheckedNodeView *body =
            loan ? nl_checked_node_view(f, loan->initializer) : NULL;
        if (loan->kind != NL_CHECKED_LOAN_HEADER || !body ||
            body->tail != p->read)
            continue;
        NLSemanticScopeView scope;
        assert(loan->loan.body_nonescape_proved &&
               loan->loan.normal_result_forwarded && !loan->loan.is_exclusive &&
               loan->loan.access == NL_ACCESS_READ &&
               loan->loan.domain == r->lifetime_domain &&
               loan->loan.scope == ref.reference.scope &&
               nl_semantic_scope_view(c, loan->loan.scope, &scope) &&
               !scope.active);
        loan_found = true;
    }
    assert(loan_found);
    /* A serialized observation is data, not an importable authority receipt. */
    printf("READ {\"root\":%zu,\"child\":%zu,\"incarnation\":%zu,\"field\":1,"
           "\"Domain\":%zu,\"scope\":%zu,\"old\":%zu,\"copy\":%zu,\"byte\":%zu,"
           "\"readonly\":true,\"loan_closed\":true,\"sealed\":true}\n",
           n->field.parent, n->field.child, n->field.parent_incarnation,
           r->lifetime_domain, ref.reference.scope, n->field.old_value,
           n->results[0].value, copy.scalar_value);
}
#endif
