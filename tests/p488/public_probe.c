/* Read-only public observation of real source-owned call/body evidence.
 * No host-issued state, certificate construction, emission or authority. */
#include "../support/node_checked.h"
#include "newlang/node_join.h"

static bool walk(const NLCheckedFragment *f, size_t depth)
{
    const NLSemanticContext *c = nl_checked_context(f);
    CHECK(c && depth < 4);
    printf("FRAGMENT depth=%zu source=%s root=%zu nodes=%zu\n", depth,
           nl_source_name(nl_checked_source(f)), nl_checked_root(f),
           nl_checked_node_count(f));
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        CHECK(v && nl_source_span_valid(nl_checked_source(f), v->span));
        printf("NODE depth=%zu id=%zu kind=%d span=%zu:%zu symbol=%zu "
               "arg=%zu nextarg=%zu result=%zu value=%zu copied=%d\n",
               depth, i, (int)v->kind, v->span.start_byte, v->span.end_byte,
               v->symbol, v->first_argument, v->next_argument, v->result_count,
               v->results[0].value, v->value_use == NL_VALUE_COPIED);
        if (v->has_reference_result)
            printf("REFERENCE depth=%zu id=%zu place=%zu incarnation=%zu "
                   "scope=%zu provenance=%d\n",
                   depth, i, v->reference_result.place,
                   v->reference_result.incarnation, v->reference_result.scope,
                   (int)v->reference_result.provenance);
        if (v->field.present) {
            const NLCheckedField *p = &v->field;
            NLSemanticValueView old = {0}, next = {0}, payload = {0};
            bool have_old = nl_semantic_value_view(c, p->old_value, &old);
            bool have_next = nl_semantic_value_view(c, p->new_value, &next);
            bool have_payload = have_next && nl_semantic_value_view(
                                                 c, next.sum_payload, &payload);
            printf("FIELD depth=%zu id=%zu parent=%zu/%zu child=%zu/%zu "
                   "index=%zu F=%zu->%zu RF=%zu->%zu occurrence=%zu->%zu "
                   "old=%zu/%zu new=%zu/%zu target=%zu/%zu "
                   "compatible=%d\n",
                   depth, i, p->parent, p->parent_incarnation, p->child,
                   p->child_incarnation, p->index, p->child_fact,
                   p->child_post_fact, p->parent_fact, p->parent_post_fact,
                   p->payload_occurrence, p->post_payload_occurrence,
                   p->old_value, have_old ? old.variant : 0, p->new_value,
                   have_next ? next.variant : 0,
                   have_payload ? payload.reference.place : 0,
                   have_payload ? payload.reference.incarnation : 0,
                   p->dependency_compatible);
        }
        if (v->kind == NL_CHECKED_LOAN_HEADER)
            printf("LOAN depth=%zu id=%zu place=%zu incarnation=%zu "
                   "domain=%zu implicit=%d access=%d scope=%zu ref=%zu "
                   "nonescape=%d forwarded=%d\n",
                   depth, i, v->loan.place, v->loan.incarnation, v->loan.domain,
                   v->loan.implicit_local, (int)v->loan.access, v->loan.scope,
                   v->loan.ref_symbol, v->loan.body_nonescape_proved,
                   v->loan.normal_result_forwarded);
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            const NLCheckedFragment *body = nl_checked_call_body(f, i);
            CHECK(body && v->body_backed && nl_checked_context(body) == c);
            printf("CALL depth=%zu id=%zu function=%zu body_backed=1 "
                   "owner_entry=%d producer_entry=%d custody_entry=%d\n",
                   depth, i, v->function, v->owner_call.entry_proved,
                   v->producer.entry_proved, v->custody_call.entry_proved);
            CHECK(walk(body, depth + 1));
        }
    }
    return true;
}
static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    CHECK(walk(n.entry, 0));
    const NLCheckedFragment *main_body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    const NLCheckedNodeView *call = NULL, *pointer = NULL, *loan = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(main_body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(main_body, i);
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            CHECK(call == NULL);
            call = v;
        }
        if (v->kind == NL_CHECKED_PTR_FROM_REF)
            pointer = v;
    }
    CHECK(call && pointer && call->argument_count == 2);
    for (size_t i = 1; i <= nl_checked_node_count(main_body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(main_body, i);
        if (v->kind == NL_CHECKED_LOAN_HEADER &&
            v->loan.access == NL_ACCESS_WRITE && v->field.present &&
            v->field.parent == pointer->reference_result.place) {
            CHECK(loan == NULL);
            loan = v;
        }
    }
    CHECK(loan);
    const NLCheckedFragment *body = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(main_body); ++i)
        if (nl_checked_node_view(main_body, i) == call)
            body = nl_checked_call_body(main_body, i);
    CHECK(body && nl_checked_context(body) == nl_checked_context(main_body));
    const NLCheckedNodeView *op = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(body, i);
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(op == NULL);
            op = v;
        }
    }
    CHECK(op && op->field.present && op->field.dependency_compatible &&
          op->field.parent == pointer->reference_result.place &&
          op->field.parent_incarnation ==
              pointer->reference_result.incarnation &&
          op->field.child == loan->loan.place &&
          op->field.child_incarnation == loan->loan.incarnation &&
          op->field.child_fact != op->field.child_post_fact &&
          op->field.parent_fact != op->field.parent_post_fact &&
          op->field.payload_occurrence == 0 &&
          op->field.post_payload_occurrence != 0 &&
          call->results[0].value == op->field.old_value &&
          op->results[0].value == op->field.old_value);
    const NLSemanticContext *c = nl_checked_context(body);
    NLSemanticValueView old, next, ptr, actual_ref;
    const NLCheckedNodeView *arg =
        nl_checked_node_view(main_body, call->first_argument);
    CHECK(arg &&
          nl_semantic_value_view(c, arg->results[0].value, &actual_ref) &&
          actual_ref.reference.place == op->field.child &&
          actual_ref.reference.incarnation == op->field.child_incarnation &&
          actual_ref.reference.scope == loan->loan.scope &&
          actual_ref.reference.provenance == NL_PROVENANCE_VALID &&
          nl_semantic_value_view(c, op->field.old_value, &old) &&
          old.variant == 1 &&
          nl_semantic_value_view(c, op->field.new_value, &next) &&
          next.variant == 2 &&
          nl_semantic_value_view(c, next.sum_payload, &ptr) &&
          ptr.reference.place == pointer->reference_result.place &&
          ptr.reference.incarnation == pointer->reference_result.incarnation &&
          ptr.reference.scope == 0 && ptr.allocation_region == 0 &&
          ptr.domain == 0 && loan->loan.domain == 0 &&
          loan->loan.implicit_local && loan->loan.body_nonescape_proved &&
          loan->loan.normal_result_forwarded);
    puts("OBSERVED actual write-ref -> original tail field, one child replace, "
         "old None returned, Some(original tail) and F/RF/occurrence change");
    puts("OBSERVATION ONLY: call-body accessor is not an independent ordinary "
         "call-effect certificate; PUBLIC PROOF NOT ESTABLISHED");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
