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
static size_t anchor_bytes(const NLCopyCallAnchor *a)
{
    return a->place_count * sizeof(*a->places) +
           a->value_count * sizeof(*a->values) +
           a->scope_count * sizeof(*a->scopes) +
           a->binding_count * sizeof(*a->bindings);
}
static bool controls(const char *file)
{
    TestNode n = {0}, foreign = {0};
    CHECK(node_checked_load(file, &n) && node_checked_load(file, &foreign) &&
          valid(&n));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    NLCheckedNodeId call = 0;
    NLCopyUnitView unit;
    for (size_t i = 1; i <= caller->count; ++i)
        if (nl_checked_copy_unit_view(caller, i, &unit))
            call = i;
    CHECK(call &&
          nl_checked_copy_unit_validate(caller, call, &unit) == NL_CHECK_OK);
    NLCheckedFragment *f = (NLCheckedFragment *)unit.body;
    NLIfFieldProof *p = f->if_fields;
    CHECK(p && p->one_normal);
    const size_t id = p->view.conditional;
    NLIfReturnView v;
    CHECK(nl_checked_if_return_view(f, id, &v));
    size_t worlds = 7, bytes = anchor_bytes(&p->entry_anchor) +
                               anchor_bytes(&p->post_anchor) +
                               anchor_bytes(&p->return_anchor) +
                               anchor_bytes(&p->exit_anchor);
    size_t graph_bytes = p->parent_count * sizeof(*p->parent_nodes);
    for (size_t a = 0; a < 3; ++a) {
        if (a < 2) {
            bytes += anchor_bytes(&p->arm_anchor[a]);
            graph_bytes += p->count[a] * sizeof(*p->nodes[a]);
        }
        for (const NLIfFieldTrace *e = p->first[a]; e; e = e->next) {
            worlds += 1 + (e->before != NULL);
            bytes +=
                anchor_bytes(&e->pre_anchor) + anchor_bytes(&e->post_anchor);
        }
    }
    printf("return history_worlds=%zu flat_anchor_bytes=%zu "
           "graph_seal_bytes=%zu (excludes nested semantic storage)\n",
           worlds, bytes, graph_bytes);
    size_t rejects = 0;
#define VIEW(change)                                                           \
    do {                                                                       \
        NLIfReturnView bad = v;                                                \
        change;                                                                \
        CHECK(nl_checked_if_return_validate(f, id, &bad) ==                    \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejects;                                                             \
    } while (0)
    VIEW(bad.normal_arm = 1 - bad.normal_arm);
    VIEW(bad.return_node++);
    VIEW(bad.replay_root++);
    VIEW(bad.field.root++);
    VIEW(bad.field.child++);
    VIEW(bad.field.index = 1 - bad.field.index);
    VIEW(bad.field.incarnation++);
    VIEW(bad.field.child_incarnation++);
    VIEW(bad.field.condition++);
    VIEW(bad.field.result.value++);
    VIEW(bad.field.entry = foreign.context);
    VIEW(bad.field.post = bad.return_ended);
    VIEW(bad.return_live = bad.field.post);
    VIEW(bad.return_ended = bad.return_live);
    VIEW(bad.returned.type++);
    VIEW(bad.returned.value++);
    VIEW(bad.replay_events++);
    VIEW(bad.replay_changes++);
#define REFUSE(change, restore)                                                \
    do {                                                                       \
        change;                                                                \
        CHECK(nl_checked_if_return_validate(f, id, NULL) ==                    \
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT &&                         \
              !valid(&n));                                                     \
        restore;                                                               \
        CHECK(valid(&n));                                                      \
        ++rejects;                                                             \
    } while (0)
    const NLCheckedFragment *foreign_caller =
        nl_checked_call_body(foreign.entry, nl_checked_root(foreign.entry));
    NLCheckedFragment *other =
        (NLCheckedFragment *)nl_checked_call_body(foreign_caller, call);
    NLIfReturnView unrelated;
    CHECK(nl_checked_if_return_view(other, id, &unrelated));
    CHECK(nl_checked_if_return_validate(f, id, &unrelated) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    ++rejects;
    size_t equal_copy_rejects = 0;
    for (size_t i = 1; i <= f->context->value_count; ++i) {
        NLSemanticValueView value = f->context->values[i - 1];
        NLSemanticValueView original =
            p->post->values[p->first[2]->view.result - 1];
        if (i != p->first[2]->view.result && value.type == original.type &&
            value.scalar_known && original.scalar_known &&
            value.scalar_value == original.scalar_value) {
            NLIfFieldEvent bad = p->first[2]->view;
            bad.result = i;
            bad.field.old_value = i;
            CHECK(nl_checked_if_return_event_validate(f, id, 2, 0, &bad) ==
                  NL_CHECK_ANALYSIS_PRECISION_LIMIT);
            printf("same-byte old Copy id=%zu differs from issued id=%zu "
                   "rejected\n",
                   i, p->first[2]->view.result);
            ++rejects;
            ++equal_copy_rejects;
        }
    }
    printf("equal-byte different Copy origin rejects=%zu\n",
           equal_copy_rejects);
#undef VIEW
    NLCheckedNodeView choice = f->nodes[id - 1];
    REFUSE(f->nodes[id - 1].normal_arms = 2, f->nodes[id - 1] = choice);
    REFUSE(f->nodes[id - 1].tail++, f->nodes[id - 1] = choice);
    REFUSE(f->nodes[id - 1].initializer++, f->nodes[id - 1] = choice);
    REFUSE(p->replaying = true, p->replaying = false);
    size_t normal = p->return_view.normal_arm;
    REFUSE(p->return_view.normal_arm = 1 - normal,
           p->return_view.normal_arm = normal);
    const NLControlTarget *target = p->exit_target;
    REFUSE(p->exit_target = NULL, p->exit_target = target);
    const NLControlState *exit_state = p->exit_state;
    REFUSE(p->exit_state = NULL, p->exit_state = exit_state);
    for (size_t a = 0; a < 3; ++a) {
        NLCheckedFragment *graph = a == 2 ? f : (NLCheckedFragment *)p->arms[a];
        for (NLIfFieldTrace *e = p->first[a]; e; e = e->next) {
            NLIfFieldEvent observed = e->view;
#define EVENT(change)                                                          \
    do {                                                                       \
        NLIfFieldEvent bad = observed;                                         \
        change;                                                                \
        CHECK(nl_checked_if_return_event_validate(f, id, a, 0, &bad) ==        \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejects;                                                             \
    } while (0)
            EVENT(bad.node++);
            EVENT(bad.field.parent++);
            EVENT(bad.field.child++);
            EVENT(bad.field.index = 1 - bad.field.index);
            EVENT(bad.field.parent_fact++);
            EVENT(bad.field.child_fact++);
            EVENT(bad.field.old_value++);
            EVENT(bad.result++);
            EVENT(bad.incoming++);
            EVENT(bad.after = foreign.context);
            if (e->before && e->view.result) {
                NLSemanticValueView issued_old =
                    e->before->values[e->view.result - 1];
                for (size_t j = 1; j <= e->before->value_count; ++j) {
                    const NLSemanticValueView other_old =
                        e->before->values[j - 1];
                    if (j != e->view.result &&
                        other_old.type == issued_old.type &&
                        issued_old.scalar_known && other_old.scalar_known &&
                        other_old.scalar_value == issued_old.scalar_value) {
                        EVENT(bad.result = j);
                        EVENT(bad.field.old_value = j);
                        printf("same-byte foreign old Copy id=%zu issued=%zu "
                               "rejected\n",
                               j, e->view.result);
                        break;
                    }
                }
            }
#undef EVENT
            NLIfFieldTrace *next = e->next;
            REFUSE(e->next = e, e->next = next);
            REFUSE(e->view.result++, e->view = observed);
            const NLCheckedFragment *origin = e->origin;
            REFUSE(e->origin = other, e->origin = origin);
            size_t origin_stream = e->stream_index;
            REFUSE(e->stream_index = 3, e->stream_index = origin_stream);
            if (e->before) {
                const NLCheckedNodeView *op =
                    nl_checked_node_view(graph, e->view.node);
                const NLCheckedNodeView *arg =
                    nl_checked_node_view(graph, op->first_argument);
                NLValueId refid = arg->results[0].value;
                NLSemanticValueView ref = e->before->values[refid - 1];
                REFUSE(e->before->values[refid - 1].reference.scope++,
                       e->before->values[refid - 1] = ref);
                REFUSE(e->before->values[refid - 1].reference.writable = false,
                       e->before->values[refid - 1] = ref);
            }
        }
        NLIfFieldTrace *first = p->first[a];
        if (first) {
            REFUSE(p->first[a] = NULL, p->first[a] = first);
        }
        for (size_t i = 0; i < graph->count; ++i) {
            NLCheckedNodeView saved = graph->nodes[i];
            if (saved.field.present) {
                REFUSE(graph->nodes[i].field.index = 1 - saved.field.index,
                       graph->nodes[i] = saved);
                REFUSE(graph->nodes[i].field.parent_fact++,
                       graph->nodes[i] = saved);
            }
            if (saved.result_count) {
                REFUSE(graph->nodes[i].results[0].value = SIZE_MAX,
                       graph->nodes[i] = saved);
            }
            if (saved.kind == NL_CHECKED_RETURN) {
                REFUSE(graph->nodes[i].terminates = false,
                       graph->nodes[i] = saved);
                REFUSE(graph->nodes[i].returned.value++,
                       graph->nodes[i] = saved);
            }
        }
        if (a < 2) {
            NLCheckedNodeView saved = graph->nodes[graph->root - 1];
            REFUSE(graph->nodes[graph->root - 1].terminates = !saved.terminates,
                   graph->nodes[graph->root - 1] = saved);
            const NLCheckedFragment *owner = graph->if_parent;
            REFUSE(graph->if_parent = other, graph->if_parent = owner);
            NLSemanticContext *world = (NLSemanticContext *)graph->context;
            NLSemanticBindingView binding =
                world->bindings[p->view.base - 1].view;
            REFUSE(world->bindings[p->view.base - 1].view.availability =
                       binding.availability == NL_AVAILABLE ? NL_CONSUMED
                                                            : NL_AVAILABLE,
                   world->bindings[p->view.base - 1].view = binding);
        }
    }
    if (p->first[0] && p->first[1]) {
        NLIfFieldTrace *a = p->first[0], *b = p->first[1];
        CHECK(a->view.node == b->view.node &&
              a->view.result == b->view.result &&
              a->view.incoming == b->view.incoming);
        printf("same-number foreign streams node=%zu old=%zu incoming=%zu "
               "different_worlds=YES\n",
               a->view.node, a->view.result, a->view.incoming);
        p->first[0] = b;
        p->first[1] = a;
        NLCheckStatus swapped = nl_checked_if_return_validate(f, id, NULL);
        bool admitted = valid(&n);
        p->first[0] = a;
        p->first[1] = b;
        if (swapped != NL_CHECK_ANALYSIS_PRECISION_LIMIT || admitted)
            fprintf(stderr, "foreign stream swap status=%d admission=%d\n",
                    (int)swapped, (int)admitted);
        CHECK(swapped == NL_CHECK_ANALYSIS_PRECISION_LIMIT && !admitted &&
              valid(&n));
        ++rejects;
    }
    if (p->return_live->scope_count) {
        bool active = p->return_live->scopes[0].active;
        REFUSE(p->return_live->scopes[0].active = !active,
               p->return_live->scopes[0].active = active);
    }
    NLSemanticPlaceView root = p->return_live->places[p->view.root - 1];
    REFUSE(p->return_live->places[p->view.root - 1].current_fact++,
           p->return_live->places[p->view.root - 1] = root);
    NLSemanticContext *ended = (NLSemanticContext *)p->return_view.return_ended;
    NLSemanticPlaceView dead = ended->places[p->view.root - 1];
    REFUSE(ended->places[p->view.root - 1].live = !dead.live,
           ended->places[p->view.root - 1] = dead);
    NLSemanticPlaceView sibling = p->post->places[p->view.sibling - 1];
    REFUSE(p->post->places[p->view.sibling - 1].current_fact++,
           p->post->places[p->view.sibling - 1] = sibling);
#undef REFUSE
    NLCopyBodyProof *cp = f->copy_body;
    size_t operation_worlds = 0;
    for (const CopyEvent *e = cp->first; e; e = e->next)
        operation_worlds += (e->before != NULL) + (e->after != NULL);
    printf("unit call_exit_worlds=4 additional_operation_worlds=%zu "
           "extra_flat_anchor_bytes=%zu (excludes "
           "nested semantic storage)\n",
           operation_worlds,
           anchor_bytes(&cp->entry_anchor) +
               anchor_bytes(&cp->normal_end_anchor) +
               anchor_bytes(&cp->handoff_anchor));
#define UNIT(change)                                                           \
    do {                                                                       \
        NLCopyUnitView bad = unit;                                             \
        change;                                                                \
        CHECK(nl_checked_copy_unit_validate(caller, call, &bad) ==             \
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);                              \
        ++rejects;                                                             \
    } while (0)
    UNIT(bad.body = other);
    UNIT(bad.function++);
    UNIT(bad.conditional++);
    UNIT(bad.parameters.parameters[0]++);
    UNIT(bad.parameters.field.parent++);
    UNIT(bad.parameters.field.child++);
    UNIT(bad.parameters.field.parent_fact++);
    UNIT(bad.parameters.field.index = 1 - bad.parameters.field.index);
    UNIT(bad.postcall = bad.normal_end);
    UNIT(bad.normal_end = bad.parameters.entry);
    UNIT(bad.parameters.arguments[0]++);
    UNIT(bad.parameters.loan++);
#undef UNIT
    const NLCheckedFragment *own = cp->owner;
    cp->owner = other;
    CHECK(nl_checked_copy_unit_validate(caller, call, NULL) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          !valid(&n));
    cp->owner = own;
    const NLSource *issued_source = cp->source;
    cp->source = foreign.entry_source;
    CHECK(nl_checked_copy_unit_validate(caller, call, NULL) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          !valid(&n));
    cp->source = issued_source;
    NLSemanticPlaceView post = cp->handoff->places[p->view.child - 1];
    cp->handoff->places[p->view.child - 1].current_fact++;
    CHECK(nl_checked_copy_unit_validate(caller, call, NULL) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          !valid(&n));
    cp->handoff->places[p->view.child - 1] = post;
    const NLControlTarget *ft = f->function_target;
    f->function_target = other->function_target;
    CHECK(nl_checked_copy_unit_validate(caller, call, NULL) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          !valid(&n));
    f->function_target = (NLControlTarget *)ft;
    CHECK(nl_checked_copy_unit_validate(caller, call, NULL) == NL_CHECK_OK &&
          valid(&n));
    TestState *snapshot = malloc(sizeof(*snapshot));
    CHECK(snapshot && test_state(n.context, snapshot));
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        calls = 0;
        injecting = true;
        NLCheckStatus s = nl_checked_copy_unit_validate(caller, call, NULL);
        injecting = false;
        CHECK(test_unchanged(n.context, snapshot));
        if (s == NL_CHECK_OK)
            break;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY);
    }
    CHECK(fail_at && fail_at < 8000);
    printf("unit-call rejects=%zu validator_OOM=%zu rollback=PASS\n", rejects,
           fail_at);
    for (fail_at = 0; fail_at < 8000; ++fail_at) {
        V0Program program = {
            .unit = 1, .u8 = nl_semantic_core_type(n.context, NL_TYPE_U8)};
        calls = 0;
        injecting = true;
        bool ok = v0_validate_call(n.entry, nl_checked_root(n.entry), &program);
        injecting = false;
        CHECK(test_unchanged(n.context, snapshot));
        if (ok)
            break;
        CHECK(program.copy_body_status == NL_CHECK_OUT_OF_MEMORY);
    }
    CHECK(fail_at && fail_at < 8000);
    printf("return admission_OOM=%zu explicit_resource=PASS\n", fail_at);
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
    free(snapshot);
    node_checked_destroy(&foreign);
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
