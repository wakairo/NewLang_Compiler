#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../../src/if_field_internal.h"
#include "../support/node_checked.h"
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t calls, fail_at;
void *__wrap_malloc(size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_malloc(n);
}
void *__wrap_calloc(size_t n, size_t s)
{
    return injecting && calls++ == fail_at ? NULL : __real_calloc(n, s);
}
void *__wrap_realloc(void *p, size_t n)
{
    return injecting && calls++ == fail_at ? NULL : __real_realloc(p, n);
}
static bool valid(TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}
static size_t find_if(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= f->count; ++i)
        if (f->nodes[i - 1].kind == NL_CHECKED_IF)
            return i;
    return 0;
}
static size_t anchor_bytes(const NLCopyCallAnchor *a)
{
    return a->place_count * sizeof(*a->places) +
           a->value_count * sizeof(*a->values) +
           a->scope_count * sizeof(*a->scopes) +
           a->binding_count * sizeof(*a->bindings);
}
static bool controls(const char *file)
{
    TestNode n = {0}, other = {0};
    CHECK(node_checked_load(file, &n) && node_checked_load(file, &other) &&
          valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    const NLCheckedFragment *foreign =
        nl_checked_call_body(other.entry, nl_checked_root(other.entry));
    const size_t id = find_if(f);
    CHECK(id);
    NLIfFieldView view;
    CHECK(nl_checked_if_field_view(f, id, &view));
    NLIfFieldProof *p = f->if_fields;
    CHECK(p && p->view.conditional == id);
    size_t worlds = 4, anchors = anchor_bytes(&p->entry_anchor) +
                                 anchor_bytes(&p->post_anchor);
    size_t graph_bytes = p->parent_count * sizeof(*p->parent_nodes);
    for (size_t a = 0; a < 2; ++a) {
        anchors += anchor_bytes(&p->arm_anchor[a]);
        graph_bytes += p->count[a] * sizeof(*p->nodes[a]);
        for (const NLIfFieldTrace *e = p->first[a]; e; e = e->next) {
            worlds += 1 + (e->before != NULL);
            anchors +=
                anchor_bytes(&e->pre_anchor) + anchor_bytes(&e->post_anchor);
        }
    }
    printf(
        "receipt history_worlds=%zu flat_anchor_bytes=%zu graph_seal_bytes=%zu "
        "(excludes nested semantic allocations)\n",
        worlds, anchors, graph_bytes);
    size_t rejects = 0, equal_byte_ids = 0;
#define VIEW(change)                                                           \
    do {                                                                       \
        NLIfFieldView bad = view;                                              \
        change;                                                                \
        CHECK(nl_checked_if_field_validate(f, id, &bad) ==                     \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejects;                                                             \
    } while (0)
    VIEW(bad.root++);
    VIEW(bad.child++);
    VIEW(bad.index = 1 - bad.index);
    VIEW(bad.sibling++);
    VIEW(bad.base++);
    VIEW(bad.incarnation++);
    VIEW(bad.child_incarnation++);
    VIEW(bad.nominal++);
    VIEW(bad.conditional++);
    VIEW(bad.condition++);
    VIEW(bad.entry = other.context);
    VIEW(bad.post = other.context);
    VIEW(bad.result.value++);
    VIEW(bad.result.type = 1);
    VIEW(bad.arms[0] = bad.arms[1]);
    VIEW(bad.arms[0].change_count++);
    VIEW(bad.arms[1].event_count++);
#undef VIEW
    NLIfFieldView vforeign;
    CHECK(nl_checked_if_field_view(foreign, id, &vforeign) &&
          vforeign.root == view.root && vforeign.post != view.post);
    CHECK(nl_checked_if_field_validate(f, id, &vforeign) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    ++rejects;
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(nl_checked_if_field_validate(f, id, NULL) ==                     \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        CHECK(!valid(&n));                                                     \
        restore;                                                               \
        CHECK(valid(&n));                                                      \
        ++rejects;                                                             \
    } while (0)
    for (size_t a = 0; a < 2; ++a) {
        NLCheckedFragment *arm = (NLCheckedFragment *)p->arms[a];
        const NLSemanticContext *world = arm->context;
        REFUSE(arm->context = p->arms[1 - a]->context, arm->context = world);
        const NLCheckedFragment *owner = arm->if_parent;
        REFUSE(arm->if_parent = foreign, arm->if_parent = owner);
        size_t index = arm->if_arm_index;
        REFUSE(arm->if_arm_index = 1 - index, arm->if_arm_index = index);
        NLCheckedNodeView ar = arm->nodes[arm->root - 1];
        REFUSE(arm->nodes[arm->root - 1].terminates = true,
               arm->nodes[arm->root - 1] = ar);
        NLSemanticContext *c = (NLSemanticContext *)world;
        char *label = c->types[view.nominal - 1].field_names[0];
        REFUSE(c->types[view.nominal - 1].field_names[0] =
                   c->types[view.nominal - 1].field_names[1],
               c->types[view.nominal - 1].field_names[0] = label);
        NLSemanticPlaceView old = c->places[view.root - 1];
        REFUSE(c->places[view.root - 1].current_fact++,
               c->places[view.root - 1] = old);
        NLSemanticBindingView binding = c->bindings[view.base - 1].view;
        REFUSE(c->bindings[view.base - 1].view.availability = NL_CONSUMED,
               c->bindings[view.base - 1].view = binding);
        for (size_t i = 0; i < view.arms[a].event_count; ++i) {
            NLIfFieldEvent e;
            CHECK(nl_checked_if_field_event(f, id, a, i, &e));
#define EVENT(change)                                                          \
    do {                                                                       \
        NLIfFieldEvent bad = e;                                                \
        change;                                                                \
        CHECK(nl_checked_if_field_event_validate(f, id, a, i, &bad) ==         \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejects;                                                             \
    } while (0)
            EVENT(bad.node++);
            EVENT(bad.field.parent++);
            EVENT(bad.field.child++);
            EVENT(bad.field.index = 1 - bad.field.index);
            EVENT(bad.field.parent_fact++);
            EVENT(bad.field.child_fact++);
            EVENT(bad.field.parent_post_fact++);
            EVENT(bad.field.child_post_fact++);
            EVENT(bad.field.old_value++);
            EVENT(bad.result++);
            EVENT(bad.incoming++);
            EVENT(bad.change = !bad.change);
            EVENT(bad.before = other.context);
            EVENT(bad.after = other.context);
#undef EVENT
        }
        for (NLIfFieldTrace *e = p->first[a]; e; e = e->next) {
            NLIfFieldTrace *next = e->next;
            REFUSE(e->next = e, e->next = next); /* bounded graph walk must not
                                                    follow injected cycles */
            NLIfFieldEvent saved = e->view;
            REFUSE(e->view.field.old_value++, e->view = saved);
            if (e->before) {
                const NLCheckedNodeView *op =
                    nl_checked_node_view(arm, e->view.node);
                const NLCheckedNodeView *arg =
                    nl_checked_node_view(arm, op->first_argument);
                NLValueId value = arg->results[0].value;
                NLSemanticValueView ref = e->before->values[value - 1];
                REFUSE(e->before->values[value - 1].reference.writable = false,
                       e->before->values[value - 1] = ref);
                REFUSE(e->before->values[value - 1].reference.scope++,
                       e->before->values[value - 1] = ref);
                REFUSE(e->before->values[value - 1].reference.place++,
                       e->before->values[value - 1] = ref);
            }
        }
        NLIfFieldTrace *first = p->first[a];
        REFUSE(p->first[a] = NULL, p->first[a] = first);
        if (first && first->next) {
            NLIfFieldTrace *second = first->next, *third = second->next;
            first->next = third;
            second->next = first;
            p->first[a] = second;
            CHECK(nl_checked_if_field_validate(f, id, NULL) ==
                      NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                  !valid(&n));
            p->first[a] = first;
            first->next = second;
            second->next = third;
            CHECK(valid(&n));
            ++rejects;
        }
        for (size_t i = 0; i < arm->count; ++i) {
            NLCheckedNodeView original = arm->nodes[i];
            NLCheckedNodeView *node = &arm->nodes[i];
            if (node->field.present) {
                REFUSE(node->field.index = 1 - node->field.index,
                       *node = original);
                REFUSE(node->field.parent_fact++, *node = original);
            }
            if (node->result_count == 1) {
                REFUSE(node->results[0].value = SIZE_MAX, *node = original);
                const NLValueId issued = original.results[0].value;
                if (issued && issued <= c->value_count &&
                    original.type == nl_semantic_core_type(c, NL_TYPE_U8)) {
                    const NLSemanticValueView actual = c->values[issued - 1];
                    for (size_t v = 1; v <= c->value_count; ++v) {
                        const NLSemanticValueView other_value =
                            c->values[v - 1];
                        if (v != issued && other_value.type == actual.type &&
                            actual.scalar_known && other_value.scalar_known &&
                            other_value.scalar_value == actual.scalar_value) {
                            REFUSE(node->results[0].value = v,
                                   *node = original);
                            ++equal_byte_ids;
                            break;
                        }
                    }
                }
            }
        }
    }
    NLSemanticPlaceView root = p->post->places[view.root - 1],
                        sibling = p->post->places[view.sibling - 1];
    REFUSE(p->post->places[view.root - 1].current_fact =
               p->entry->places[view.root - 1].current_fact,
           p->post->places[view.root - 1] = root);
    REFUSE(p->post->places[view.sibling - 1].current_fact++,
           p->post->places[view.sibling - 1] = sibling);
    NLSemanticContext *saved = p->entry;
    REFUSE(p->entry = other.context, p->entry = saved);
    NLCheckedNodeView choice = f->nodes[id - 1];
    REFUSE(f->nodes[id - 1].normal_arms = 1, f->nodes[id - 1] = choice);
    REFUSE(f->nodes[id - 1].initializer++, f->nodes[id - 1] = choice);
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_FIELD_READ) {
            NLCheckedNodeView saved_node = f->nodes[i];
            REFUSE(f->nodes[i].field.parent_fact++, f->nodes[i] = saved_node);
            REFUSE(f->nodes[i].field.old_value++, f->nodes[i] = saved_node);
        }
#undef REFUSE
    printf("equal-byte different source Copy IDs rejected=%zu\n",
           equal_byte_ids);
    TestState *state = malloc(sizeof(*state));
    CHECK(state && test_state(n.context, state));
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckStatus status = nl_checked_if_field_validate(f, id, NULL);
        injecting = false;
        CHECK(test_unchanged(n.context, state));
        if (status == NL_CHECK_OK)
            break;
        CHECK(status == NL_CHECK_OUT_OF_MEMORY);
    }
    CHECK(fail_at && fail_at < 8000);
    printf("IF rejects=%zu validator_OOM=%zu rollback=PASS\n", rejects,
           fail_at);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        V0Program admission = {
            .unit = 1, .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
        calls = 0;
        injecting = true;
        bool ok =
            v0_validate_call(n.entry, nl_checked_root(n.entry), &admission);
        injecting = false;
        CHECK(test_unchanged(n.context, state));
        if (ok)
            break;
        CHECK(admission.copy_body_status == NL_CHECK_OUT_OF_MEMORY);
    }
    CHECK(fail_at && fail_at < 8000);
    printf("IF admission_OOM=%zu explicit_resource=PASS\n", fail_at);
    V0Program program = {.unit = 1,
                         .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
    CHECK(v0_validate_call(n.entry, nl_checked_root(n.entry), &program));
    FILE *out = tmpfile();
    CHECK(out && setvbuf(out, NULL, _IONBF, 0) == 0);
    calls = fail_at = 0;
    injecting = true;
    bool emitted = v0_emit_c(out, &program);
    injecting = false;
    CHECK(emitted && !calls && fclose(out) == 0);
    out = fopen("/dev/full", "w");
    CHECK(out && setvbuf(out, NULL, _IONBF, 0) == 0 &&
          !v0_emit_c(out, &program));
    (void)fclose(out);
    free(state);
    node_checked_destroy(&other);
    node_checked_destroy(&n);
    return true;
}
static bool oom(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    NLSemanticContext *c = NULL;
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *units[] = {unit};
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckStatus s = nl_semantic_register_function_unit(c, units, 1, NULL);
        injecting = false;
        if (s == NL_CHECK_OK)
            break;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("registration OOM positions=%zu rollback=PASS\n", fail_at);
    CHECK(test_state(c, before));
    NLSource *entry = NULL;
    NLSyntaxTree *call = NULL;
    parser = NULL;
    CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
          nl_parser_create(entry, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(parser, &call, NULL) ==
          NL_PARSE_OK);
    nl_parser_destroy(parser);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckedFragment *out = NULL;
        NLCheckStatus s = nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (s == NL_CHECK_OK) {
            nl_checked_destroy(out);
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && !out && test_unchanged(c, before));
    }
    CHECK(fail_at > 0 && fail_at < 8000);
    printf("main-call OOM positions=%zu rollback=PASS artifact=NULL\n",
           fail_at);
    free(before);
    nl_syntax_tree_destroy(call);
    nl_source_destroy(entry);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(unit);
    nl_source_destroy(source);
    return true;
}
static bool driver_oom(const char *path)
{
    FILE *diagnostics = tmpfile();
    CHECK(diagnostics);
    int saved = dup(STDERR_FILENO);
    CHECK(saved >= 0 && fflush(stderr) == 0 &&
          dup2(fileno(diagnostics), STDERR_FILENO) >= 0);
    bool ok = true;
    size_t host_oom_count = 0;
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        FILE *output = tmpfile();
        if (!output || setvbuf(output, NULL, _IONBF, 0) != 0) {
            ok = false;
            break;
        }
        CHECK(fflush(stderr) == 0 && fflush(diagnostics) == 0 &&
              ftruncate(fileno(diagnostics), 0) == 0 &&
              fseek(diagnostics, 0, SEEK_SET) == 0);
        calls = 0;
        injecting = true;
        int result = compile_v0_to(path, false, NULL, output);
        injecting = false;
        long length = ftell(output);
        if (fclose(output) != 0) {
            ok = false;
            break;
        }
        if (result == 0) {
            ok = length > 0;
            break;
        }
        bool host_oom = false;
        if (result == 3) {
            CHECK(fflush(stderr) == 0 && fflush(diagnostics) == 0 &&
                  fseek(diagnostics, 0, SEEK_SET) == 0);
            char buffer[2048] = {0};
            const size_t read_count =
                fread(buffer, 1, sizeof(buffer) - 1, diagnostics);
            CHECK(!ferror(diagnostics));
            buffer[read_count] = '\0';
            host_oom = strstr(buffer, "error(host)") &&
                       strstr(buffer, "OUT-OF-MEMORY");
            CHECK(fseek(diagnostics, 0, SEEK_END) == 0);
            if (host_oom)
                ++host_oom_count;
        }
        if ((result != 1 && !host_oom) || length != 0) {
            (void)fprintf(diagnostics,
                          "driver stop point=%zu status=%d length=%ld\n",
                          fail_at, result, length);
            ok = false;
            break;
        }
    }
    CHECK(fflush(stderr) == 0 && dup2(saved, STDERR_FILENO) >= 0 &&
          close(saved) == 0);
    if (!ok) {
        CHECK(fflush(diagnostics) == 0 && fseek(diagnostics, 0, SEEK_SET) == 0);
        char line[512];
        while (fgets(line, sizeof(line), diagnostics))
            fputs(line, stderr);
    }
    CHECK(fclose(diagnostics) == 0 && ok && fail_at && fail_at < 8000);
    printf("normal driver OOM positions=%zu existing_host_oom_CLI3=%zu "
           "zero_partial_C=PASS\n",
           fail_at, host_oom_count);
    return true;
}
/* Snapshot the real registration/actual-call public state on semantic and
 * precision failure; no semantic seed or invented owning value. */
static bool rejection(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSemanticContext *c = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&c) == NL_CHECK_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(c, before));
    const NLSyntaxTree *units[] = {unit};
    NLFunctionUnitDiagnostic diagnostic = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(c, units, 1, &diagnostic);
    const char *phase = "registration";
    if (status == NL_CHECK_OK) {
        CHECK(test_state(c, before));
        NLSource *entry = NULL;
        NLParser *call_parser = NULL;
        NLSyntaxTree *call = NULL;
        NLCheckedFragment *out = NULL;
        CHECK(nl_source_create("main()", 6, "entry", &entry) == NL_SOURCE_OK &&
              nl_parser_create(entry, &call_parser) == NL_PARSE_OK &&
              nl_parser_parse_expression_fragment(call_parser, &call, NULL) ==
                  NL_PARSE_OK);
        status = nl_semantic_check_expression(c, call, &out, NULL);
        CHECK(!out);
        nl_syntax_tree_destroy(call);
        nl_parser_destroy(call_parser);
        nl_source_destroy(entry);
        phase = "actual_main";
    }
    CHECK(status != NL_CHECK_OK && status != NL_CHECK_OUT_OF_MEMORY &&
          status != NL_CHECK_INTERNAL_ERROR && test_unchanged(c, before));
    printf("rejection phase=%s status=%d rollback=PASS artifact=NULL\n", phase,
           (int)status);
    free(before);
    nl_semantic_destroy(c);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "reject") == 0)
        return rejection(argv[2]) ? 0 : 1;
    if (argc < 3)
        return 1;
    for (int i = 1; i < argc; ++i)
        if (!controls(argv[i]))
            return 1;
    return oom(argv[1]) && driver_oom(argv[1]) ? 0 : 1;
}
