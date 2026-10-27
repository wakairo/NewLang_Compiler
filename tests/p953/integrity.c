#include "../../src/copy_body_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>
static NLCheckedFragment *caller(NLCheckedFragment *f, NLCheckedNodeId *id)
{
    for (size_t n = 1; n <= f->count; ++n) {
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, n, &p)) {
            *id = n;
            return f;
        }
    }
    for (size_t i = 0; i < f->arm_count; ++i) {
        NLCheckedFragment *found = caller(f->arms[i].artifact, id);
        if (found)
            return found;
    }
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedFragment *found = caller(f->bodies[i], id);
        if (found)
            return found;
    }
    return NULL;
}
static bool probe(const char *path)
{
    TestNode a = {0}, b = {0};
    NLCheckedNodeId id = 0, other = 0;
    CHECK(node_checked_load(path, &a) && node_checked_load(path, &b));
    NLCheckedFragment *f = caller(a.entry, &id),
                      *foreign = caller(b.entry, &other);
    NLCopyBodyParameters p;
    NLCopyBodyView body;
    CHECK(f && foreign && id == other &&
          nl_checked_copy_body_parameters(f, id, &p) &&
          nl_checked_copy_body_view(f, id, &body));
    CHECK(nl_checked_copy_body_validate(f, id, &body) == NL_CHECK_OK &&
          nl_checked_copy_body_parameters_validate(f, id, &p) == NL_CHECK_OK);
    char *baseline = NULL;
    size_t bytes = 0;
    CHECK(nl_checked_c_node(a.entry, &baseline, &bytes) == NL_NODE_C_OK &&
          bytes);
    NLSemanticContext *entry = (NLSemanticContext *)p.entry;
    size_t mutations = 0;
#define TRY(change, restore)                                                   \
    do {                                                                       \
        change;                                                                \
        char *c = NULL;                                                        \
        size_t z = 0;                                                          \
        NLNodeCStatus status = nl_checked_c_node(a.entry, &c, &z);             \
        restore;                                                               \
        CHECK(status != NL_NODE_C_OK && !c && !z);                             \
        CHECK(nl_checked_c_node(a.entry, &c, &z) == NL_NODE_C_OK &&            \
              z == bytes && !memcmp(c, baseline, z));                          \
        free(c);                                                               \
        ++mutations;                                                           \
    } while (0)
    NLSemanticPlaceView saved = entry->places[p.field.child - 1];
    TRY(entry->places[p.field.child - 1].current_fact++,
        entry->places[p.field.child - 1] = saved);
    TRY(entry->places[p.field.child - 1].parent_field_index ^= 1,
        entry->places[p.field.child - 1] = saved);
    TRY(entry->places[p.field.child - 1].parent_aggregate = p.field.parent + 3,
        entry->places[p.field.child - 1] = saved);
    NLSemanticScopeView scope = entry->scopes[0];
    TRY(entry->scopes[0].active = true, entry->scopes[0] = scope);
    NLSemanticDomainView domain = entry->domains[0];
    TRY(entry->domains[0].live = false, entry->domains[0] = domain);
    NLCheckedNodeView call = f->nodes[id - 1];
    TRY(f->nodes[id - 1].first_argument = 0, f->nodes[id - 1] = call);
    NLCheckedArm arm = f->arms[0];
    TRY(f->arms[0].context_origin = foreign->arms[0].context_origin,
        f->arms[0] = arm);
    NLCheckedNodeId constructor = 0;
    for (size_t n = 1; n <= f->count; n++)
        if (f->nodes[n - 1].kind == NL_CHECKED_AGGREGATE &&
            f->nodes[n - 1].type == p.field.nominal) {
            constructor = n;
            break;
        }
    CHECK(constructor);
    NLCheckedNodeView aggregate = f->nodes[constructor - 1];
    TRY(f->nodes[constructor - 1].first_argument = 0,
        f->nodes[constructor - 1] = aggregate);
    NLCheckedNodeId declaration = 0;
    for (size_t n = 1; n <= f->count; ++n)
        if (f->nodes[n - 1].kind == NL_CHECKED_BINDING &&
            f->nodes[n - 1].initializer == constructor)
            declaration = n;
    CHECK(declaration);
    NLCheckedNodeView binding = f->nodes[declaration - 1];
    TRY(f->nodes[declaration - 1].next_item = declaration,
        f->nodes[declaration - 1] = binding);
    NLCheckedNodeId field = aggregate.first_argument;
    NLCheckedNodeView member = f->nodes[field - 1];
    TRY(f->nodes[field - 1].field_index ^= 1, f->nodes[field - 1] = member);
    printf("P953 actual public Copy OK; %zu tamper reject/restore; C=%zu\n",
           mutations, bytes);
    free(baseline);
    node_checked_destroy(&a);
    node_checked_destroy(&b);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && probe(argv[1]) ? 0 : 1;
}
