#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../../src/copy_body_internal.h"
#include "../../src/guarded_usize_internal.h"
#include "../support/node_checked.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <dirent.h>

static bool injecting;
static size_t calls, fail_at, hits;
static bool sealing_entry_world;
static size_t entry_world_failures;

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
char *__real_strdup(const char *);
char *__real_realpath(const char *, char *);
FILE *__real_fdopen(int, const char *);

static bool fail_here(void)
{
    if (!injecting || calls++ != fail_at)
        return false;
    ++hits;
    entry_world_failures += sealing_entry_world;
    errno = ENOMEM;
    return true;
}
void *__wrap_malloc(size_t size)
{
    return fail_here() ? NULL : __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    return fail_here() ? NULL : __real_calloc(count, size);
}
void *__wrap_realloc(void *ptr, size_t size)
{
    return fail_here() ? NULL : __real_realloc(ptr, size);
}
char *__wrap_strdup(const char *s)
{
    return fail_here() ? NULL : __real_strdup(s);
}
char *__wrap_realpath(const char *path, char *resolved)
{
    return fail_here() ? NULL : __real_realpath(path, resolved);
}
FILE *__wrap_fdopen(int fd, const char *mode)
{
    return fail_here() ? NULL : __real_fdopen(fd, mode);
}

NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *body,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    const bool saved = sealing_entry_world;
    sealing_entry_world = world && !parent->body_owner;
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, body, id, world);
    sealing_entry_world = saved;
    return status;
}

static void select_failure(size_t index)
{
    calls = hits = 0;
    fail_at = index;
    injecting = true;
}

bool p1673_receipt_mutation_rejected(NLCheckedFragment *, NLCheckedNodeId);

static bool receipts(const char *path)
{
    TestNode original = {0}, independent = {0};
    CHECK(node_checked_load(path, &original));
    CHECK(node_checked_load(path, &independent));
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        original.entry, nl_checked_root(original.entry));
    const NLCheckedFragment *other = nl_checked_call_body(
        independent.entry, nl_checked_root(independent.entry));
    CHECK(body && other && body->context != other->context);
    const size_t products[] = {391, 493, 253, 247, 0, 23, SIZE_MAX, 0};
    const size_t left_values[] = {23, 29, 23, 13, 0, 23, SIZE_MAX / 3, 0};
    const size_t right_values[] = {17, 17, 11, 19, 17, 1, 3, SIZE_MAX};
    size_t count = 0;
    NLCheckedNodeId first = 0, third = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *op = nl_checked_node_view(body, id);
        if (op->kind != NL_CHECKED_USIZE_MULTIPLICATION)
            continue;
        CHECK(count < sizeof(products) / sizeof(*products));
        CHECK(nl_usize_multiplication_valid(body, id));
        CHECK(nl_usize_multiplication_valid(other, id));
        const NLCheckedNodeView *left =
            nl_checked_node_view(body, op->first_argument);
        CHECK(left && op->first_argument < left->next_argument &&
              left->next_argument < id);
        const NLCheckedNodeView *right =
            nl_checked_node_view(body, left->next_argument);
        NLSemanticValueView a, b, result;
        CHECK(
            right &&
            nl_semantic_value_view(body->context, left->results[0].value, &a) &&
            nl_semantic_value_view(body->context, right->results[0].value,
                                   &b) &&
            nl_semantic_value_view(body->context, op->results[0].value,
                                   &result));
        CHECK(a.scalar_known && a.scalar_value == left_values[count] &&
              b.scalar_known && b.scalar_value == right_values[count] &&
              result.scalar_known && result.scalar_value == products[count] &&
              op->results[0].value != left->results[0].value &&
              op->results[0].value != right->results[0].value);
        printf("receipt %zu left=%zu right=%zu product=%zu\n", id,
               a.scalar_value, b.scalar_value, result.scalar_value);
        if (count == 0)
            first = id;
        if (count == 2)
            third = id;
        ++count;
    }
    CHECK(count == sizeof(products) / sizeof(*products) && first && third);

    /* Copy metadata alone cannot replace a receipt or a distinct use event.
     * Restore each test-local perturbation before checking its result. */
    struct NLUsizeAdditionProof *saved = body->usize_additions;
    body->usize_additions = other->usize_additions;
    const bool foreign_rejected = !nl_usize_multiplication_valid(body, first);
    body->usize_additions = saved;
    CHECK(foreign_rejected && nl_usize_multiplication_valid(body, first));

    NLCheckedNodeView *op = &body->nodes[third - 1];
    const NLCheckedNodeId original_left = op->first_argument;
    op->first_argument = nl_checked_node_view(body, first)->first_argument;
    const bool stale_rejected = !nl_usize_multiplication_valid(body, third);
    op->first_argument = original_left;
    CHECK(stale_rejected && nl_usize_multiplication_valid(body, third));

    const size_t original_product = op->scalar_result.value;
    op->scalar_result.value = 1;
    const bool mutation_rejected = !nl_usize_multiplication_valid(body, third);
    op->scalar_result.value = original_product;
    CHECK(mutation_rejected && nl_usize_multiplication_valid(body, third));

    CHECK(p1673_receipt_mutation_rejected(body, third));

    node_checked_destroy(&independent);
    node_checked_destroy(&original);
    puts("own receipts=8 foreign/stale/mutated node and receipt rejected");
    return true;
}

static bool dependencies(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *context = NULL, *dependent = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_source_fragment(parser, &tree, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&context) == NL_CHECK_OK);
    NLSymbolId anchor = 0;
    CHECK(nl_semantic_seed_value(context, "anchor",
                                 nl_semantic_core_type(context, NL_TYPE_USIZE),
                                 NL_DEPENDENCY_FREE, &anchor) == NL_CHECK_OK);
    NLSemanticBindingView anchor_binding;
    NLSemanticPlaceView anchor_place;
    CHECK(
        nl_semantic_binding_view(context, anchor, &anchor_binding) &&
        nl_semantic_place_view(context, anchor_binding.place, &anchor_place) &&
        anchor_place.live && anchor_place.current_fact);
    TestChecked seed = {0};
    CHECK(
        test_run(context, "usize(37)", TEST_SOURCE, NL_CHECK_OK, NULL, &seed));
    NLSymbolId symbol = 0;
    const NLValueId id = test_root(&seed)->results[0].value;
    CHECK(nl_semantic_bind_result(context, "extent", id, &symbol) ==
          NL_CHECK_OK);
    test_checked_destroy(&seed);
    CHECK(nl_sem_clone(context, &dependent) == NL_CHECK_OK);
    NLSemanticValueView *fixture = &dependent->values[id - 1];
    fixture->dependencies = NL_EXACT_VALUE_DEPENDENCIES;
    fixture->value_dependency_count = 1;
    fixture->value_dependencies[0] =
        (NLValueDependency){anchor_binding.place, anchor_place.current_fact};
    CHECK(nl_sem_validate(dependent) == NL_CHECK_OK);

    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(dependent, before));
    NLCheckedFragment *out = NULL;
    NLCheckDiagnostic diag = {0};
    CHECK(nl_semantic_check_source_fragment(dependent, tree, &out, &diag) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(!out && diag.diagnostic.code &&
          !strcmp(diag.diagnostic.code, "USIZE-MULTIPLICATION-PRECISION") &&
          test_unchanged(dependent, before));

    CHECK(nl_semantic_check_source_fragment(context, tree, &out, NULL) ==
          NL_CHECK_OK);
    const NLCheckedNodeView *node =
        nl_checked_node_view(out, nl_checked_root(out));
    CHECK(node && node->kind == NL_CHECKED_USIZE_MULTIPLICATION &&
          node->scalar_result.known && node->scalar_result.value == 111);
    nl_checked_destroy(out);
    free(before);
    nl_semantic_destroy(dependent);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    puts("known source-fragment product=111; dependent Copy rejected, exact "
         "rollback");
    return true;
}

typedef enum {
    ENTRY_PRODUCT,
    HELPER_PRODUCT,
    LOCAL_PRODUCT,
    COMPOSED_PRODUCT,
    GUARDED_PRODUCT,
    GUARDED_ADDITION_DECISION,
    GUARDED_SELECTOR,
    GUARDED_BYTE_MODE,
    GUARDED_BYTE_MODE_HELPER,
    GUARDED_BYTE_POLICY_WRITER,
    GUARDED_BYTE_NAMED_POLICY_WRITER,
    GUARDED_BYTE_DELEGATED_POLICY_WRITER
} ProductRoute;

static bool composed_product_result(const NLDivisionUsizePlan *helper)
{
    const NLCheckedFragment *f = helper->body;
    const NLCheckedNodeId id = helper->result;
    const NLCheckedNodeView *result = nl_checked_node_view(f, id);
    if (!result)
        return false;
    if (result->kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView receipt;
        return nl_checked_usize_division_view(f, id, &receipt) &&
               nl_checked_usize_division_valid(f, id, &receipt) &&
               receipt.numerator_known && receipt.quotient_known &&
               receipt.numbers[0] == 1073 && receipt.numbers[1] == 31 &&
               receipt.quotient == 34;
    }
    NLUsizeRemainderView receipt;
    return nl_checked_usize_remainder_view(f, id, &receipt) &&
           nl_checked_usize_remainder_valid(f, id, &receipt) &&
           receipt.numerator_known && receipt.remainder_known &&
           receipt.numbers[0] == 1073 && receipt.numbers[1] == 31 &&
           receipt.remainder == 19;
}

static bool guarded_product_result(const NLCheckedFragment *caller,
                                   NLCheckedNodeId id,
                                   const NLDivisionUsizePlan *helper)
{
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    const NLCheckedNodeView *arg =
        call ? nl_checked_node_view(caller, call->first_argument) : NULL;
    const NLCheckedFragment *body = helper->body;
    const NLCheckedFragment *arm = nl_checked_if_arm(body, helper->result, 0);
    const NLCheckedNodeView *root =
        nl_checked_node_view(arm, nl_checked_root(arm));
    const NLCheckedNodeView *block =
        root ? nl_checked_node_view(arm, root->initializer) : NULL;
    const NLCheckedNodeView *product =
        block ? nl_checked_node_view(arm, block->tail) : NULL;
    const NLCheckedNodeView *left =
        product ? nl_checked_node_view(arm, product->first_argument) : NULL;
    NLSemanticBindingView formal;
    NLSemanticValueView input, result;
    return arg && left && product->kind == NL_CHECKED_USIZE_MULTIPLICATION &&
           left->symbol == helper->formal &&
           left->value_use == NL_VALUE_COPIED &&
           nl_semantic_binding_view(body->context, helper->formal, &formal) &&
           formal.value == arg->results[0].value &&
           nl_semantic_value_view(body->context, formal.value, &input) &&
           !input.scalar_known &&
           nl_semantic_value_view(arm->context, product->results[0].value,
                                  &result) &&
           product->has_scalar_result && !product->scalar_result.known &&
           !result.scalar_known && formal.type == result.type &&
           nl_h_custody_result_valid(caller, id) &&
           !nl_usize_multiplication_valid(arm, block->tail);
}

static bool guarded_addition_result(const NLCheckedFragment *caller,
                                    NLCheckedNodeId id,
                                    const NLDivisionUsizePlan *helper)
{
    NLGuardedUsizePlan addition;
    if (!nl_guarded_usize_call_valid(caller, id, &addition) ||
        addition.body != helper->body || addition.formal != helper->formal ||
        addition.conditional != helper->result)
        return false;
    const NLCheckedFragment *arm =
        nl_checked_if_arm(addition.body, addition.conditional, 0);
    const NLCheckedNodeView *sum = nl_checked_node_view(arm, addition.addition);
    const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
    const NLCheckedNodeView *actual =
        nl_checked_node_view(caller, call->first_argument);
    NLSemanticBindingView formal;
    NLSemanticValueView input, result;
    return sum && sum->kind == NL_CHECKED_USIZE_ADDITION &&
           !sum->scalar_result.known &&
           !nl_checked_usize_addition_valid(arm, addition.addition, NULL) &&
           actual && actual->kind == NL_CHECKED_IDENTIFIER &&
           nl_semantic_binding_view(caller->context, addition.formal,
                                    &formal) &&
           formal.value == actual->results[0].value &&
           nl_semantic_value_view(caller->context, formal.value, &input) &&
           nl_semantic_value_view(caller->context, call->results[0].value,
                                  &result) &&
           !input.scalar_known && !result.scalar_known &&
           input.type == result.type && nl_h_custody_result_valid(caller, id);
}

static bool guarded_selector_result(const NLDivisionUsizePlan *selector)
{
    const NLCheckedFragment *body = selector->body;
    NLDivisionUsizePlan guarded;
    if (body->body_count != 1 ||
        !nl_quantity_usize_call_valid(body, body->body_calls[0], &guarded) ||
        !guarded_product_result(body, body->body_calls[0], &guarded))
        return false;
    const NLCheckedNodeView *root = nl_checked_node_view(body, body->root);
    const NLCheckedNodeView *binding =
        root ? nl_checked_node_view(body, root->first_item) : NULL;
    const NLCheckedNodeView *call =
        nl_checked_node_view(body, body->body_calls[0]);
    const NLCheckedNodeView *actual =
        call ? nl_checked_node_view(body, call->first_argument) : NULL;
    const NLCheckedNodeView *choice =
        nl_checked_node_view(body, selector->result);
    const NLCheckedNodeView *condition =
        choice ? nl_checked_node_view(body, choice->initializer) : NULL;
    const NLCheckedNodeView *left =
        condition ? nl_checked_node_view(body, condition->first_argument)
                  : NULL;
    NLSemanticBindingView bytes;
    NLSemanticValueView mode;
    return binding && actual && left && root->tail == selector->result &&
           binding->initializer == body->body_calls[0] &&
           actual->symbol == selector->formal &&
           choice->kind == NL_CHECKED_IF && choice->normal_arms == 2 &&
           condition->kind == NL_CHECKED_USIZE_LESS_THAN &&
           left->symbol == binding->symbol &&
           left->value_use == NL_VALUE_COPIED &&
           nl_semantic_binding_view(body->context, binding->symbol, &bytes) &&
           bytes.value == call->results[0].value &&
           nl_semantic_value_view(body->context, choice->results[0].value,
                                  &mode) &&
           !mode.scalar_known;
}

static bool retry_product(const NLCheckedFragment *body, ProductRoute route)
{
    NLDivisionUsizePlan helper;
    if (body->body_count != 1 ||
        !nl_quantity_usize_call_valid(body, body->body_calls[0], &helper))
        return false;
    if (route == GUARDED_PRODUCT)
        return guarded_product_result(body, body->body_calls[0], &helper);
    if (route == GUARDED_ADDITION_DECISION)
        return guarded_addition_result(body, body->body_calls[0], &helper);
    if (route == GUARDED_SELECTOR)
        return nl_h_custody_result_valid(body, body->body_calls[0]) &&
               guarded_selector_result(&helper);
    if (helper.formal)
        return false;
    if (route == HELPER_PRODUCT)
        return nl_usize_multiplication_valid(helper.body, helper.result);
    if (route == COMPOSED_PRODUCT)
        return nl_h_custody_result_valid(body, body->body_calls[0]) &&
               composed_product_result(&helper);

    const NLCheckedNodeView *tail =
        nl_checked_node_view(helper.body, helper.result);
    const NLCheckedNodeView *root =
        nl_checked_node_view(helper.body, nl_checked_root(helper.body));
    if (!tail || tail->kind != NL_CHECKED_IDENTIFIER || !root)
        return false;
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *item = nl_checked_node_view(helper.body, id);
        if (!item)
            return false;
        if (item->symbol == tail->symbol)
            return item->kind == NL_CHECKED_BINDING &&
                   nl_usize_multiplication_valid(helper.body,
                                                 item->initializer);
        id = item->next_item;
    }
    return false;
}

static bool policy_writer_result(const NLDivisionUsizePlan *plan,
                                 ProductRoute route)
{
    const bool named = route != GUARDED_BYTE_POLICY_WRITER;
    const bool delegated = route == GUARDED_BYTE_DELEGATED_POLICY_WRITER;
    const NLCheckedFragment *main = plan->body;
    const NLCheckedNodeId call = plan->byte_update_call;
    NLCopyPolicyWriter writer;
    CHECK(nl_copy_policy_writer(main, call, &writer) &&
          (writer.selector_call != 0) == named);
    NLCopyBodyParameters public_parameters;
    NLCopyBodyView public_body;
    CHECK(!nl_checked_copy_body_parameters(main, call, &public_parameters) &&
          !nl_checked_copy_body_view(main, call, &public_body));
    const NLCopyBodyProof *proof = writer.body->copy_body;
    const NLCheckedNodeView *actual =
        nl_checked_node_view(main, proof->parameters.arguments[1]);
    const NLCheckedNodeView *loan = nl_checked_node_view(main, writer.loan);
    const NLCheckedNodeView *block = nl_checked_node_view(main, main->root);
    NLSemanticBindingView formal;
    NLSemanticValueView budget;
    NLSemanticScopeView scope;
    NLSemanticPlaceView descriptor, field;
    CHECK(actual && actual->kind == NL_CHECKED_IDENTIFIER &&
          actual->value_use == NL_VALUE_COPIED && loan &&
          nl_semantic_binding_view(proof->entry, writer.budget, &formal) &&
          formal.value == actual->results[0].value &&
          nl_semantic_value_view(main->context, formal.value, &budget) &&
          budget.type == nl_semantic_core_type(main->context, NL_TYPE_USIZE) &&
          !budget.scalar_known && budget.dependencies == NL_DEPENDENCY_FREE);
    if (named) {
        NLPureU8WriterPolicy policy;
        CHECK(nl_pure_u8_writer_policy_at(writer.body, writer.body->context,
                                          writer.selector_call, &policy) &&
              (policy.delegate != 0) == delegated &&
              policy.threshold.body == writer.selection_body &&
              policy.threshold.selection == writer.selection);
        const NLPureU8Call *selector =
            delegated ? &policy.wrapper : &policy.threshold;
        const NLCheckedNodeView *choice =
            nl_checked_node_view(writer.body, writer.selector_call);
        const NLCheckedNodeView *arg =
            nl_checked_node_view(writer.body, choice->first_argument);
        NLSemanticBindingView selector_formal, selected;
        CHECK(arg && arg->symbol == writer.budget &&
              arg->value_use == NL_VALUE_COPIED &&
              nl_semantic_binding_view(writer.body->context, selector->formal,
                                       &selector_formal) &&
              selector_formal.value == arg->results[0].value &&
              nl_semantic_binding_view(proof->first->before, writer.selected,
                                       &selected) &&
              selected.value == choice->results[0].value);
        if (delegated) {
            const NLCheckedNodeView *leaf =
                nl_checked_node_view(selector->body, policy.delegate);
            const NLCheckedNodeView *forwarded =
                nl_checked_node_view(selector->body, leaf->first_argument);
            NLSemanticBindingView leaf_formal;
            CHECK(forwarded && forwarded->symbol == selector->formal &&
                  forwarded->value_use == NL_VALUE_COPIED &&
                  nl_semantic_binding_view(selector->body->context,
                                           policy.threshold.formal,
                                           &leaf_formal) &&
                  leaf_formal.value == forwarded->results[0].value &&
                  leaf->results[0].value == selected.value);
        }
    }
    CHECK(proof->view.event_count == 1 &&
          proof->first->view.kind == NL_COPY_BODY_CHANGE &&
          writer.change.child == loan->field.child &&
          writer.change.child_incarnation == loan->field.child_incarnation &&
          loan->results[0].value == proof->view.result.value &&
          nl_semantic_scope_view(main->context, loan->loan.scope, &scope) &&
          !scope.active &&
          nl_semantic_place_view(main->context, writer.change.parent,
                                 &descriptor) &&
          !descriptor.live &&
          descriptor.incarnation == writer.change.parent_incarnation &&
          nl_semantic_place_view(main->context, writer.change.child, &field) &&
          !field.live && field.incarnation == writer.change.child_incarnation);
    CHECK(block);
    if (!block->terminates) {
        const NLTypeId unit = nl_semantic_unit_type(main->context);
        const NLCheckedNodeView *tail = nl_checked_node_view(main, block->tail);
        CHECK(block->type == unit && !block->result_count &&
              !block->results[0].value && !nl_control_exits_count(main->exits));
        CHECK(!block->tail ||
              (tail && tail->kind == NL_CHECKED_UNIT && tail->type == unit &&
               !tail->result_count && !tail->results[0].value));
    }
    return true;
}

static bool guarded_byte_mode_result(const NLCheckedFragment *entry,
                                     ProductRoute route)
{
    const bool delegated = route == GUARDED_BYTE_MODE_HELPER;
    const NLCheckedNodeId site = nl_checked_root(entry);
    NLDivisionUsizePlan main, product;
    if (!nl_h_custody_result_world_present(entry, site) ||
        !nl_quantity_usize_call_valid(entry, site, &main) ||
        main.body->body_count != (delegated ? 3u : 2u) ||
        !nl_quantity_usize_call_valid(main.body, main.body->body_calls[0],
                                      &product) ||
        !guarded_product_result(main.body, main.body->body_calls[0], &product))
        return false;
    if (route == GUARDED_BYTE_POLICY_WRITER ||
        route == GUARDED_BYTE_NAMED_POLICY_WRITER ||
        route == GUARDED_BYTE_DELEGATED_POLICY_WRITER)
        return !main.byte_selector &&
               main.byte_update_call == main.body->body_calls[1] &&
               policy_writer_result(&main, route);
    if (main.byte_selector != main.body->body_calls[1] ||
        (delegated ? main.byte_update_call != main.body->body_calls[2]
                   : main.byte_update_call != 0))
        return false;
    NLPureU8Call selector;
    const NLCheckedNodeView *call =
        nl_checked_node_view(main.body, main.byte_selector);
    NLSemanticValueView mode;
    return call && nl_pure_u8_call(main.body, main.byte_selector, &selector) &&
           nl_semantic_value_view(main.body->context, call->results[0].value,
                                  &mode) &&
           mode.type == nl_semantic_core_type(main.body->context, NL_TYPE_U8) &&
           !mode.scalar_known && mode.dependencies == NL_DEPENDENCY_FREE;
}

static bool check_oom(const char *path, ProductRoute route)
{
    const bool byte_mode = route == GUARDED_BYTE_MODE ||
                           route == GUARDED_BYTE_MODE_HELPER ||
                           route == GUARDED_BYTE_POLICY_WRITER ||
                           route == GUARDED_BYTE_NAMED_POLICY_WRITER ||
                           route == GUARDED_BYTE_DELEGATED_POLICY_WRITER;
    TestNode original = {0};
    CHECK(node_checked_load(path, &original));
    if (byte_mode)
        CHECK(guarded_byte_mode_result(original.entry, route));
    /* Use the registered declarations from the input. Each allocation trial
     * gets its own context; a failed check is retried on that same context. */
    NLParser *parser = NULL;
    NLSyntaxTree *call = NULL;
    CHECK(nl_parser_create(original.entry_source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &call, NULL) ==
              NL_PARSE_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before);
    size_t count = 0;
    for (; count < 2000; ++count) {
        NLSemanticContext *c = NULL;
        NLCheckedFragment *out = NULL;
        CHECK(nl_sem_clone(original.context, &c) == NL_CHECK_OK &&
              test_state(c, before));
        select_failure(count);
        const NLCheckStatus status =
            nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (!hits) {
            CHECK(status == NL_CHECK_OK && out && calls == count);
            nl_checked_destroy(out);
            nl_semantic_destroy(c);
            break;
        }
        CHECK(hits == 1 && status == NL_CHECK_OUT_OF_MEMORY && !out &&
              test_unchanged(c, before));
        CHECK(nl_semantic_check_expression(c, call, &out, NULL) ==
                  NL_CHECK_OK &&
              out);
        const NLCheckedFragment *body =
            nl_checked_call_body(out, nl_checked_root(out));
        CHECK(body);
        if (byte_mode) {
            CHECK(guarded_byte_mode_result(out, route));
        } else if (route != ENTRY_PRODUCT) {
            CHECK(retry_product(body, route));
        } else {
            CHECK(body->usize_additions);
        }
        nl_checked_destroy(out);
        nl_semantic_destroy(c);
    }
    free(before);
    nl_syntax_tree_destroy(call);
    nl_parser_destroy(parser);
    node_checked_destroy(&original);
    CHECK(count && count < 2000);
    printf("CHECK allocation failures=%zu exact state, no artifact, "
           "same-context retry\n",
           count);
    if (byte_mode) {
        CHECK(entry_world_failures);
        printf("Whole-entry world seal allocation failures=%zu, "
               "revalidated whole world and both real child results on retry\n",
               entry_world_failures);
    }
    return true;
}

static bool file_has(const char *path, const char *bytes, size_t length)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    char block[8192];
    const size_t read = fread(block, 1, sizeof(block), f);
    const bool equal = !ferror(f) && read == length && length < sizeof(block) &&
                       memcmp(bytes, block, length) == 0;
    return fclose(f) == 0 && equal;
}

static bool no_staging_file(const char *path)
{
    DIR *d = opendir(path);
    if (!d)
        return false;
    bool clean = true;
    const struct dirent *entry;
    while ((entry = readdir(d)))
        if (strstr(entry->d_name, ".tmp."))
            clean = false;
    return closedir(d) == 0 && clean;
}

static bool check_named_failures(const char *input, const char *dir,
                                 size_t limit)
{
    char target[128], absent[128], expected[8192];
    CHECK(snprintf(target, sizeof(target), "%s/output.c", dir) > 0 &&
          snprintf(absent, sizeof(absent), "%s/absent.c", dir) > 0);
    CHECK(emit_c17_to(target, input) == 0);
    FILE *f = fopen(target, "rb");
    CHECK(f);
    const size_t expected_length = fread(expected, 1, sizeof(expected), f);
    CHECK(!ferror(f) && fclose(f) == 0 && expected_length &&
          expected_length < sizeof(expected));

    size_t count = 0;
    for (; count < limit; ++count) {
        struct stat before, after;
        CHECK(stat(target, &before) == 0);
        select_failure(count);
        const int result = emit_c17_to(target, input);
        injecting = false;
        if (!hits) {
            CHECK(result == 0 && calls == count);
            break;
        }
        CHECK(hits == 1 && result != 0 && stat(target, &after) == 0 &&
              before.st_ino == after.st_ino &&
              before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
              before.st_mtim.tv_nsec == after.st_mtim.tv_nsec &&
              file_has(target, expected, expected_length));

        select_failure(count);
        const int absent_result = emit_c17_to(absent, input);
        injecting = false;
        CHECK(hits == 1 && absent_result != 0 && access(absent, F_OK) != 0 &&
              errno == ENOENT && no_staging_file(dir));
        CHECK(emit_c17_to(target, input) == 0 &&
              file_has(target, expected, expected_length));
    }
    CHECK(unlink(target) == 0 && no_staging_file(dir) && count &&
          count < limit);
    printf("named C17 allocation failures=%zu each for existing/absent; "
           "no-clobber and same-process retry\n",
           count);
    return true;
}

static bool named_oom(const char *input, size_t limit)
{
    char dir[] = "p1673-stage.XXXXXX";
    FILE *diagnostics = tmpfile();
    const int saved = dup(STDERR_FILENO);
    CHECK(diagnostics && saved >= 0 && mkdtemp(dir));
    CHECK(fflush(stderr) == 0 && dup2(fileno(diagnostics), STDERR_FILENO) >= 0);
    const bool ok = check_named_failures(input, dir, limit);
    CHECK(fflush(stderr) == 0 && dup2(saved, STDERR_FILENO) >= 0 &&
          close(saved) == 0);
    if (!ok) {
        CHECK(fseek(diagnostics, 0, SEEK_END) == 0);
        const long size = ftell(diagnostics);
        CHECK(size >= 0);
        CHECK(fseek(diagnostics, size > 2000 ? size - 2000 : 0, SEEK_SET) == 0);
        char line[512];
        while (fgets(line, sizeof(line), diagnostics))
            fputs(line, stderr);
    }
    CHECK(fclose(diagnostics) == 0 && ok && rmdir(dir) == 0);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    if (!strcmp(argv[1], "receipt"))
        return receipts(argv[2]) && dependencies(argv[3]) ? 0 : 1;
    ProductRoute route;
    if (!strcmp(argv[1], "oom"))
        route = ENTRY_PRODUCT;
    else if (!strcmp(argv[1], "helper-oom"))
        route = HELPER_PRODUCT;
    else if (!strcmp(argv[1], "local-product-oom"))
        route = LOCAL_PRODUCT;
    else if (!strcmp(argv[1], "composed-product-oom"))
        route = COMPOSED_PRODUCT;
    else if (!strcmp(argv[1], "guarded-product-oom"))
        route = GUARDED_PRODUCT;
    else if (!strcmp(argv[1], "guarded-addition-decision-oom"))
        route = GUARDED_ADDITION_DECISION;
    else if (!strcmp(argv[1], "guarded-selector-oom"))
        route = GUARDED_SELECTOR;
    else if (!strcmp(argv[1], "guarded-byte-mode-oom"))
        route = GUARDED_BYTE_MODE;
    else if (!strcmp(argv[1], "guarded-byte-mode-helper-oom"))
        route = GUARDED_BYTE_MODE_HELPER;
    else if (!strcmp(argv[1], "guarded-byte-policy-writer-oom"))
        route = GUARDED_BYTE_POLICY_WRITER;
    else if (!strcmp(argv[1], "guarded-byte-named-policy-writer-oom"))
        route = GUARDED_BYTE_NAMED_POLICY_WRITER;
    else if (!strcmp(argv[1], "guarded-byte-delegated-policy-writer-oom"))
        route = GUARDED_BYTE_DELEGATED_POLICY_WRITER;
    else
        return 2;
    /* A named selector and its optional sealed leaf need a larger sweep. */
    const size_t named_limit =
        route == GUARDED_BYTE_NAMED_POLICY_WRITER ||
                route == GUARDED_BYTE_DELEGATED_POLICY_WRITER
            ? 3000
            : 2000;
    return check_oom(argv[2], route) && named_oom(argv[2], named_limit) ? 0 : 1;
}
