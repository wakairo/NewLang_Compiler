#include "../../src/captured_read_internal.h"
#include "../../src/h_origin_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *entry_syntax;
    NLSemanticContext *context;
    NLCheckedFragment *entry;
} Program;
static void destroy(Program *p)
{
    nl_checked_destroy(p->entry);
    nl_syntax_tree_destroy(p->entry_syntax);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
    nl_syntax_tree_destroy(p->unit);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    *p = (Program){0};
}
static NLCheckStatus prepare(const char *path, Program *p)
{
    if (nl_source_load(path, &p->source) != NL_SOURCE_OK ||
        nl_parser_create(p->source, &p->parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(p->parser, &p->unit, NULL) !=
            NL_PARSE_OK ||
        nl_semantic_create(&p->context) != NL_CHECK_OK)
        return NL_CHECK_INTERNAL_ERROR;
    NLSemanticSnapshot before, after;
    if (!nl_semantic_snapshot(p->context, &before))
        return NL_CHECK_INTERNAL_ERROR;
    const NLSyntaxTree *units[] = {p->unit};
    NLCheckStatus s =
        nl_semantic_register_function_unit(p->context, units, 1, NULL);
    if (s != NL_CHECK_OK) {
        if (!nl_semantic_snapshot(p->context, &after) ||
            memcmp(&before, &after, sizeof(before)))
            return NL_CHECK_INTERNAL_ERROR;
        return s;
    }
    if (nl_source_create("main()", 6, "p1387-actual-entry", &p->entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(p->entry_source, &p->entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(p->entry_parser, &p->entry_syntax,
                                            NULL) != NL_PARSE_OK ||
        !nl_semantic_snapshot(p->context, &before))
        return NL_CHECK_INTERNAL_ERROR;
    s = nl_semantic_check_expression(p->context, p->entry_syntax, &p->entry,
                                     NULL);
    if (s != NL_CHECK_OK && (!nl_semantic_snapshot(p->context, &after) ||
                             memcmp(&before, &after, sizeof(before))))
        return NL_CHECK_INTERNAL_ERROR;
    return s;
}
static NLCheckedNodeId match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_MATCH)
            return i;
    return 0;
}

static bool temporal(Program *, bool);
static bool foreign_destroy(const char *);
static bool oom(const char *);

static bool verify(const char *path)
{
    Program p = {0};
    CHECK(prepare(path, &p) == NL_CHECK_OK);
    CHECK(p.entry && nl_h_custody_validate(p.entry));
    const NLCheckedFragment *top =
        nl_checked_call_body(p.entry, nl_checked_root(p.entry));
    CHECK(top);
    NLCheckedNodeId first = match(top);
    CHECK(first &&
          nl_checked_captured_closure_validate(top, first) == NL_CHECK_OK);
    NLCapturedClosureView proof;
    CHECK(nl_checked_captured_closure_view(top, first, &proof));
    for (size_t i = 0; i <= 5; ++i)
        CHECK(proof.release_worlds[i] == 1);
    const NLCheckedFragment *leaf = top;
    for (size_t i = 0; i < 5; ++i) {
        leaf = nl_checked_match_arm(leaf, match(leaf), 1);
        CHECK(leaf);
    }
    CHECK(nl_checked_captured_change_count(leaf) == 9);
    size_t reads = 0, matches = 0;
    for (size_t i = 1; i <= nl_checked_node_count(leaf); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(leaf, i);
        if (n->kind == NL_CHECKED_LINK_READ)
            ++reads;
        if (n->kind == NL_CHECKED_MATCH)
            ++matches;
    }
    CHECK(reads == 1 && matches == 1);
    printf("CHECK/owned/custody PASS; original release worlds 0..5 once; "
           "reads=%zu matches=%zu\n",
           reads, matches);
    CHECK(temporal(&p, true));
    destroy(&p);
    CHECK(foreign_destroy(path));
    return true;
}
static const NLCheckedFragment *top_of(Program *p)
{
    return nl_checked_call_body(p->entry, nl_checked_root(p->entry));
}
static NLCheckedFragment *leaf_of(const NLCheckedFragment *top)
{
    const NLCheckedFragment *f = top;
    for (size_t i = 0; i < 5 && f; ++i)
        f = nl_checked_match_arm(f, match(f), 1);
    return (NLCheckedFragment *)f;
}
static bool denied(const NLCheckedFragment *entry,
                   const NLCheckedFragment *issued_top)
{
    /* Capture this borrowed identity BEFORE poisoning display views. Current
     * public getters may correctly refuse a foreign graph; both the original
     * proof and the real entry-call consumer must independently reject it. */
    const NLCheckedFragment *top = issued_top;
    CHECK(top);
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    char *out = NULL;
    size_t n = 98765;
    CHECK(nl_checked_c_node(entry, &out, &n) == NL_NODE_C_UNSUPPORTED &&
          out == NULL && n == 98765);
    return true;
}
static bool temporal(Program *p, bool attack)
{
    const NLCheckedFragment *top = top_of(p);
    NLCheckedFragment *leaf = leaf_of(top);
    CHECK(leaf && leaf->captured_reads && leaf->captured_reads->count == 2);
    NLCapturedChangeView last;
    CHECK(nl_checked_captured_change_view(leaf, 8, &last));
    NLCapturedClosureView ancestors;
    const NLCheckedFragment *parent = top;
    for (size_t i = 0; i < 4; ++i)
        parent = nl_checked_match_arm(parent, match(parent), 1);
    CHECK(nl_checked_captured_closure_view(parent, match(parent), &ancestors));
    const NLPlaceId c_root = ancestors.originals[3].root;
    CHECK(last.after->places[c_root - 1].live);
    ReadEvent *first = &leaf->captured_reads->events[0];
    ReadEvent *fork = &leaf->captured_reads->events[1];
    CHECK(first->kind == NL_CHECKED_LINK_READ &&
          fork->kind == NL_CHECKED_MATCH);
    const NLCheckedFragment *some = NULL;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedFragment *arm =
            nl_checked_match_arm(leaf, fork->event.node, i);
        CHECK(arm && arm == fork->arms[i]);
        if (nl_checked_node_view(arm, nl_checked_root(arm))->variant == 2)
            some = arm;
    }
    CHECK(some && some->captured_reads && some->captured_reads->count == 1);
    ReadEvent *second = &some->captured_reads->events[0];
    ReadEvent *events[] = {first, second};
    for (size_t i = 0; i < 2; ++i) {
        const NLSemanticContext *b = events[i]->event.before;
        const NLSemanticContext *a = events[i]->event.after;
        const NLCheckedFragment *f = i == 0 ? leaf : some;
        const NLCheckedNodeView *read =
            nl_checked_node_view(f, events[i]->event.node);
        CHECK(b && a && b != a && b != last.after &&
              !b->places[c_root - 1].live);
        CHECK(b->places[c_root - 1].governing_domain == 0 &&
              !b->domains[ancestors.originals[3].domain - 1].live &&
              !b->regions[3].view.live);
        CHECK(read->field.parent == ancestors.originals[i == 0 ? 1 : 2].root &&
              read->field.index == (i == 0 ? 1u : 0u));
        CHECK(b->places[read->field.parent - 1].live &&
              b->regions[i == 0 ? 1 : 2].view.live);
        const NLSemanticValueView result =
            a->values[read->results[0].value - 1];
        CHECK(result.dependencies == NL_DEPENDENCY_FREE &&
              result.value_dependency_count == 0);
        CHECK(result.variant == (i == 0 ? 2u : 1u));
        if (i == 0) {
            CHECK(result.sum_payload &&
                  a->values[result.sum_payload - 1].reference.place ==
                      ancestors.originals[2].root);
            CHECK(a->values[result.sum_payload - 1]
                      .reference.occurrence_dependency == 0);
        } else
            CHECK(result.sum_payload == 0);
        printf("READ %zu %zu %zu %zu %zu\n", read->field.parent,
               read->field.index, read->field.child, read->field.child_fact,
               result.variant);
    }
    if (!attack)
        return true;
    /* Point-in-time origin substitution cannot turn C-live or all-dead into
     * the required post-free read state. Owned origin slots remain intact. */
    NLSemanticContext *saved = first->event.before;
    first->event.before = (NLSemanticContext *)last.after;
    CHECK(denied(p->entry, top));
    first->event.before = saved;
    first->event.before = (NLSemanticContext *)leaf->context;
    CHECK(denied(p->entry, top));
    first->event.before = saved;
    NLCheckedNodeView *read = &leaf->nodes[first->event.node - 1];
    NLCheckedField field = read->field;
    ++read->field.child_fact;
    CHECK(denied(p->entry, top));
    read->field = field;
    read->field.index = 0;
    CHECK(denied(p->entry, top));
    read->field = field;
    NLSemanticValueView *copy =
        &first->event.after->values[read->results[0].value - 1];
    size_t variant = copy->variant;
    copy->variant = 1;
    CHECK(denied(p->entry, top));
    copy->variant = variant;
    NLSemanticPlaceView *root = &first->event.before->places[field.parent - 1];
    NLDomainId d = root->governing_domain;
    root->governing_domain = ancestors.originals[2].domain;
    CHECK(denied(p->entry, top));
    root->governing_domain = d;
    NLSemanticScopeView *scope = NULL;
    for (size_t i = 0; i < first->event.before->scope_count; ++i)
        if (first->event.before->scopes[i].active)
            scope = &first->event.before->scopes[i];
    CHECK(scope);
    scope->active = false;
    CHECK(denied(p->entry, top));
    scope->active = true;
    size_t count = leaf->captured_reads->count;
    leaf->captured_reads->count = 1;
    CHECK(denied(p->entry, top));
    leaf->captured_reads->count = count;
    ReadEvent *sr = &((NLCheckedFragment *)some)->captured_reads->events[0];
    const NLCheckedNodeView *sread = nl_checked_node_view(some, sr->event.node);
    for (size_t i = 0; i < some->count; ++i) {
        NLCheckedNodeView *node = &((NLCheckedFragment *)some)->nodes[i];
        if (node->kind == NL_CHECKED_REF_FROM_PTR) {
            NLCheckedNodeId arg = node->first_argument;
            NLCheckedNodeView *operand =
                &((NLCheckedFragment *)some)->nodes[arg - 1];
            NLSymbolId sym = operand->symbol;
            operand->symbol = 0;
            CHECK(denied(p->entry, top));
            operand->symbol = sym;
        }
    }
    CHECK(sread->kind == NL_CHECKED_LINK_READ);
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    printf("temporal origin/field/current/Copy/Domain/loan/coverage/binder "
           "poison rejection PASS\n");
    return true;
}
static bool foreign_destroy(const char *path)
{
    Program a = {0}, b = {0};
    CHECK(prepare(path, &a) == NL_CHECK_OK && prepare(path, &b) == NL_CHECK_OK);
    const NLCheckedFragment *at = top_of(&a), *bt = top_of(&b);
    NLCheckedFragment *al = leaf_of(at), *bl = leaf_of(bt);
    CHECK(al && bl);
    NLCheckedNodeId am = al->captured_reads->events[1].event.node;
    NLCheckedNodeId bm = bl->captured_reads->events[1].event.node;
    for (size_t i = 0; i < al->arm_count; ++i)
        if (al->arms[i].match == am) {
            const NLCheckedNodeView *ar = nl_checked_node_view(
                al->arms[i].artifact, nl_checked_root(al->arms[i].artifact));
            if (ar->variant == 2) {
                for (size_t j = 0; j < bl->arm_count; ++j)
                    if (bl->arms[j].match == bm) {
                        const NLCheckedNodeView *br = nl_checked_node_view(
                            bl->arms[j].artifact,
                            nl_checked_root(bl->arms[j].artifact));
                        if (br->variant == 2)
                            al->arms[i].artifact = bl->arms[j].artifact;
                    }
            }
        }
    CHECK(denied(a.entry, at));
    destroy(&a); /* deliberately unrestored foreign display view */
    CHECK(nl_h_custody_validate(b.entry) &&
          nl_checked_captured_closure_validate(bt, match(bt)) == NL_CHECK_OK);
    CHECK(temporal(&b, false));
    destroy(&b);
    printf("A unrestored foreign Some view destroy; genuine B "
           "survives/destroys PASS\n");
    return true;
}
static size_t capture_calls, wanted_call, wanted_offset, allocation_index;
static bool armed, injection_window, injected, fail_buffer;
static const NLSemanticContext *selected_world;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool lose(void)
{
    if (!armed || !injection_window)
        return false;
    if (allocation_index++ != wanted_offset)
        return false;
    injected = true;
    return true;
}
void *__wrap_malloc(size_t n)
{
    if (armed && fail_buffer && n == 131072) {
        injected = true;
        return NULL;
    }
    return lose() ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return lose() ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return lose() ? NULL : __real_realloc(p, n);
}
NLCheckStatus __real_nl_sem_clone(const NLSemanticContext *,
                                  NLSemanticContext **);
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    if (selected_world == NULL)
        return __real_nl_sem_clone(c, out);
    injection_window = c == selected_world;
    if (injection_window)
        allocation_index = 0;
    NLCheckStatus status = __real_nl_sem_clone(c, out);
    injection_window = false;
    return status;
}
NLCheckStatus __real_nl_captured_read_begin(NLCheckedFragment *, NLCheckedKind,
                                            const NLSemanticContext *);
NLCheckStatus __real_nl_captured_read_end(NLCheckedFragment *, NLCheckedNodeId,
                                          const NLSemanticContext *);
static void enter_capture(void)
{
    injection_window = ++capture_calls == wanted_call;
    allocation_index = 0;
}
NLCheckStatus __wrap_nl_captured_read_begin(NLCheckedFragment *f,
                                            NLCheckedKind kind,
                                            const NLSemanticContext *c)
{
    enter_capture();
    NLCheckStatus s = __real_nl_captured_read_begin(f, kind, c);
    injection_window = false;
    return s;
}
NLCheckStatus __wrap_nl_captured_read_end(NLCheckedFragment *f,
                                          NLCheckedNodeId n,
                                          const NLSemanticContext *c)
{
    enter_capture();
    NLCheckStatus s = __real_nl_captured_read_end(f, n, c);
    injection_window = false;
    return s;
}
static bool oom(const char *path)
{
    Program good = {0};
    capture_calls = 0;
    CHECK(prepare(path, &good) == NL_CHECK_OK);
    size_t total = capture_calls;
    CHECK(total >= 6 && total <= 48);
    destroy(&good);
    size_t reached = 0;
    for (size_t event = 1; event <= total; ++event)
        for (size_t off = 0; off < 4; ++off) {
            Program p = {0};
            wanted_call = event;
            wanted_offset = off;
            injected = false;
            capture_calls = 0;
            armed = true;
            NLCheckStatus s = prepare(path, &p);
            armed = false;
            CHECK(injected && s == NL_CHECK_OUT_OF_MEMORY && p.entry == NULL);
            destroy(&p);
            ++reached;
            CHECK(prepare(path, &p) == NL_CHECK_OK);
            destroy(&p);
            printf("OOM capture call=%zu offset=%zu status=OUT_OF_MEMORY "
                   "rollback/retry PASS\n",
                   event, off);
        }
    Program p = {0};
    CHECK(prepare(path, &p) == NL_CHECK_OK);
    const NLCheckedFragment *top = top_of(&p);
    const size_t offsets[] = {0, 1, 2, 3, 7, 15, 31, 63};
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        wanted_offset = offsets[i];
        allocation_index = 0;
        injected = false;
        armed = true;
        injection_window = true;
        NLCheckStatus s = nl_checked_captured_closure_validate(top, match(top));
        armed = injection_window = false;
        CHECK(injected && s == NL_CHECK_OUT_OF_MEMORY);
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
        ++reached;
        printf("OOM validator offset=%zu status=OUT_OF_MEMORY healthy retry "
               "PASS\n",
               offsets[i]);
    }
    NLCheckedFragment *leaf = leaf_of(top);
    CHECK(leaf && leaf->captured_reads);
    ReadEvent *fork = &leaf->captured_reads->events[1];
    const NLCheckedFragment *some = NULL;
    for (size_t i = 0; i < 2; ++i)
        if (nl_checked_node_view(fork->arms[i], nl_checked_root(fork->arms[i]))
                ->variant == 2)
            some = fork->arms[i];
    CHECK(some && some->captured_reads);
    const NLSemanticContext *worlds[] = {
        leaf->captured_reads->events[0].event.before, fork->event.before,
        some->captured_reads->events[0].event.before};
    for (size_t point = 0; point < 3; ++point)
        for (size_t off = 0; off < 4; ++off) {
            selected_world = worlds[point];
            wanted_offset = off;
            injected = false;
            armed = true;
            NLCheckStatus status =
                nl_checked_captured_closure_validate(top, match(top));
            armed = false;
            selected_world = NULL;
            CHECK(injected && status == NL_CHECK_OUT_OF_MEMORY);
            CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
                  NL_CHECK_OK);
            ++reached;
            printf("OOM actual temporal rederive point=%zu offset=%zu "
                   "OUT_OF_MEMORY retry PASS\n",
                   point, off);
        }
    for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        wanted_offset = offsets[i];
        allocation_index = 0;
        injected = false;
        armed = injection_window = true;
        char *out = NULL;
        size_t length = 98765;
        NLNodeCStatus status = nl_checked_c_node(p.entry, &out, &length);
        armed = injection_window = false;
        CHECK(injected && status == NL_NODE_C_OUT_OF_MEMORY && out == NULL &&
              length == 98765);
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
        ++reached;
        printf(
            "OOM emitter validation offset=%zu OUT_OF_MEMORY no-clobber PASS\n",
            offsets[i]);
    }
    injected = false;
    fail_buffer = armed = true;
    char *out = NULL;
    size_t length = 98765;
    NLNodeCStatus status = nl_checked_c_node(p.entry, &out, &length);
    armed = fail_buffer = false;
    CHECK(injected && status == NL_NODE_C_OUT_OF_MEMORY && out == NULL &&
          length == 98765);
    ++reached;
    CHECK(nl_checked_c_node(p.entry, &out, &length) == NL_NODE_C_OK && out &&
          length > 0);
    free(out);
    destroy(&p);
    printf("actual selected capture+validator+emitter OOM=%zu PASS\n", reached);
    return true;
}

/* C795: a historical checkpoint does not license changed operands of an
 * actual read. These are host artifact faults, never legal source negatives.
 * No poisoned C is compiled or executed. */
static bool current_read_denied(Program *p, const NLCheckedFragment *top,
                                size_t point, const char *fault)
{
    NLCheckStatus proof = nl_checked_captured_closure_validate(top, match(top));
    char *out = NULL;
    size_t length = 98765;
    NLNodeCStatus emitted = nl_checked_c_node(p->entry, &out, &length);
    printf("C795 point=%zu fault=%s validator=%d emitted=%d out_NULL=%d "
           "length=%zu\n",
           point, fault, (int)proof, (int)emitted, out == NULL, length);
    const bool rejected = proof == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                          emitted == NL_NODE_C_UNSUPPORTED && out == NULL &&
                          length == 98765;
    free(
        out); /* retain no buffer even if the unexpected-success oracle fails */
    return rejected;
}
static bool current_read_mutations(Program *p)
{
    CHECK(nl_h_custody_validate(p->entry));
    const NLCheckedFragment *top = top_of(p);
    CHECK(top &&
          nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    NLCheckedFragment *leaf = leaf_of(top);
    CHECK(leaf && leaf->captured_reads && leaf->captured_reads->count == 2);
    ReadEvent *fork = &leaf->captured_reads->events[1];
    NLCheckedFragment *some = NULL;
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm =
            nl_checked_match_arm(leaf, fork->event.node, a);
        CHECK(arm);
        if (nl_checked_node_view(arm, nl_checked_root(arm))->variant == 2)
            some = (NLCheckedFragment *)arm;
    }
    CHECK(some && some->captured_reads && some->captured_reads->count == 1);
    NLCheckedFragment *fragments[2] = {leaf, some};
    ReadEvent *events[2] = {&leaf->captured_reads->events[0],
                            &some->captured_reads->events[0]};
    for (size_t point = 0; point < 2; ++point) {
        NLCheckedFragment *f = fragments[point];
        ReadEvent *event = events[point];
        NLCheckedNodeView *read = &f->nodes[event->event.node - 1];
        NLSemanticContext *before = event->event.before;
        const NLValueId root_ref =
            before->bindings[read->field.base - 1].view.value;
        const NLCheckedNodeView *acquired = NULL;
        for (size_t i = 1; i < event->event.node; ++i) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, i);
            if (n->kind == NL_CHECKED_REF_FROM_PTR && n->result_count == 1 &&
                n->results[0].value == root_ref)
                acquired = n;
        }
        CHECK(acquired);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(f, acquired->first_argument);
        const NLCheckedNodeView *darg =
            arg ? nl_checked_node_view(f, arg->next_argument) : NULL;
        CHECK(arg && darg && arg->result_count == 1 && darg->result_count == 1);
        NLSemanticContext *current = (NLSemanticContext *)f->context;
        NLSemanticValueView *ptr = &current->values[arg->results[0].value - 1];
        NLSemanticValueView *domain =
            &current->values[darg->results[0].value - 1];
        const NLReferenceFacts pointer_saved = ptr->reference;
        const NLReferenceFacts domain_saved = domain->reference;
        CHECK(pointer_saved.readable && domain_saved.readable);
        bool rejected;

        /* Exact original FIRST mutation, at A then independently at B. */
        ptr->reference.readable = false;
        rejected =
            current_read_denied(p, top, point, "current-ptr-readable=false");
        ptr->reference = pointer_saved;
        CHECK(rejected);

        ptr->reference.place =
            nl_checked_node_view(fragments[1 - point],
                                 events[1 - point]->event.node)
                ->field.parent;
        CHECK(ptr->reference.readable &&
              ptr->reference.place != pointer_saved.place);
        rejected =
            current_read_denied(p, top, point, "readable-ptr-other-original");
        ptr->reference = pointer_saved;
        CHECK(rejected);

        ptr->reference.provenance = NL_PROVENANCE_UNKNOWN;
        CHECK(ptr->reference.readable);
        rejected = current_read_denied(p, top, point,
                                       "readable-ptr-unknown-provenance");
        ptr->reference = pointer_saved;
        CHECK(rejected);

        domain->reference.place = read->field.parent; /* Node is not D owner */
        CHECK(domain->reference.readable);
        rejected =
            current_read_denied(p, top, point, "readable-wrong-domain-place");
        domain->reference = domain_saved;
        CHECK(rejected);

        const NLCheckedField field_saved = read->field;
        ++read->field.child_fact;
        rejected =
            current_read_denied(p, top, point, "changed-current-field-fact");
        read->field = field_saved;
        CHECK(rejected);

        const NLScopeId scope = before->values[root_ref - 1].reference.scope;
        CHECK(scope && scope <= before->scope_count &&
              before->scopes[scope - 1].active);
        before->scopes[scope - 1].active = false;
        rejected = current_read_denied(p, top, point, "closed-checkpoint-loan");
        before->scopes[scope - 1].active = true;
        CHECK(rejected);

        /* Still readable in current view, but permission no longer agrees
         * with the actual saved operand. Never copy historical rights back. */
        NLReferenceFacts *saved_operand =
            &before->values[arg->results[0].value - 1].reference;
        const NLReferenceFacts issued = *saved_operand;
        saved_operand->readable = false;
        CHECK(ptr->reference.readable);
        rejected = current_read_denied(p, top, point,
                                       "checkpoint-permission-mismatch");
        *saved_operand = issued;
        CHECK(rejected);
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
    }
    char *out = NULL;
    size_t length = 98765;
    CHECK(nl_checked_c_node(p->entry, &out, &length) == NL_NODE_C_OK && out &&
          length > 0);
    free(out);
    puts("C795 current A/B read conformance: 14 independent faults rejected; "
         "original authority/restored C17 PASS");
    return true;
}
static bool current_auth(const char *path)
{
    Program p = {0};
    if (prepare(path, &p) != NL_CHECK_OK) {
        destroy(&p);
        return false;
    }
    const bool ok = current_read_mutations(&p);
    destroy(&p);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    const bool ok = strcmp(argv[1], "auth") == 0  ? current_auth(argv[2])
                    : strcmp(argv[1], "oom") == 0 ? oom(argv[2])
                                                  : verify(argv[2]);
    return ok ? 0 : 1;
}
