/* Test-only synchronous observation of REAL source checker snapshots.
 * No hook/API or source/semantic behavior is added to Production. */
#include "../../src/if_field_internal.h"
#include "newlang/parser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define R(x)                                                                   \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P930 line%u: %s\n", (unsigned)__LINE__, #x);      \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

typedef struct {
    const char *file, *names[2], *labels[2];
    size_t values[2][2], order[2][2], operands[2];
    bool truth;
} Expected;
static const Expected cases[] = {{"budget_true.nl",
                                  {"primary", "backup"},
                                  {"consumed", "remaining"},
                                  {{186, 9}, {72, 224}},
                                  {{1, 0}, {0, 1}},
                                  {12, 40},
                                  true},
                                 {"budget_false.nl",
                                  {"queued", "reserve"},
                                  {"remaining", "consumed"},
                                  {{17, 232}, {64, 105}},
                                  {{0, 1}, {1, 0}},
                                  {21, 22},
                                  false},
                                 {"order_only.nl",
                                  {"primary", "backup"},
                                  {"consumed", "remaining"},
                                  {{186, 9}, {72, 224}},
                                  {{0, 1}, {0, 1}},
                                  {12, 40},
                                  true},
                                 {"root_values_swapped.nl",
                                  {"primary", "backup"},
                                  {"consumed", "remaining"},
                                  {{72, 224}, {186, 9}},
                                  {{1, 0}, {0, 1}},
                                  {12, 40},
                                  true}};
typedef struct {
    uintptr_t unit, body, fragment, entry, current_world, artifact_world;
    size_t session, phase, roots[2], scalars[2][2], order[2][2];
    size_t incarnations[6], facts[6];
    bool truth;
} Observation;
typedef struct {
    bool active, negative, preparing;
    size_t session, phase, count, source_offset, plan_count, refusals;
    int fault;
    size_t hit;
    const Expected *expect;
    const NLSource *unit; /* borrow ONLY inside live Input session */
    NLFunctionBody *plan; /* actual creation token, cleared before cleanup */
    NLSourceSpan body_span;
    Observation observations[2]; /* scalar copies, NEVER C/world pointers */
} Session;
static Session state;

NLCheckStatus __real_nl_body_create_span(const NLSource *, NLSourceSpan,
                                         const NLFunctionParameter *, size_t,
                                         NLFunctionBody **);
NLCheckStatus __real_nl_if_field_prepare(NLCheckedFragment *, NLCheckedNodeId,
                                         const NLSemanticContext *);
NLCheckStatus __wrap_nl_body_create_span(const NLSource *unit,
                                         NLSourceSpan span,
                                         const NLFunctionParameter *parameters,
                                         size_t count, NLFunctionBody **out)
{
    NLCheckStatus status =
        __real_nl_body_create_span(unit, span, parameters, count, out);
    if (!state.active || status != NL_CHECK_OK)
        return status;
    R(unit == state.unit && !state.plan && !state.plan_count && count == 0);
    state.plan = *out;
    state.body_span = span;
    ++state.plan_count;
    NLSourceView original, copy;
    R(nl_source_view(unit, span, &original) &&
      nl_source_view(state.plan->source,
                     (NLSourceSpan){0, nl_source_length(state.plan->source)},
                     &copy) &&
      original.length == copy.length &&
      !memcmp(original.bytes, copy.bytes, copy.length));
    state.source_offset = span.start_byte;
    return status;
}
static bool root_matches(const NLSemanticContext *c, size_t symbol,
                         size_t place, size_t field, const char *label,
                         size_t value)
{
    NLSemanticBindingView b;
    NLSemanticPlaceView r, ch;
    NLSemanticValueView v;
    NLAggregateField f;
    return nl_semantic_binding_view(c, symbol, &b) && b.place == place &&
           nl_semantic_place_view(c, place, &r) &&
           field < r.fixed_field_count &&
           nl_semantic_place_view(c, r.fixed_fields[field], &ch) &&
           ch.parent_aggregate == place && ch.parent_field_index == field &&
           ch.parent_incarnation == r.incarnation &&
           nl_semantic_aggregate_field_view(c, r.type, field, &f) &&
           !strcmp(f.name, label) &&
           nl_semantic_value_view(c, ch.current_value, &v) && v.scalar_known &&
           v.scalar_value == value;
}
static bool orders_match(const Observation *o, const Expected *e)
{
    return !memcmp(o->order, e->order, sizeof(o->order));
}
static void observe(NLCheckedFragment *f, NLCheckedNodeId id,
                    const NLSemanticContext *actual)
{
    const Expected *e = state.expect;
    NLIfFieldProof *p = f->if_fields;
    R(p && p->owner == f && p->source == f->source &&
      p->view.conditional == id && p->entry && p->entry != actual &&
      p->view.entry == p->entry && !p->arms[0] && !p->arms[1] &&
      !f->arm_count && !p->ready && !p->sealed);
    R(state.plan && state.plan_count == 1 && f->source == state.plan->source &&
      nl_syntax_tree_source(state.plan->syntax) == f->source);
    R(state.phase == 1 ? !f->body_owner : f->body_owner == state.plan);
    const NLSemanticContext *c = p->entry;
    size_t main_id = 0;
    for (size_t i = 0; i < c->function_count; ++i)
        if (!strcmp(c->functions[i].name, "main")) {
            R(!main_id);
            main_id = i + 1;
        }
    R(main_id && c->functions[main_id - 1].body == state.plan &&
      !c->functions[main_id - 1].count);
    NLSemanticSnapshot inventory;
    R(nl_semantic_snapshot(c, &inventory) && inventory.bindings == 2 &&
      inventory.places == 6 && inventory.values == 9 && !inventory.domains &&
      !inventory.scopes && !inventory.backing_regions &&
      !inventory.occurrences);
    R(state.count < 2 && state.phase == state.count + 1);
    Observation o = {.unit = (uintptr_t)state.unit,
                     .body = (uintptr_t)state.plan,
                     .fragment = (uintptr_t)f,
                     .entry = (uintptr_t)c,
                     .current_world = (uintptr_t)actual,
                     .artifact_world = (uintptr_t)f->context,
                     .session = state.session,
                     .phase = state.phase};
    size_t symbols[2] = {0};
    for (size_t i = 0; i < 2; ++i) {
        symbols[i] = nl_semantic_find_binding(c, e->names[i]);
        NLSemanticBindingView b;
        NLSemanticPlaceView root;
        NLSemanticValueView package;
        NLSemanticTypeView nominal;
        R(symbols[i] && nl_semantic_binding_view(c, symbols[i], &b) &&
          b.availability == NL_AVAILABLE &&
          nl_semantic_place_view(c, b.place, &root) && root.live &&
          root.independent_root && root.implicit_local &&
          !root.governing_domain && root.fixed_field_count == 2 &&
          nl_semantic_value_view(c, root.current_value, &package) &&
          package.carrier == NL_CARRIER_PLACE &&
          package.owner_place == b.place && package.field_count == 2 &&
          package.type == root.type && root.type == b.type &&
          package.dependencies == NL_DEPENDENCY_FREE &&
          !package.value_dependency_count &&
          nl_semantic_type_view(c, root.type, &nominal) &&
          nominal.kind == NL_TYPE_NOMINAL && nominal.field_count == 2 &&
          nominal.is_copy && nominal.is_discardable);
        o.roots[i] = b.place;
        o.incarnations[i * 3] = root.incarnation;
        o.facts[i * 3] = root.current_fact;
        for (size_t j = 0; j < 2; ++j) {
            NLSemanticPlaceView ch;
            NLSemanticValueView value;
            R(root_matches(c, symbols[i], b.place, j, e->labels[j],
                           e->values[i][j]));
            R(nl_semantic_place_view(c, root.fixed_fields[j], &ch) && ch.live &&
              !ch.independent_root &&
              nl_semantic_value_view(c, ch.current_value, &value) &&
              value.carrier == NL_CARRIER_AGGREGATE &&
              value.aggregate_owner == root.current_value &&
              package.fields[j] == ch.current_value &&
              value.dependencies == NL_DEPENDENCY_FREE &&
              !value.value_dependency_count && value.type == ch.type &&
              value.type == nl_semantic_core_type(c, NL_TYPE_U8));
            o.scalars[i][j] = value.scalar_value;
            o.incarnations[i * 3 + j + 1] = ch.incarnation;
            o.facts[i * 3 + j + 1] = ch.current_fact;
        }
    }
    R(o.roots[0] != o.roots[1]);
    for (size_t i = 0; i < 6; ++i) {
        R(o.incarnations[i] && o.facts[i] &&
          o.incarnations[i] <= inventory.last_incarnation &&
          o.facts[i] <= inventory.last_value_fact);
        for (size_t j = 0; j < i; ++j)
            R(o.incarnations[i] != o.incarnations[j] &&
              o.facts[i] != o.facts[j]);
    }
    const NLCheckedNodeView *block = nl_checked_node_view(f, 1);
    R(block && block->kind == NL_CHECKED_BLOCK && block->item_count == 2);
    NLCheckedNodeId item = block->first_item;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *b = nl_checked_node_view(f, item);
        const NLCheckedNodeView *a =
            b ? nl_checked_node_view(f, b->initializer) : NULL;
        R(b && b->kind == NL_CHECKED_BINDING && b->symbol == symbols[i] && a &&
          a->kind == NL_CHECKED_AGGREGATE && a->argument_count == 2);
        NLCheckedNodeId field = a->first_argument;
        for (size_t j = 0; j < 2; ++j) {
            const NLCheckedNodeView *v = nl_checked_node_view(f, field);
            R(v && v->kind == NL_CHECKED_AGGREGATE_FIELD && v->field_index < 2);
            o.order[i][j] = v->field_index;
            NLSourceView label;
            R(nl_source_view(f->source, v->name, &label) &&
              strlen(e->labels[v->field_index]) == label.length &&
              !memcmp(label.bytes, e->labels[v->field_index], label.length));
            const NLCheckedNodeView *literal =
                nl_checked_node_view(f, v->initializer);
            R(literal && literal->has_scalar_result &&
              literal->scalar_result.known &&
              literal->scalar_result.value == e->values[i][v->field_index]);
            field = v->next_argument;
        }
        R(!field);
        item = b->next_item;
    }
    R(!item && orders_match(&o, e));
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    const NLCheckedNodeView *condition =
        choice ? nl_checked_node_view(f, choice->initializer) : NULL;
    R(choice && choice->kind == NL_CHECKED_IF && !choice->normal_arms &&
      condition &&
      condition->kind ==
          (e->truth ? NL_CHECKED_USIZE_LESS_THAN : NL_CHECKED_USIZE_EQUALITY) &&
      p->view.condition == choice->initializer);
    NLSourceView body_text, whole_text;
    NLSourceSpan translated = {state.source_offset + choice->span.start_byte,
                               state.source_offset + choice->span.end_byte};
    R(translated.end_byte <= state.body_span.end_byte &&
      nl_source_view(f->source, choice->span, &body_text) &&
      nl_source_view(state.unit, translated, &whole_text) &&
      body_text.length == whole_text.length &&
      !memcmp(body_text.bytes, whole_text.bytes, body_text.length));
    NLSemanticValueView boolean;
    R(condition->result_count == 1 &&
      nl_semantic_value_view(c, condition->results[0].value, &boolean) &&
      boolean.type == nl_semantic_core_type(c, NL_TYPE_BOOL) &&
      boolean.scalar_known && boolean.scalar_value == (size_t)e->truth &&
      boolean.carrier == NL_CARRIER_ENDED);
    o.truth = boolean.scalar_value != 0;
    NLCheckedNodeId operand = condition->first_argument;
    for (size_t k = 0; k < 2; ++k) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, operand);
        NLSemanticValueView value;
        R(v && v->kind == NL_CHECKED_USIZE_LITERAL && v->result_count == 1 &&
          nl_semantic_value_view(c, v->results[0].value, &value) &&
          value.type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
          value.scalar_known && value.scalar_value == e->operands[k] &&
          value.carrier == NL_CARRIER_ENDED);
        operand = v->next_argument;
    }
    R(!operand);
    NLIfFieldView unavailable;
    R(!nl_checked_if_field_view(f, id, &unavailable));
    if (state.negative) {
        R(!root_matches(c, symbols[0], o.roots[1], 0, e->labels[0],
                        e->values[0][0]));
        ++state.refusals;
        R(!root_matches(c, symbols[0], o.roots[0], 1, e->labels[0],
                        e->values[0][0]));
        ++state.refusals;
        Expected wrong = *e;
        wrong.order[0][0] = 1 - wrong.order[0][0];
        R(!orders_match(&o, &wrong));
        ++state.refusals;
    }
    printf("P930 session%zu phase%zu input=%zu owner=%zu actual-world=%zu "
           "artifact-world=%zu snapshot=%zu main%zu roots%zu/%zu "
           "inc/fact-distinct6 values[%zu,%zu]/[%zu,%zu] order%zu%zu/%zu%zu "
           "truth%d cut-before-arms public-view-absent\n",
           o.session, o.phase, (size_t)o.unit, (size_t)o.fragment,
           (size_t)o.current_world, (size_t)o.artifact_world, (size_t)o.entry,
           main_id, o.roots[0], o.roots[1], o.scalars[0][0], o.scalars[0][1],
           o.scalars[1][0], o.scalars[1][1], o.order[0][0], o.order[0][1],
           o.order[1][0], o.order[1][1], o.truth);
    state.observations[state.count++] = o;
}
NLCheckStatus __wrap_nl_if_field_prepare(NLCheckedFragment *f,
                                         NLCheckedNodeId id,
                                         const NLSemanticContext *c)
{
    if (!state.active)
        return __real_nl_if_field_prepare(f, id, c);
    state.preparing = true;
    NLCheckStatus status = __real_nl_if_field_prepare(f, id, c);
    state.preparing = false;
    if (status == NL_CHECK_OK)
        observe(f, id, c);
    return status; /* never alter real status or state */
}
#ifdef P930_OOM
void *__real_calloc(size_t, size_t);
void *__real_malloc(size_t);
void *__wrap_calloc(size_t n, size_t size)
{
    if (state.preparing && state.fault == 1 && !state.hit) {
        R(n == 1 && size == sizeof(NLIfFieldProof));
        ++state.hit;
        return NULL;
    }
    return __real_calloc(n, size);
}
void *__wrap_malloc(size_t size)
{
    if (state.preparing && state.fault == 2 && !state.hit &&
        size == sizeof(NLSemanticContext)) {
        ++state.hit;
        return NULL;
    }
    return __real_malloc(size);
}
#endif
static void check_main(NLSemanticContext *c)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLCheckedFragment *f = NULL;
    R(nl_source_create("main()", 6, "<p930-ordinary-entry>", &source) ==
          NL_SOURCE_OK &&
      nl_parser_create(source, &parser) == NL_PARSE_OK &&
      nl_parser_parse_expression_fragment(parser, &tree, NULL) == NL_PARSE_OK);
    state.phase = 2;
    R(nl_semantic_check_expression(c, tree, &f, NULL) == NL_CHECK_OK && f &&
      state.count == 2);
    nl_checked_destroy(f);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
}
static bool same_inventory(NLSemanticSnapshot a, NLSemanticSnapshot b)
{
    return a.types == b.types && a.functions == b.functions &&
           a.bindings == b.bindings && a.values == b.values &&
           a.places == b.places && a.domains == b.domains &&
           a.scopes == b.scopes && a.last_incarnation == b.last_incarnation &&
           a.last_value_fact == b.last_value_fact &&
           a.backing_regions == b.backing_regions &&
           a.raw_intervals == b.raw_intervals && a.occurrences == b.occurrences;
}
static Observation run(const char *path, size_t index, int fault)
{
    static size_t sessions;
    const size_t previous_refusals = state.refusals;
    const bool negative = state.negative;
    state = (Session){.active = true,
                      .negative = negative,
                      .session = ++sessions,
                      .phase = 1,
                      .fault = fault,
                      .refusals = previous_refusals,
                      .expect = &cases[index]};
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    R(nl_source_load(path, &source) == NL_SOURCE_OK &&
      nl_parser_create(source, &parser) == NL_PARSE_OK &&
      nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK &&
      nl_semantic_create(&c) == NL_CHECK_OK);
    state.unit = source;
    NLSemanticSnapshot before, after;
    R(nl_semantic_snapshot(c, &before));
    const NLSyntaxTree *inputs[] = {tree};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(c, inputs, 1, &diagnostic);
    if (fault) {
        R(status == NL_CHECK_OUT_OF_MEMORY && state.hit == 1 &&
          state.count == 0);
        /* Failed registration already destroyed its plan; clear the token
         * without dereferencing it. The original context/input still live. */
        state.plan = NULL;
        state.plan_count = 0;
        state.fault = 0;
        R(nl_semantic_snapshot(c, &after) && same_inventory(before, after));
        printf("P930 actual prepare OOM site%d hit%zu: zero observations, "
               "original inventory unchanged; SAME context retry\n",
               fault, state.hit);
        status = nl_semantic_register_function_unit(c, inputs, 1, &diagnostic);
    }
    R(status == NL_CHECK_OK && state.count == 1);
    check_main(c);
    Observation copied = state.observations[1];
    /* All original borrows end BEFORE any original input/registry owner. */
    state.active = false;
    state.plan = NULL;
    state.unit = NULL;
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return copied;
}
int main(int argc, char **argv)
{
    R(argc == 3);
    char path[1024];
    if (!strcmp(argv[1], "oom")) {
        R(snprintf(path, sizeof(path), "%s/%s", argv[2], cases[0].file) > 0);
        (void)run(path, 0, 1);
        (void)run(path, 0, 2);
        return 0;
    }
    state.negative = !strcmp(argv[1], "negative");
    Observation baseline = {0};
    for (size_t i = 0; i < 4; ++i) {
        R(snprintf(path, sizeof(path), "%s/%s", argv[2], cases[i].file) > 0);
        Observation copied = run(path, i, 0);
        if (!i)
            baseline = copied;
        if (state.negative && i == 2) {
            R(!orders_match(&copied, &cases[0]));
            ++state.refusals;
            R(copied.session != baseline.session);
        }
        if (state.negative && i == 3) {
            R(memcmp(copied.scalars, baseline.scalars,
                     sizeof(copied.scalars)) != 0);
            ++state.refusals;
        }
    }
    if (state.negative)
        R(state.refusals == 26);
    printf("P930 copied scalar observations only; mismatched root/field/order "
           "comparisons rejected%zu; no public authority or C17 admission\n",
           state.refusals);
    return 0;
}
