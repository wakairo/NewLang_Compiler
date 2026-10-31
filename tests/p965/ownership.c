#include "shared.h"

void __real_free(void *);
typedef struct {
    const void *pointer;
    size_t frees;
} Watched;
static Watched watched[4096];
static size_t watch_count, a_count;
static bool watching;
void __wrap_free(void *p)
{
    if (watching && p)
        for (size_t i = 0; i < watch_count; ++i)
            if (watched[i].pointer == p)
                ++watched[i].frees;
    __real_free(p);
}
static bool watch(const void *p)
{
    if (!p)
        return true;
    CHECK(watch_count < sizeof(watched) / sizeof(*watched));
    for (size_t i = 0; i < watch_count; ++i)
        CHECK(watched[i].pointer != p);
    watched[watch_count++] = (Watched){p, 0};
    return true;
}
typedef struct {
    NLCheckedFragment *f[512];
    size_t count;
} Tree;
static bool collect(NLCheckedFragment *f, Tree *t)
{
    CHECK(t->count < sizeof(t->f) / sizeof(*t->f));
    t->f[t->count++] = f;
    CHECK(watch(f) && watch(f->arms) && watch(f->bodies) &&
          watch(f->body_calls));
    for (size_t i = 0; i < f->arm_count; ++i)
        CHECK(collect(f->arms[i].artifact, t));
    for (size_t i = 0; i < f->body_count; ++i)
        CHECK(collect(f->bodies[i], t));
    for (size_t i = 0; i < 2; ++i)
        if (f->custody_continuations[i])
            CHECK(collect(f->custody_continuations[i], t));
    return true;
}
/* Each case has a restore trial and a separate unrestored destruction trial.
 * Both genuine CHECKs use the same registry, hence the same retained plans. */
static bool trial(NLSemanticContext *base, size_t parent, unsigned fault,
                  bool restore, const char *baseline, size_t bytes)
{
    TestNode a = {0}, b = {0};
    Tree ta = {0}, tb = {0};
    CHECK(checked(base, &a) && checked(base, &b));
    watch_count = 0;
    CHECK(collect(a.entry, &ta));
    a_count = watch_count;
    CHECK(collect(b.entry, &tb) && ta.count == tb.count && parent < ta.count);
    NLCheckedFragment *x = ta.f[parent], *y = tb.f[parent];
    CHECK(a.entry->h_origin != b.entry->h_origin);
    CHECK(a.entry->bodies[0]->body_owner == b.entry->bodies[0]->body_owner);
    NLCheckedArm *arms = x->arms, saved = {0};
    NLCheckedFragment **bodies = x->bodies, *body = NULL, *cont = NULL;
    NLCheckedNodeId *calls = x->body_calls;
    size_t ac = x->arm_count, bc = x->body_count;
    size_t kind = ac ? (size_t)x->nodes[x->arms[0].match - 1].kind : 0;
    if (ac)
        saved = x->arms[0];
    if (bc)
        body = x->bodies[0];
    switch (fault) {
    case 0:
        x->arms[0] = y->arms[0];
        break;
    case 1:
        x->arm_count = 0;
        break;
    case 2:
        x->arm_count = 1;
        break;
    case 3:
        x->arm_count = SIZE_MAX;
        break;
    case 4:
        x->arms = y->arms;
        break;
    case 5:
        x->arms = y->arms;
        x->arm_count = SIZE_MAX;
        break;
    case 6:
        x->arms[0].match = 0;
        break;
    case 7:
        x->arms[0].context_origin = y->arms[0].context_origin;
        break;
    case 8:
        x->bodies[0] = y->bodies[0];
        break;
    case 9:
        x->body_count = 0;
        break;
    case 10:
        x->body_count = SIZE_MAX;
        break;
    case 11:
        x->bodies = y->bodies;
        break;
    case 12:
        x->body_calls = y->body_calls;
        break;
    case 13:
        x->bodies = y->bodies;
        x->body_count = SIZE_MAX;
        break;
    case 14:
        cont = x->custody_continuations[0];
        x->custody_continuations[0] = y->custody_continuations[0];
        break;
    default:
        CHECK(false);
    }
    CHECK(!nl_h_custody_validate(a.entry) && emit(a.entry, false, NULL, 0));
    CHECK(!nl_checked_call_body(a.entry, a.entry->root));
    if (restore) {
        x->arms = arms;
        x->arm_count = ac;
        if (ac)
            x->arms[0] = saved;
        x->bodies = bodies;
        x->body_calls = calls;
        x->body_count = bc;
        if (bc)
            x->bodies[0] = body;
        if (fault == 14)
            x->custody_continuations[0] = cont;
        CHECK(nl_h_custody_validate(a.entry) &&
              emit(a.entry, true, baseline, bytes));
    }
    watching = true;
    node_checked_destroy(&a);
    watching = false;
    for (size_t i = 0; i < watch_count; ++i)
        CHECK(watched[i].frees == (i < a_count ? 1u : 0u));
    CHECK(emit(b.entry, true, baseline, bytes));
    watching = true;
    node_checked_destroy(&b);
    watching = false;
    for (size_t i = 0; i < watch_count; ++i)
        CHECK(watched[i].frees == 1);
    printf("P965 parent=%zu kind=%zu fault=%u restore=%d A-originals=%zu "
           "B-live exactly-once PASS\n",
           parent, kind, fault, restore, a_count);
    return true;
}
static bool probe(const char *path)
{
    NLSemanticContext *base = NULL;
    TestNode n = {0};
    Tree t = {0};
    CHECK(registry(path, &base) && checked(base, &n));
    char *baseline = NULL;
    size_t bytes = 0;
    if (node_backend) {
        CHECK(nl_checked_c_node(n.entry, &baseline, &bytes) == NL_NODE_C_OK &&
              bytes);
    } else {
        CHECK(emit(n.entry, true, NULL, 0));
    }
    watch_count = 0;
    CHECK(collect(n.entry, &t));
    size_t arms[512] = {0}, bodies[512] = {0};
    bool continuation[512] = {false};
    for (size_t i = 0; i < t.count; ++i) {
        arms[i] = t.f[i]->arm_count;
        bodies[i] = t.f[i]->body_count;
        continuation[i] = t.f[i]->custody_continuations[0] != NULL;
    }
    size_t count = t.count, order[512] = {0}, ordered = 0;
    for (size_t i = 0; i < count; ++i)
        if (arms[i] &&
            t.f[i]->nodes[t.f[i]->arms[0].match - 1].kind == NL_CHECKED_IF)
            order[ordered++] = i;
    for (size_t i = 0; i < count; ++i)
        if (!arms[i] ||
            t.f[i]->nodes[t.f[i]->arms[0].match - 1].kind != NL_CHECKED_IF)
            order[ordered++] = i;
    CHECK(ordered == count);
    node_checked_destroy(&n);
    for (size_t index = 0; index < count; ++index)
        for (unsigned fault = 0; fault < 15; ++fault) {
            size_t i = order[index];
            if ((fault < 8 && (!arms[i] || (fault == 2 && arms[i] == 1))) ||
                (fault >= 8 && fault < 14 && !bodies[i]) ||
                (fault == 14 && !continuation[i]))
                continue;
            /* A shared body-call buffer's contents are equal; its foreign base
             * is still an ownership violation and must be rejected before
             * selection. */
            CHECK(trial(base, i, fault, true, baseline, bytes));
            CHECK(trial(base, i, fault, false, baseline, bytes));
        }
    free(baseline);
    nl_semantic_destroy(base);
    printf("P965 source=%s canonical fragments=%zu; completed all eligible "
           "whole-entry/count/buffer cases\n",
           path, count);
    return true;
}
int main(int argc, char **argv)
{
    if (argc != 3)
        return 1;
    node_backend = strcmp(argv[2], "node") == 0;
    return probe(argv[1]) ? 0 : 1;
}
