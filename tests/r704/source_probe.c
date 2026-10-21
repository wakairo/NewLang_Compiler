/* Track R #704: observe genuine registered-source temporal evidence only. */
#include "newlang/copy_body.h"
#include "node_checked.h"
#include <stdio.h>
#include <stdlib.h>

static bool scalar(const NLSemanticContext *c, NLValueId id, size_t expected)
{
    NLSemanticValueView v;
    CHECK(nl_semantic_value_view(c, id, &v));
    CHECK(v.type == nl_semantic_core_type(c, NL_TYPE_U8));
    CHECK(v.scalar_known && v.scalar_value == expected);
    CHECK(v.dependencies == NL_DEPENDENCY_FREE &&
          v.value_dependency_count == 0 && v.reference_count == 0 &&
          v.reference.place == 0 && v.reference.scope == 0 && v.domain == 0 &&
          v.slot_place == 0 && v.allocation_region == 0);
    return true;
}

static bool probe(const char *path, size_t result, size_t internal_old,
                  size_t internal_new, size_t caller_old, size_t field,
                  size_t dest_field)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    const NLSemanticContext *c = nl_checked_context(f);
    NLCheckedNodeId call_id = 0;
    const NLCheckedNodeView *call = NULL, *caller_write = NULL;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_REGISTERED_CALL) {
            CHECK(!call);
            call = v;
            call_id = i;
        }
        if (v->kind == NL_CHECKED_REPLACE) {
            CHECK(!caller_write);
            caller_write = v;
        }
    }
    CHECK(call && caller_write && call->body_backed && !call->argument_count &&
          call->result_count == 1 && call_id != 0);
    const NLCheckedFragment *body = nl_checked_call_body(f, call_id);
    NLCopyBodyView view;
    CHECK(body && nl_checked_copy_body_view(f, call_id, &view));
    CHECK(view.body == body && view.function == call->function &&
          view.event_count >= 3 && view.result.value == call->results[0].value);
    CHECK(nl_checked_copy_body_validate(f, call_id, NULL) == NL_CHECK_OK);
    CHECK(nl_checked_copy_body_validate(f, call_id, &view) == NL_CHECK_OK);
    CHECK(scalar(c, view.result.value, result));
    NLCopyBodyView altered = view;
    altered.event_count++;
    CHECK(nl_checked_copy_body_validate(f, call_id, &altered) != NL_CHECK_OK);
    altered = view;
    altered.function++;
    CHECK(nl_checked_copy_body_validate(f, call_id, &altered) != NL_CHECK_OK);
    altered = view;
    altered.live_exit = NULL;
    CHECK(nl_checked_copy_body_validate(f, call_id, &altered) != NL_CHECK_OK);
    CHECK(nl_checked_copy_body_validate(f, call_id + 1, &view) != NL_CHECK_OK);

    size_t init = 0, changed = 0, reads = 0;
    NLPlaceId callee_root = 0;
    for (size_t i = 0; i < view.event_count; ++i) {
        NLCopyBodyEventView e;
        CHECK(nl_checked_copy_body_event(f, call_id, i, &e));
        CHECK(nl_checked_copy_body_event_validate(f, call_id, i, &e) ==
              NL_CHECK_OK);
        CHECK(e.after != NULL);
        NLCopyBodyEventView corrupted = e;
        corrupted.node++;
        CHECK(nl_checked_copy_body_event_validate(f, call_id, i, &corrupted) !=
              NL_CHECK_OK);
        corrupted = e;
        corrupted.field.parent++;
        CHECK(nl_checked_copy_body_event_validate(f, call_id, i, &corrupted) !=
              NL_CHECK_OK);
        corrupted = e;
        corrupted.after = NULL;
        CHECK(nl_checked_copy_body_event_validate(f, call_id, i, &corrupted) !=
              NL_CHECK_OK);
        if (e.kind == NL_COPY_BODY_INITIALIZE) {
            CHECK(i == 0 && ++init == 1 && e.field.parent);
            callee_root = e.field.parent;
        } else if (e.kind == NL_COPY_BODY_CHANGE) {
            CHECK(callee_root && e.field.parent == callee_root &&
                  e.field.index == field && e.before && ++changed == 1);
            CHECK(scalar(c, e.result, internal_old));
            CHECK(scalar(c, e.incoming, internal_new));
            corrupted = e;
            corrupted.field.index = 1 - field;
            CHECK(nl_checked_copy_body_event_validate(f, call_id, i,
                                                      &corrupted) != NL_CHECK_OK);
            corrupted = e;
            corrupted.before = e.after;
            CHECK(nl_checked_copy_body_event_validate(f, call_id, i,
                                                      &corrupted) != NL_CHECK_OK);
            corrupted = e;
            corrupted.incoming = e.result;
            CHECK(nl_checked_copy_body_event_validate(f, call_id, i,
                                                      &corrupted) != NL_CHECK_OK);
        } else if (e.kind == NL_COPY_BODY_READ) {
            CHECK(e.field.parent == callee_root && ++reads <= 2);
        } else
            CHECK(false);
    }
    CHECK(init == 1 && changed == 1 && reads == 2);
    CHECK(!nl_checked_copy_body_event(f, call_id, view.event_count,
                                     &(NLCopyBodyEventView){0}));
    CHECK(caller_write->field.present &&
          caller_write->field.parent != callee_root &&
          caller_write->field.index == dest_field);
    CHECK(scalar(c, caller_write->field.old_value, caller_old));
    CHECK(scalar(c, caller_write->field.new_value, result));
    CHECK(nl_checked_copy_body_validate(f, call_id, NULL) == NL_CHECK_OK);
    printf("{\"call\":%zu,\"events\":%zu,\"result\":%zu,\"callee_root\":%zu,\"caller_root\":%zu,\"internal_field\":%zu,\"caller_field\":%zu,\"copied_event_corruptions_rejected\":true}\n",
           call_id, view.event_count, result, callee_root,
           caller_write->field.parent, field, dest_field);
    node_checked_destroy(&n);
    return true;
}

int main(int argc, char **argv)
{
    return argc == 8 &&
                   probe(argv[1], strtoul(argv[2], NULL, 10),
                         strtoul(argv[3], NULL, 10),
                         strtoul(argv[4], NULL, 10),
                         strtoul(argv[5], NULL, 10),
                         strtoul(argv[6], NULL, 10),
                         strtoul(argv[7], NULL, 10))
               ? 0
               : 1;
}
