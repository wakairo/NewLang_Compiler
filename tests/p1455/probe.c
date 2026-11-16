/* P1455 reuses the adopted P-owned loader/CHECK/allocation hooks only.
 * The old fixture-specific inspect()/positive() are never called here. */
#define main p1435_original_main
#define __wrap_malloc old_p1435_malloc
#define __wrap_nl_sem_clone old_p1435_clone
#include "../p1435/probe.c"
#undef main
#undef __wrap_malloc
#undef __wrap_nl_sem_clone
#include "../../src/pure_u8_internal.h"
#include "../../src/readonly_u8_consumer_internal.h"

static bool in_result;
void *__wrap_malloc(size_t size)
{
    if (!hit && ((fault == 2 && in_clone) || (fault == 4 && in_result) ||
                 (fault == 3 && size == 131072))) {
        hit = true;
        return NULL;
    }
    return __real_malloc(size);
}
NLCheckStatus __wrap_nl_sem_clone(const NLSemanticContext *c,
                                  NLSemanticContext **out)
{
    bool prefix = false;
    if (fault == 2 && !hit && c->region_count == 2)
        for (size_t i = 0; i < c->binding_count; ++i)
            prefix |= !c->bindings[i].hidden && c->bindings[i].name &&
                      !strcmp(c->bindings[i].name, "payload_copy");
    const bool saved = in_clone;
    in_clone = prefix;
    NLCheckStatus status = __real_nl_sem_clone(c, out);
    in_clone = saved;
    return status;
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *p,
                                              NLCheckedFragment *b,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    const NLCheckedNodeView *n = nl_checked_node_view(p, id);
    const NLSemanticContext *c = nl_checked_context(p);
    const bool saved = in_result;
    in_result = fault == 4 && !hit && n && c && c->functions && n->function &&
                n->function <= c->function_count &&
                !strcmp(c->functions[n->function - 1].name, "gate_byte");
    NLCheckStatus status = __real_nl_h_custody_seal_result(p, b, id, world);
    in_result = saved;
    return status;
}

typedef struct {
    size_t relay, installer, payload, closed_loans, changes;
    const NLCheckedFragment *origin;
    NLCheckedFragment *relay_owner;
    NLCheckedNodeId relay_call;
    NLCheckedFragment *installer_owner;
    NLCheckedNodeId installer_call;
} CompositionTrace;

static bool composition(const NLCheckedFragment *f, CompositionTrace *t,
                        size_t depth)
{
    CHECK(f && depth < 32);
    if (!t->origin) {
        CHECK(depth == 0);
        t->origin = f;
    }
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        CHECK(n);
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->type == u8) {
            CHECK(c->functions && n->function &&
                  n->function <= c->function_count);
            const NLFunctionEntry *fn = &c->functions[n->function - 1];
            if (!strcmp(fn->name, "gate_byte")) {
                const NLCheckedFragment *body = nl_checked_call_body(f, id);
                const NLCheckedNodeView *flag =
                    nl_checked_node_view(f, n->first_argument);
                const NLCheckedNodeView *arg =
                    flag ? nl_checked_node_view(f, flag->next_argument) : NULL;
                CHECK(fn->count == 2 && fn->parameters &&
                      fn->parameters[0] ==
                          nl_semantic_core_type(c, NL_TYPE_BOOL) &&
                      fn->parameters[1] == u8 && n->argument_count == 2 &&
                      n->body_backed && body && fn->body &&
                      body->body_owner == fn->body &&
                      body->source == fn->body->source && flag && arg &&
                      !arg->next_argument && flag->result_count == 1 &&
                      arg->type == u8 && arg->result_count == 1 &&
                      n->result_count == 1);
                NLSemanticValueView input, result, condition;
                CHECK(
                    nl_semantic_value_view(c, arg->results[0].value, &input) &&
                    nl_semantic_value_view(c, flag->results[0].value,
                                           &condition) &&
                    nl_semantic_value_view(c, n->results[0].value, &result));
                CHECK(!input.scalar_known && !condition.scalar_known &&
                      !result.scalar_known && !input.reference_count &&
                      !input.owner_place && !input.domain &&
                      !result.reference_count && !result.owner_place &&
                      !result.domain);
                NLReadonlySelection selection = {0};
                CHECK(nl_readonly_selection_consumer(t->origin, f, id,
                                                     &selection) &&
                      selection.body == body && selection.parameters[0] &&
                      selection.parameters[1] &&
                      selection.parameters[0] != selection.parameters[1]);
                printf("selector owner=%p call=%zu actual-bool/u8 Unknown; "
                       "result Unknown; "
                       "source/body/seal/world/formals=OK\n",
                       (const void *)f, id);
                NLCopyBodyView view;
                const bool copy = nl_checked_copy_body_view(f, id, &view);
                NLCheckStatus validated = NL_CHECK_OK;
                if (copy)
                    validated = nl_checked_copy_body_validate(f, id, &view);
                CHECK(validated == NL_CHECK_OK);
                printf("relay owner=%p world=%p call=%zu function=%zu "
                       "body-owner-exact=1 actual-known=%d result-known=%d "
                       "actual-deps=%d result-deps=%d Copy-view=%d ",
                       (const void *)f, (const void *)c, id, n->function,
                       input.scalar_known, result.scalar_known,
                       (int)input.dependencies, (int)result.dependencies, copy);
                if (copy)
                    printf("Copy-validate=%d\n", (int)validated);
                else
                    printf("Copy-validate=N/A (generic typed body)\n");
                ++t->relay;
                t->relay_owner = (NLCheckedFragment *)f;
                t->relay_call = id;
            }
        }
        if (n->kind == NL_CHECKED_REGISTERED_CALL && n->type == u8 &&
            n->argument_count == 2 && c->functions && n->function &&
            !strcmp(c->functions[n->function - 1].name, "install_result")) {
            NLCopyBodyView view;
            NLCopyBodyParameters parameters;
            NLCopyBodyEventView event;
            const bool sealed = nl_h_custody_result_valid(f, id);
            const bool have_view = nl_checked_copy_body_view(f, id, &view);
            const NLCheckStatus body_status =
                have_view ? nl_checked_copy_body_validate(f, id, &view)
                          : NL_CHECK_SEMANTIC_ERROR;
            const bool have_parameters =
                nl_checked_copy_body_parameters(f, id, &parameters);
            const NLCheckStatus parameters_status =
                have_parameters ? nl_checked_copy_body_parameters_validate(
                                      f, id, &parameters)
                                : NL_CHECK_SEMANTIC_ERROR;
            const bool have_event =
                nl_checked_copy_body_event(f, id, 0, &event);
            const NLCheckStatus event_status =
                have_event
                    ? nl_checked_copy_body_event_validate(f, id, 0, &event)
                    : NL_CHECK_SEMANTIC_ERROR;
            printf("T diagnostic seal=%d view=%d body-status=%d params=%d "
                   "params-status=%d event=%d event-status=%d\n",
                   sealed, have_view, (int)body_status, have_parameters,
                   (int)parameters_status, have_event, (int)event_status);
            CHECK(have_view && have_parameters && have_event);
            printf("T diagnostic events=%zu kind=%d entry=%p handoff=%p "
                   "present=%d access=%d index=%zu parent=%zu child=%zu "
                   "formal=%zu/%zu result=%zu/%zu\n",
                   view.event_count, (int)event.kind,
                   (const void *)parameters.entry,
                   (const void *)parameters.handoff, parameters.field.present,
                   (int)parameters.field.access, parameters.field.index,
                   parameters.field.parent, parameters.field.child,
                   parameters.parameters[0], parameters.parameters[1],
                   n->results[0].value, view.result.value);
            /* The distinct T Copy protocol has its own public validators;
             * the H-body seal above belongs to the readonly scalar call. */
            CHECK(nl_checked_copy_body_view(f, id, &view) &&
                  nl_checked_copy_body_validate(f, id, &view) == NL_CHECK_OK &&
                  nl_checked_copy_body_parameters(f, id, &parameters) &&
                  nl_checked_copy_body_parameters_validate(
                      f, id, &parameters) == NL_CHECK_OK &&
                  view.event_count == 1 &&
                  nl_checked_copy_body_event(f, id, 0, &event) &&
                  nl_checked_copy_body_event_validate(f, id, 0, &event) ==
                      NL_CHECK_OK &&
                  event.kind == NL_COPY_BODY_CHANGE && parameters.entry &&
                  parameters.handoff && parameters.field.present &&
                  parameters.field.access == NL_ACCESS_WRITE &&
                  parameters.field.index == 0 && parameters.field.parent &&
                  parameters.field.child && parameters.parameters[0] &&
                  parameters.parameters[1] && view.result.type == u8 &&
                  n->result_count == 1 &&
                  n->results[0].value == view.result.value);
            NLSemanticPlaceView before, after;
            CHECK(nl_semantic_place_view(parameters.entry,
                                         parameters.field.child, &before) &&
                  nl_semantic_place_view(parameters.handoff,
                                         parameters.field.child, &after) &&
                  before.live && after.live &&
                  before.incarnation == after.incarnation &&
                  before.current_fact != after.current_fact &&
                  before.current_value != after.current_value);
            printf("T installer owner=%p call=%zu function=%zu entry=%p "
                   "handoff=%p "
                   "original=%zu field=%zu child=%zu old-value=%zu "
                   "postcurrent=%zu "
                   "Copy/body/parameters/event=OK\n",
                   (const void *)f, id, n->function,
                   (const void *)parameters.entry,
                   (const void *)parameters.handoff, parameters.field.parent,
                   parameters.field.index, parameters.field.child,
                   before.current_value, after.current_value);
            ++t->installer;
            t->installer_owner = (NLCheckedFragment *)f;
            t->installer_call = id;
        }
        if (n->kind == NL_CHECKED_LINK_READ && n->type == u8) {
            CHECK(n->field.present && n->field.index == 1 &&
                  n->field.access == NL_ACCESS_READ);
            ++t->payload;
        }
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(n->loan.body_nonescape_proved &&
                  n->loan.normal_result_forwarded && n->loan.scope &&
                  n->loan.scope <= c->scope_count &&
                  !c->scopes[n->loan.scope - 1].active);
            ++t->closed_loans;
        }
        if (n->kind == NL_CHECKED_REPLACE && n->type == u8) {
            if (n->field.present)
                ++t->changes;
        }
        const NLCheckedFragment *child = nl_checked_call_body(f, id);
        if (child)
            CHECK(composition(child, t, depth + 1));
        if (n->kind == NL_CHECKED_MATCH || n->kind == NL_CHECKED_IF)
            for (size_t arm = 0; arm < n->item_count; ++arm) {
                child = n->kind == NL_CHECKED_MATCH
                            ? nl_checked_match_arm(f, id, arm)
                            : nl_checked_if_arm(f, id, arm);
                CHECK(composition(child, t, depth + 1));
            }
    }
    return true;
}

static bool negative_exact(const char *bad, const char *control,
                           const char *code)
{
    Program p = {0}, valid = {0};
    CHECK(load(bad, &p) && load(control, &valid));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    NLFunctionUnitDiagnostic d = {0};
    const NLCheckStatus status = define(&p, &d);
    printf("negative status=%d code=%s artifact=%p\n", (int)status,
           d.diagnostic.diagnostic.code, (void *)p.artifact);
    CHECK(status == NL_CHECK_SEMANTIC_ERROR && !p.artifact &&
          !strcmp(d.diagnostic.diagnostic.code, code) &&
          nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)));
    NLCheckedFragment *refused = NULL;
    CHECK(nl_semantic_check_expression(p.context, p.call, &refused, NULL) ==
              NL_CHECK_SEMANTIC_ERROR &&
          !refused && nl_copy_state_matches(p.context, &anchor));
    const NLSyntaxTree *units[] = {valid.unit};
    CHECK(nl_semantic_register_function_unit(p.context, units, 1, &d) ==
              NL_CHECK_OK &&
          actual(&p));
    printf("snapshot/anchor unchanged; same-context valid retry MACHINE\n");
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    destroy(&valid);
    return true;
}

static bool invalid_call(Program *p, const char *label)
{
    const NLCheckedProofClass kind = nl_checked_proof_class(p->artifact);
    printf("%s proof-class=%d\n", label, (int)kind);
    CHECK(kind == NL_CHECKED_PROOF_INVALID);
    char *text = NULL;
    size_t length = 98765;
    const NLNodeCStatus status = nl_checked_c_node(p->artifact, &text, &length);
    printf("invalid emit=%d output=%p length=%zu\n", (int)status, (void *)text,
           length);
    CHECK(status == NL_NODE_C_UNSUPPORTED && !text && length == 98765);
    return true;
}

static bool authentication(const char *a, const char *b)
{
    Program p = {0}, other = {0};
    CompositionTrace t = {0}, foreign = {0};
    CHECK(ready(a, &p) && actual(&p) && composition(p.artifact, &t, 0) &&
          ready(b, &other) && actual(&other) &&
          composition(other.artifact, &foreign, 0));
    CHECK(t.relay == 1 && foreign.relay == 1);
    NLCheckedFragment *owner = t.relay_owner;
    NLSemanticContext *c = (NLSemanticContext *)nl_checked_context(owner);
    NLCheckedNodeView *call =
        (NLCheckedNodeView *)nl_checked_node_view(owner, t.relay_call);
    const NLCheckedNodeView *foreign_call =
        nl_checked_node_view(foreign.relay_owner, foreign.relay_call);
    CHECK(c->functions && call->function &&
          call->function <= c->function_count);
    NLFunctionEntry *fn = &c->functions[call->function - 1];
    const NLSemanticContext *other_c = nl_checked_context(foreign.relay_owner);
    CHECK(other_c->functions && foreign_call->function &&
          foreign_call->function <= other_c->function_count);
    NLFunctionBody *foreign_body =
        other_c->functions[foreign_call->function - 1].body;
    CHECK(fn->parameters && fn->body && foreign_body);
#define DAMAGE(target, replacement, label)                                     \
    do {                                                                       \
        unsigned char original[sizeof(target)];                                \
        memcpy(original, &(target), sizeof(target));                           \
        (target) = (replacement);                                              \
        const bool rejected = invalid_call(&p, label);                         \
        memcpy(&(target), original, sizeof(target));                           \
        CHECK(rejected && nl_checked_proof_class(p.artifact) ==                \
                              NL_CHECKED_MACHINE_PROVED);                      \
    } while (0)
    DAMAGE(fn->parameters[0], nl_semantic_core_type(c, NL_TYPE_USIZE),
           "wrong formal/type");
    DAMAGE(fn->parameters, NULL, "missing signature allocation");
    DAMAGE(c->function_count, 0, "current function count");
    DAMAGE(call->function, 0, "missing function identity");
    DAMAGE(fn->body, foreign_body, "foreign function source/body");
    DAMAGE(call->first_argument, 0, "missing actual call identity");
    NLCheckedNodeView *argument =
        (NLCheckedNodeView *)nl_checked_node_view(owner, call->first_argument);
    CHECK(argument);
    DAMAGE(argument->parameter_type, nl_semantic_core_type(c, NL_TYPE_USIZE),
           "wrong actual type");
    NLSemanticValueView *result = &c->values[call->results[0].value - 1];
    CHECK(!result->scalar_known);
    DAMAGE(result->scalar_known, true, "forged Unknown-to-Known");
    const NLCheckedFragment *body = nl_checked_call_body(owner, t.relay_call);
    const NLCheckedFragment *foreign_selector =
        nl_checked_call_body(foreign.relay_owner, foreign.relay_call);
    CHECK(body && foreign_selector && body->arm_count == 2 &&
          foreign_selector->arm_count == 2);
    DAMAGE(((NLCheckedFragment *)body)->arms[0], foreign_selector->arms[0],
           "foreign selector arm");
    CHECK(c->scope_count && !c->scopes[0].active);
    DAMAGE(c->scopes[0].active, true, "reopened loan world");
    CHECK(c->domain_count);
    DAMAGE(c->domains[0].live, !c->domains[0].live, "changed Domain world");
    DAMAGE(owner->context, foreign.relay_owner->context, "foreign call world");
    DAMAGE(p.artifact->h_origin, other.artifact->h_origin,
           "foreign root/call owner");
#undef DAMAGE
    NLCopyBodyView foreign_view;
    NLCopyBodyParameters foreign_parameters;
    NLCopyBodyEventView foreign_event;
    CHECK(t.installer == 1 && foreign.installer == 1 &&
          nl_checked_copy_body_view(foreign.installer_owner,
                                    foreign.installer_call, &foreign_view) &&
          nl_checked_copy_body_parameters(foreign.installer_owner,
                                          foreign.installer_call,
                                          &foreign_parameters) &&
          nl_checked_copy_body_event(foreign.installer_owner,
                                     foreign.installer_call, 0,
                                     &foreign_event));
    CHECK(nl_checked_copy_body_validate(t.installer_owner, t.installer_call,
                                        &foreign_view) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          nl_checked_copy_body_parameters_validate(
              t.installer_owner, t.installer_call, &foreign_parameters) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          nl_checked_copy_body_event_validate(
              t.installer_owner, t.installer_call, 0, &foreign_event) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
          nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
    printf("foreign T body/entry/world/event candidates rejected; original "
           "MACHINE\n");
    destroy(&other);
    destroy(&p);
    return true;
}

static bool emitted(const char *path)
{
    Program p = {0};
    CHECK(ready(path, &p) && actual(&p));
    char *text = NULL;
    size_t length = 98765;
    const NLNodeCStatus status = nl_checked_c_node(p.artifact, &text, &length);
    printf("emit status=%d output=%p length=%zu\n", (int)status, (void *)text,
           length);
    const bool valid =
        status == NL_NODE_C_OK && text && length && length != 98765 &&
        strlen(text) == length &&
        nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED;
    free(text);
    destroy(&p);
    CHECK(valid);
    return true;
}

static bool composition_oom(const char *path, unsigned point)
{
    Program p = {0};
    CHECK(load(path, &p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    NLFunctionUnitDiagnostic diagnostic = {0};
    if (point == 1) {
        fault = point;
        hit = false;
        const NLCheckStatus status = define(&p, &diagnostic);
        fault = 0;
        printf("registration OOM hit=%d status=%d output=%p\n", hit,
               (int)status, (void *)p.artifact);
        CHECK(hit && status == NL_CHECK_OUT_OF_MEMORY && !p.artifact &&
              nl_copy_state_matches(p.context, &anchor) &&
              nl_semantic_snapshot(p.context, &after) &&
              !memcmp(&before, &after, sizeof(before)));
        CHECK(define(&p, &diagnostic) == NL_CHECK_OK && actual(&p));
    } else if (point == 2 || point == 4) {
        CHECK(define(&p, &diagnostic) == NL_CHECK_OK);
        nl_copy_state_destroy(&anchor);
        CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
              nl_semantic_snapshot(p.context, &before));
        fault = point;
        hit = false;
        NLCheckDiagnostic d = {0};
        const NLCheckStatus status =
            nl_semantic_check_expression(p.context, p.call, &p.artifact, &d);
        fault = 0;
        printf("scalar %s OOM hit=%d status=%d output=%p\n",
               point == 2 ? "IF clone" : "result seal", hit, (int)status,
               (void *)p.artifact);
        CHECK(hit && status == NL_CHECK_OUT_OF_MEMORY && !p.artifact &&
              nl_copy_state_matches(p.context, &anchor) &&
              nl_semantic_snapshot(p.context, &after) &&
              !memcmp(&before, &after, sizeof(before)));
        CHECK(actual(&p));
    } else {
        CHECK(define(&p, &diagnostic) == NL_CHECK_OK && actual(&p));
        char *text = NULL;
        size_t length = 98765;
        fault = point;
        hit = false;
        const NLNodeCStatus status =
            nl_checked_c_node(p.artifact, &text, &length);
        fault = 0;
        printf("C buffer OOM hit=%d status=%d output=%p length=%zu\n", hit,
               (int)status, (void *)text, length);
        CHECK(hit && status == NL_NODE_C_OUT_OF_MEMORY && !text &&
              length == 98765 &&
              nl_checked_proof_class(p.artifact) == NL_CHECKED_MACHINE_PROVED);
    }
    char *retry = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(p.artifact, &retry, &length) == NL_NODE_C_OK &&
          retry && length);
    free(retry);
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}

int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "emit"))
        return emitted(argv[2]) ? 0 : 1;
    if (argc == 3 && (!strcmp(argv[1], "oom1") || !strcmp(argv[1], "oom2") ||
                      !strcmp(argv[1], "oom3") || !strcmp(argv[1], "oom4")))
        return composition_oom(argv[2], (unsigned)(argv[1][3] - '0')) ? 0 : 1;
    if (argc == 5 && !strcmp(argv[1], "negative"))
        return negative_exact(argv[2], argv[3], argv[4]) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "auth"))
        return authentication(argv[2], argv[3]) ? 0 : 1;
    if ((argc != 3 && argc != 4) || strcmp(argv[1], "observe"))
        return 2;
    Program p = {0};
    CompositionTrace trace = {0};
    if (!ready(argv[2], &p) || !actual(&p) ||
        !composition(p.artifact, &trace, 0)) {
        destroy(&p);
        return 1;
    }
    printf("MACHINE_PROVED relay=%zu H-payload-sites=%zu "
           "closed-loans=%zu T-Changes=%zu\n",
           trace.relay, trace.payload, trace.closed_loans, trace.changes);
    const size_t expected = argc == 4 ? (size_t)strtoul(argv[3], NULL, 10) : 1;
    const bool ok = trace.relay == expected && trace.payload &&
                    trace.closed_loans && trace.installer == 1;
    destroy(&p);
    return ok ? 0 : 1;
}
