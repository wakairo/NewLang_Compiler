#define main p1435_fixture_main
#include "../../p1435/probe.c"
#undef main
#include "../../../src/readonly_u8_consumer_internal.h"

typedef struct {
    const NLCheckedFragment *origin, *caller, *body, *arm;
    NLCheckedNodeId call, declaration, initializer, arm_tail;
    size_t helper_count, installer_count, closed_loans;
    bool expected_consumer;
} Fragments;

static bool visit(const NLCheckedFragment *f, Fragments *t, size_t depth)
{
    CHECK(f && depth < 32);
    const NLSemanticContext *c = nl_checked_context(f);
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        CHECK(n);
        if (n->kind == NL_CHECKED_REGISTERED_CALL) {
            CHECK(c->functions && n->function &&
                  n->function <= c->function_count);
            const NLFunctionEntry *fn = &c->functions[n->function - 1];
            if (!strcmp(fn->name, "gate_byte")) {
                const NLCheckedFragment *body = nl_checked_call_body(f, id);
                const NLCheckedNodeView *root =
                    body ? nl_checked_node_view(body, body->root) : NULL;
                CHECK(body && root && root->kind == NL_CHECKED_BLOCK &&
                      root->item_count >= 2);
                NLCheckedNodeId item = root->first_item, last = 0;
                for (size_t i = 0; i < root->item_count; ++i) {
                    const NLCheckedNodeView *d =
                        nl_checked_node_view(body, item);
                    CHECK(d && d->kind == NL_CHECKED_BINDING && !d->terminates);
                    last = item;
                    item = d->next_item;
                }
                CHECK(!item);
                const NLCheckedNodeView *declaration =
                    nl_checked_node_view(body, last);
                const NLCheckedNodeView *init =
                    nl_checked_node_view(body, declaration->initializer);
                const NLCheckedNodeView *selection =
                    nl_checked_node_view(body, root->tail);
                const NLCheckedFragment *arm =
                    selection ? nl_checked_if_arm(body, root->tail, 0) : NULL;
                const NLCheckedNodeView *ar =
                    arm ? nl_checked_node_view(arm, arm->root) : NULL;
                const NLCheckedNodeView *block =
                    ar ? nl_checked_node_view(arm, ar->initializer) : NULL;
                const NLCheckedNodeView *tail =
                    block ? nl_checked_node_view(arm, block->tail) : NULL;
                CHECK(init && init->kind == NL_CHECKED_IDENTIFIER &&
                      selection && selection->kind == NL_CHECKED_IF &&
                      selection->normal_arms == 2 && arm && ar && block &&
                      tail && !ar->terminates &&
                      tail->kind == NL_CHECKED_IDENTIFIER &&
                      tail->symbol == declaration->symbol &&
                      init->type == tail->type && init->result_count == 1 &&
                      declaration->span.end_byte <= tail->span.start_byte &&
                      body->source == arm->source && arm->if_parent == body &&
                      arm->if_parent_node == root->tail &&
                      arm->if_arm_index == 0 &&
                      declaration->initializer > nl_checked_node_count(arm) &&
                      !nl_checked_node_view(arm, declaration->initializer));
                NLReadonlySelection plan = {0};
                const bool accepted =
                    nl_readonly_selection_consumer(t->origin, f, id, &plan);
                CHECK(accepted == t->expected_consumer);
                if (accepted)
                    CHECK(plan.body == body && plan.parameters[0] &&
                          plan.parameters[1]);
                printf("noncoincident parent-init=%zu child-count=%zu "
                       "child-init=ABSENT parent-kind=%d consumer=%d "
                       "root-MACHINE=1\n",
                       declaration->initializer, nl_checked_node_count(arm),
                       (int)init->kind, accepted);
                t->caller = f;
                t->body = body;
                t->arm = arm;
                t->call = id;
                t->declaration = last;
                t->initializer = declaration->initializer;
                t->arm_tail = block->tail;
                ++t->helper_count;
            } else if (!strcmp(fn->name, "install_result")) {
                NLCopyBodyView v;
                NLCopyBodyParameters p;
                NLCopyBodyEventView e;
                CHECK(nl_checked_copy_body_view(f, id, &v) &&
                      nl_checked_copy_body_validate(f, id, &v) == NL_CHECK_OK &&
                      nl_checked_copy_body_parameters(f, id, &p) &&
                      nl_checked_copy_body_parameters_validate(f, id, &p) ==
                          NL_CHECK_OK &&
                      nl_checked_copy_body_event(f, id, 0, &e) &&
                      nl_checked_copy_body_event_validate(f, id, 0, &e) ==
                          NL_CHECK_OK &&
                      e.kind == NL_COPY_BODY_CHANGE && v.event_count == 1);
                ++t->installer_count;
            }
        }
        if (n->kind == NL_CHECKED_LOAN_HEADER) {
            CHECK(n->loan.scope && n->loan.scope <= c->scope_count &&
                  !c->scopes[n->loan.scope - 1].active &&
                  n->loan.body_nonescape_proved &&
                  n->loan.normal_result_forwarded);
            ++t->closed_loans;
        }
        const NLCheckedFragment *child = nl_checked_call_body(f, id);
        if (child)
            CHECK(visit(child, t, depth + 1));
        if (n->kind == NL_CHECKED_MATCH || n->kind == NL_CHECKED_IF)
            for (size_t a = 0; a < n->item_count; ++a) {
                child = n->kind == NL_CHECKED_MATCH
                            ? nl_checked_match_arm(f, id, a)
                            : nl_checked_if_arm(f, id, a);
                CHECK(visit(child, t, depth + 1));
            }
    }
    return true;
}
static bool refused(Program *p, const char *label)
{
    const NLCheckedProofClass proof = nl_checked_proof_class(p->artifact);
    CHECK(proof == NL_CHECKED_PROOF_INVALID);
    char *out = NULL;
    size_t length = 98765;
    const NLNodeCStatus status = nl_checked_c_node(p->artifact, &out, &length);
    printf("%s first-public=INVALID emit=%d out=%p length=%zu\n", label,
           (int)status, (void *)out, length);
    CHECK(status == NL_NODE_C_UNSUPPORTED && !out && length == 98765);
    return true;
}
static bool fragment_negative(Program *p, Fragments *t, const char *control)
{
    Program other = {0};
    CHECK(ready(control, &other) && actual(&other));
    NLCheckedNodeView *decl =
        (NLCheckedNodeView *)nl_checked_node_view(t->body, t->declaration);
    NLCheckedNodeView *init =
        (NLCheckedNodeView *)nl_checked_node_view(t->body, t->initializer);
    NLCheckedFragment *arm = (NLCheckedFragment *)t->arm;
    NLSemanticContext *c = (NLSemanticContext *)nl_checked_context(t->caller);
    const NLCheckedNodeView *call = nl_checked_node_view(t->caller, t->call);
    NLSemanticValueView *result = &c->values[call->results[0].value - 1];
#define DAMAGE(target, replacement, label)                                     \
    do {                                                                       \
        unsigned char saved[sizeof(target)];                                   \
        memcpy(saved, &(target), sizeof(target));                              \
        (target) = (replacement);                                              \
        const bool bad = refused(p, label);                                    \
        memcpy(&(target), saved, sizeof(target));                              \
        CHECK(bad && nl_checked_proof_class(p->artifact) ==                    \
                         NL_CHECKED_MACHINE_PROVED);                           \
    } while (0)
    CHECK(decl->initializer != t->arm_tail && !result->scalar_known);
    DAMAGE(decl->initializer, t->arm_tail,
           "wrong initializer / child ID in parent");
    DAMAGE(init->kind, NL_CHECKED_UNIT, "wrong initializer kind");
    DAMAGE(arm->if_parent, arm, "wrong fragment owner");
    DAMAGE(arm->context, other.artifact->context, "foreign arm world");
    DAMAGE(result->scalar_known, true, "forged Unknown-to-Known");
#undef DAMAGE
    NLReadonlySelection selection = {0};
    CHECK(nl_readonly_selection_consumer(p->artifact, t->caller, t->call,
                                         &selection));
    char *out = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(p->artifact, &out, &length) == NL_NODE_C_OK &&
          out && length);
    free(out);
    destroy(&other);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    Program p = {0};
    const bool expect = strcmp(argv[1], "C2") != 0;
    if (!ready(argv[2], &p) || !actual(&p))
        return 1;
    Fragments t = {.origin = p.artifact, .expected_consumer = expect};
    if (!visit(p.artifact, &t, 0))
        return 1;
    if (t.helper_count != 1 || t.installer_count != 1 || !t.closed_loans) {
        destroy(&p);
        return 1;
    }
    if (!strcmp(argv[1], "negative") && !fragment_negative(&p, &t, argv[3])) {
        destroy(&p);
        return 1;
    }
    destroy(&p);
    return 0;
}
