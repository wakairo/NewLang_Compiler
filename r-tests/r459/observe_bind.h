/* R461 independently written passive C17 observer; single emitted unit.
 * Only read guest original carriers at source-issued symbol-identified hooks.
 * No source/ABI authority, no mutation, no host-derived topology oracle.
 */
#ifndef R459_OBSERVE_BIND_H
#define R459_OBSERVE_BIND_H
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef R459_ANCHOR_SYMBOL
#error "R459_ANCHOR_SYMBOL must come from source-issued public checked view"
#endif
#ifndef R459_PEER_SYMBOL
#error "R459_PEER_SYMBOL must come from source-issued public checked view"
#endif
#ifndef R459_SNAPSHOT_SYMBOL
#error "R459_SNAPSHOT_SYMBOL must come from source-issued public checked view"
#endif
#ifndef R459_EXPECT_SOME
#error "R459_EXPECT_SOME must come from frozen independent R source"
#endif
#if R459_ANCHOR_SYMBOL == R459_PEER_SYMBOL || R459_ANCHOR_SYMBOL == R459_SNAPSHOT_SYMBOL || R459_PEER_SYMBOL == R459_SNAPSHOT_SYMBOL
#error "Lexical root and snapshot IDs must be distinct"
#endif
static const nl_node *r459_anchor;
static const nl_node *r459_peer;
static nl_node_option r459_copied_option;
static unsigned r459_anchor_events, r459_peer_events, r459_copy_events, r459_unexpected;
static void r459_node_bind(unsigned long frame, unsigned long sym, unsigned long kind,
                           const nl_node *p) {
    if (frame != 0 || kind != 1) {
        if (sym == R459_ANCHOR_SYMBOL || sym == R459_PEER_SYMBOL)
            ++r459_unexpected;
        return;
    }
    if (sym == R459_ANCHOR_SYMBOL) {
        ++r459_anchor_events;
        r459_anchor = p;
        printf("R459_OBSERVE original=anchor frame=%lu symbol=%lu physical=%p tag=%u ptr=%p payload=%u\n",
               frame, sym, (const void *)p, p->f0.tag, (const void *)p->f0.ptr, (unsigned)p->f1);
    } else if (sym == R459_PEER_SYMBOL) {
        ++r459_peer_events;
        r459_peer = p;
        printf("R459_OBSERVE original=peer frame=%lu symbol=%lu physical=%p tag=%u ptr=%p payload=%u\n",
               frame, sym, (const void *)p, p->f0.tag, (const void *)p->f0.ptr, (unsigned)p->f1);
    }
}
static void r459_option_bind(unsigned long frame, unsigned long sym, unsigned long kind,
                             const nl_node_option *p) {
    if (sym != R459_SNAPSHOT_SYMBOL) return;
    if (frame != 0 || kind != 3) { ++r459_unexpected; return; }
    ++r459_copy_events;
    r459_copied_option = *p;
    printf("R459_OBSERVE copy=snapshot frame=%lu symbol=%lu tag=%u ptr=%p\n",
           frame, sym, p->tag, (const void *)p->ptr);
}
static void r459_ignore(unsigned long frame, unsigned long sym, unsigned long kind,
                        const void *p) {
    (void)frame; (void)kind; (void)p;
    if (sym == R459_ANCHOR_SYMBOL || sym == R459_PEER_SYMBOL ||
        sym == R459_SNAPSHOT_SYMBOL) ++r459_unexpected;
}
static void r459_finish(void) {
    unsigned good = r459_anchor_events == 1 && r459_peer_events == 1 &&
                    r459_copy_events == 1 && r459_unexpected == 0 &&
                    r459_anchor && r459_peer && r459_anchor != r459_peer;
    if (good) {
        good = r459_anchor->f1 == 37 && r459_peer->f1 == 211 &&
               r459_anchor->f0.tag == 0 && r459_anchor->f0.ptr == NULL;
    }
    if (good && R459_EXPECT_SOME) {
        good = r459_peer->f0.tag == 1 &&
               r459_peer->f0.ptr == r459_anchor &&
               r459_copied_option.tag == 1 &&
               r459_copied_option.ptr == r459_anchor;
    } else if (good) {
        good = r459_peer->f0.tag == 0 && r459_peer->f0.ptr == NULL &&
               r459_copied_option.tag == 0 && r459_copied_option.ptr == NULL;
    }
    printf("R459_OBSERVER_FINAL status=%s expected_some=%d roots=%u,%u copy=%u unexpected=%u anchor=%p peer=%p anchor_tag=%u peer_tag=%u peer_ptr=%p anchor_payload=%u peer_payload=%u copy_tag=%u copy_ptr=%p\n",
           good ? "PASS" : "FAIL", R459_EXPECT_SOME,
           r459_anchor_events, r459_peer_events, r459_copy_events, r459_unexpected,
           (const void *)r459_anchor, (const void *)r459_peer,
           r459_anchor ? r459_anchor->f0.tag : 999u,
           r459_peer ? r459_peer->f0.tag : 999u,
           (const void *)(r459_peer ? r459_peer->f0.ptr : NULL),
           r459_anchor ? (unsigned)r459_anchor->f1 : 999u,
           r459_peer ? (unsigned)r459_peer->f1 : 999u,
           r459_copied_option.tag, (const void *)r459_copied_option.ptr);
    fflush(stdout);
    if (!good) exit(43);
}
#define NL_NODE_BIND(frame, symbol, kind, addr) \
    _Generic((addr), \
        nl_node *: r459_node_bind, \
        const nl_node *: r459_node_bind, \
        nl_node_option *: r459_option_bind, \
        const nl_node_option *: r459_option_bind, \
        default: r459_ignore)((frame), (symbol), (kind), (addr))
#define NL_NODE_REPLACE(...) ((void)0)
#define NL_NODE_ARM(...) ((void)0)
#define NL_NODE_RELOAN(...) ((void)0)
#define NL_NODE_FINISH() r459_finish()
#endif
