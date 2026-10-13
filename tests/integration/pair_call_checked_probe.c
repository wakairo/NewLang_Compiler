#include "../support/node_checked.h"

/* Only observes source-issued artifacts. No seeds, invented reference facts,
 * custom prelude, replay context, or emitter output enter this oracle. */
static bool scalar(const NLSemanticContext *c, NLValueId id, size_t *out)
{
    NLSemanticValueView value;
    CHECK(nl_semantic_value_view(c, id, &value));
    CHECK(value.scalar_known);
    *out = value.scalar_value;
    return true;
}

static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    const NLSemanticContext *c = nl_checked_context(f);
    printf("{\"kind\":\"status\",\"parse\":0,\"register\":0,\"check\":0,"
           "\"owned\":true}\n");
    NLCheckedField changes[2] = {{0}};
    NLSymbolId roots[2] = {0};
    NLValueId initial_fields[2][2] = {{0}};
    size_t count = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *rhs =
                nl_checked_node_view(f, v->initializer);
            if (rhs->kind == NL_CHECKED_AGGREGATE) {
                CHECK(count < 2);
                NLSemanticValueView initial;
                NLSemanticBindingView binding;
                NLSemanticPlaceView parent;
                CHECK(
                    nl_semantic_value_view(c, rhs->results[0].value, &initial));
                CHECK(initial.field_count == 2);
                CHECK(nl_semantic_binding_view(c, v->symbol, &binding));
                CHECK(nl_semantic_place_view(c, binding.place, &parent));
                CHECK(parent.implicit_local && !parent.live &&
                      parent.governing_domain == 0);
                size_t values[2];
                for (size_t j = 0; j < 2; ++j) {
                    CHECK(scalar(c, initial.fields[j], &values[j]));
                    initial_fields[count][j] = initial.fields[j];
                }
                roots[count++] = v->symbol;
                printf("{\"kind\":\"record\",\"symbol\":%zu,\"parent\":%zu,"
                       "\"incarnation\":%zu,\"initial\":[%zu,%zu],"
                       "\"initial_values\":[%zu,%zu]}\n",
                       v->symbol, binding.place, parent.incarnation, values[0],
                       values[1], initial.fields[0], initial.fields[1]);
            } else {
                NLSourceView label;
                NLSemanticBindingView binding;
                size_t value;
                CHECK(nl_source_view(nl_checked_source(f), v->name, &label));
                CHECK(nl_semantic_binding_view(c, v->symbol, &binding));
                CHECK(scalar(c, binding.value, &value));
                printf("{\"kind\":\"binding\",\"name\":\"%.*s\","
                       "\"symbol\":%zu,\"value\":%zu}\n",
                       (int)label.length, (const char *)label.bytes, v->symbol,
                       value);
            }
        }
        if (v->kind != NL_CHECKED_LOAN_HEADER)
            continue;
        const NLCheckedNodeView *block =
            nl_checked_node_view(f, v->initializer);
        const NLCheckedNodeView *call = nl_checked_node_view(f, block->tail);
        CHECK(v->loan.body_nonescape_proved &&
              v->loan.normal_result_forwarded && v->loan.implicit_local &&
              !v->loan.is_exclusive && v->loan.access == NL_ACCESS_WRITE &&
              v->field.present);
        CHECK(call->kind == NL_CHECKED_REGISTERED_CALL && call->body_backed &&
              call->argument_count == 2);
        const NLCheckedFragment *body = nl_checked_call_body(f, block->tail);
        CHECK(body != NULL && nl_checked_context(body) == c);
        const NLCheckedNodeView *root =
            nl_checked_node_view(body, nl_checked_root(body));
        const NLCheckedNodeView *replace =
            nl_checked_node_view(body, root->tail);
        CHECK(root->item_count == 0 && replace->kind == NL_CHECKED_REPLACE &&
              replace->field.present && replace->field.dependency_compatible);
        const NLCheckedNodeView *actual[2], *formal[2];
        actual[0] = nl_checked_node_view(f, call->first_argument);
        actual[1] = nl_checked_node_view(f, actual[0]->next_argument);
        formal[0] = nl_checked_node_view(body, replace->first_argument);
        formal[1] = nl_checked_node_view(body, formal[0]->next_argument);
        for (size_t j = 0; j < 2; ++j) {
            NLSemanticBindingView parameter;
            CHECK(nl_semantic_binding_view(c, formal[j]->symbol, &parameter));
            CHECK(parameter.value == actual[j]->results[0].value &&
                  parameter.type == actual[j]->parameter_type);
        }
        NLSemanticValueView ref;
        CHECK(nl_semantic_value_view(c, actual[0]->results[0].value, &ref));
        CHECK(ref.reference_count == 0 &&
              ref.reference.provenance == NL_PROVENANCE_VALID &&
              ref.reference.writable && ref.reference.place == v->field.child &&
              ref.reference.incarnation == v->field.child_incarnation &&
              ref.reference.scope == v->loan.scope);
        NLSemanticScopeView scope;
        CHECK(nl_semantic_scope_view(c, v->loan.scope, &scope) &&
              !scope.active);
        CHECK(replace->field.child == v->field.child &&
              replace->field.parent == v->field.parent &&
              replace->field.parent_incarnation ==
                  v->field.parent_incarnation &&
              replace->field.child_incarnation == v->field.child_incarnation &&
              replace->field.parent_fact == v->field.parent_fact &&
              replace->field.child_fact == v->field.child_fact &&
              replace->field.parent_post_fact != v->field.parent_fact &&
              replace->field.child_post_fact != v->field.child_fact &&
              replace->field.old_value == v->field.old_value &&
              replace->field.new_value == formal[1]->results[0].value &&
              replace->field.old_value == call->results[0].value &&
              call->results[0].value == v->results[0].value);
        size_t old, next;
        CHECK(scalar(c, replace->field.old_value, &old));
        CHECK(scalar(c, replace->field.new_value, &next));
        const size_t which = v->field.base == roots[0] ? 0 : 1;
        CHECK(roots[which] == v->field.base && !changes[which].present);
        changes[which] = replace->field;
        printf("{\"kind\":\"call\",\"node\":%zu,\"function\":%zu,"
               "\"record\":%zu,\"index\":%zu,\"ref_symbol\":%zu,"
               "\"scope\":%zu,\"formal_symbols\":[%zu,%zu],"
               "\"actual_values\":[%zu,%zu],\"old\":%zu,\"new\":%zu,"
               "\"parent\":%zu,\"child\":%zu,\"incarnation\":%zu,"
               "\"facts\":[%zu,%zu],\"post_facts\":[%zu,%zu]}\n",
               block->tail, call->function, v->field.base, v->field.index,
               actual[0]->symbol, v->loan.scope, formal[0]->symbol,
               formal[1]->symbol, actual[0]->results[0].value,
               actual[1]->results[0].value, old, next, v->field.parent,
               v->field.child, v->field.child_incarnation, v->field.parent_fact,
               v->field.child_fact, replace->field.parent_post_fact,
               replace->field.child_post_fact);
    }
    CHECK(count == 2 && changes[0].present && changes[1].present &&
          changes[0].parent != changes[1].parent &&
          changes[0].child != changes[1].child);
    /* Point-in-time post-call reads, not final live-place assumptions. */
    size_t reads = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind != NL_CHECKED_FIELD_READ)
            continue;
        const size_t which = v->field.base == roots[0] ? 0 : 1;
        CHECK(roots[which] == v->field.base);
        const NLCheckedField change = changes[which];
        CHECK(v->field.parent == change.parent &&
              v->field.parent_incarnation == change.parent_incarnation &&
              v->field.parent_fact == change.parent_post_fact &&
              v->field.dependency_compatible);
        if (v->field.index == change.index)
            CHECK(v->field.child == change.child &&
                  v->field.child_incarnation == change.child_incarnation &&
                  v->field.child_fact == change.child_post_fact &&
                  v->field.old_value == change.new_value);
        else {
            NLSemanticPlaceView child;
            CHECK(nl_semantic_place_view(c, v->field.child, &child));
            CHECK(v->field.dependency_compatible && v->field.child_fact != 0 &&
                  child.parent_aggregate == change.parent &&
                  child.incarnation == v->field.child_incarnation &&
                  v->field.old_value == initial_fields[which][v->field.index]);
        }
        printf("{\"kind\":\"read\",\"record\":%zu,\"index\":%zu,"
               "\"child\":%zu,\"incarnation\":%zu,\"parent_fact\":%zu,"
               "\"child_fact\":%zu,\"current_value\":%zu}\n",
               v->field.base, v->field.index, v->field.child,
               v->field.child_incarnation, v->field.parent_fact,
               v->field.child_fact, v->field.old_value);
        ++reads;
    }
    CHECK(reads == 4);
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
