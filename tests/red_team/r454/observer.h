/* R454 independent, read-only positive C17 runtime observer.
 * No source semantic state or proof authority is changed. */
#ifndef R454_OBSERVER_H
#define R454_OBSERVER_H
#include <stdio.h>
#include <stdlib.h>
#ifndef R454_CASE_SOME
#error R454_CASE_SOME must be supplied by independent runner
#endif
static const nl_node *r454_anchor = NULL, *r454_peer = NULL;
static unsigned r454_roots = 0, r454_replacements = 0, r454_arms = 0;
static unsigned r454_old_none = 0, r454_old_some = 0;
static void r454_fail(const char *reason) {
    fprintf(stderr, "R454_OBSERVER_REFUSED: %s\n", reason);
    abort();
}
static void r454_bind(unsigned kind, const void *address) {
    if (kind == 1U) {
        const nl_node *p = (const nl_node *)address;
        if (p->f0.tag != 0U || p->f0.ptr != NULL)
            r454_fail("one lexical root not constructed None");
        if ((unsigned)p->f1 == (R454_CASE_SOME ? 47U : 31U)) {
            if (r454_anchor) r454_fail("duplicate anchor");
            r454_anchor = p;
        } else if ((unsigned)p->f1 == (R454_CASE_SOME ? 83U : 79U)) {
            if (r454_peer) r454_fail("duplicate peer");
            r454_peer = p;
        } else r454_fail("unexpected root/sibling value");
        ++r454_roots;
        fprintf(stderr, "R454_ROOT kind=%u physical=%p sibling=%u init_tag=%u\n",
                kind, (const void *)p, (unsigned)p->f1, p->f0.tag);
        return;
    }
    if (kind == 3U && r454_replacements != 0) {
        const nl_node_option *copy = (const nl_node_option *)address;
        if (r454_anchor && address == (const void *)&r454_anchor->f0)
            r454_fail("old Copy aliases mutable root field");
        if (copy->tag == 0U && copy->ptr == NULL) {
            ++r454_old_none;
            fprintf(stderr,"R454_OLD_COPY None storage=%p\n",address);
        }
        if (R454_CASE_SOME && r454_replacements == 2U &&
            copy->tag == 1U && copy->ptr == r454_peer) {
            ++r454_old_some;
            fprintf(stderr,"R454_OLD_COPY Some storage=%p ptr=%p\n",
                    address,(const void *)copy->ptr);
        }
    }
}
static void r454_replace(const nl_node *changed, const nl_node_option *old) {
    if (r454_roots != 2U || !r454_anchor || !r454_peer ||
        r454_anchor == r454_peer || changed != r454_anchor)
        r454_fail("replace was not on original anchor");
    if ((const void *)old == (const void *)&changed->f0)
        r454_fail("old Copy aliases overwritten field");
    if (r454_replacements == 0U) {
        if (old->tag != 0U || old->ptr != NULL ||
            changed->f0.tag != 1U || changed->f0.ptr != r454_peer)
            r454_fail("first None->Some or first old Copy incorrect");
        if (!R454_CASE_SOME && r454_arms != 1U)
            r454_fail("None-arm replace occurred outside selected arm");
        if (R454_CASE_SOME && r454_arms != 0U)
            r454_fail("pre-match Some establishment too late");
    } else if (R454_CASE_SOME && r454_replacements == 1U) {
        if (r454_arms != 1U || old->tag != 1U || old->ptr != r454_peer ||
            changed->f0.tag != 0U || changed->f0.ptr != NULL)
            r454_fail("selected Some->None or old pointer incorrect");
    } else r454_fail("unexpected replacement count");
    ++r454_replacements;
    fprintf(stderr,"R454_REPLACE seq=%u original=%p old_storage=%p old_tag=%u old_ptr=%p new_tag=%u new_ptr=%p\n",
            r454_replacements,(const void*)changed,(const void*)old,
            old->tag,(const void *)old->ptr,changed->f0.tag,(const void*)changed->f0.ptr);
}
static void r454_arm(unsigned variant, const nl_node *found) {
    if (!r454_anchor || !r454_peer || r454_arms != 0U)
        r454_fail("not one selected native arm");
    if (R454_CASE_SOME) {
        if (variant != 2U || found != r454_peer ||
            r454_anchor->f0.tag != 1U ||
            r454_anchor->f0.ptr != r454_peer || r454_replacements != 1U)
            r454_fail("Some tag/binder/physical carrier mismatch");
    } else {
        if (variant != 1U || found != NULL ||
            r454_anchor->f0.tag != 0U ||
            r454_anchor->f0.ptr != NULL || r454_replacements != 0U)
            r454_fail("None arm selected from incorrect physical tag");
    }
    ++r454_arms;
    fprintf(stderr,"R454_SELECTED_ARM variant=%u found=%p anchor=%p peer=%p\n",
            variant,(const void *)found,(const void *)r454_anchor,(const void *)r454_peer);
}
static void r454_finish(void) {
    if (r454_roots != 2U || !r454_peer || !r454_anchor ||
        r454_peer == r454_anchor || r454_arms != 1U ||
        r454_replacements != (R454_CASE_SOME ? 2U : 1U) ||
        r454_old_none == 0U || (R454_CASE_SOME && r454_old_some == 0U))
        r454_fail("missing physical roots/selected arm/replaces/old Copy");
    if (r454_anchor->f1 != (R454_CASE_SOME ? 47U : 31U) ||
        r454_peer->f1 != (R454_CASE_SOME ? 83U : 79U) ||
        r454_peer->f0.tag != 0U || r454_peer->f0.ptr != NULL)
        r454_fail("sibling or untouched peer was changed");
    if (r454_anchor->f0.tag != (R454_CASE_SOME ? 0U : 1U) ||
        r454_anchor->f0.ptr != (R454_CASE_SOME ? NULL : r454_peer))
        r454_fail("final original anchor tag/pointer mismatch");
    fprintf(stderr,"R454_NATIVE_OBSERVER_OK case=%s original_anchor=%p original_peer=%p sibling=%u,%u tag=%u ptr=%p replacements=%u arms=%u old_none=%u old_some=%u\n",
            R454_CASE_SOME ? "some" : "none",
            (const void*)r454_anchor,(const void*)r454_peer,
            (unsigned)r454_anchor->f1,(unsigned)r454_peer->f1,
            r454_anchor->f0.tag,(const void*)r454_anchor->f0.ptr,
            r454_replacements,r454_arms,r454_old_none,r454_old_some);
}
#define NL_NODE_BIND(F,S,K,P) r454_bind((K),(const void *)(P))
#define NL_NODE_REPLACE(P,O) r454_replace((P),(O))
#define NL_NODE_ARM(V,P) r454_arm((V),(P))
#define NL_NODE_RELOAN(P) ((void)(P))
#define NL_NODE_FINISH() r454_finish()
#endif
