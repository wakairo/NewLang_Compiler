/* Independent public checked-only expectations, after source/AST teardown.
 * Walk only the independently validated all-Some world for five originals
 * and its source field operations. Other terminal worlds are covered by
 * the certificate revalidator and native failure-path observer. */
#include "../../src/captured_read_internal.h"
#include "../../src/h_origin_internal.h"
#include "../support/node_checked.h"
#include "newlang/captured_closure.h"
#include <string.h>

typedef struct {
    NLPlaceId roots[5];
    size_t initialized, writes, calls;
    const NLCheckedFragment *first_body;
    NLScopeId first_scope;
} Observation;

static bool walk(const NLCheckedFragment *, Observation *);

static bool shared_call(const NLCheckedFragment *f, NLCheckedNodeId id,
                        const NLCheckedNodeView *call, Observation *o)
{
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    CHECK(body && nl_h_custody_result_valid(f, id));
    CHECK(o->initialized == 5 && o->calls < 2 &&
          o->writes == (o->calls == 0 ? 1u : 7u));
    NLCapturedChangeView event;
    CHECK(nl_checked_captured_change_count(body) == 1 &&
          nl_checked_captured_change_view(body, 0, &event));
    const NLCheckedNodeView *change =
        nl_checked_node_view(body, event.operation);
    const NLCheckedNodeView *field =
        nl_checked_node_view(body, change->first_argument);
    const NLCheckedField e = change->field;
    const NLScopeId scope = field->reference_result.scope;
    NLSemanticScopeView active, closed;
    CHECK(change->kind == NL_CHECKED_REPLACE && e.parent == o->roots[1] &&
          e.index == 1 && e.post_payload_occurrence != 0 &&
          nl_semantic_scope_view(event.before, scope, &active) &&
          active.active &&
          nl_semantic_scope_view(nl_checked_context(f), scope, &closed) &&
          !closed.active && call->result_count == 1 &&
          call->results[0].value == e.old_value);
    NLSemanticValueView displaced, installed, ptr;
    CHECK(nl_semantic_value_view(event.after, e.old_value, &displaced) &&
          displaced.carrier == NL_CARRIER_LOOSE &&
          displaced.variant == o->calls + 1 &&
          nl_semantic_value_view(event.after, e.new_value, &installed) &&
          installed.variant == 2 &&
          nl_semantic_value_view(event.after, installed.sum_payload, &ptr) &&
          ptr.reference.place == o->roots[o->calls == 0 ? 3 : 2]);
    if (o->calls == 0) {
        CHECK(displaced.sum_payload == 0 && e.payload_occurrence == 0);
        o->first_body = body;
        o->first_scope = scope;
    } else {
        NLSemanticOccurrenceView ended;
        CHECK(
            body != o->first_body &&
            body->body_owner == o->first_body->body_owner &&
            scope != o->first_scope && e.payload_occurrence != 0 &&
            e.payload_occurrence != e.post_payload_occurrence &&
            nl_semantic_occurrence_view(event.after, e.payload_occurrence,
                                        &ended) &&
            !ended.live &&
            nl_semantic_value_view(event.after, displaced.sum_payload, &ptr) &&
            ptr.reference.place == o->roots[3] &&
            ptr.reference.occurrence_dependency == 0);
    }
    printf("CALL %zu %zu %zu %zu %zu\n", call->function, o->writes + 1,
           e.parent, scope, displaced.variant);
    ++o->calls;
    return walk(body, o);
}

static bool walk(const NLCheckedFragment *f, Observation *o)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_INITIALIZE) {
            const NLCheckedNodeView *slot =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *h =
                nl_checked_node_view(f, slot->next_argument);
            const NLCheckedNodeView *field =
                nl_checked_node_view(f, h->first_argument);
            while (field != NULL && field->field_index != 3)
                field = nl_checked_node_view(f, field->next_argument);
            CHECK(field != NULL);
            const NLCheckedNodeView *scalar =
                nl_checked_node_view(f, field->initializer);
            CHECK(scalar != NULL && scalar->has_scalar_result &&
                  scalar->scalar_result.known && o->initialized < 5);
            o->roots[o->initialized++] = v->lifetime_place;
            printf("INIT %zu %zu %zu %zu %zu\n", v->backing, v->lifetime_place,
                   v->lifetime_incarnation, v->lifetime_domain,
                   scalar->scalar_result.value);
        }
        if (v->kind == NL_CHECKED_REF_FROM_PTR) {
            const NLCheckedNodeView *p =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *s =
                p ? nl_checked_node_view(f, p->next_argument) : NULL;
            CHECK(p && s && p->kind == NL_CHECKED_IDENTIFIER &&
                  s->kind == NL_CHECKED_IDENTIFIER);
            printf("ROOT %zu %zu %zu %zu %zu %zu %u\n", p->symbol, s->symbol,
                   v->lifetime_place, v->lifetime_incarnation,
                   v->lifetime_domain, v->reference_result.scope,
                   v->reference_result.writable ? 1u : 0u);
        }
        if (v->kind == NL_CHECKED_FIELD_REF)
            printf("FIELD %zu %zu %zu %zu %zu %zu %u\n", v->field.parent,
                   v->field.index, v->field.nominal, v->field.child,
                   v->field.child_incarnation, v->reference_result.scope,
                   v->reference_result.writable ? 1u : 0u);
        if (v->kind == NL_CHECKED_REPLACE) {
            ++o->writes;
            printf("CHANGE %zu %zu\n", v->field.payload_occurrence,
                   v->field.post_payload_occurrence);
        }
        if (v->kind == NL_CHECKED_REGISTERED_CALL)
            CHECK(shared_call(f, i, v, o));
        if (v->kind == NL_CHECKED_LINK_READ) {
            CHECK(f->captured_reads);
            const NLSemanticContext *after = NULL;
            for (size_t k = 0; k < f->captured_reads->count; ++k)
                if (f->captured_reads->events[k].event.node == i)
                    after = f->captured_reads->events[k].event.after;
            CHECK(after);
            printf("READ %zu %zu %zu %zu %zu %zu\n", v->field.parent,
                   v->field.index, v->field.child, v->field.child_fact,
                   v->field.payload_occurrence,
                   after->values[v->results[0].value - 1].variant);
        }
        if (v->kind == NL_CHECKED_MATCH) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, v->initializer);
            CHECK(init);
            if (init->kind == NL_CHECKED_TRY_ALLOCATE_ONE)
                CHECK(nl_checked_captured_closure_validate(f, i) ==
                      NL_CHECK_OK);
            for (size_t a = 0; a < v->item_count; ++a) {
                const NLCheckedFragment *arm = nl_checked_match_arm(f, i, a);
                CHECK(arm != NULL);
                const NLCheckedNodeView *root =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                if (root->variant == 2)
                    CHECK(walk(arm, o));
            }
        }
    }
    return true;
}
int main(int argc, char **argv)
{
    TestNode n = {0};
    const bool shared = argc == 3 && strcmp(argv[2], "--shared-writer") == 0;
    if (!(argc == 2 || shared) || !node_checked_load(argv[1], &n))
        return 1;
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    Observation o = {0};
    bool ok = body != NULL && walk(body, &o) && o.calls == (shared ? 2u : 0u);
    node_checked_destroy(&n);
    return ok ? 0 : 1;
}
