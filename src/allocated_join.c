#include "semantic_internal.h"
#include <stdlib.h>

/* A bounded proof target for two unit-result lifecycle paths. No branch
 * context enters here. Construction does not authorize any source operation;
 * each independently checked arm must subsequently prove this entire prefix.
 */
NLCheckStatus nl_allocated_closed_prefix(const NLSemanticContext *before,
                                         NLSemanticContext **out,
                                         NLCheckedNodeView *certificate)
{
    if (nl_sem_validate(before) != NL_CHECK_OK || before->region_count != 1 ||
        before->domain_count != 1 || !before->regions[0].view.live ||
        !before->domains[0].live)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLPlaceId root = 0;
    for (size_t i = 0; i < before->place_count; ++i) {
        const NLSemanticPlaceView p = before->places[i];
        if (p.live && p.placement.region != 0) {
            if (root != 0 || p.placement.region != 1 ||
                p.governing_domain != 1 || !p.independent_root ||
                !nl_recursive_local_type(before, p.type) ||
                p.placement.start != 0 ||
                p.placement.length != before->regions[0].view.size)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            root = i + 1;
        }
    }
    if (root == 0)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLSymbolId allocation = 0, domain = 0;
    for (size_t i = 0; i < before->binding_count; ++i) {
        const NLSemanticBindingView b = before->bindings[i].view;
        if (b.availability != NL_AVAILABLE)
            continue;
        const NLSemanticValueView v = before->values[b.value - 1];
        const NLSemanticTypeView t = before->types[b.type - 1].view;
        if (t.kind == NL_TYPE_ALLOCATION && v.allocation_region == 1) {
            if (allocation != 0)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            allocation = i + 1;
        } else if (b.type == nl_semantic_domain_type(before) && v.domain == 1) {
            if (domain != 0)
                return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
            domain = i + 1;
        } else if (!t.is_copy)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    if (allocation == 0 || domain == 0)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = 0; i < before->value_count; ++i)
        if (before->values[i].dependencies != NL_DEPENDENCY_FREE ||
            before->values[i].value_dependency_count != 0)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    for (size_t i = 0; i < before->scope_count; ++i)
        if (before->scopes[i].active)
            return NL_CHECK_ANALYSIS_PRECISION_LIMIT;

    NLSemanticContext *post = NULL;
    NLCheckStatus s = nl_sem_clone(before, &post);
    if (s != NL_CHECK_OK)
        return s;
    const NLSemanticPlaceView p = post->places[root - 1];
    nl_sum_detach(post, root);
    nl_fixed_detach(post, root);
    nl_sem_end_value(post, p.current_value);
    post->places[root - 1].live = false;
    post->places[root - 1].current_value = 0;
    post->places[root - 1].current_fact = 0;
    post->places[root - 1].governing_domain = 0;
    post->places[root - 1].placement = (NLBackingRange){0};
    const NLSymbolId owners[] = {allocation, domain};
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticBindingView *b = &post->bindings[owners[i] - 1].view;
        b->availability = NL_CONSUMED;
        NLSemanticPlaceView *local = &post->places[b->place - 1];
        local->live = false;
        local->current_value = 0;
        local->current_fact = 0;
        local->governing_domain = 0;
        nl_sem_end_value(post, b->value);
    }
    post->domains[0].live = false;
    post->regions[0].view.live = false;
    post->raw_interval_count -= post->regions[0].count;
    free(post->regions[0].intervals);
    post->regions[0].intervals = NULL;
    post->regions[0].count = 0;
    if (nl_sem_validate(post) != NL_CHECK_OK) {
        nl_semantic_destroy(post);
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    }
    *certificate = (NLCheckedNodeView){.captured_frame_closed = true,
                                       .captured_backing = 1,
                                       .captured_root = root,
                                       .captured_incarnation = p.incarnation,
                                       .captured_domain = 1,
                                       .captured_allocation = allocation,
                                       .captured_domain_binding = domain};
    *out = post;
    return NL_CHECK_OK;
}
