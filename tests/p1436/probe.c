/* Reuse the immutable P1435 actual-source apparatus; add observations of the
 * same result receipt. No substitute source, host Known, or second issuer. */
#define main p1435_original_main
#define __wrap_malloc p1435_original_malloc
#include "../p1435/probe.c"
#undef __wrap_malloc
#undef main
#include "../../src/h_result_state_internal.h"

static bool in_result_seal;
static size_t snapshot_size;

void *__wrap_malloc(size_t size)
{
    if (fault == 4 && in_result_seal && !hit && size == snapshot_size) {
        hit = true;
        printf("snapshot malloc hit bytes=%zu\n", size);
        return NULL;
    }
    return p1435_original_malloc(size);
}
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    const bool saved = in_result_seal;
    if (fault == 4 && world && child->context == parent->context) {
        HResultBytes count = {0};
        if (!result_state(&count, child, world) || !count.used)
            return NL_CHECK_INTERNAL_ERROR;
        snapshot_size = count.used;
        in_result_seal = true;
    }
    NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, call, world);
    in_result_seal = saved;
    return status;
}

typedef struct {
    NLSemanticContext *world[64];
    size_t count;
} Worlds;
static bool worlds(NLCheckedFragment *f, Worlds *out, size_t depth)
{
    CHECK(depth < 16);
    NLSemanticContext *c = (NLSemanticContext *)nl_checked_context(f);
    size_t i = 0;
    while (i < out->count && out->world[i] != c)
        ++i;
    if (i == out->count) {
        CHECK(out->count < 64);
        out->world[out->count++] = c;
    }
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(f); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(worlds((NLCheckedFragment *)body, out, depth + 1));
        if (n->kind == NL_CHECKED_IF || n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a) {
                body = n->kind == NL_CHECKED_IF
                           ? nl_checked_if_arm(f, id, a)
                           : nl_checked_match_arm(f, id, a);
                CHECK(body &&
                      worlds((NLCheckedFragment *)body, out, depth + 1));
            }
    }
    return true;
}
static bool invalid(Program *p, const char *label)
{
    NLCheckedProofClass observed = nl_checked_proof_class(p->artifact);
    printf("%s class=%d\n", label, (int)observed);
    CHECK(observed == NL_CHECKED_PROOF_INVALID);
    CHECK(output(p, NL_NODE_C_UNSUPPORTED));
    return true;
}
#define DAMAGE(program, target, replacement, label)                            \
    do {                                                                       \
        unsigned char saved_bytes[sizeof(target)];                             \
        memcpy(saved_bytes, &(target), sizeof(target));                        \
        (target) = (replacement);                                              \
        bool rejected = invalid(program, label);                               \
        memcpy(&(target), saved_bytes, sizeof(target));                        \
        CHECK(rejected);                                                       \
        CHECK(nl_checked_proof_class((program)->artifact) ==                   \
              NL_CHECKED_MACHINE_PROVED);                                      \
    } while (0)

static bool authentication(const char *a, const char *b)
{
    Program p = {0}, other = {0};
    Trace t = {0};
    Worlds all = {0};
    CHECK(ready(a, &p) && actual(&p) && inspect(p.artifact, &t, 0));
    CHECK(ready(b, &other) && actual(&other));
    CHECK(nl_h_custody_result_world_present(p.artifact,
                                            nl_checked_root(p.artifact)));
    CHECK(t.decision && !t.decision->scalar_known);
    printf("healthy class=%d full-world=1\n",
           (int)nl_checked_proof_class(p.artifact));
    DAMAGE(&p, t.decision->scalar_known, true, "original Unknown-to-Known");
    CHECK(worlds(p.artifact, &all, 0) && all.count >= 7);
    for (size_t w = 0; w < all.count; ++w) {
        NLSemanticContext *c = all.world[w];
        const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
        const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
        size_t v = 0;
        while (v < c->value_count && c->values[v].type != boolean &&
               c->values[v].type != u8)
            ++v;
        CHECK(v < c->value_count);
        printf("world=%zu/%zu value=%zu type=%zu\n", w, all.count, v + 1,
               c->values[v].type);
        DAMAGE(&p, c->values[v].scalar_known, !c->values[v].scalar_known,
               "canonical world primitive mutation");
        if (c->domain_count)
            DAMAGE(&p, c->domains[0].live, !c->domains[0].live,
                   "original Domain mutation");
        if (c->scope_count)
            DAMAGE(&p, c->scopes[0].active, !c->scopes[0].active,
                   "actual loan scope mutation");
    }
    DAMAGE(&p, t.if_owner->context, other.context, "foreign actual world");
    DAMAGE(&p, p.artifact->h_origin, other.artifact->h_origin,
           "foreign actual owner");
    CHECK(output(&p, NL_NODE_C_OK));
    destroy(&other);
    destroy(&p);
    return true;
}

static bool snapshot_oom(const char *path)
{
    Program p = {0};
    CHECK(ready(path, &p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    fault = 4;
    hit = false;
    NLCheckStatus status =
        nl_semantic_check_expression(p.context, p.call, &p.artifact, NULL);
    fault = 0;
    printf("snapshot OOM hit=%d bytes=%zu status=%d out=%p\n", hit,
           snapshot_size, (int)status, (void *)p.artifact);
    CHECK(hit && snapshot_size && status == NL_CHECK_OUT_OF_MEMORY &&
          !p.artifact && nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof(before)));
    CHECK(actual(&p) && nl_h_custody_result_world_present(
                            p.artifact, nl_checked_root(p.artifact)));
    CHECK(output(&p, NL_NODE_C_OK));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 4 && !strcmp(argv[1], "auth"))
        return authentication(argv[2], argv[3]) &&
                       authentication(argv[3], argv[2])
                   ? 0
                   : 1;
    if (argc == 3 && !strcmp(argv[1], "snapshot-oom"))
        return snapshot_oom(argv[2]) ? 0 : 1;
    return 2;
}
