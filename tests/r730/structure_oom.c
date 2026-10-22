/* R730 independent, public borrowed view structure and bounded allocator-fault
 * replay. */
#include "../support/node_checked.h"
#include "newlang/copy_body.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern void *__real_malloc(size_t);
extern void *__real_calloc(size_t, size_t);
extern void *__real_realloc(void *, size_t);

static bool fault_active;
static bool fault_triggered;
static size_t fault_count;
static size_t fault_at;

static bool fail_now(void)
{
    if (!fault_active)
        return false;
    ++fault_count;
    if (fault_count != fault_at)
        return false;
    fault_triggered = true;
    return true;
}

void *__wrap_malloc(size_t size)
{
    return fail_now() ? NULL : __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    return fail_now() ? NULL : __real_calloc(count, size);
}
void *__wrap_realloc(void *pointer, size_t size)
{
    return fail_now() ? NULL : __real_realloc(pointer, size);
}

static bool verify(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *caller =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(caller != NULL);
    size_t seen = 0;
    for (size_t id = 1; id <= nl_checked_node_count(caller); ++id) {
        NLCopyUnitView view;
        if (!nl_checked_copy_unit_view(caller, id, &view))
            continue;
        CHECK(nl_checked_copy_unit_validate(caller, id, &view) == NL_CHECK_OK);
        CHECK(nl_checked_copy_body_parameters_validate(
                  caller, id, &view.parameters) == NL_CHECK_OK);
        const NLCheckedNodeView *loan =
            nl_checked_node_view(caller, view.parameters.loan);
        CHECK(loan != NULL && loan->kind == NL_CHECKED_LOAN_HEADER);
        const NLCheckedNodeView *block =
            nl_checked_node_view(caller, loan->initializer);
        CHECK(block != NULL && block->kind == NL_CHECKED_BLOCK);
        CHECK(block->item_count == 1 && block->first_item != 0);
        CHECK(block->tail == 0 && block->next_item == 0);
        const NLCheckedNodeView *statement =
            nl_checked_node_view(caller, block->first_item);
        CHECK(statement != NULL && statement->kind == NL_CHECKED_STATEMENT);
        CHECK(statement->initializer == id && statement->next_item == 0);
        CHECK(statement->tail == 0 && statement->item_count == 0);
        CHECK(statement->first_argument == 0 && statement->argument_count == 0);
        const NLCheckedNodeView *call = nl_checked_node_view(caller, id);
        CHECK(call != NULL && call->kind == NL_CHECKED_REGISTERED_CALL);
        CHECK(call->argument_count == 2 && call->result_count == 0);
        CHECK(call->function == view.function);
        size_t parent_references = 0;
        for (size_t k = 1; k <= nl_checked_node_count(caller); ++k) {
            const NLCheckedNodeView *node = nl_checked_node_view(caller, k);
            CHECK(node != NULL);
            parent_references +=
                (node->initializer == id) + (node->first_item == id) +
                (node->next_item == id) + (node->tail == id) +
                (node->first_argument == id) + (node->next_argument == id);
        }
        CHECK(parent_references == 1);
        bool saw_oom = false;
        for (size_t at = 1; at <= 48; ++at) {
            fault_at = at;
            fault_count = 0;
            fault_triggered = false;
            fault_active = true;
            NLCheckStatus status =
                nl_checked_copy_unit_validate(caller, id, &view);
            fault_active = false;
            if (fault_triggered) {
                if (status != NL_CHECK_OUT_OF_MEMORY) {
                    fprintf(stderr,
                            "R730 OOM fail-at=%zu status=%d (expected %d)\\n",
                            at, (int)status, (int)NL_CHECK_OUT_OF_MEMORY);
                    return false;
                }
                saw_oom = true;
            } else {
                CHECK(status == NL_CHECK_OK);
            }
            CHECK(nl_checked_copy_unit_validate(caller, id, &view) ==
                  NL_CHECK_OK);
        }
        CHECK(saw_oom);
        ++seen;
    }
    CHECK(seen == 1);
    node_checked_destroy(&n);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 5)
        return 2;
    for (int i = 1; i < argc; ++i)
        if (!verify(argv[i]))
            return 1;
    return 0;
}
