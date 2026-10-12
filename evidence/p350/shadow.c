/* Finite non-authorizing chronology validator, no cJSON event/name/root order.
 * True primitive replay is reused; bijection, gap and currentness checks below
 * are independent. Existing 0..5 closure validation is explicitly reused. */
#include "shadow.h"
#include <string.h>

typedef struct {
    NLPlaceId root;
    NLIncarnationId incarnation;
    NLDomainId domain;
    NLBackingRange extent;
    NLValueId allocation, domain_value;
} Original;
static NLCheckStatus stop(P350Report *r, const char *reason,
                          NLCheckedNodeId node)
{
    r->reason = reason;
    r->node = node;
    return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
}
static bool inventory(const NLSemanticContext *c, const Original *orig,
                      size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        const Original o = orig[i];
        NLSemanticPlaceView p;
        NLSemanticValueView a, d;
        if (!nl_semantic_place_view(c, o.root, &p) || !p.live ||
            p.incarnation != o.incarnation || p.governing_domain != o.domain ||
            p.fixed_field_count != 4 ||
            memcmp(&p.placement, &o.extent, sizeof(o.extent)) ||
            !nl_semantic_value_view(c, o.allocation, &a) ||
            !nl_semantic_value_view(c, o.domain_value, &d) ||
            a.allocation_region != o.extent.region || d.domain != o.domain ||
            a.carrier != NL_CARRIER_PLACE || d.carrier != NL_CARRIER_PLACE)
            return false;
        NLSemanticPlaceView ah, dh;
        if (!nl_semantic_place_view(c, a.owner_place, &ah) ||
            !nl_semantic_place_view(c, d.owner_place, &dh) || !ah.live ||
            !dh.live || ah.current_value != o.allocation ||
            dh.current_value != o.domain_value)
            return false;
        size_t owners_a = 0, owners_d = 0;
        NLSemanticSnapshot snap;
        if (!nl_semantic_snapshot(c, &snap))
            return false;
        for (size_t j = 1; j <= snap.bindings; ++j) {
            NLSemanticBindingView b;
            if (!nl_semantic_binding_view(c, j, &b))
                return false;
            if (b.availability == NL_AVAILABLE) {
                owners_a += b.value == o.allocation;
                owners_d += b.value == o.domain_value;
            }
        }
        if (owners_a != 1 || owners_d != 1)
            return false;
    }
    return true;
}
static bool heap_same(const NLSemanticContext *a, const NLSemanticContext *b,
                      const Original *orig, size_t count, P350Report *r)
{
    for (size_t i = 0; i < count; ++i) {
        NLSemanticPlaceView x, y;
        if (!nl_semantic_place_view(a, orig[i].root, &x) ||
            !nl_semantic_place_view(b, orig[i].root, &y) ||
            memcmp(&x, &y, sizeof(x)))
            return false;
        for (size_t k = 0; k < x.fixed_field_count; ++k) {
            NLSemanticPlaceView u, v;
            NLSemanticValueView uv, vv;
            if (!nl_semantic_place_view(a, x.fixed_fields[k], &u) ||
                !nl_semantic_place_view(b, y.fixed_fields[k], &v) ||
                memcmp(&u, &v, sizeof(u)) ||
                !nl_semantic_value_view(a, u.current_value, &uv) ||
                !nl_semantic_value_view(b, v.current_value, &vv) ||
                memcmp(&uv, &vv, sizeof(uv)))
                return false;
            NLAggregateField field;
            NLSemanticTypeView type;
            if (!nl_semantic_aggregate_field_view(a, x.type, k, &field) ||
                !nl_semantic_type_view(a, field.type, &type))
                return false;
            if (type.kind == NL_TYPE_U8)
                ++r->scalar_checks;
        }
        NLSemanticValueView av, bv, dv, ev;
        if (!nl_semantic_value_view(a, orig[i].allocation, &av) ||
            !nl_semantic_value_view(b, orig[i].allocation, &bv) ||
            memcmp(&av, &bv, sizeof(av)) ||
            !nl_semantic_value_view(a, orig[i].domain_value, &dv) ||
            !nl_semantic_value_view(b, orig[i].domain_value, &ev) ||
            memcmp(&dv, &ev, sizeof(dv)))
            return false;
    }
    return true;
}
static bool copy_node(const NLCheckedFragment *f, const NLCheckedNodeView *n)
{
    NLSemanticTypeView t;
    switch (n->kind) {
    case NL_CHECKED_BLOCK:
    case NL_CHECKED_STATEMENT:
    case NL_CHECKED_UNIT:
    case NL_CHECKED_TYPE:
        return true;
    case NL_CHECKED_IDENTIFIER:
    case NL_CHECKED_BINDING:
    case NL_CHECKED_RECEIVER:
    case NL_CHECKED_U8_LITERAL:
    case NL_CHECKED_SUM_CONSTRUCTOR:
    case NL_CHECKED_AGGREGATE_FIELD:
        return n->type && nl_semantic_type_view(f->context, n->type, &t) &&
               t.is_copy;
    case NL_CHECKED_REF_FROM_PTR:
    case NL_CHECKED_FIELD_REF:
        return n->has_reference_result;
    case NL_CHECKED_LOAN_HEADER:
        return n->loan.access == NL_ACCESS_READ && !n->loan.is_exclusive &&
               n->loan.body_nonescape_proved && n->loan.prevent_lifetime_end;
    default:
        return false;
    }
}
static NLCheckStatus gap(const NLCheckedFragment *f,
                         const NLCapturedChange *previous,
                         const NLCapturedChange *next, const Original *orig,
                         size_t count, P350Report *r)
{
    const NLCheckedNodeView *p = nl_checked_node_view(f, previous->node),
                            *n = nl_checked_node_view(f, next->node);
    for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
        const NLCheckedNodeView *q = nl_checked_node_view(f, j);
        if (q->span.start_byte >= p->span.end_byte &&
            q->span.start_byte < n->span.start_byte && !copy_node(f, q))
            return stop(r, "UNMODELLED_INTERMEDIATE_SOURCE_EFFECT", j);
    }
    const NLCheckedNodeView *project =
        nl_checked_node_view(f, p->first_argument);
    NLSemanticScopeView ended;
    if (!project ||
        !nl_semantic_scope_view(next->before, project->reference_result.scope,
                                &ended) ||
        ended.active)
        return stop(r, "PREVIOUS_LOAN_NOT_CLOSED", previous->node);
    if (!heap_same(previous->after, next->before, orig, count, r))
        return stop(r, "ORIGINAL_HEAP_ADJACENCY", next->node);
    ++r->gaps_checked;
    return NL_CHECK_OK;
}
NLCheckStatus p350_shadow(const NLCheckedFragment *entry,
                          const NLCapturedChange *const *tape, size_t length,
                          P350Report *r)
{
    if (!entry || !r || !tape)
        return NL_CHECK_INTERNAL_ERROR;
    *r = (P350Report){.reason = "OK"};
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    if (!f)
        return stop(r, "SOURCE_OWNED_CALL_PATH", 0);
    const NLCheckedFragment *parents[NL_CAPTURED_MAX_RELEASES] = {0};
    NLCheckedNodeId matches[NL_CAPTURED_MAX_RELEASES] = {0};
    Original originals[NL_CAPTURED_MAX_RELEASES] = {0};
    size_t count = 0;
    for (;;) {
        NLCheckedNodeId match = 0;
        for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, j),
                                    *a =
                                        nl_checked_node_view(f, n->initializer);
            if (n->kind == NL_CHECKED_MATCH && a &&
                a->kind == NL_CHECKED_TRY_ALLOCATE_ONE) {
                if (match)
                    return stop(r, "MULTIPLE_UNMODELLED_FORKS", j);
                match = j;
            }
        }
        if (!match)
            break;
        if (count == NL_CAPTURED_MAX_RELEASES)
            return stop(r, "ORIGINAL_PROFILE_BOUND", match);
        NLCapturedClosureView closure;
        if (!nl_checked_captured_closure_view(f, match, &closure))
            return stop(r, "CAPTURED_ORIGIN", match);
        parents[count] = f;
        matches[count] = match;
        f = nl_checked_match_arm(f, match, 1);
        if (!f)
            return stop(r, "SOURCE_OWNED_ARM_PATH", match);
        size_t grants = 0, inits = 0, domains = 0;
        Original o = {0};
        for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(f, j);
            if (n->allocation_success) {
                o.allocation = n->allocation_authority;
                ++grants;
            }
            if (n->kind == NL_CHECKED_DOMAIN_CREATE && n->result_count == 1) {
                o.domain_value = n->results[0].value;
                ++domains;
            }
            if (n->kind == NL_CHECKED_INITIALIZE) {
                o.root = n->lifetime_place;
                o.incarnation = n->lifetime_incarnation;
                o.domain = n->lifetime_domain;
                o.extent = n->lifetime_range;
                ++inits;
            }
        }
        if (grants != 1 || inits != 1 || domains != 1)
            return stop(r, "SOURCE_ISSUER_BIJECTION", 0);
        for (size_t i = 0; i < count; ++i)
            if (o.root == originals[i].root ||
                o.domain == originals[i].domain ||
                o.extent.region == originals[i].extent.region ||
                o.allocation == originals[i].allocation)
                return stop(r, "ORIGINAL_NOT_INDEPENDENT", 0);
        originals[count++] = o;
    }
    r->originals = count;
    if (count != NL_CAPTURED_MAX_RELEASES)
        return stop(r, "ORIGINAL_PROFILE_COUNT", 0);
    size_t capacity = sizeof(f->field_changes) / sizeof(f->field_changes[0]);
    NLCheckedNodeId
        nodes[sizeof(f->field_changes) / sizeof(f->field_changes[0])];
    size_t eligible = 0;
    for (size_t j = 1; j <= nl_checked_node_count(f); ++j) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, j);
        if (n->kind == NL_CHECKED_REGISTERED_CALL)
            return stop(r, "UNMODELLED_CALLEE", j);
        if (n->kind == NL_CHECKED_REPLACE) {
            if (eligible == capacity || !n->field.present)
                return stop(r, "SOURCE_EVENT_PROFILE_BOUND", j);
            nodes[eligible++] = j;
        }
    }
    r->eligible = eligible;
    if (!eligible || length != eligible || length != f->field_change_count)
        return stop(r, "TAPE_SOURCE_CARDINALITY", 0);
    for (size_t i = 0; i < length; ++i) {
        const NLCapturedChange *c = tape[i];
        if (!c || c != f->field_changes[i] || c->world != f->context ||
            c->node != nodes[i])
            return stop(r, "TAPE_SOURCE_BIJECTION", nodes[i]);
        const NLCheckedNodeView *n = nl_checked_node_view(f, c->node);
        NLSourceView operation_source;
        if (!nl_source_view(nl_checked_source(f), n->span, &operation_source) ||
            operation_source.length < 8 ||
            memcmp(operation_source.bytes, "replace(", 8))
            return stop(r, "SOURCE_OPERATION_SPAN", c->node);
        if (i && nl_checked_node_view(f, tape[i - 1]->node)->span.end_byte >=
                     n->span.start_byte)
            return stop(r, "SOURCE_SPAN_ORDER", c->node);
        if (!inventory(c->before, originals, count) ||
            !inventory(c->after, originals, count))
            return stop(r, "CURRENT_ORIGINAL_AUTHORITY", c->node);
        if (i) {
            NLCheckStatus status = gap(f, tape[i - 1], c, originals, count, r);
            if (status != NL_CHECK_OK)
                return status;
        }
        NLCheckStatus status = nl_test_p350_primitive(f, c);
        if (status != NL_CHECK_OK) {
            r->reason = "PRIMITIVE_REPLAY";
            r->node = c->node;
            return status;
        }
        ++r->replayed;
        /* Every non-target fourth or link field must remain current. The
         * primitive already replays all state; this explicitly audits scalar.
         */
        for (size_t j = 0; j < count; ++j) {
            NLSemanticPlaceView b, a;
            if (!nl_semantic_place_view(c->before, originals[j].root, &b) ||
                !nl_semantic_place_view(c->after, originals[j].root, &a))
                return stop(r, "ROOT_VIEW", c->node);
            for (size_t k = 0; k < b.fixed_field_count; ++k)
                if (b.fixed_fields[k] != n->field.child) {
                    NLSemanticPlaceView x, y;
                    NLSemanticValueView vx, vy;
                    if (!nl_semantic_place_view(c->before, b.fixed_fields[k],
                                                &x) ||
                        !nl_semantic_place_view(c->after, a.fixed_fields[k],
                                                &y) ||
                        memcmp(&x, &y, sizeof(x)) ||
                        !nl_semantic_value_view(c->before, x.current_value,
                                                &vx) ||
                        !nl_semantic_value_view(c->after, y.current_value, &vy))
                        return stop(r, "UNRELATED_FIXED_FIELD", c->node);
                    /* Whole replace reanchors unchanged members in the new
                     * parent aggregate. The primitive replay proves custody. */
                    vx.aggregate_owner = 0;
                    vy.aggregate_owner = 0;
                    if (memcmp(&vx, &vy, sizeof(vx)))
                        return stop(r, "UNRELATED_FIXED_VALUE", c->node);
                    if (vx.scalar_known)
                        ++r->scalar_checks;
                }
        }
    }
    /* Reuse the existing independent five-world source terminal proof;
     * it is not the new chronology test and is called only after shadow checks.
     */
    for (size_t i = 0; i < count; ++i) {
        NLSemanticSnapshot before, after;
        NLCapturedClosureView closure;
        if (!nl_semantic_snapshot(parents[i]->context, &before))
            return stop(r, "TERMINAL_CONTEXT", matches[i]);
        NLCheckStatus status =
            nl_checked_captured_closure_validate(parents[i], matches[i]);
        if (status != NL_CHECK_OK) {
            r->reason = "STATIC_TERMINAL_PUBLIC_REDERIVATION";
            return status;
        }
        if (!nl_semantic_snapshot(parents[i]->context, &after) ||
            memcmp(&before, &after, sizeof(before)))
            return stop(r, "VALIDATOR_MUTATION", matches[i]);
        if (!i) {
            if (!nl_checked_captured_closure_view(parents[i], matches[i],
                                                  &closure))
                return stop(r, "TERMINAL_VIEW", matches[i]);
            for (size_t j = 0; j <= count; ++j)
                if (closure.release_worlds[j] != 1)
                    return stop(r, "STATIC_TERMINAL_WORLDS", matches[i]);
        }
    }
    return NL_CHECK_OK;
}
