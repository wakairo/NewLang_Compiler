/* Test-only unchanged-main public evidence; no internal header or seed API. */
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

static bool inspect(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *body =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(body != NULL && nl_checked_context(body) == n.context);
    size_t matches = 0, writes = 0;
    for (size_t i = 1; i <= nl_checked_node_count(body); ++i) {
        const NLCheckedNodeView *m = nl_checked_node_view(body, i);
        /* saved_copy also has an earlier payload-extraction match. */
        if (m->kind != NL_CHECKED_MATCH || m->normal_arms != 2 ||
            m->normal_frame_unchanged)
            continue;
        ++matches;
        CHECK(m->normal_arms == 2 && m->item_count == 2 &&
              !m->normal_frame_unchanged && m->match_binding_prefix != 0);
        CHECK(m->packet_fork.entry_world == NULL &&
              m->packet_fork.post_world == NULL &&
              nl_checked_captured_post(body, i) == NULL);
        CHECK(!nl_checked_packet_fork_valid(body, i) &&
              !nl_checked_packet_retaining_join_valid(body, i) &&
              !nl_checked_custody_valid(body, i));
        const NLSemanticContext *worlds[2] = {0};
        for (size_t a = 0; a < 2; ++a) {
            const NLCheckedFragment *arm = nl_checked_match_arm(body, i, a);
            CHECK(arm != NULL);
            worlds[a] = nl_checked_context(arm);
            CHECK(worlds[a] != n.context);
            const NLCheckedNodeView *root =
                nl_checked_node_view(arm, nl_checked_root(arm));
            CHECK(root != NULL && !root->terminates &&
                  root->kind == NL_CHECKED_MATCH_ARM && root->type == m->type);
            for (size_t j = 1; j <= nl_checked_node_count(arm); ++j) {
                const NLCheckedNodeView *v = nl_checked_node_view(arm, j);
                if (v->kind != NL_CHECKED_REPLACE)
                    continue;
                CHECK(v->field.present && v->field.index == 0 &&
                      v->field.parent_fact != v->field.parent_post_fact &&
                      v->field.child_fact != v->field.child_post_fact);
                ++writes;
                printf("ARM %zu ROOT %zu INC %zu FIELD %zu F %zu %zu RF %zu "
                       "%zu OCC %zu %zu\n",
                       a, v->field.parent, v->field.parent_incarnation,
                       v->field.child, v->field.child_fact,
                       v->field.child_post_fact, v->field.parent_fact,
                       v->field.parent_post_fact, v->field.payload_occurrence,
                       v->field.post_payload_occurrence);
            }
        }
        CHECK(worlds[0] != worlds[1]);
        printf("MATCH span=%zu:%zu normal=2 unchanged=false prefix=%zu "
               "owned_worlds=separate packet_certificate=absent "
               "captured_post=absent\n",
               m->span.start_byte, m->span.end_byte, m->match_binding_prefix);
    }
    CHECK(matches == 1 && writes >= 1 && writes <= 2);
    char *output = NULL;
    size_t bytes = 31;
    NLSemanticSnapshot before = {0}, after = {0};
    CHECK(nl_semantic_snapshot(n.context, &before));
    CHECK(nl_checked_c_node(n.entry, &output, &bytes) == NL_NODE_C_UNSUPPORTED);
    CHECK(output == NULL && bytes == 31);
    CHECK(nl_semantic_snapshot(n.context, &after));
    CHECK(memcmp(&before, &after, sizeof(before)) == 0);
    puts("SOURCE_OWNED PASS BACKEND UNSUPPORTED output_slots_unchanged=true");
    node_checked_destroy(&n);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && inspect(argv[1]) ? 0 : 1;
}
