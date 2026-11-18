/* Reuse the existing P-owned real-source loader and OOM wrappers only. */
#define main retained_p1463_main
#include "../p1463/probe.c"
#undef main

typedef struct {
    NLCheckedFragment *f;
    size_t id;
} Site;
static Site nodes1517[512];
static size_t count1517;
static NLCheckedFragment *receiver1517;
static size_t receiver_call1517;
static NLCopyBodyEventView last_copy1517;

static bool collect1517(NLCheckedFragment *f, size_t depth)
{
    CHECK(f && depth < 16);
    for (size_t id = 1; id <= f->count; ++id) {
        NLCheckedNodeView *n = &f->nodes[id - 1];
        CHECK(count1517 < 512);
        nodes1517[count1517++] = (Site){f, id};
        if (n->owner_call.entry_proved) {
            CHECK(!receiver1517);
            receiver1517 = f;
            receiver_call1517 = id;
        }
        NLCopyBodyParameters p;
        if (nl_checked_copy_body_parameters(f, id, &p)) {
            CHECK(nl_checked_copy_body_parameters_validate(f, id, &p) ==
                  NL_CHECK_OK);
            CHECK(nl_checked_copy_body_event(f, id, 0, &last_copy1517));
        }
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        if (body)
            CHECK(collect1517((NLCheckedFragment *)body, depth + 1));
        if (n->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < n->item_count; ++a)
                CHECK(collect1517(
                    (NLCheckedFragment *)nl_checked_match_arm(f, id, a),
                    depth + 1));
    }
    return true;
}
static bool prepare1517(const char *path, Program *p)
{
    count1517 = 0;
    receiver1517 = NULL;
    receiver_call1517 = 0;
    memset(&last_copy1517, 0, sizeof last_copy1517);
    CHECK(ready(path, p) && actual(p) && collect1517(p->artifact, 0));
    CHECK(receiver1517 && receiver_call1517 && last_copy1517.before);
    return true;
}
static bool attack1517(const char *path)
{
    Program p = {0}, foreign = {0};
    CHECK(prepare1517(path, &p) && ready(path, &foreign) && actual(&foreign));
    size_t checked = 0;
    NLCheckedNodeView *call = &receiver1517->nodes[receiver_call1517 - 1];
    NLCheckedNodeView saved = *call;
    for (size_t k = 0; k < 9; ++k) {
        switch (k) {
        case 0:
            call->owner_call.inputs[1] = call->owner_call.inputs[2];
            break;
        case 1:
            call->owner_call.parameters[1] = call->owner_call.donor[1];
            break;
        case 2:
            call->owner_call.donor[2] = call->owner_call.parameters[2];
            break;
        case 3:
            call->owner_call.root = 0;
            break;
        case 4:
            call->owner_call.incarnation = 0;
            break;
        case 5:
            call->owner_call.domain = 0;
            break;
        case 6:
            call->owner_call.range.region = 0;
            break;
        case 7:
            call->owner_call.definition.step_count = 0;
            break;
        case 8:
            call->owner_call.post_proved = false;
            break;
        }
        bool bad = refused(&p, "receiver actual/formal/original/domain/post");
        *call = saved;
        CHECK(bad);
        ++checked;
    }
    const NLSemanticContext *world = receiver1517->context;
    receiver1517->context = foreign.context;
    bool bad = refused(&p, "receiver foreign world");
    receiver1517->context = (NLSemanticContext *)world;
    CHECK(bad);
    ++checked;
    const NLSource *source = receiver1517->source;
    receiver1517->source = foreign.source;
    bad = refused(&p, "receiver foreign source");
    receiver1517->source = source;
    CHECK(bad);
    ++checked;
    for (size_t b = 0; b < receiver1517->body_count; ++b) {
        if (receiver1517->body_calls[b] != receiver_call1517)
            continue;
        NLCheckedFragment *body = receiver1517->bodies[b];
        receiver1517->bodies[b] = (NLCheckedFragment *)nl_checked_call_body(
            foreign.artifact, nl_checked_root(foreign.artifact));
        bad = refused(&p, "receiver unrelated callee body");
        receiver1517->bodies[b] = body;
        CHECK(bad);
        ++checked;
    }
    for (size_t k = 0; k < count1517; ++k) {
        Site site = nodes1517[k];
        NLCheckedNodeView *n = &site.f->nodes[site.id - 1], old = *n;
        bool damage = true;
        switch (n->kind) {
        case NL_CHECKED_DESTROY:
        case NL_CHECKED_ERASE_SLOT:
        case NL_CHECKED_DOMAIN_FINALIZE:
        case NL_CHECKED_DEALLOCATE:
            n->first_argument = 0;
            break;
        case NL_CHECKED_LOAN_HEADER:
            n->loan.body_nonescape_proved = false;
            break;
        case NL_CHECKED_REGISTERED_CALL:
            if (n->result_count)
                n->results[0].value = 0;
            else
                damage = false;
            break;
        default:
            damage = false;
        }
        if (!damage)
            continue;
        bad = refused(&p, "callee primitive/loan/result substitution");
        *n = old;
        CHECK(bad);
        ++checked;
    }
    struct NLHOriginOwner *origin = p.artifact->h_origin;
    p.artifact->h_origin = foreign.artifact->h_origin;
    bad = refused(&p, "same numeric IDs foreign authentic origin");
    p.artifact->h_origin = origin;
    CHECK(bad && emit(&p, NL_NODE_C_OK));
    printf("P1517 artifact negatives=%zu plus foreign origin; pristine retry "
           "PASS\n",
           checked);
    destroy(&foreign);
    destroy(&p);
    return true;
}
static bool negative1517(const char *path)
{
    Program p = {0};
    CHECK(load(path, &p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    NLFunctionUnitDiagnostic d = {0};
    NLCheckStatus s = define(&p, &d);
    printf("P1517 source negative=%s status=%d code=%s\n", path, (int)s,
           d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                        : "none");
    CHECK((s == NL_CHECK_SEMANTIC_ERROR ||
           s == NL_CHECK_ANALYSIS_PRECISION_LIMIT) &&
          !p.artifact && nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof before));
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
static bool oom1517(const char *path, unsigned point)
{
    Program p = {0};
    if (point == 0)
        CHECK(load(path, &p));
    else
        CHECK(ready(path, &p));
    if (point >= 3)
        CHECK(actual(&p));
    NLCopyCallAnchor anchor = {0};
    NLSemanticSnapshot before, after;
    CHECK(nl_copy_state_anchor(p.context, &anchor) == NL_CHECK_OK &&
          nl_semantic_snapshot(p.context, &before));
    hits = text_allocs = 0;
    if (point <= 2) {
        clone_target = p.context;
        fault = point == 2 ? 2 : 1;
        NLFunctionUnitDiagnostic d = {0};
        NLCheckStatus s = point == 0
                              ? define(&p, &d)
                              : nl_semantic_check_expression(p.context, p.call,
                                                             &p.artifact, NULL);
        fault = 0;
        clone_target = NULL;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && hits == 1 && !p.artifact);
    } else {
        if (point <= 4)
            fault = point;
        else {
            count1517 = 0;
            receiver1517 = NULL;
            CHECK(collect1517(p.artifact, 0));
            clone_target = last_copy1517.before;
            fault = 2;
        }
        bool rejected = emit(&p, NL_NODE_C_OUT_OF_MEMORY);
        fault = 0;
        clone_target = NULL;
        CHECK(rejected && hits == 1);
    }
    CHECK(nl_copy_state_matches(p.context, &anchor) &&
          nl_semantic_snapshot(p.context, &after) &&
          !memcmp(&before, &after, sizeof before));
    if (point == 0) {
        NLFunctionUnitDiagnostic d = {0};
        CHECK(define(&p, &d) == NL_CHECK_OK);
    }
    if (point <= 2)
        CHECK(actual(&p));
    CHECK(emit(&p, NL_NODE_C_OK));
    printf("P1517 host OOM site=%u fault+retry=2 hit=1 NULL/length/state "
           "rollback PASS\n",
           point);
    nl_copy_state_destroy(&anchor);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 3 && !strcmp(argv[1], "attack"))
        return attack1517(argv[2]) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "oom"))
        return oom1517(argv[2], (unsigned)strtoul(argv[3], NULL, 10)) ? 0 : 1;
    if (argc >= 3 && !strcmp(argv[1], "negative")) {
        for (int i = 2; i < argc; ++i)
            if (!negative1517(argv[i]))
                return 1;
        return 0;
    }
    return 2;
}
