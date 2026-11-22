#define main p1638_existing_probe_main
#include "../p1638/probe.c"
#undef main

#include <stddef.h>
#include <stdint.h>

typedef struct {
    NLCheckStatus status;
    bool held;
    bool emitted;
    bool same;
} Transaction;

static Transaction transaction(const Program *p, const char *pristine,
                               size_t pristine_size)
{
    char sentinel[] = "p1643-keep\0tail";
    char before[sizeof(sentinel)];
    memcpy(before, sentinel, sizeof(sentinel));
    char *output = sentinel;
    size_t length = sizeof(sentinel);
    const NLCheckStatus status =
        nl_quantity_usize_c17(p->entry, &output, &length);
    const bool held = output == sentinel && length == sizeof(sentinel) &&
                      !memcmp(sentinel, before, sizeof(sentinel));
    const bool emitted = output != sentinel && output && length;
    const bool same = emitted && length == pristine_size &&
                      !memcmp(output, pristine, pristine_size);
    if (output != sentinel)
        free(output);
    return (Transaction){status, held, emitted, same};
}

static bool restored(const Program *p, const char *pristine,
                     size_t pristine_size)
{
    char *output = NULL;
    size_t length = 0;
    const bool ok = emit(p, &output, &length) && length == pristine_size &&
                    !memcmp(output, pristine, pristine_size);
    free(output);
    return ok;
}

static void print_transaction(const char *name, Transaction result, bool retry)
{
    printf("CASE %s status=%d held=%d emitted=%d same=%d retry=%d\n", name,
           result.status, result.held, result.emitted, result.same, retry);
}

static bool rejected_transaction(Transaction result)
{
    return result.status == NL_CHECK_SEMANTIC_UNSUPPORTED && result.held &&
           !result.emitted;
}

static bool authenticity(Program *first, const char *file)
{
    Program donor = {0};
    Evidence own = {0}, foreign = {0};
    REQUIRE(check(first) == NL_CHECK_OK && inspect(first, &own) &&
            prepare(&donor, file) && check(&donor) == NL_CHECK_OK &&
            inspect(&donor, &foreign));
    char *pristine = NULL;
    size_t pristine_size = 0;
    REQUIRE(emit(first, &pristine, &pristine_size));

    NLControlExits *const own_arm = own.return_arm->exits;
    NLControlExits *const own_outer = own.helper->exits;
    NLControlTarget *const own_target = own.helper->function_target;
    NLControlExits *arm_clone = NULL, *outer_clone = NULL;
    REQUIRE(nl_control_exits_union(&arm_clone, own_arm) == NL_CHECK_OK &&
            nl_control_exits_union(&outer_clone, own_outer) == NL_CHECK_OK);

    own.return_arm->exits = arm_clone;
    own.helper->exits = outer_clone;
    Transaction cloned = transaction(first, pristine, pristine_size);
    own.helper->exits = own_outer;
    own.return_arm->exits = own_arm;
    bool retry = restored(first, pristine, pristine_size);
    print_transaction("same-issuer-cloned-arm-outer", cloned, retry);
    bool ok =
        cloned.status == NL_CHECK_OK && cloned.emitted && cloned.same && retry;

    own.return_arm->exits = own_outer;
    Transaction outer_as_arm = transaction(first, pristine, pristine_size);
    own.return_arm->exits = own_arm;
    retry = restored(first, pristine, pristine_size);
    print_transaction("same-issuer-outer-as-arm", outer_as_arm, retry);
    ok = ok && outer_as_arm.status == NL_CHECK_OK && outer_as_arm.emitted &&
         outer_as_arm.same && retry;

    own.helper->function_target = foreign.helper->function_target;
    Transaction foreign_target = transaction(first, pristine, pristine_size);
    own.helper->function_target = own_target;
    retry = restored(first, pristine, pristine_size);
    print_transaction("foreign-target", foreign_target, retry);
    ok = ok && rejected_transaction(foreign_target) && retry;

    own.return_arm->exits = foreign.return_arm->exits;
    Transaction foreign_arm = transaction(first, pristine, pristine_size);
    own.return_arm->exits = own_arm;
    retry = restored(first, pristine, pristine_size);
    print_transaction("foreign-arm-package", foreign_arm, retry);
    ok = ok && rejected_transaction(foreign_arm) && retry;

    own.helper->exits = foreign.helper->exits;
    Transaction foreign_outer = transaction(first, pristine, pristine_size);
    own.helper->exits = own_outer;
    retry = restored(first, pristine, pristine_size);
    print_transaction("foreign-outer-package", foreign_outer, retry);
    ok = ok && rejected_transaction(foreign_outer) && retry;

    own.helper->function_target = foreign.helper->function_target;
    own.return_arm->exits = foreign.return_arm->exits;
    own.helper->exits = foreign.helper->exits;
    Transaction foreign_complete = transaction(first, pristine, pristine_size);
    own.helper->exits = own_outer;
    own.return_arm->exits = own_arm;
    own.helper->function_target = own_target;
    retry = restored(first, pristine, pristine_size);
    print_transaction("foreign-target-arm-outer", foreign_complete, retry);
    ok = ok && rejected_transaction(foreign_complete) && retry;

    printf("AUTH same-issuer-clone=%s outer-as-arm=%s foreign-individual=%s "
           "foreign-complete=%s restoration=%s\n",
           cloned.status == NL_CHECK_OK ? "accepted" : "rejected",
           outer_as_arm.status == NL_CHECK_OK ? "accepted" : "rejected",
           rejected_transaction(foreign_target) &&
                   rejected_transaction(foreign_arm) &&
                   rejected_transaction(foreign_outer)
               ? "rejected"
               : "unexpected",
           rejected_transaction(foreign_complete) ? "rejected" : "ACCEPTED",
           retry ? "PASS" : "FAIL");

    nl_control_exits_destroy(outer_clone);
    nl_control_exits_destroy(arm_clone);
    free(pristine);
    destroy(&donor);
    return ok;
}

static bool slot_substitution(Program *p)
{
    Evidence own = {0};
    REQUIRE(check(p) == NL_CHECK_OK && inspect(p, &own));
    char *pristine = NULL;
    size_t pristine_size = 0;
    REQUIRE(emit(p, &pristine, &pristine_size));

    NLControlExits *const own_arm = own.return_arm->exits;
    const NLControlExitView *edge = nl_control_exit_view(own_arm, 0);
    REQUIRE(edge && edge->state && edge->target && edge->count == 1);
    NLSemanticContext *edge_world =
        nl_control_state_context((NLControlState *)edge->state);
    NLSemanticValueView original;
    REQUIRE(
        edge_world &&
        nl_semantic_value_view(edge_world, edge->values[0].value, &original) &&
        original.carrier == NL_CARRIER_LOOSE && original.scalar_known);

    NLControlState *derived = NULL;
    REQUIRE(nl_control_state_fork(edge->state, &derived) == NL_CHECK_OK);
    NLSemanticContext *derived_world = nl_control_state_context(derived);
    NLValueId alternate = 0;
    REQUIRE(nl_sem_new_value(
                derived_world,
                (NLSemanticValueView){.type = edge->values[0].type,
                                      .scalar_known = true,
                                      .scalar_value = original.scalar_value},
                &alternate) == NL_CHECK_OK &&
            alternate != edge->values[0].value);
    NLSemanticValueView alternate_view;
    REQUIRE(nl_semantic_value_view(derived_world, alternate, &alternate_view) &&
            alternate_view.carrier == NL_CARRIER_LOOSE &&
            alternate_view.scalar_known &&
            alternate_view.scalar_value == original.scalar_value);
    NLControlExits *alternate_set = NULL;
    const NLCheckedResult replacement = {edge->values[0].type, alternate};
    REQUIRE(nl_control_exit_append(&alternate_set, NL_EXIT_RETURN,
                                   (NLControlTarget *)edge->target, derived,
                                   &replacement, 1) == NL_CHECK_OK);
    nl_control_state_destroy(derived);

    own.return_arm->exits = alternate_set;
    Transaction different_copy = transaction(p, pristine, pristine_size);
    own.return_arm->exits = own_arm;
    bool retry = restored(p, pristine, pristine_size);
    print_transaction("same-scalar-different-copy", different_copy, retry);
    bool ok = rejected_transaction(different_copy) && retry;

    const NLValueCarrier carrier =
        edge_world->values[edge->values[0].value - 1].carrier;
    edge_world->values[edge->values[0].value - 1].carrier = NL_CARRIER_ENDED;
    Transaction ended_slot = transaction(p, pristine, pristine_size);
    edge_world->values[edge->values[0].value - 1].carrier = carrier;
    retry = restored(p, pristine, pristine_size);
    print_transaction("ended-return-slot", ended_slot, retry);
    ok = ok && rejected_transaction(ended_slot) && retry;

    printf("SLOTS original=%zu alternate=%zu scalar=%zu "
           "different-copy=%s ended=%s restoration=%s\n",
           (size_t)edge->values[0].value, (size_t)alternate,
           original.scalar_value,
           rejected_transaction(different_copy) ? "rejected" : "ACCEPTED",
           rejected_transaction(ended_slot) ? "rejected" : "ACCEPTED",
           retry ? "PASS" : "FAIL");

    nl_control_exits_destroy(alternate_set);
    free(pristine);
    return ok;
}

typedef enum {
    RECEIPT_FAULT_NONE,
    RECEIPT_FAULT_ALLOCATION,
    RECEIPT_FAULT_TARGET_OVERFLOW,
    RECEIPT_FAULT_ORIGIN_OVERFLOW
} ReceiptFault;

typedef struct {
    NLControlTargetKind kind;
    size_t owners;
} TargetLayout;

static ReceiptFault receipt_fault;
static size_t receipt_calls, receipt_hits, receipt_bytes;
static bool receipt_reached, receipt_clean;

static void object_size_read(const void *object, size_t offset, size_t *value)
{
    memcpy(value, (const unsigned char *)object + offset, sizeof(*value));
}

static void object_size_write(void *object, size_t offset, size_t value)
{
    memcpy((unsigned char *)object + offset, &value, sizeof(value));
}

NLCheckStatus __real_nl_control_quantity_return_receipt_issue(
    NLCheckedFragment *, const NLCheckedFragment *, NLCheckedNodeId,
    const NLSemanticContext *);

NLCheckStatus __wrap_nl_control_quantity_return_receipt_issue(
    NLCheckedFragment *body, const NLCheckedFragment *caller,
    NLCheckedNodeId call, const NLSemanticContext *working)
{
    const NLCheckedNodeView *invocation = nl_checked_node_view(caller, call);
    if (receipt_fault == RECEIPT_FAULT_NONE || receipt_reached || !body ||
        !invocation || invocation->kind != NL_CHECKED_REGISTERED_CALL ||
        invocation->argument_count != 2 || body->arm_count != 2)
        return __real_nl_control_quantity_return_receipt_issue(body, caller,
                                                               call, working);

    receipt_reached = true;
    ++receipt_calls;
    NLCheckStatus status = NL_CHECK_INTERNAL_ERROR;
    if (receipt_fault == RECEIPT_FAULT_ALLOCATION) {
        visits = hits = bytes = 0;
        fail_after = 1;
        status = __real_nl_control_quantity_return_receipt_issue(body, caller,
                                                                 call, working);
        fail_after = 0;
        receipt_hits = hits;
        receipt_bytes = bytes;
        receipt_clean = status == NL_CHECK_OUT_OF_MEMORY && hits == 1 &&
                        body->quantity_return_receipt == NULL;
        return status;
    }

    const size_t target_offset = offsetof(TargetLayout, owners);
    size_t target_before = 0, target_after = 0;
    object_size_read(body->function_target, target_offset, &target_before);
    if (receipt_fault == RECEIPT_FAULT_TARGET_OVERFLOW) {
        object_size_write(body->function_target, target_offset, SIZE_MAX);
        status = __real_nl_control_quantity_return_receipt_issue(body, caller,
                                                                 call, working);
        object_size_write(body->function_target, target_offset, target_before);
        object_size_read(body->function_target, target_offset, &target_after);
        receipt_hits = 1;
        receipt_clean = status == NL_CHECK_RESOURCE_LIMIT &&
                        body->quantity_return_receipt == NULL &&
                        target_after == target_before;
        return status;
    }

    const NLControlExitView *edge = NULL;
    for (size_t i = 0; i < body->arm_count; ++i) {
        NLCheckedFragment *arm = body->arms[i].artifact;
        if (nl_control_exits_count(arm->exits) == 1) {
            if (edge)
                return NL_CHECK_INTERNAL_ERROR;
            edge = nl_control_exit_view(arm->exits, 0);
        }
    }
    void *origin = NULL;
    if (!edge || !edge->state)
        return NL_CHECK_INTERNAL_ERROR;
    memcpy(&origin, edge->state, sizeof(origin));
    if (!origin)
        return NL_CHECK_INTERNAL_ERROR;
    size_t origin_before = 0;
    object_size_read(origin, 0, &origin_before);
    object_size_write(origin, 0, SIZE_MAX);
    status = __real_nl_control_quantity_return_receipt_issue(body, caller, call,
                                                             working);
    object_size_write(origin, 0, origin_before);
    object_size_read(body->function_target, target_offset, &target_after);
    receipt_hits = 1;
    receipt_clean = status == NL_CHECK_RESOURCE_LIMIT &&
                    body->quantity_return_receipt == NULL &&
                    target_after == target_before;
    return status;
}

typedef struct {
    unsigned char context[sizeof(NLSemanticContext)];
    unsigned char binding[sizeof(NLBindingEntry)];
    unsigned char place[sizeof(NLSemanticPlaceView)];
    unsigned char value[sizeof(NLSemanticValueView)];
    NLSymbolId anchor;
    NLSemanticBindingView view;
} ContextSnapshot;

static bool snapshot_context(Program *p, ContextSnapshot *snapshot)
{
    if (!snapshot->anchor) {
        const NLScalarValue seed = {
            nl_semantic_core_type(p->context, NL_TYPE_USIZE), true, 1643};
        REQUIRE(nl_semantic_seed_scalar(p->context, "p1643_anchor", seed,
                                        &snapshot->anchor) == NL_CHECK_OK &&
                snapshot->anchor);
    }
    snapshot->view = p->context->bindings[snapshot->anchor - 1].view;
    memcpy(snapshot->context, p->context, sizeof(*p->context));
    memcpy(snapshot->binding, &p->context->bindings[snapshot->anchor - 1],
           sizeof(*p->context->bindings));
    memcpy(snapshot->place, &p->context->places[snapshot->view.place - 1],
           sizeof(*p->context->places));
    memcpy(snapshot->value, &p->context->values[snapshot->view.value - 1],
           sizeof(*p->context->values));
    return true;
}

static bool context_unchanged(Program *p, const ContextSnapshot *snapshot)
{
    return !p->entry &&
           !memcmp(snapshot->context, p->context, sizeof(*p->context)) &&
           !memcmp(snapshot->binding,
                   &p->context->bindings[snapshot->anchor - 1],
                   sizeof(*p->context->bindings)) &&
           !memcmp(snapshot->place,
                   &p->context->places[snapshot->view.place - 1],
                   sizeof(*p->context->places)) &&
           !memcmp(snapshot->value,
                   &p->context->values[snapshot->view.value - 1],
                   sizeof(*p->context->values)) &&
           nl_sem_validate(p->context) == NL_CHECK_OK;
}

static bool receipt_failure(Program *p, ContextSnapshot *snapshot,
                            ReceiptFault fault, NLCheckStatus expected,
                            const char *name)
{
    receipt_fault = fault;
    receipt_calls = receipt_hits = receipt_bytes = 0;
    receipt_reached = receipt_clean = false;
    const NLCheckStatus status = check(p);
    receipt_fault = RECEIPT_FAULT_NONE;
    const bool rollback = status == expected && receipt_reached &&
                          receipt_calls == 1 && receipt_hits == 1 &&
                          receipt_clean && context_unchanged(p, snapshot);
    printf("FAILURE %s status=%d calls=%zu hits=%zu bytes=%zu "
           "partial=0 rollback=%d\n",
           name, status, receipt_calls, receipt_hits, receipt_bytes, rollback);
    return rollback;
}

static bool receipt_failures(Program *p)
{
    ContextSnapshot snapshot = {0};
    REQUIRE(snapshot_context(p, &snapshot));
    REQUIRE(receipt_failure(p, &snapshot, RECEIPT_FAULT_ALLOCATION,
                            NL_CHECK_OUT_OF_MEMORY, "allocation"));
    REQUIRE(receipt_failure(p, &snapshot, RECEIPT_FAULT_TARGET_OVERFLOW,
                            NL_CHECK_RESOURCE_LIMIT, "target-retain-overflow"));
    REQUIRE(receipt_failure(p, &snapshot, RECEIPT_FAULT_ORIGIN_OVERFLOW,
                            NL_CHECK_RESOURCE_LIMIT, "origin-retain-overflow"));
    REQUIRE(check(p) == NL_CHECK_OK && inspect(p, NULL) && same_emission(p));
    puts("FAILURE receipt lifecycle rollback=PASS retry=PASS teardown=PASS");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(argv[1], "authenticity"))
        ok = authenticity(&p, argv[2]);
    else if (ok && !strcmp(argv[1], "slot-substitution"))
        ok = slot_substitution(&p);
    else if (ok && !strcmp(argv[1], "receipt-failures"))
        ok = receipt_failures(&p);
    else
        ok = false;
    destroy(&p);
    return ok ? 0 : 1;
}
