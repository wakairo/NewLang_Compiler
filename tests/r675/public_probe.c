#include "../support/node_checked.h"
#include "newlang/checked.h"
#include "newlang/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read-only registered-source observer: no fake IDs or private checked records. */
static bool inspect(const char *path, const char *expected_field, size_t index)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f);
    const NLSemanticContext *context = nl_checked_context(f);
    const NLTypeId core_u8 = nl_semantic_core_type(context, NL_TYPE_U8);
    NLTypeId nominal = 0;
    NLPlaceId root = 0, child = 0;
    NLSymbolId local = 0;
    size_t found = 0, read_count = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        CHECK(v);
        const NLCheckedField *field = &v->field;
        if (!field->present)
            continue;
        CHECK(field->nominal != 0 && field->type == core_u8 &&
              field->index < 2 && field->base != 0 && field->parent != 0 &&
              field->child != 0 && field->parent != field->child &&
              field->parent_incarnation != 0 && field->child_incarnation != 0 &&
              field->parent_fact != 0 && field->child_fact != 0);
        NLAggregateField declared;
        CHECK(nl_semantic_aggregate_field_view(context, field->nominal,
                                                field->index, &declared));
        CHECK(declared.type == core_u8 && declared.name);
        if (nominal != 0)
            CHECK(nominal == field->nominal);
        nominal = field->nominal;
        if (root != 0)
            CHECK(root == field->parent && local == field->base);
        root = field->parent;
        local = field->base;
        if (field->index == index) {
            CHECK(strcmp(declared.name, expected_field) == 0);
            if (child != 0)
                CHECK(child == field->child);
            child = field->child;
            ++found;
            if (v->kind == NL_CHECKED_FIELD_READ)
                ++read_count;
        }
        printf("R675 OWNED node=%zu kind=%d source_field=%s nominal=%zu index=%zu "
               "base=%zu parent=%zu child=%zu inc=(%zu,%zu) "
               "facts=(%zu,%zu) post=(%zu,%zu) old=%zu new=%zu\n",
               i, (int)v->kind, declared.name, (size_t)field->nominal,
               field->index, (size_t)field->base, (size_t)field->parent,
               (size_t)field->child, (size_t)field->parent_incarnation,
               (size_t)field->child_incarnation, (size_t)field->parent_fact,
               (size_t)field->child_fact, (size_t)field->parent_post_fact,
               (size_t)field->child_post_fact, (size_t)field->old_value,
               (size_t)field->new_value);
    }
    CHECK(nominal != 0 && root != 0 && child != 0 && found >= 1 &&
          read_count >= 1);
    printf("R675 OWNED READ-ONLY OK found=%zu reads=%zu original_root=%zu\n",
           found, read_count, (size_t)root);
    node_checked_destroy(&n);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    char *end = NULL;
    unsigned long index = strtoul(argv[3], &end, 10);
    return end && *end == '\0' && index < 2 &&
                   inspect(argv[1], argv[2], (size_t)index)
               ? 0
               : 1;
}
