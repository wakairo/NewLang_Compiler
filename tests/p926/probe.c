#include "../../src/copy_body_internal.h"
#include "../../src/if_field_internal.h"
#include "../support/semantic_check.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define R(x)                                                                   \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P926 line%d %s\n", __LINE__, #x);                 \
            exit(1);                                                           \
        }                                                                      \
    } while (0)
typedef struct {
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *tree;
    NLSemanticContext *context;
} Input;
static Input load(const char *path)
{
    Input in = {0};
    R(nl_source_load(path, &in.source) == 0 &&
      nl_parser_create(in.source, &in.parser) == 0 &&
      nl_parser_parse_function_unit(in.parser, &in.tree, NULL) == 0 &&
      nl_semantic_create(&in.context) == 0);
    return in;
}
static void release(Input *in)
{
    nl_semantic_destroy(in->context);
    nl_syntax_tree_destroy(in->tree);
    nl_parser_destroy(in->parser);
    nl_source_destroy(in->source);
    *in = (Input){0};
}
static NLDefinitionOwned *construct(Input *in)
{
    TestState *before = calloc(1, sizeof(*before));
    R(before && test_state(in->context, before));
    NLDefinitionOwned *w = NULL;
    R(nl_definition_prefix_open(&w) == 0);
    const NLSyntaxTree *units[] = {in->tree};
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s =
        nl_semantic_register_function_unit(in->context, units, 1, &d);
    nl_definition_owned_close(w);
    R(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
      !strcmp(d.diagnostic.diagnostic.code, "P6-JOIN-PRECISION") &&
      test_unchanged(in->context, before));
    free(before);
    return w;
}
static NLDefinitionPrefixResult positive(NLDefinitionOwned *w)
{
    NLDefinitionPrefixResult r = {0};
    NLCheckStatus s = nl_definition_prefix_validate(w, &r);
    printf("prefix status%d registry%d roots%d condition%d entry%d truth%d "
           "roots%zu/%zu IF%zu failure%zu\n",
           s, r.registry_verified, r.roots_verified, r.condition_verified,
           r.entry_verified, r.truth, r.roots[0], r.roots[1], r.conditional,
           r.failure_line);
    fflush(stdout);
    R(s == 0 && r.registry_verified && r.roots_verified &&
      r.condition_verified && r.entry_verified && r.roots[0] == 1 &&
      r.roots[1] == 4);
    for (size_t i = 0; i < 2; ++i) {
        NLDefinitionCallResult c;
        R(nl_definition_owned_validate(w, i, nl_definition_owned_link(w, i),
                                       &c) == 0 &&
          c.pre_verified && c.return_old_verified && c.loan_closed);
    }
    return r;
}
static size_t refusals;
static void refuses(NLDefinitionOwned *w, const char *label)
{
    NLDefinitionPrefixResult r = {0};
    NLCheckStatus s = nl_definition_prefix_validate(w, &r);
    printf("negative %s status%d registry%d roots%d condition%d entry%d "
           "failure%zu\n",
           label, s, r.registry_verified, r.roots_verified,
           r.condition_verified, r.entry_verified, r.failure_line);
    R(s == NL_CHECK_ANALYSIS_PRECISION_LIMIT && !r.entry_verified);
    ++refusals;
}
static void mutations(NLDefinitionOwned *w, NLDefinitionOwned *foreign)
{
    NLDefinitionPrefixResult good = positive(w);
    const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, 0);
    NLCheckedFragment *parent = (NLCheckedFragment *)v->parent;
    NLIfFieldProof *p = parent->if_fields;
    NLSemanticContext *e = p->entry;
    NLTypeEntry *t = &e->types[e->places[good.roots[0] - 1].type - 1];
    char *label = t->field_names[0];
    t->field_names[0] = t->field_names[1];
    t->field_names[1] = label;
    refuses(w,
            "actual nominal declaration index mismatch with retained raw unit");
    label = t->field_names[0];
    t->field_names[0] = t->field_names[1];
    t->field_names[1] = label;
    positive(w);
    NLPlaceId place = e->bindings[0].view.place;
    e->bindings[0].view.place = e->bindings[1].view.place;
    refuses(w, "first original binding spliced to second root");
    e->bindings[0].view.place = place;
    positive(w);
    ++e->places[good.roots[0] - 1].incarnation;
    refuses(w, "same numeric payload wrong incarnation");
    --e->places[good.roots[0] - 1].incarnation;
    positive(w);
    ++e->places[2].current_fact;
    refuses(w, "child stale/wrong ValueFact");
    --e->places[2].current_fact;
    positive(w);
    ++e->last_value_fact;
    refuses(w, "fresh fact counter not derived by source");
    --e->last_value_fact;
    positive(w);
    ++e->last_incarnation;
    refuses(w, "fresh incarnation counter not derived by source");
    --e->last_incarnation;
    positive(w);
    NLDependencyKnowledge deps = e->values[0].dependencies;
    e->values[0].dependencies = NL_DEPENDENCIES_UNKNOWN;
    refuses(w, "literal dependency mismatch");
    e->values[0].dependencies = deps;
    positive(w);
    NLValueCarrier carrier = e->values[0].carrier;
    e->values[0].carrier = NL_CARRIER_LOOSE;
    refuses(w, "field numeric scalar with wrong aggregate carrier");
    e->values[0].carrier = carrier;
    positive(w);
    NLCheckedNodeId first = parent->nodes[0].first_item,
                    second = parent->nodes[first - 1].next_item;
    NLCheckedNodeView saved0 = parent->nodes[first - 1],
                      saved1 = parent->nodes[second - 1];
    parent->nodes[first - 1] = saved1;
    parent->nodes[second - 1] = saved0;
    refuses(w, "whole checked root binding source-order splice");
    parent->nodes[first - 1] = saved0;
    parent->nodes[second - 1] = saved1;
    positive(w);
    NLCheckedNodeView *aggregate = &parent->nodes[saved0.initializer - 1];
    NLCheckedNodeView *field = &parent->nodes[aggregate->first_argument - 1];
    size_t index = field->field_index;
    field->field_index = 1 - index;
    refuses(w, "checked initializer field order/index");
    field->field_index = index;
    positive(w);
    NLCheckedNodeView *literal = &parent->nodes[field->initializer - 1];
    size_t scalar = literal->scalar_result.value;
    ++literal->scalar_result.value;
    refuses(w, "checked literal not source decimal");
    literal->scalar_result.value = scalar;
    positive(w);
    NLCheckedNodeView *condition = &parent->nodes[good.condition - 1];
    scalar = condition->scalar_result.value;
    condition->scalar_result.value = 1 - scalar;
    refuses(w, "computed condition truth falsified");
    condition->scalar_result.value = scalar;
    positive(w);
    NLCheckedNodeView *operand = &parent->nodes[condition->first_argument - 1];
    scalar = operand->scalar_result.value;
    ++operand->scalar_result.value;
    refuses(w, "condition source operand changed");
    operand->scalar_result.value = scalar;
    positive(w);
    NLCheckedKind kind = condition->kind;
    condition->kind = NL_CHECKED_USIZE_LESS_THAN;
    refuses(w, "condition operator changed");
    condition->kind = kind;
    positive(w);
    p->entry = (NLSemanticContext *)p->arms[0]->context;
    p->view.entry = p->entry;
    refuses(w, "genuine after-fork world supplied as common entry");
    p->entry = e;
    p->view.entry = e;
    positive(w);
    const NLDefinitionOwnedLink *other = nl_definition_owned_link(foreign, 0);
    NLFunctionBody *body = e->functions[v->caller_function - 1].body;
    e->functions[v->caller_function - 1].body =
        other->pre->functions[other->caller_function - 1].body;
    refuses(w, "same-text genuine foreign function plan owner");
    e->functions[v->caller_function - 1].body = body;
    positive(w);
}

enum {
    NO_FAULT,
    FOREIGN_UNIT,
    RAW_DECL,
    WRONG_ORDINAL,
    UNIT_OOM_1,
    UNIT_OOM_2,
    VERIFY_OOM
};
static int fault;
static bool unit_phase;
#ifdef P926_OOM
static bool verify_phase, fresh_phase;
#endif
static size_t hits, allocation_requests;
static NLSource *alternate;
void __real_nl_definition_prefix_unit_begin(const NLSource *, size_t);
void __real_nl_definition_prefix_plan(const NLSource *, NLSourceSpan, size_t,
                                      NLFunctionBody *);
NLSourceStatus __real_nl_source_create(const void *, size_t, const char *,
                                       NLSource **);
NLCheckStatus __real_nl_definition_prefix_validate(const NLDefinitionOwned *,
                                                   NLDefinitionPrefixResult *);
void __wrap_nl_definition_prefix_unit_begin(const NLSource *s, size_t count)
{
    unit_phase = true;
    if (fault == FOREIGN_UNIT) {
        R(alternate && alternate != s);
        ++hits;
        __real_nl_definition_prefix_unit_begin(alternate, count);
    } else
        __real_nl_definition_prefix_unit_begin(s, count);
    unit_phase = false;
}
void __wrap_nl_definition_prefix_plan(const NLSource *s, NLSourceSpan span,
                                      size_t function, NLFunctionBody *body)
{
    if (fault == WRONG_ORDINAL && !hits) {
        R(function > 2);
        function = 1;
        ++hits;
    }
    __real_nl_definition_prefix_plan(s, span, function, body);
}
NLSourceStatus __wrap_nl_source_create(const void *bytes, size_t length,
                                       const char *name, NLSource **out)
{
    if (unit_phase && fault == RAW_DECL && !hits) {
        NLSourceView v;
        R(alternate &&
          nl_source_view(alternate,
                         (NLSourceSpan){0, nl_source_length(alternate)}, &v) &&
          v.length == length);
        ++hits;
        return __real_nl_source_create(v.bytes, v.length, name, out);
    }
    return __real_nl_source_create(bytes, length, name, out);
}
NLCheckStatus
__wrap_nl_definition_prefix_validate(const NLDefinitionOwned *w,
                                     NLDefinitionPrefixResult *out)
{
#ifdef P926_OOM
    verify_phase = true;
#endif
    NLCheckStatus s = __real_nl_definition_prefix_validate(w, out);
#ifdef P926_OOM
    verify_phase = false;
#endif
    return s;
}
#ifdef P926_OOM
void *__real_malloc(size_t);
NLCheckStatus __real_nl_semantic_create(NLSemanticContext **);
void *__wrap_malloc(size_t size)
{
    if (unit_phase && (fault == UNIT_OOM_1 || fault == UNIT_OOM_2)) {
        ++allocation_requests;
        size_t target = fault == UNIT_OOM_1 ? 1 : 2;
        if (!hits && allocation_requests == target) {
            ++hits;
            return NULL;
        }
    }
    if (verify_phase && fresh_phase && fault == VERIFY_OOM && !hits) {
        R(size == sizeof(NLSemanticContext));
        ++allocation_requests;
        ++hits;
        return NULL;
    }
    return __real_malloc(size);
}
NLCheckStatus __wrap_nl_semantic_create(NLSemanticContext **out)
{
    fresh_phase = verify_phase;
    NLCheckStatus s = __real_nl_semantic_create(out);
    fresh_phase = false;
    return s;
}
#endif
static void origin_faults(const char *path, const char *changed)
{
    Input in = load(path);
    NLDefinitionOwned *original = construct(&in);
    positive(original);
    for (int mode = FOREIGN_UNIT; mode <= WRONG_ORDINAL; ++mode) {
        if (mode == FOREIGN_UNIT) {
            NLSourceView b;
            R(nl_source_view(in.source,
                             (NLSourceSpan){0, nl_source_length(in.source)},
                             &b) &&
              nl_source_create(b.bytes, b.length,
                               "same-text foreign unit owner",
                               &alternate) == 0);
        }
        if (mode == RAW_DECL)
            R(nl_source_load(changed, &alternate) == 0);
        fault = mode;
        hits = 0;
        NLDefinitionOwned *bad = construct(&in);
        fault = NO_FAULT;
        R(hits == 1);
        if (mode == FOREIGN_UNIT) {
            R(nl_definition_owned_count(bad) == 0 &&
              nl_definition_owned_status(bad) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        } else {
            R(nl_definition_owned_count(bad) == 2);
            refuses(bad,
                    mode == RAW_DECL
                        ? "changed raw nominal only, same function bodies"
                        : "source ordinal substituted as registry FunctionId");
        }
        nl_definition_owned_destroy(bad);
        nl_source_destroy(alternate);
        alternate = NULL;
        positive(original);
        NLDefinitionOwned *retry = construct(&in);
        positive(retry);
        nl_definition_owned_destroy(retry);
        printf("origin fault%d reached, rejected, original owner preserved / "
               "SAME registration context retry OK\n",
               mode);
    }
    release(&in);
    positive(original);
    nl_definition_owned_destroy(original);
    /* Changed declaration is itself legal. Only the mismatched origin above
     * is negative; never declare this ordinary different program invalid. */
    in = load(changed);
    NLDefinitionOwned *legitimate = construct(&in);
    release(&in);
    positive(legitimate);
    nl_definition_owned_destroy(legitimate);
}
static void oom_cases(const char *path)
{
    Input in = load(path);
    for (int mode = UNIT_OOM_1; mode <= UNIT_OOM_2; ++mode) {
        fault = mode;
        hits = allocation_requests = 0;
        NLDefinitionOwned *bad = construct(&in);
        fault = NO_FAULT;
        R(hits == 1 && allocation_requests == (mode == UNIT_OOM_1 ? 1U : 2U) &&
          nl_definition_owned_count(bad) == 0 &&
          nl_definition_owned_status(bad) == NL_CHECK_OUT_OF_MEMORY);
        NLDefinitionPrefixResult r;
        R(nl_definition_prefix_validate(bad, &r) == NL_CHECK_OUT_OF_MEMORY &&
          !r.entry_verified);
        nl_definition_owned_destroy(bad);
        NLDefinitionOwned *retry = construct(&in);
        positive(retry);
        nl_definition_owned_destroy(retry);
        printf("owned raw unit malloc fault%d actual hit1, no partial record, "
               "SAME context retry OK\n",
               mode);
    }
    NLDefinitionOwned *w = construct(&in);
    release(&in);
    positive(w);
    fault = VERIFY_OOM;
    hits = allocation_requests = 0;
    NLDefinitionPrefixResult r;
    R(nl_definition_prefix_validate(w, &r) == NL_CHECK_OUT_OF_MEMORY &&
      hits == 1 && allocation_requests == 1 && !r.entry_verified);
    fault = NO_FAULT;
    positive(w);
    nl_definition_owned_destroy(w);
    puts("fresh prefix machine actual malloc OOM / SAME retained owner retry "
         "OK");
}
static void oracle(NLDefinitionOwned *w, bool frame)
{
    const size_t expected[2][2] = {{frame ? 160 : 149, frame ? 33 : 77},
                                   {frame ? 41 : 28, frame ? 203 : 215}};
    const NLDefinitionOwnedLink *v = nl_definition_owned_link(w, 0);
    const NLSemanticContext *e = v->parent->if_fields->entry;
    R(e->binding_count == 2 && e->place_count == 6 && e->value_count == 9 &&
      e->last_incarnation == 6 && e->last_value_fact == 6 && !e->domain_count &&
      !e->scope_count && !e->region_count && !e->occurrence_count);
    for (size_t i = 0; i < 2; ++i) {
        const NLSemanticPlaceView *root = &e->places[i * 3];
        R(root->independent_root && root->implicit_local &&
          root->governing_domain == 0 && root->incarnation == i * 3 + 1);
        for (size_t j = 0; j < 2; ++j) {
            const NLSemanticPlaceView *child =
                &e->places[root->fixed_fields[j] - 1];
            const NLSemanticValueView *value =
                &e->values[child->current_value - 1];
            R(!child->independent_root &&
              child->parent_aggregate == i * 3 + 1 &&
              child->parent_field_index == j && value->scalar_known &&
              value->scalar_value == expected[i][j] &&
              value->carrier == NL_CARRIER_AGGREGATE &&
              value->dependencies == NL_DEPENDENCY_FREE &&
              !value->value_dependency_count);
        }
    }
    R(v->caller_function == (frame ? 11U : 10U) &&
      v->field.parent == (frame ? 4U : 1U) &&
      v->field.index == (frame ? 1U : 0U));
    for (size_t i = 0; i < 2; ++i) {
        NLDefinitionCallResult c;
        R(nl_definition_owned_validate(w, i, nl_definition_owned_link(w, i),
                                       &c) == 0 &&
          c.old_value == (frame ? 203U : 149U) &&
          c.incoming == (frame ? (i ? 87U : 11U) : (i ? 92U : 51U)) &&
          c.sibling == (frame ? 41U : 77U));
    }
    puts("independent "
         "two-root/child/carrier/incarnation/field/old/sibling/registry oracle "
         "PASS (NO Domain mint)");
}
int main(int argc, char **argv)
{
    R(argc >= 3);
    if (!strcmp(argv[1], "oom")) {
        oom_cases(argv[2]);
        return 0;
    }
    bool negative = !strcmp(argv[1], "negative");
    for (int i = 2; i < argc; ++i) {
        Input in = load(argv[i]);
        NLDefinitionOwned *w = construct(&in);
        release(&in);
        R(nl_definition_owned_status(w) == 0 &&
          nl_definition_owned_count(w) == 2);
        NLDefinitionPrefixResult r = positive(w);
        bool frame = strstr(argv[i], "frame") != NULL;
        R(r.truth != frame);
        oracle(w, frame);
        if (negative) {
            Input other = load(argv[i]);
            NLDefinitionOwned *foreign = construct(&other);
            release(&other);
            mutations(w, foreign);
            nl_definition_owned_destroy(foreign);
        }
        nl_definition_owned_destroy(w);
        if (negative) {
            char alt[1024];
            const char *slash = strrchr(argv[i], '/');
            R(slash);
            size_t prefix = (size_t)(slash - argv[i]);
            R(prefix >= 7 && prefix + 40 < sizeof(alt));
            memcpy(alt, argv[i], prefix - 7);
            snprintf(alt + prefix - 7, sizeof(alt) - prefix + 7,
                     "negatives/%s_decl_swapped.nl", frame ? "frame" : "quota");
            origin_faults(argv[i], alt);
        }
    }
    printf("P926 source-prefix refusals%zu; PARTIAL C memory only / rich HOLD "
           "/ ordinary CHECK3\n",
           refusals);
    return 0;
}
