#include "../shared.h"
#include "newlang/copy_body.h"

typedef struct {
    NLCheckedFragment *parent, *child;
    NLCheckedNodeId call;
    bool committed, detached;
} Transfer;
static Transfer transfers[128];
static size_t transfer_count, pending_checks, detached_checks, trace_errors;
static bool tracing;
NLCheckStatus __real_nl_h_custody_reserve(NLCheckedFragment *,
                                          NLCheckedFragment *, NLCustodyRole,
                                          NLCheckedNodeId, size_t);
void __real_nl_h_custody_commit(NLCheckedFragment *, NLCheckedFragment *,
                                NLCustodyRole, size_t);
void __real_nl_checked_destroy(NLCheckedFragment *);
NLCheckStatus __wrap_nl_h_custody_reserve(NLCheckedFragment *parent,
                                          NLCheckedFragment *child,
                                          NLCustodyRole role,
                                          NLCheckedNodeId call, size_t slot)
{
    NLCheckStatus s =
        __real_nl_h_custody_reserve(parent, child, role, call, slot);
    if (tracing && role == NL_CUSTODY_BODY && s == NL_CHECK_OK) {
        if (transfer_count == sizeof(transfers) / sizeof(*transfers))
            ++trace_errors;
        else {
            transfers[transfer_count++] =
                (Transfer){parent, child, call, false, false};
            if (nl_h_custody_observe_body(parent, call) != NULL)
                ++trace_errors;
            ++pending_checks;
        }
    }
    return s;
}
void __wrap_nl_h_custody_commit(NLCheckedFragment *parent,
                                NLCheckedFragment *child, NLCustodyRole role,
                                size_t slot)
{
    __real_nl_h_custody_commit(parent, child, role, slot);
    if (tracing && role == NL_CUSTODY_BODY)
        for (size_t i = 0; i < transfer_count; ++i)
            if (transfers[i].parent == parent && transfers[i].child == child &&
                !transfers[i].committed) {
                transfers[i].committed = true;
                if (nl_h_custody_observe_body(parent, transfers[i].call) !=
                    child)
                    ++trace_errors;
            }
}
void __wrap_nl_checked_destroy(NLCheckedFragment *child)
{
    if (tracing && child)
        for (size_t i = 0; i < transfer_count; ++i)
            if (transfers[i].child == child && transfers[i].committed &&
                !transfers[i].detached) {
                if (nl_h_custody_observe_body(transfers[i].parent,
                                              transfers[i].call) != NULL)
                    ++trace_errors;
                transfers[i].detached = true;
                ++detached_checks;
            }
    __real_nl_checked_destroy(child);
}
typedef struct {
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call;
    size_t slot;
    NLCopyBodyView view;
    NLCopyBodyParameters parameters;
    NLCopyBodyEventView event;
    NLCopyUnitView unit;
    bool has_event;
} Observation;
static bool find(NLCheckedFragment *f, bool unit, Observation *out)
{
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedNodeId call = f->body_calls[i];
        NLCopyBodyParameters p;
        NLCopyUnitView u;
        bool is_unit = nl_checked_copy_unit_view(f, call, &u);
        if (is_unit == unit && nl_checked_copy_body_parameters(f, call, &p)) {
            *out = (Observation){.caller = f,
                                 .body = f->bodies[i],
                                 .call = call,
                                 .slot = i,
                                 .parameters = p};
            if (!nl_checked_copy_body_view(f, call, &out->view))
                return false;
            if (unit)
                out->unit = u;
            out->has_event = out->view.event_count != 0;
            return !out->has_event ||
                   nl_checked_copy_body_event(f, call, 0, &out->event);
        }
        if (find(f->bodies[i], unit, out))
            return true;
    }
    for (size_t i = 0; i < f->arm_count; ++i)
        if (find(f->arms[i].artifact, unit, out))
            return true;
    return false;
}
static bool gets(const Observation *o, bool unit)
{
    NLCopyBodyView v;
    NLCopyBodyParameters p;
    CHECK(nl_h_custody_observe_body(o->caller, o->call) == o->body);
    CHECK(nl_checked_copy_body_view(o->caller, o->call, &v));
    CHECK(nl_checked_copy_body_parameters(o->caller, o->call, &p));
    CHECK(v.body == o->view.body && v.root == o->view.root &&
          v.function == o->view.function &&
          v.event_count == o->view.event_count &&
          v.live_exit == o->view.live_exit &&
          v.result.type == o->view.result.type &&
          v.result.value == o->view.result.value);
    CHECK(p.entry == o->parameters.entry &&
          p.handoff == o->parameters.handoff && p.loan == o->parameters.loan &&
          p.arguments[0] == o->parameters.arguments[0] &&
          p.arguments[1] == o->parameters.arguments[1]);
    if (o->has_event) {
        NLCopyBodyEventView e;
        CHECK(nl_checked_copy_body_event(o->caller, o->call, 0, &e));
        CHECK(e.node == o->event.node && e.kind == o->event.kind &&
              e.before == o->event.before && e.after == o->event.after &&
              e.result == o->event.result && e.incoming == o->event.incoming);
    }
    if (unit) {
        NLCopyUnitView u;
        CHECK(nl_checked_copy_unit_view(o->caller, o->call, &u));
        CHECK(u.body == o->unit.body && u.conditional == o->unit.conditional &&
              u.normal_end == o->unit.normal_end &&
              u.postcall == o->unit.postcall && u.function == o->unit.function);
    }
    return true;
}
static bool validates(const Observation *o, bool unit, bool accept)
{
    NLCheckStatus a =
        unit ? nl_checked_copy_unit_validate(o->caller, o->call, &o->unit)
             : nl_checked_copy_body_validate(o->caller, o->call, &o->view);
    NLCheckStatus b = nl_checked_copy_body_parameters_validate(
        o->caller, o->call, &o->parameters);
    CHECK(a == (accept ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT));
    CHECK(b == (accept ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT));
    if (!unit && o->has_event)
        CHECK(nl_checked_copy_body_event_validate(o->caller, o->call, 0,
                                                  &o->event) ==
              (accept ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT));
    return true;
}
static bool probe(const char *path, bool unit)
{
    NLSemanticContext *base = NULL, *other = NULL;
    TestNode a = {0}, b = {0};
    CHECK(registry(path, &base) && registry(path, &other));
    tracing = true;
    CHECK(checked(base, &a));
    tracing = false;
    CHECK(checked(other, &b));
    Observation o, foreign;
    CHECK(find(a.entry, unit, &o) && find(b.entry, unit, &foreign));
    CHECK(o.caller != foreign.caller &&
          o.caller->context != foreign.caller->context &&
          o.caller->source != foreign.caller->source && o.caller->count > 1);
    CHECK(gets(&o, unit) && validates(&o, unit, true));
    NLCheckedFragment shadow = *o.caller;
    CHECK(!nl_h_custody_observe_body(&shadow, o.call));
    CHECK(!nl_h_custody_observe_body(o.caller, 0));
    NLCopyBodyView sentinel = {.function = SIZE_MAX};
    CHECK(!nl_checked_copy_body_view(&shadow, o.call, &sentinel) &&
          sentinel.function == SIZE_MAX);
    NLCheckedFragment saved = *o.caller, saved_body = *o.body;
    NLCheckedNodeId old_call = saved.body_calls[o.slot];
    NLCheckedFragment *old_body = saved.bodies[o.slot];
    NLCheckedNodeView *clone = malloc(saved.count * sizeof(*clone));
    CHECK(clone);
    memcpy(clone, saved.nodes, saved.count * sizeof(*clone));
    for (unsigned fault = 0; fault < 12; ++fault) {
        switch (fault) {
        case 0:
            o.caller->context = foreign.caller->context;
            break;
        case 1:
            o.caller->source = foreign.caller->source;
            break;
        case 2:
            o.caller->nodes = clone;
            break;
        case 3:
            o.caller->root = 0;
            break;
        case 4:
            --o.caller->count;
            break;
        case 5:
            o.caller->body_calls[o.slot] = 0;
            break;
        case 6:
            o.caller->body_count = 0;
            break;
        case 7:
            o.caller->body_count = SIZE_MAX;
            break;
        case 8:
            o.caller->bodies[o.slot] = foreign.body;
            break;
        case 9:
            o.caller->bodies = foreign.caller->bodies;
            break;
        case 10:
            o.body->source = foreign.body->source;
            break;
        case 11:
            o.body->context = foreign.body->context;
            break;
        }
        CHECK(gets(&o, unit) && validates(&o, unit, false));
        *o.caller = saved;
        saved.body_calls[o.slot] = old_call;
        saved.bodies[o.slot] = old_body;
        *o.body = saved_body;
        CHECK(gets(&o, unit) && validates(&o, unit, true));
        printf("C89 observer %s fault=%u historical GET=true current "
               "VALIDATE=INVALID "
               "restore=OK\n",
               unit ? "unit" : "u8", fault);
    }
    free(clone);
    tracing = true;
    node_checked_destroy(&a);
    tracing = false;
    CHECK(transfer_count && pending_checks == transfer_count &&
          detached_checks == transfer_count && !trace_errors);
    CHECK(gets(&foreign, unit) && validates(&foreign, unit, true));
    node_checked_destroy(&b);
    nl_semantic_destroy(base);
    nl_semantic_destroy(other);
    printf("C89 observer actual BODY transfers=%zu pending GET=NULL; committed "
           "GET=original; detached GET=NULL; B healthy\n",
           transfer_count);
    return true;
}
int main(int argc, char **argv)
{
    return argc == 3 && probe(argv[1], strcmp(argv[2], "unit") == 0) ? 0 : 1;
}
