/* R644: producer-aware source calibration; independently observed mutations.
 * Genuine registered source is checked, not a host-seeded semantic artifact. */
#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

static bool r644_valid(const TestNode *n)
{
    V0Program p = {.unit = nl_semantic_unit_type(n->context),
                   .u8 = nl_semantic_core_type(n->context, NL_TYPE_U8)};
    return v0_validate_call(n->entry, nl_checked_root(n->entry), &p);
}

static bool r644_public(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n) && r644_valid(&n));
    const NLCheckedFragment *f =
        nl_checked_call_body(n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_IF) {
            CHECK(id == 0);
            id = i;
        }
    CHECK(id != 0);
    const NLCheckedNodeView *join = nl_checked_node_view(f, id);
    const NLCheckedNodeView *cond = nl_checked_node_view(f, join->initializer);
    NLSemanticValueView truth = {0};
    CHECK(join->normal_arms == 2 && join->result_count == 1);
    CHECK(cond != NULL && cond->result_count == 1);
    const NLSemanticContext *parent = nl_checked_context(f);
    CHECK(nl_semantic_value_view(parent, cond->results[0].value, &truth));
    CHECK(truth.scalar_known && truth.scalar_value <= 1);
    size_t values[2] = {0, 0}, locals[2] = {0, 0};
    const NLSemanticContext *worlds[2] = {NULL, NULL};
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, a);
        CHECK(arm && nl_checked_source(arm) == nl_checked_source(f));
        worlds[a] = nl_checked_context(arm);
        const NLCheckedNodeView *root =
            nl_checked_node_view(arm, nl_checked_root(arm));
        const NLCheckedNodeView *block =
            root ? nl_checked_node_view(arm, root->initializer) : NULL;
        NLSemanticValueView value = {0};
        CHECK(root && block && block->item_count <= 1);
        CHECK(root->result_count == 1);
        CHECK(nl_semantic_value_view(worlds[a], root->results[0].value, &value));
        CHECK(value.scalar_known && value.scalar_value <= 255);
        values[a] = value.scalar_value;
        locals[a] = block->item_count;
        if (locals[a]) {
            const NLCheckedNodeView *decl =
                nl_checked_node_view(arm, block->first_item);
            NLSemanticBindingView binding;
            NLSemanticPlaceView place;
            CHECK(decl != NULL);
            CHECK(nl_semantic_binding_view(worlds[a], decl->symbol, &binding));
            CHECK(binding.availability == NL_CONSUMED);
            CHECK(nl_semantic_place_view(worlds[a], binding.place, &place));
            CHECK(!place.live && place.current_value == 0);
        }
    }
    CHECK(worlds[0] != worlds[1] && worlds[0] != nl_checked_context(f) &&
          worlds[1] != nl_checked_context(f));
    printf("{\"bool\":%zu,\"arms\":[%zu,%zu],"
           "\"locals\":[%zu,%zu],\"distinct\":true,\"ended\":true}\n",
           truth.scalar_value, values[0], values[1], locals[0], locals[1]);
    node_checked_destroy(&n);
    return true;
}

static bool r644_attacks(const char *path)
{
    TestNode protected = {0}, n = {0};
    CHECK(node_checked_load(path, &protected) && r644_valid(&protected));
    CHECK(node_checked_load(path, &n) && r644_valid(&n));
    NLCheckedFragment *f = (NLCheckedFragment *)nl_checked_call_body(
        n.entry, nl_checked_root(n.entry));
    CHECK(f != NULL);
    NLCheckedNodeId id = 0;
    for (size_t i = 1; i <= f->count; ++i)
        if (f->nodes[i - 1].kind == NL_CHECKED_IF)
            id = i;
    CHECK(id != 0);
    const NLCheckedFragment *arms[2] = {nl_checked_if_arm(f, id, 0),
                                        nl_checked_if_arm(f, id, 1)};
    CHECK(arms[0] && arms[1] && arms[0]->context != arms[1]->context);
    size_t slots[2] = {SIZE_MAX, SIZE_MAX};
    for (size_t i = 0; i < f->arm_count; ++i)
        for (size_t a = 0; a < 2; ++a)
            if (f->arms[i].artifact == arms[a])
                slots[a] = i;
    CHECK(slots[0] != SIZE_MAX && slots[1] != SIZE_MAX);
    const NLCheckedFragment saved_parent = *f;
    size_t rejected = 0;
    NLCheckedFragment shadow = *arms[0];
    shadow.context = arms[1]->context;
    f->arms[slots[0]].artifact = &shadow;
    const bool crossed = r644_valid(&n);
    f->arms[slots[0]].artifact = (NLCheckedFragment *)arms[0];
    CHECK(!crossed && r644_valid(&n) && r644_valid(&protected));
    ++rejected;
    shadow = *arms[0];
    shadow.if_arm_index = 1;
    f->arms[slots[0]].artifact = &shadow;
    const bool wrong_arm = r644_valid(&n);
    f->arms[slots[0]].artifact = (NLCheckedFragment *)arms[0];
    CHECK(!wrong_arm && r644_valid(&n));
    ++rejected;
    shadow = *arms[0];
    shadow.if_parent_node = id + 1;
    f->arms[slots[0]].artifact = &shadow;
    const bool wrong_parent = r644_valid(&n);
    f->arms[slots[0]].artifact = (NLCheckedFragment *)arms[0];
    CHECK(!wrong_parent && r644_valid(&n));
    ++rejected;
    size_t invalid = 0;
    for (size_t a = 0; a < 2; ++a) {
        NLSemanticContext *world = (NLSemanticContext *)arms[a]->context;
        CHECK(nl_fixed_validate(world) == NL_CHECK_OK);
        NLPlaceId root_id = 0;
        for (size_t i = 0; i < world->place_count; ++i)
            if (world->places[i].live && world->places[i].independent_root &&
                world->places[i].fixed_field_count == 2) {
                root_id = i + 1;
                break;
            }
        CHECK(root_id != 0);
        NLSemanticPlaceView *root = &world->places[root_id - 1];
        CHECK(root->fixed_fields[0] != 0 &&
              root->fixed_fields[0] <= world->place_count);
        NLSemanticPlaceView *child = &world->places[root->fixed_fields[0] - 1];
        const NLSemanticPlaceView saved_root = *root;
        const NLSemanticPlaceView saved_child = *child;
        for (size_t mutant = 0; mutant < 6; ++mutant) {
            if (mutant == 0)
                root->current_value = 0;
            else if (mutant == 1)
                root->current_value = world->value_count + 1;
            else if (mutant == 2)
                child->current_value = 0;
            else if (mutant == 3)
                child->current_value = world->value_count + 1;
            else if (mutant == 4)
                child->parent_aggregate = world->place_count + 1;
            else
                child->parent_field_index = NL_SEMANTIC_MAX_FIELDS;
            const NLCheckStatus result = nl_fixed_validate(world);
            *root = saved_root;
            *child = saved_child;
            CHECK(result == NL_CHECK_INTERNAL_ERROR);
            CHECK(nl_fixed_validate(world) == NL_CHECK_OK);
            CHECK(nl_sem_validate(world) == NL_CHECK_OK);
            CHECK(r644_valid(&n) && r644_valid(&protected));
            ++invalid;
        }
    }
    CHECK(memcmp(f, &saved_parent, sizeof(*f)) == 0);
    CHECK(r644_valid(&n) && r644_valid(&protected));
    printf("R644 ATTACKS rejected=%zu malformed=%zu original=UNCHANGED "
           "ASAN_UBSAN=RUNNER\n",
           rejected, invalid);
    node_checked_destroy(&n);
    node_checked_destroy(&protected);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    if (strcmp(argv[1], "public") == 0)
        return r644_public(argv[2]) ? 0 : 1;
    if (strcmp(argv[1], "attack") == 0)
        return r644_attacks(argv[2]) ? 0 : 1;
    return 2;
}
