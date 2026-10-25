#include "copy_body_internal.h"
#include "if_field_internal.h"
#include "newlang/parser.h"
#include "newlang/source.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    NLSource *source;
    NLSyntaxTree *tree;
    NLSemanticContext *context;
} Input;
typedef struct {
    const char *path;
    unsigned old, incoming[2], sibling, other[2];
    size_t field_index;
} Trial;
typedef enum { NONE, HOST_CALLOC, PENDING_CALLOC, PRE_MALLOC, POST_MALLOC } Fault;
typedef enum { IDLE, OPEN_SESSION, PENDING_CALL, ATTACH_POST } Stage;
static Fault requested;
static Stage stage;
static bool cloning;
static size_t hits;
static bool failure;

static void fail(const char *what)
{
    fprintf(stderr, "R922 STOP %s\n", what);
    failure = true;
}
void *__real_calloc(size_t, size_t);
void *__real_malloc(size_t);
void *__wrap_calloc(size_t n, size_t sz)
{
    if (!hits && ((requested == HOST_CALLOC && stage == OPEN_SESSION) ||
                  (requested == PENDING_CALLOC && stage == PENDING_CALL))) {
        ++hits;
        fprintf(stderr, "R922 REACHED calloc fault stage=%d n=%zu sz=%zu\n",
                (int)stage, n, sz);
        return NULL;
    }
    return __real_calloc(n, sz);
}
void *__wrap_malloc(size_t sz)
{
    if (!hits && cloning &&
        ((requested == PRE_MALLOC && stage == PENDING_CALL) ||
         (requested == POST_MALLOC && stage == ATTACH_POST))) {
        ++hits;
        fprintf(stderr, "R922 REACHED nl_sem_clone malloc stage=%d sz=%zu\n",
                (int)stage, sz);
        return NULL;
    }
    return __real_malloc(sz);
}
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    cloning = true;
    NLCheckStatus s = __real_nl_sem_clone(c, out);
    cloning = false;
    return s;
}
NLCheckStatus __real_nl_definition_owned_open(NLDefinitionOwned **);
NLCheckStatus __wrap_nl_definition_owned_open(NLDefinitionOwned **p)
{
    stage = OPEN_SESSION;
    NLCheckStatus s = __real_nl_definition_owned_open(p);
    stage = IDLE;
    return s;
}
NLDefinitionOwnedPending *__real_nl_definition_owned_call(
    const NLCheckedFragment *, NLCheckedNodeId, const NLCheckedNodeId[2],
    size_t, const NLSemanticContext *);
NLDefinitionOwnedPending *__wrap_nl_definition_owned_call(
    const NLCheckedFragment *f, NLCheckedNodeId call,
    const NLCheckedNodeId args[2], size_t function, const NLSemanticContext *c)
{
    stage = PENDING_CALL;
    NLDefinitionOwnedPending *p =
        __real_nl_definition_owned_call(f, call, args, function, c);
    stage = IDLE;
    return p;
}
void __real_nl_definition_owned_attach(NLDefinitionOwnedPending *,
                                       const NLCheckedFragment *,
                                       const NLSymbolId[2],
                                       const NLSemanticContext *);
void __wrap_nl_definition_owned_attach(NLDefinitionOwnedPending *p,
                                       const NLCheckedFragment *body,
                                       const NLSymbolId args[2],
                                       const NLSemanticContext *c)
{
    stage = ATTACH_POST;
    __real_nl_definition_owned_attach(p, body, args, c);
    stage = IDLE;
}

static bool input_open(Input *in, const char *path)
{
    NLParser *p = NULL;
    NLParseDiagnostic d = {0};
    bool ok = nl_source_load(path, &in->source) == NL_SOURCE_OK &&
              nl_parser_create(in->source, &p) == NL_PARSE_OK &&
              nl_parser_parse_function_unit(p, &in->tree, &d) == NL_PARSE_OK &&
              nl_semantic_create(&in->context) == NL_CHECK_OK;
    nl_parser_destroy(p);
    if (!ok)
        fprintf(stderr, "R922 parse/init failed path=%s\n", path);
    return ok;
}
static void input_destroy(Input *in)
{
    nl_semantic_destroy(in->context);
    nl_syntax_tree_destroy(in->tree);
    nl_source_destroy(in->source);
    *in = (Input){0};
}
static NLCheckStatus register_input(Input *in)
{
    const NLSyntaxTree *unit[] = {in->tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s = nl_semantic_register_function_unit(in->context, unit, 1,
                                                          &d);
    fprintf(stderr, "R922 source-check status=%d code=%s\n", (int)s,
            d.diagnostic.diagnostic.code);
    if (requested == NONE &&
        (s != NL_CHECK_ANALYSIS_PRECISION_LIMIT ||
         strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION") != 0))
        fail("source CHECK3/P6 prediction not met");
    return s;
}
static NLDefinitionOwned *capture(Input *in)
{
    NLDefinitionOwned *owned = NULL;
    if (nl_definition_owned_open(&owned) != NL_CHECK_OK || !owned) {
        fail("private owned open failed");
        return NULL;
    }
    (void)register_input(in);
    nl_definition_owned_close(owned);
    return owned;
}
static bool scalar(const NLSemanticContext *c, NLPlaceId p, unsigned want)
{
    NLSemanticPlaceView place = {0};
    NLSemanticValueView value = {0};
    return nl_semantic_place_view(c, p, &place) &&
           nl_semantic_value_view(c, place.current_value, &value) &&
           value.scalar_known && value.scalar_value == want;
}
static bool positive(const NLDefinitionOwned *w, const Trial *t,
                     NLDefinitionOwnedLink links[2])
{
    if (nl_definition_owned_status(w) != NL_CHECK_OK ||
        nl_definition_owned_count(w) != 2)
        return false;
    bool seen[2] = {false, false};
    for (size_t i = 0; i != 2; ++i) {
        const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, i);
        NLDefinitionCallResult r = {0};
        if (!v || v->arm > 1 || seen[v->arm])
            return false;
        seen[v->arm] = true;
        links[v->arm] = *v;
        if (nl_definition_owned_validate(w, i, v, &r) != NL_CHECK_OK ||
            !r.pre_verified || !r.return_old_verified || !r.loan_closed ||
            !r.old_known || !r.sibling_known || r.replacements != 1 ||
            r.old_value != t->old || r.incoming != t->incoming[v->arm] ||
            r.sibling != t->sibling || v->field.index != t->field_index ||
            v->field.child == v->other || v->field.parent == v->other)
            return false;
        NLSemanticPlaceView pre3[3], post3[3], other0[3], other1[3];
        if (!nl_copy_trio(v->pre, v->field.parent, pre3) ||
            !nl_copy_trio(v->post, v->field.parent, post3) ||
            !nl_copy_trio(v->pre, v->other, other0) ||
            !nl_copy_trio(v->post, v->other, other1))
            return false;
        const size_t idx = t->field_index + 1, sib = 2 - t->field_index;
        if (pre3[idx].current_fact == post3[idx].current_fact ||
            pre3[0].current_fact == post3[0].current_fact ||
            pre3[sib].current_fact != post3[sib].current_fact ||
            other0[0].current_fact != other1[0].current_fact ||
            !scalar(v->pre, v->field.child, t->old) ||
            !scalar(v->post, v->field.child, t->incoming[v->arm]) ||
            !scalar(v->pre, v->sibling, t->sibling) ||
            !scalar(v->post, v->sibling, t->sibling))
            return false;
        for (size_t j = 0; j < 2; ++j)
            if (!scalar(v->pre, other0[0].fixed_fields[j], t->other[j]) ||
                !scalar(v->post, other1[0].fixed_fields[j], t->other[j]))
                return false;
        fprintf(stderr,
                "R922 PRIVATE arm=%zu old=%llu new=%llu sibling=%llu field=%zu positive\n",
                v->arm, (unsigned long long)r.old_value,
                (unsigned long long)r.incoming,
                (unsigned long long)r.sibling, v->field.index);
    }
    return seen[0] && seen[1];
}
static bool both_reject(const NLDefinitionOwned *w)
{
    for (size_t i = 0; i < 2; ++i) {
        const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, i);
        NLDefinitionCallResult r = {0};
        if (!v || nl_definition_owned_validate(w, i, v, &r) == NL_CHECK_OK)
            return false;
    }
    return true;
}
static bool one_reject(const NLDefinitionOwned *w, size_t index)
{
    const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, index);
    NLDefinitionCallResult r = {0};
    return v && nl_definition_owned_validate(w, index, v, &r) != NL_CHECK_OK;
}
static bool mutate_restore(NLDefinitionOwned *w, const Trial *t)
{
    NLDefinitionOwnedLink v[2];
    if (!positive(w, t, v))
        return false;
    NLCheckedFragment *parent = (NLCheckedFragment *)(uintptr_t)v[0].parent;
    if (!parent || v[1].parent != v[0].parent || parent->arm_count != 2 ||
        !parent->if_fields)
        return false;
    NLCheckedArm orig[2] = {parent->arms[0], parent->arms[1]};
    NLCheckedNodeId id = v[0].caller->if_parent_node;
    NLIfFieldProof *p = parent->if_fields;
    parent->arms[0] = orig[1];
    parent->arms[1] = orig[0];
    bool swapped = nl_checked_if_arm(parent, id, 0) == orig[1].artifact &&
                   nl_checked_if_arm(parent, id, 1) == orig[0].artifact &&
                   both_reject(w);
    parent->arms[0] = orig[0];
    parent->arms[1] = orig[1];
    if (!swapped || !positive(w, t, v))
        return false;
    parent->arms[0].match = 0;
    bool match_rejected = both_reject(w);
    parent->arms[0] = orig[0];
    if (!match_rejected || !positive(w, t, v))
        return false;
    parent->arms[0].context_origin = orig[1].context_origin;
    bool origin_rejected = both_reject(w);
    parent->arms[0] = orig[0];
    if (!origin_rejected || !positive(w, t, v))
        return false;
    const NLCheckedFragment *saved = p->arms[0];
    p->arms[0] = p->arms[1];
    bool proof_rejected = both_reject(w);
    p->arms[0] = saved;
    if (!proof_rejected || !positive(w, t, v))
        return false;
    fprintf(stderr,
            "R922 mutation whole-swap/match/context_origin/proof reject-restored\n");
    return true;
}
static bool test_positive(const Trial *t)
{
    Input in = {0};
    if (!input_open(&in, t->path)) {
        input_destroy(&in);
        return false;
    }
    NLDefinitionOwned *w = capture(&in);
    /* Owner must retain source/AST/checked graph after original input dies. */
    input_destroy(&in);
    if (!w)
        return false;
    bool ok = mutate_restore(w, t);
    nl_definition_owned_destroy(w);
    fprintf(stderr, "R922 owned source/context destroyed, owned cleaned\n");
    return ok;
}
static bool foreign(const Trial *t)
{
    Input first = {0}, second = {0};
    if (!input_open(&first, t->path) ||
        !input_open(&second, t->path)) {
        input_destroy(&first);
        input_destroy(&second);
        return false;
    }
    NLDefinitionOwned *a = capture(&first);
    NLDefinitionOwned *b = capture(&second);
    input_destroy(&first);
    input_destroy(&second);
    NLDefinitionOwnedLink l[2], foreign_links[2];
    bool ok = a && b && positive(a, t, l) && positive(b, t, foreign_links) &&
              l[0].call == foreign_links[0].call &&
              l[0].caller != foreign_links[0].caller &&
              l[0].parent != foreign_links[0].parent &&
              l[0].callee != foreign_links[0].callee &&
              nl_checked_source(l[0].caller) !=
                  nl_checked_source(foreign_links[0].caller);
    if (ok) {
        NLCheckedFragment *caller = (NLCheckedFragment *)(uintptr_t)l[0].caller;
        NLCheckedFragment *parent = (NLCheckedFragment *)(uintptr_t)l[0].parent;
        const NLCheckedFragment *callee_save = caller->bodies[0];
        caller->bodies[0] =
            (NLCheckedFragment *)(uintptr_t)foreign_links[0].callee;
        bool refused = one_reject(a, 0);
        caller->bodies[0] = (NLCheckedFragment *)(uintptr_t)callee_save;
        if (refused && positive(a, t, l)) {
            NLCheckedArm prior = parent->arms[0];
            parent->arms[0].artifact =
                (NLCheckedFragment *)(uintptr_t)foreign_links[0].caller;
            refused = both_reject(a);
            parent->arms[0] = prior;
            ok = refused && positive(a, t, l);
        } else
            ok = false;
    }
    nl_definition_owned_destroy(b);
    nl_definition_owned_destroy(a);
    if (ok)
        fprintf(stderr,
                "R922 genuine foreign independently owned equal numeric NodeId rejected/restored\n");
    return ok;
}
static bool fault(const Trial *t, Fault f)
{
    Input in = {0};
    if (!input_open(&in, t->path)) {
        input_destroy(&in);
        return false;
    }
    requested = f;
    hits = 0;
    NLDefinitionOwned *w = NULL;
    bool triggered = false;
    if (f == HOST_CALLOC) {
        triggered = nl_definition_owned_open(&w) == NL_CHECK_OUT_OF_MEMORY &&
                    w == NULL && hits == 1;
    } else {
        w = capture(&in);
        triggered = hits == 1 && w &&
                    nl_definition_owned_status(w) != NL_CHECK_OK &&
                    nl_definition_owned_count(w) == 0;
        nl_definition_owned_destroy(w);
        w = NULL;
    }
    requested = NONE;
    if (triggered) {
        w = capture(&in);
        NLDefinitionOwnedLink links[2];
        triggered = w && positive(w, t, links);
        nl_definition_owned_destroy(w);
    }
    input_destroy(&in);
    if (triggered)
        fprintf(stderr, "R922 reached OOM fault=%d rollback/same-context retry\n",
                (int)f);
    return triggered;
}
static bool negative(const char *path)
{
    Input in = {0};
    if (!input_open(&in, path)) {
        input_destroy(&in);
        return false;
    }
    NLDefinitionOwned *w = NULL;
    if (nl_definition_owned_open(&w) != NL_CHECK_OK) {
        input_destroy(&in);
        return false;
    }
    const NLSyntaxTree *units[] = {in.tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(in.context, units, 1, &d);
    nl_definition_owned_close(w);
    bool refused = nl_definition_owned_count(w) != 2;
    if (!refused) {
        refused = true;
        for (size_t i = 0; i < 2; ++i) {
            const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, i);
            NLDefinitionCallResult result = {0};
            if (nl_definition_owned_validate(w, i, v, &result) ==
                NL_CHECK_OK)
                refused = false;
        }
    }
    if (refused)
        fprintf(stderr, "R922 negative original-old issuer refused path=%s code=%d\n",
                path, (int)status);
    nl_definition_owned_destroy(w);
    input_destroy(&in);
    return refused;
}
int main(int argc, char **argv)
{
    if (argc < 4)
        return 2;
    const Trial t[2] = {
        {.path = argv[2],
         .old = 26,
         .incoming = {63, 118},
         .sibling = 137,
         .other = {79, 204},
         .field_index = 0},
        {.path = argv[3],
         .old = 75,
         .incoming = {33, 142},
         .sibling = 229,
         .other = {14, 186},
         .field_index = 1}};
    if (!strcmp(argv[1], "owned")) {
        for (size_t i = 0; i < 2; ++i)
            if (!test_positive(&t[i]))
                fail("positive owned corruption/restore/lifetime");
    } else if (!strcmp(argv[1], "foreign")) {
        for (size_t i = 0; i < 2; ++i)
            if (!foreign(&t[i]))
                fail("authentic independent foreign graph");
    } else if (!strcmp(argv[1], "fault")) {
        for (size_t i = 0; i < 2; ++i)
            for (Fault f = HOST_CALLOC; f <= POST_MALLOC; ++f)
                if (!fault(&t[i], f))
                    fail("reached host allocation/rollback retry");
    } else if (!strcmp(argv[1], "negative")) {
        for (int i = 4; i < argc; ++i)
            if (!negative(argv[i]))
                fail("source mutation original-old proof unexpectedly valid");
    } else
        return 2;
    return failure ? 1 : 0;
}
