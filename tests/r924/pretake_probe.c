#include "copy_body_internal.h"
#include "if_field_internal.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { NO_MUTATION, SWAP, MATCH, ORIGIN } Mutation;
typedef struct {
    const char *path, *name;
    unsigned old, newer[2], sibling, other[2];
    size_t index;
} Case;
typedef struct {
    NLSource *source;
    NLSyntaxTree *tree;
    NLSemanticContext *context;
} Input;
static Mutation injected;
static bool armed, failed;
static unsigned take_hits, call_hits, attach_hits;
static const char *name;

static void fail(const char *cause)
{
    fprintf(stderr, "R924 STOP: %s\n", cause);
    failed = true;
}
static const char *kind(Mutation m)
{
    switch (m) {
    case SWAP: return "swap";
    case MATCH: return "match";
    case ORIGIN: return "context_origin";
    default: return "none";
    }
}
NLDefinitionOwnedPending *__real_nl_definition_owned_call(
    const NLCheckedFragment *, NLCheckedNodeId, const NLCheckedNodeId[2],
    size_t, const NLSemanticContext *);
NLDefinitionOwnedPending *__wrap_nl_definition_owned_call(
    const NLCheckedFragment *f, NLCheckedNodeId call,
    const NLCheckedNodeId args[2], size_t function,
    const NLSemanticContext *context)
{
    NLDefinitionOwnedPending *p =
        __real_nl_definition_owned_call(f, call, args, function, context);
    if (armed && p)
        ++call_hits;
    return p;
}
void __real_nl_definition_owned_attach(NLDefinitionOwnedPending *,
                                       const NLCheckedFragment *,
                                       const NLSymbolId[2],
                                       const NLSemanticContext *);
void __wrap_nl_definition_owned_attach(NLDefinitionOwnedPending *p,
                                       const NLCheckedFragment *body,
                                       const NLSymbolId formals[2],
                                       const NLSemanticContext *context)
{
    __real_nl_definition_owned_attach(p, body, formals, context);
    if (armed && p)
        ++attach_hits;
}
void __real_nl_definition_owned_take(NLCheckedFragment **, NLSemanticContext **);
void __wrap_nl_definition_owned_take(NLCheckedFragment **root,
                                     NLSemanticContext **context)
{
    if (!armed) {
        __real_nl_definition_owned_take(root, context);
        return;
    }
    armed = false;
    ++take_hits;
    NLCheckedFragment *parent = root ? *root : NULL;
    NLSemanticContext *world = context ? *context : NULL;
    if (!parent || !world || parent->context != world ||
        parent->arm_count != 2 || !parent->arms || !parent->if_fields ||
        call_hits != 2 || attach_hits != 2) {
        fail("pre-take parent/context/two pending calls not live");
        __real_nl_definition_owned_take(root, context);
        return;
    }
    NLCheckedArm before[2] = {parent->arms[0], parent->arms[1]};
    if (!before[0].artifact || !before[1].artifact ||
        before[0].artifact == before[1].artifact ||
        !before[0].context_origin || !before[1].context_origin ||
        before[0].context_origin == before[1].context_origin) {
        fail("two distinct original arms and worlds not found");
        __real_nl_definition_owned_take(root, context);
        return;
    }
    NLCheckedNodeId id = before[0].artifact->if_parent_node;
    const NLCheckedNodeView *node = nl_checked_node_view(parent, id);
    const NLIfFieldProof *proof = parent->if_fields;
    if (!id || before[0].match != id || before[1].match != id ||
        !node || node->kind != NL_CHECKED_IF || node->normal_arms != 2 ||
        before[0].artifact->if_parent != parent ||
        before[1].artifact->if_parent != parent ||
        before[0].artifact->if_arm_index != 0 ||
        before[1].artifact->if_arm_index != 1 ||
        proof->view.conditional != id ||
        proof->arms[0] != before[0].artifact ||
        proof->arms[1] != before[1].artifact ||
        nl_checked_if_arm(parent, id, 0) != before[0].artifact ||
        nl_checked_if_arm(parent, id, 1) != before[1].artifact) {
        fail("original source IF authority/getter was not authentic");
        __real_nl_definition_owned_take(root, context);
        return;
    }
    if (injected == SWAP) {
        parent->arms[0] = before[1];
        parent->arms[1] = before[0];
    } else if (injected == MATCH) {
        parent->arms[0].match = 0;
    } else if (injected == ORIGIN) {
        parent->arms[0].context_origin = before[1].context_origin;
    } else {
        fail("unknown injection");
        __real_nl_definition_owned_take(root, context);
        return;
    }
    bool changed = false;
    if (injected == SWAP)
        changed = nl_checked_if_arm(parent, id, 0) == before[1].artifact &&
                  nl_checked_if_arm(parent, id, 1) == before[0].artifact;
    else if (injected == MATCH)
        changed = nl_checked_if_arm(parent, id, 0) == before[1].artifact &&
                  nl_checked_if_arm(parent, id, 1) == NULL;
    else
        changed = nl_checked_if_arm(parent, id, 0) == before[0].artifact &&
                  parent->arms[0].context_origin ==
                      before[1].context_origin;
    fprintf(stderr,
            "R924 PRE-TAKE-HIT family=%s mutation=%s "
            "two_pending=1 canonical_changed=%d\n",
            name, kind(injected), (int)changed);
    if (!changed)
        fail("corruption did not affect canonical source edge");
    __real_nl_definition_owned_take(root, context);
    bool no_move = *root == parent && *context == world;
    fprintf(stderr,
            "R924 REAL-TAKE family=%s mutation=%s root_retained=%d "
            "context_retained=%d\n",
            name, kind(injected), (int)(*root == parent),
            (int)(*context == world));
    if (!no_move) {
        fail("unexpected ownership transfer; no unsafe restore");
        return;
    }
    parent->arms[0] = before[0];
    parent->arms[1] = before[1];
    if (nl_checked_if_arm(parent, id, 0) != before[0].artifact ||
        nl_checked_if_arm(parent, id, 1) != before[1].artifact)
        fail("restored canonical original arms disagree");
    fprintf(stderr, "R924 RESTORE family=%s mutation=%s before_cleanup=1\n",
            name, kind(injected));
}

static void dispose(Input *in)
{
    nl_semantic_destroy(in->context);
    nl_syntax_tree_destroy(in->tree);
    nl_source_destroy(in->source);
    *in = (Input){0};
}
static bool open_input(Input *in, const char *path)
{
    NLParser *parser = NULL;
    NLParseDiagnostic d = {0};
    bool ok = nl_source_load(path, &in->source) == NL_SOURCE_OK &&
              nl_parser_create(in->source, &parser) == NL_PARSE_OK &&
              nl_parser_parse_function_unit(parser, &in->tree, &d) ==
                  NL_PARSE_OK &&
              nl_semantic_create(&in->context) == NL_CHECK_OK;
    nl_parser_destroy(parser);
    return ok;
}
static NLDefinitionOwned *registered(Input *in)
{
    NLDefinitionOwned *w = NULL;
    if (nl_definition_owned_open(&w) != NL_CHECK_OK || !w) {
        fail("owned open failed");
        return NULL;
    }
    const NLSyntaxTree *inputs[] = {in->tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(in->context, inputs, 1,
                                           &diagnostic);
    if (status != NL_CHECK_ANALYSIS_PRECISION_LIMIT ||
        !diagnostic.diagnostic.diagnostic.code ||
        strcmp(diagnostic.diagnostic.diagnostic.code, "P6-JOIN-PRECISION"))
        fail("inherited source ordinary CHECK3/P6 changed");
    nl_definition_owned_close(w);
    return w;
}
static bool validate(const NLDefinitionOwned *w, const Case *t)
{
    if (!w || nl_definition_owned_status(w) != NL_CHECK_OK ||
        nl_definition_owned_count(w) != 2)
        return false;
    bool seen[2] = {false, false};
    for (size_t i = 0; i < 2; ++i) {
        const NLDefinitionOwnedLink *link = nl_definition_owned_link(w, i);
        NLDefinitionCallResult result = {0};
        if (!link || link->arm > 1 || seen[link->arm] ||
            link->field.index != t->index ||
            nl_definition_owned_validate(w, i, link, &result) != NL_CHECK_OK ||
            !result.pre_verified || !result.return_old_verified ||
            !result.loan_closed || !result.old_known ||
            !result.sibling_known || result.replacements != 1 ||
            result.old_value != t->old ||
            result.incoming != t->newer[link->arm] ||
            result.sibling != t->sibling ||
            link->field.parent == link->other ||
            link->field.child == link->other)
            return false;
        seen[link->arm] = true;
        NLSemanticPlaceView pre[3], post[3], other_a[3], other_b[3];
        if (!nl_copy_trio(link->pre, link->field.parent, pre) ||
            !nl_copy_trio(link->post, link->field.parent, post) ||
            !nl_copy_trio(link->pre, link->other, other_a) ||
            !nl_copy_trio(link->post, link->other, other_b))
            return false;
        size_t changed = t->index + 1, sibling = 2 - t->index;
        if (pre[0].current_fact == post[0].current_fact ||
            pre[changed].current_fact == post[changed].current_fact ||
            pre[sibling].current_fact != post[sibling].current_fact ||
            other_a[0].current_fact != other_b[0].current_fact)
            return false;
        for (size_t j = 0; j < 2; ++j) {
            NLSemanticValueView value = {0};
            if (!nl_semantic_value_view(link->pre,
                                        other_a[j + 1].current_value, &value) ||
                !value.scalar_known || value.scalar_value != t->other[j])
                return false;
        }
        fprintf(stderr,
                "R924 RETRY-POSITIVE family=%s arm=%zu old=%llu new=%llu "
                "sibling=%llu index=%zu\n",
                t->name, link->arm,
                (unsigned long long)result.old_value,
                (unsigned long long)result.incoming,
                (unsigned long long)result.sibling, link->field.index);
    }
    return seen[0] && seen[1];
}
static bool trial(const Case *t, Mutation m)
{
    Input input = {0};
    if (!open_input(&input, t->path)) {
        fail("parse/init");
        dispose(&input);
        return false;
    }
    NLSemanticContext *same_context = input.context;
    name = t->name;
    injected = m;
    armed = true;
    take_hits = call_hits = attach_hits = 0;
    NLDefinitionOwned *negative = registered(&input);
    armed = false;
    bool rejected = negative && take_hits == 1 && call_hits == 2 &&
                    attach_hits == 2 && input.context == same_context &&
                    nl_definition_owned_status(negative) != NL_CHECK_OK &&
                    nl_definition_owned_count(negative) == 0 &&
                    nl_definition_owned_link(negative, 0) == NULL;
    nl_definition_owned_destroy(negative);
    fprintf(stderr,
            "R924 REFUSAL family=%s mutation=%s hits=%u no_private_link=%d\n",
            t->name, kind(m), take_hits, (int)rejected);
    if (!rejected || failed) {
        dispose(&input);
        return false;
    }
    injected = NO_MUTATION;
    NLDefinitionOwned *retry = registered(&input);
    bool context_identical =
        input.context == same_context && retry &&
        nl_definition_owned_status(retry) == NL_CHECK_OK;
    /* The successful owned witness must survive source and registration
       teardown, unlike the failed checker-owned original. */
    dispose(&input);
    bool positive = context_identical && validate(retry, t);
    nl_definition_owned_destroy(retry);
    fprintf(stderr,
            "R924 SAME-CONTEXT-RETRY family=%s mutation=%s "
            "original_context=%d retained_positive=%d\n",
            t->name, kind(m), (int)context_identical, (int)positive);
    return positive && !failed;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    const Case cases[] = {
        {argv[1], "meridian-index0", 26, {63, 118}, 137, {79, 204}, 0},
        {argv[2], "glacier-index1", 75, {33, 142}, 229, {14, 186}, 1}};
    for (size_t i = 0; i < 2 && !failed; ++i)
        for (Mutation m = SWAP; m <= ORIGIN && !failed;
             m = (Mutation)(m + 1))
            if (!trial(&cases[i], m))
                fail("pre-take reject/restore/retry failed");
    return failed ? 1 : 0;
}
