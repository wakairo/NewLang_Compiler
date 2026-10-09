#include "semantic_internal.h"

/* Legacy one-captured-root adapter. Public source admission remains bounded
 * to two trials; target derivation uses the same owned multi-original engine.
 * The scalar checked fields keep existing two-H consumers unchanged.
 */
NLCheckStatus nl_allocated_closed_prefix(const NLSemanticContext *before,
                                         NLSemanticContext **out,
                                         NLCheckedNodeView *certificate)
{
    if (before == NULL || out == NULL || *out != NULL || certificate == NULL)
        return NL_CHECK_INTERNAL_ERROR;
    if (before->region_count != 1)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLCapturedClosure *owned = NULL;
    NLCheckStatus s = nl_captured_closure_create(before, &owned);
    if (s != NL_CHECK_OK)
        return s;
    const NLCapturedOriginal o = owned->view.originals[0];
    *certificate =
        (NLCheckedNodeView){.captured_frame_closed = true,
                            .captured_backing = o.extent.region,
                            .captured_root = o.root,
                            .captured_incarnation = o.incarnation,
                            .captured_domain = o.domain,
                            .captured_allocation = o.allocation_binding,
                            .captured_domain_binding = o.domain_binding};
    *out = (NLSemanticContext *)owned->view.closed_post;
    owned->view.closed_post = NULL; /* transfer target ownership only */
    owned->closed_owned = NULL;
    nl_captured_closure_destroy(owned);
    return NL_CHECK_OK;
}
