/* R583 separately authored NON-BLIND inspector. R580 oracle/source untouched.
 * Registered source only; no host-created semantic roots/authorities. */
#include "node_checked.h"
#include "semantic_internal.h"
#include "newlang/node_links.h"
#include "newlang/checked_c_node.h"
#include <stdio.h>
#include <string.h>

static bool roots_by_name(const NLCheckedFragment *f, NLPlaceId out[3],
                          const char *names[3]) {
    const NLSemanticContext *ctx = nl_checked_context(f);
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (!v || v->kind != NL_CHECKED_BINDING) continue;
        NLSourceView name = {0};
        if (!nl_source_view(nl_checked_source(f), v->name, &name)) return false;
        for (size_t j = 0; j < 3; ++j)
            if (name.length == strlen(names[j]) &&
                !memcmp(name.bytes, names[j], name.length)) {
                const NLCheckedNodeView *ident =
                    nl_checked_node_view(f, v->first_argument);
                NLSemanticBindingView b;
                if (!ident || out[j] ||
                    !nl_semantic_binding_view(ctx, ident->symbol, &b))
                    return false;
                out[j] = b.place;
            }
    }
    return out[0] && out[1] && out[2] &&
           out[0] != out[1] && out[1] != out[2] && out[0] != out[2];
}
static bool bad_public_rejected(const NLCheckedFragment *f,
                                const NLNodeLinksView *v) {
    NLNodeLinksView bad = *v;
    bad.originals[0] = v->originals[1];
    CHECK(nl_checked_node_links_validate(f, &bad) != NL_CHECK_OK);
    bad = *v;
    bad.changes[0].operation = v->changes[1].operation;
    CHECK(nl_checked_node_links_validate(f, &bad) != NL_CHECK_OK);
    bad = *v;
    bad.changes[1].before = v->changes[0].before;
    CHECK(nl_checked_node_links_validate(f, &bad) != NL_CHECK_OK);
    return true;
}
static bool actual_snapshot_rejections(const NLCheckedFragment *f,
                                       const NLNodeLinksView *v) {
    NLCheckedFragment *owned = (NLCheckedFragment *)f;
    const NLCheckedNodeView *op = nl_checked_node_view(f, v->changes[0].operation);
    CHECK(op && op->kind == NL_CHECKED_REPLACE);
    NLSemanticContext *b = owned->field_changes[0]->before;
    NLSemanticContext *a = owned->field_changes[0]->after;
    NLSemanticContext *b2 = owned->field_changes[1]->before;
    /* Fault edits affect actual owned issuer snapshots, NOT an invented world.
       Each mutation is restored before the next and must reject fail-closed. */
    NLValueFactId saved = b->places[op->field.child - 1].current_fact;
    b->places[op->field.child - 1].current_fact = 0;
    NLCheckStatus st = nl_checked_node_links_validate(f, NULL);
    b->places[op->field.child - 1].current_fact = saved;
    CHECK(st != NL_CHECK_OK);
    NLValueFactId bridge = b2->places[v->originals[1] - 1].current_fact;
    b2->places[v->originals[1] - 1].current_fact = 0;
    st = nl_checked_node_links_validate(f, NULL);
    b2->places[v->originals[1] - 1].current_fact = bridge;
    CHECK(st != NL_CHECK_OK);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, op->first_argument);
    CHECK(arg && arg->result_count);
    NLValueId ref_value = arg->results[0].value;
    CHECK(ref_value && ref_value <= b->value_count);
    bool writable = b->values[ref_value - 1].reference.writable;
    b->values[ref_value - 1].reference.writable = false;
    st = nl_checked_node_links_validate(f, NULL);
    b->values[ref_value - 1].reference.writable = writable;
    CHECK(st != NL_CHECK_OK);
    NLOccurrenceId occ = op->field.post_payload_occurrence;
    if (occ) {
        CHECK(occ <= a->occurrence_count);
        bool live = a->occurrences[occ - 1].live;
        a->occurrences[occ - 1].live = false;
        st = nl_checked_node_links_validate(f, NULL);
        a->occurrences[occ - 1].live = live;
        CHECK(st != NL_CHECK_OK);
    }
    CHECK(nl_checked_node_links_validate(f, NULL) == NL_CHECK_OK);
    return true;
}
static bool inspect(const char *source, bool require_valid) {
    TestNode n = {0}, foreign_node = {0};
    CHECK(node_checked_load(source, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f && nl_checked_context(f) && nl_checked_source(f));
    size_t write_count = 0;
    for (size_t id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        CHECK(v);
        if (v->kind == NL_CHECKED_LOAN_HEADER &&
            v->loan.access == NL_ACCESS_WRITE) {
            printf("LOAN ordinal=%zu id=%zu from_ptr=%d nonescape=%d "
                   "conflicting_access=%d implicit_local=%d lifetime_end=%d "
                   "exclusive=%d target=%zu\n",
                   ++write_count, id, v->loan.from_ptr,
                   v->loan.body_nonescape_proved,
                   v->loan.prevent_conflicting_access,
                   v->loan.implicit_local, v->loan.prevent_lifetime_end,
                   v->loan.is_exclusive, v->loan.place);
        }
    }
    CHECK(write_count == 2);
    NLNodeLinksView v = {0};
    const bool view_ok = nl_checked_node_links_view(f, &v);
    NLCheckStatus status = nl_checked_node_links_validate(f, view_ok ? &v : NULL);
    printf("PUBLIC view=%d validator=%d\n", view_ok, (int)status);
    fflush(stdout);
    if (!require_valid) {
        node_checked_destroy(&n);
        return true; /* Historical oracle result is OBSERVED, not relabelled. */
    }
    CHECK(view_ok && status == NL_CHECK_OK);
    const char *names[3] = {"c", "a", "b"};
    NLPlaceId source_roots[3] = {0};
    CHECK(roots_by_name(f, source_roots, names));
    for (size_t j = 0; j < 3; ++j) {
        CHECK(v.originals[j] == source_roots[j]);
        NLSemanticPlaceView p;
        CHECK(nl_semantic_place_view(v.changes[0].before, v.originals[j], &p)
              && p.live && p.implicit_local && p.independent_root &&
              !p.governing_domain && !p.placement.region &&
              p.fixed_field_count == 2 && p.incarnation);
        printf("ROOT role=%s place=%zu inc=%zu next=%zu payload=%zu\n",
               names[j], v.originals[j], p.incarnation,
               p.fixed_fields[0], p.fixed_fields[1]);
    }
    for (size_t i = 0; i < 2; ++i) {
        const NLSemanticContext *before = v.changes[i].before;
        const NLSemanticContext *after = v.changes[i].after;
        const NLCheckedNodeView *op =
            nl_checked_node_view(f, v.changes[i].operation);
        CHECK(before && after && op && op->kind == NL_CHECKED_REPLACE &&
              op->field.parent == v.originals[i + 1]);
        NLSemanticPlaceView pb, pa, cb, ca;
        CHECK(nl_semantic_place_view(before, op->field.parent, &pb) &&
              nl_semantic_place_view(after, op->field.parent, &pa) &&
              nl_semantic_place_view(before, op->field.child, &cb) &&
              nl_semantic_place_view(after, op->field.child, &ca) &&
              pb.live && pa.live && cb.live && ca.live &&
              pb.incarnation == pa.incarnation &&
              cb.incarnation == ca.incarnation &&
              pb.current_fact == op->field.parent_fact &&
              pa.current_fact == op->field.parent_post_fact &&
              cb.current_fact == op->field.child_fact &&
              ca.current_fact == op->field.child_post_fact &&
              pb.current_fact != pa.current_fact &&
              cb.current_fact != ca.current_fact &&
              cb.payload_occurrence == op->field.payload_occurrence &&
              ca.payload_occurrence == op->field.post_payload_occurrence);
        NLSemanticValueView old, next, ptr;
        CHECK(nl_semantic_value_view(before, op->field.old_value, &old) &&
              old.variant == 1 &&
              nl_semantic_value_view(after, ca.current_value, &next) &&
              next.variant == 2 &&
              nl_semantic_value_view(after, next.sum_payload, &ptr) &&
              ptr.reference.place == v.originals[i ? 0 : 2] &&
              ptr.reference.provenance == NL_PROVENANCE_VALID &&
              !ptr.reference.scope && !ptr.reference.writable);
        printf("STEP ordinal=%zu root=%zu field=%zu rootFact=%zu:%zu "
               "fieldFact=%zu:%zu occurrence=%zu:%zu ptrTarget=%zu\n",
               i + 1, op->field.parent, op->field.child,
               pb.current_fact, pa.current_fact,
               cb.current_fact, ca.current_fact,
               cb.payload_occurrence, ca.payload_occurrence,
               ptr.reference.place);
    }
    /* Before of second must preserve the actual original a link written at
       step 1; never equate raw pointer target IDs to independent authority. */
    const NLSemanticContext *post1 = v.changes[0].after;
    const NLSemanticContext *pre2 = v.changes[1].before;
    for(size_t j = 0; j < 3; ++j) {
        NLSemanticPlaceView p, q;
        CHECK(nl_semantic_place_view(post1, v.originals[j], &p) &&
              nl_semantic_place_view(pre2, v.originals[j], &q) &&
              p.current_fact == q.current_fact &&
              p.current_value == q.current_value &&
              p.incarnation == q.incarnation);
        for(size_t k = 0; k < 2; ++k) {
            NLSemanticPlaceView c, d;
            CHECK(nl_semantic_place_view(post1, p.fixed_fields[k], &c) &&
                  nl_semantic_place_view(pre2, q.fixed_fields[k], &d) &&
                  c.current_fact == d.current_fact &&
                  c.current_value == d.current_value &&
                  c.payload_occurrence == d.payload_occurrence);
        }
    }
    CHECK(bad_public_rejected(f, &v));
    CHECK(node_checked_load(source, &foreign_node));
    const NLCheckedFragment *foreign = nl_checked_call_body(
        foreign_node.entry, nl_checked_root(foreign_node.entry));
    NLNodeLinksView hostile = v;
    hostile.origin = foreign;
    CHECK(nl_checked_node_links_validate(f, &hostile) != NL_CHECK_OK &&
          nl_checked_node_links_validate(foreign, &v) != NL_CHECK_OK);
    node_checked_destroy(&foreign_node);
    CHECK(actual_snapshot_rejections(f, &v));
    puts("R583 REGISTERED CALIBRATED PUBLIC VALID, selected hostility rejected");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv) {
    return (argc == 3 && inspect(argv[2], strcmp(argv[1], "certify") == 0))
        ? 0 : 1;
}
