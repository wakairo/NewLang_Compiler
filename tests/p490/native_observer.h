/* Passive reads only. Role macros come from public checked bindings, values
 * from pre-execution frozen.json. Never retain a branch-local old address. */
#include <stdio.h>
#include <stdlib.h>
static const nl_node *tail, *head, *matched;
static const nl_node *const *tail_pointer;
static const nl_node_option *old_none, *observed, *joined_tail, *old_some;
static size_t step;
static void require(int condition)
{
    if (!condition) {
        fputs("P490 passive topology assertion failed\n", stderr);
        abort();
    }
}
static void option(const nl_node_option *o, unsigned tag)
{
    require(o && o->tag == tag && o->ptr == (tag ? tail : NULL));
}
static void observed_bind(size_t frame, size_t symbol, size_t kind,
                          const void *p)
{
    require(p != NULL);
    if (step == 0) {
        require(frame == 0 && symbol == P490_TAIL && kind == 1);
        tail = p;
        require(tail->f1 == 2);
        option(&tail->f0, 0);
        printf("tail=%p payload=%u\n", (const void *)tail, (unsigned)tail->f1);
    } else if (step == 1) {
        require(frame == 0 && symbol == P490_PTR && kind == 2);
        tail_pointer = p;
        require(*tail_pointer == tail);
    } else if (step == 2) {
        require(frame == 0 && symbol == P490_HEAD && kind == 1);
        head = p;
        require(head != tail && head->f1 == 1);
        option(&head->f0, 0);
        printf("head=%p payload=%u distinct=1\n", (const void *)head,
               (unsigned)head->f1);
    } else if (step == 4) {
        require(frame == 0 && symbol == P490_OLD_NONE && kind == 3);
        old_none = p;
        require(old_none != &head->f0 && old_none != &tail->f0);
        option(old_none, 0);
    } else if (step == 5) {
        require(frame == 0 && symbol == P490_OBSERVED && kind == 3);
        observed = p;
        require(observed != &head->f0 && observed != old_none);
        option(observed, 1);
    } else if (step == 8) {
        require(frame != 0 && symbol == P490_OLD_TAIL && kind == 3);
        const nl_node_option *old = p;
        require(old != &tail->f0 && old != observed);
        option(old,
               0); /* Checked here only: no out-of-scope address retained. */
        printf("old_tail=None new_tail=Some(%p) self_link=1\n",
               (const void *)tail->f0.ptr);
    } else if (step == 9) {
        require(frame == 0 && symbol == P490_JOINED && kind == 3);
        joined_tail = p;
        require(joined_tail != &tail->f0 && joined_tail != &head->f0 &&
                joined_tail != observed && joined_tail != old_none);
        option(joined_tail, 1);
        option(&tail->f0, 1);
        require(joined_tail->ptr == matched && matched == tail);
        printf("post_join_read=Some(%p) original_tail=%p current=1\n",
               (const void *)joined_tail->ptr, (const void *)tail);
    } else if (step == 11) {
        require(frame == 0 && symbol == P490_OLD_SOME && kind == 3);
        old_some = p;
        require(old_some != &head->f0 && old_some != observed);
        option(old_some, 1);
    } else
        require(0);
    ++step;
}
static void observed_replace(const nl_node *root, const nl_node_option *old)
{
    require(old != &root->f0);
    if (step == 3) {
        require(root == head);
        option(old, 0);
        option(&head->f0, 1);
    } else if (step == 7) {
        require(root == tail && matched == tail && tail->f0.ptr == matched);
        option(old, 0);
        option(&tail->f0, 1);
        option(&head->f0, 1);
        option(observed, 1);
    } else if (step == 10) {
        require(root == head);
        option(old, 1);
        option(&head->f0, 0);
        option(&tail->f0, 1);
    } else
        require(0);
    require(tail->f1 == 2 && head->f1 == 1);
    ++step;
}
static void observed_arm(size_t variant, const nl_node *q)
{
    require(step++ == 6 && variant == 2 && q == tail && q == *tail_pointer);
    matched = q;
    printf("Some(q)=%p same_original_tail=1\n", (const void *)q);
}
static void observed_finish(void)
{
    require(step == 12 && head != tail && matched == tail);
    require(head->f1 == 1 && tail->f1 == 2 && *tail_pointer == tail);
    option(&head->f0, 0);
    option(&tail->f0, 1);
    option(old_none, 0);
    option(observed, 1);
    option(joined_tail, 1);
    option(old_some, 1);
    puts("native topology observations complete");
}
#define NL_NODE_BIND(f, s, k, p) observed_bind(f, s, k, p)
#define NL_NODE_REPLACE(r, o) observed_replace(r, o)
#define NL_NODE_ARM(v, p) observed_arm(v, p)
#define NL_NODE_FINISH() observed_finish()
