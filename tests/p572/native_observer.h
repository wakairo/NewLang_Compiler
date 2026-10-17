#include <stdio.h>
static const nl_node *p572_roots[3];
static const nl_node_option *p572_fields[3];
static unsigned p572_bindings, p572_writes;
static unsigned p572_seen[32];
static void p572_require(int ok)
{
    if (!ok) {
        fputs("p572 physical assertion failed\n", stderr);
        abort();
    }
}
static size_t p572_role(size_t symbol)
{
    const size_t ids[] = {
        P572_middle,     P572_left,        P572_right,           P572_p_middle,
        P572_p_left,     P572_p_right,     P572_old_left,        P572_old_right,
        P572_final_left, P572_final_right, P572_untouched_middle};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
        if (ids[i] == symbol)
            return i;
    return 99;
}
static void p572_option(const nl_node_option *p, unsigned tag,
                        const nl_node *target)
{
    p572_require(p->tag == tag && p->ptr == target);
}
static void p572_payloads(void)
{
    const unsigned expected[] = {22, 11, 33};
    for (size_t i = 0; i < 3; ++i)
        p572_require(p572_roots[i] && p572_roots[i]->f1 == expected[i] &&
                     &p572_roots[i]->f0 == p572_fields[i]);
}
static void p572_bind(size_t frame, size_t symbol, size_t kind,
                      const void *address)
{
    size_t role = p572_role(symbol);
    p572_require(!frame && role < 11 && address && !p572_seen[role]++);
    ++p572_bindings;
    if (role < 3) {
        p572_require(kind == 1);
        const nl_node *p = address;
        for (size_t j = 0; j < role; ++j) {
            p572_require(p572_roots[j] != p);
            const uintptr_t a = (uintptr_t)(const void *)p572_roots[j],
                            b = (uintptr_t)(const void *)p;
            p572_require(a + sizeof(*p) <= b || b + sizeof(*p) <= a);
        }
        p572_roots[role] = p;
        p572_fields[role] = &p->f0;
        p572_require(p->f1 == (role == 0 ? 22u : role == 1 ? 11u : 33u));
        p572_option(&p->f0, role == 1 ? 1u : 0u,
                    role == 1 ? p572_roots[0] : NULL);
        printf("ROOT role=%zu address=%p next_address=%p payload=%u\n", role,
               (const void *)p, (const void *)&p->f0, (unsigned)p->f1);
    } else if (role < 6) {
        p572_require(kind == 2 &&
                     *(const nl_node *const *)address == p572_roots[role - 3]);
    } else {
        p572_require(kind == 3);
        unsigned tag = role == 6 || role == 8 || role == 9;
        const nl_node *target = role == 6   ? p572_roots[0]
                                : role == 8 ? p572_roots[2]
                                : role == 9 ? p572_roots[1]
                                            : NULL;
        p572_option(address, tag, target);
        printf("COPY role=%zu address=%p target=%p tag=%u\n", role, address,
               (const void *)target, tag);
    }
}
static void p572_replace(const nl_node *root, const nl_node_option *old)
{
    p572_require(p572_writes < 2);
    size_t step = p572_writes++;
    p572_require(root == p572_roots[step + 1] && old != &root->f0);
    p572_option(old, step ? 0u : 1u, step ? NULL : p572_roots[0]);
    p572_option(&root->f0, 1, p572_roots[step ? 1 : 2]);
    p572_payloads();
    printf("CHANGE step=%zu original=%p old=%u/%p new=%u/%p\n", step,
           (const void *)root, old->tag, (const void *)old->ptr, root->f0.tag,
           (const void *)root->f0.ptr);
}
static void p572_finish(void)
{
    p572_require(p572_writes == 2 && p572_bindings == 11);
    p572_payloads();
    p572_option(&p572_roots[0]->f0, 0, NULL);
    p572_option(&p572_roots[1]->f0, 1, p572_roots[2]);
    p572_option(&p572_roots[2]->f0, 1, p572_roots[1]);
    puts("p572 physical originals/oldCopy/reciprocal links PASS");
}
#define NL_NODE_BIND(frame, symbol, kind, address)                             \
    p572_bind(frame, symbol, kind, address)
#define NL_NODE_REPLACE(root, old) p572_replace(root, old)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() p572_finish()
