#ifndef NEWLANG_P671_FRAGMENT_H
#define NEWLANG_P671_FRAGMENT_H
#include "../support/semantic_check.h"
/* The two authorized Other-oracle migrations share public observations only.
 * Existing rejection helpers and all other historical assertions stay intact.
 */
static inline bool p671_other_read(NLSemanticContext *c, const char *name,
                                   const char *source, size_t expected,
                                   size_t other)
{
    const NLSymbolId symbol = nl_semantic_find_binding(c, name);
    NLSemanticBindingView binding;
    NLSemanticTypeView nominal;
    NLAggregateField fields[2];
    NLSemanticPlaceView parent, child[2], after;
    NLSemanticValueView values[2];
    CHECK(symbol && nl_semantic_binding_view(c, symbol, &binding) &&
          nl_semantic_type_view(c, binding.type, &nominal) &&
          nl_semantic_place_view(c, binding.place, &parent));
    CHECK(nominal.kind == NL_TYPE_NOMINAL && nominal.field_count == 2 &&
          nominal.is_copy && nominal.is_discardable &&
          binding.availability == NL_AVAILABLE && parent.live &&
          parent.implicit_local && parent.independent_root &&
          parent.fixed_field_count == 2 && parent.type == binding.type);
    for (size_t i = 0; i < 2; ++i) {
        CHECK(
            nl_semantic_aggregate_field_view(c, binding.type, i, &fields[i]) &&
            nl_semantic_place_view(c, parent.fixed_fields[i], &child[i]) &&
            nl_semantic_value_view(c, child[i].current_value, &values[i]));
        CHECK(fields[i].type == nl_semantic_core_type(c, NL_TYPE_U8) &&
              child[i].live && !child[i].independent_root &&
              child[i].parent_aggregate == binding.place &&
              child[i].parent_incarnation == parent.incarnation &&
              child[i].parent_field_index == i && child[i].current_fact &&
              values[i].scalar_known &&
              values[i].scalar_value == (i ? other : expected));
    }
    CHECK(strcmp(fields[0].name, "left") == 0 &&
          strcmp(fields[1].name, "right") == 0);
    TestChecked read = {0};
    CHECK(test_run(c, source, TEST_SOURCE, NL_CHECK_OK, NULL, &read));
    const NLCheckedNodeView *v = test_root(&read);
    CHECK(v && v->kind == NL_CHECKED_FIELD_READ && v->field.present &&
          v->field.dependency_compatible && v->field.nominal == binding.type &&
          v->field.base == symbol && v->field.parent == binding.place &&
          v->field.parent_incarnation == parent.incarnation &&
          v->field.parent_fact == parent.current_fact && v->field.index == 0 &&
          v->field.child == parent.fixed_fields[0] &&
          v->field.child_incarnation == child[0].incarnation &&
          v->field.child_fact == child[0].current_fact &&
          v->field.old_value == child[0].current_value &&
          v->result_count == 1 && v->results[0].type == fields[0].type &&
          v->has_scalar_result && v->scalar_result.value == expected);
    NLSemanticValueView copied;
    CHECK(nl_semantic_value_view(c, v->results[0].value, &copied) &&
          copied.type == fields[0].type && copied.scalar_known &&
          copied.scalar_value == expected &&
          copied.dependencies == NL_DEPENDENCY_FREE);
    CHECK(nl_semantic_place_view(c, binding.place, &after) && after.live &&
          after.incarnation == parent.incarnation &&
          after.current_fact == parent.current_fact &&
          after.current_value == parent.current_value);
    for (size_t i = 0; i < 2; ++i) {
        CHECK(nl_semantic_place_view(c, parent.fixed_fields[i], &after) &&
              after.live && after.incarnation == child[i].incarnation &&
              after.current_fact == child[i].current_fact &&
              after.current_value == child[i].current_value);
    }
    test_checked_destroy(&read);
    return true;
}
#endif
