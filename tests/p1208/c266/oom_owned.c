/* Test-only passive inspection of the exact compiled checker, like P1193's
 * owned census. No production object, API, issuer or allocation is changed. */
#include "semantic_check.c"
#include <assert.h>

size_t p1208_allocation_ordinal(const void *, size_t *);

void p1208_census_owned(const NLCheckedFragment *root,
                        const NLCheckedFragment *body)
{
    assert(root && body && nl_h_custody_validate(root));
    assert(nl_checked_call_body(root, nl_checked_root(root)) == body);
    size_t found = 0;
    for (NLLinearBreakProof *b = body->linear_breaks; b; b = b->next) {
        if (!b->current_cycle)
            continue;
        NLLinearBreakView view;
        assert(nl_linear_break_validate(body, b->loop, &view) == NL_CHECK_OK);
        const NLCurrentLoopProof *s = b->current_cycle;
        assert(s->sealed && s->returns && s->return_count == 3);
        size_t bytes = 0;
        const size_t array = p1208_allocation_ordinal(s->returns, &bytes);
        assert(bytes == s->return_count * sizeof(*s->returns));
        const size_t graph = p1208_allocation_ordinal(s->graphs[0].nodes, NULL);
        const size_t signature =
            p1208_allocation_ordinal(s->signature_entries, &bytes);
        assert(s->signature_entry_count &&
               bytes ==
                   s->signature_entry_count * sizeof(*s->signature_entries));
        assert(array + 20 < graph && graph < signature);
        for (size_t i = 0; i < s->return_count; ++i) {
            const StutterWorld *w = &s->returns[i].world;
            assert(w->copy && w->copy != w->world);
            const size_t clone = p1208_allocation_ordinal(w->copy, NULL);
            assert(array < clone && clone < graph);
            const void *parts[] = {w->anchor.places, w->anchor.values,
                                   w->anchor.scopes, w->anchor.bindings};
            for (size_t j = 0; j < 4; ++j)
                if (parts[j]) {
                    const size_t anchor =
                        p1208_allocation_ordinal(parts[j], NULL);
                    assert(clone < anchor && anchor < graph);
                }
        }
        printf("C266 OWNED return_array=%zu graph_start=%zu signature=%zu "
               "returns=%zu\n",
               array, graph, signature, s->return_count);
        ++found;
    }
    assert(found == 1);
}
