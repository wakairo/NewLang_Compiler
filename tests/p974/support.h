#ifndef P974_SUPPORT_H
#define P974_SUPPORT_H
#include "../../src/copy_body_internal.h"
#include "../p965/shared.h"

typedef struct {
    NLCheckedFragment *caller, *body;
    NLCheckedNodeId call;
    NLCopyBodyView view;
    NLCopyBodyParameters parameters;
    NLCopyUnitView unit;
} P974Observation;
static bool p974_find(NLCheckedFragment *f, bool unit, P974Observation *out)
{
    for (size_t i = 0; i < f->body_count; ++i) {
        NLCheckedNodeId call = f->body_calls[i];
        NLCopyBodyParameters p;
        NLCopyUnitView u;
        if (nl_checked_copy_unit_view(f, call, &u) == unit &&
            nl_checked_copy_body_parameters(f, call, &p)) {
            *out = (P974Observation){.caller = f,
                                     .body = f->bodies[i],
                                     .call = call,
                                     .parameters = p};
            CHECK(nl_checked_copy_body_view(f, call, &out->view));
            if (unit)
                out->unit = u;
            return true;
        }
        if (p974_find(f->bodies[i], unit, out))
            return true;
    }
    for (size_t i = 0; i < f->arm_count; ++i)
        if (p974_find(f->arms[i].artifact, unit, out))
            return true;
    return false;
}
static bool p974_get(const P974Observation *o, bool unit)
{
    NLCopyBodyView v;
    NLCopyBodyParameters p;
    CHECK(nl_checked_copy_body_view(o->caller, o->call, &v));
    CHECK(nl_checked_copy_body_parameters(o->caller, o->call, &p));
    CHECK(v.body == o->body && v.function == o->view.function &&
          v.root == o->view.root && v.live_exit == o->view.live_exit &&
          p.entry == o->parameters.entry && p.handoff == o->parameters.handoff);
    if (unit) {
        NLCopyUnitView u;
        CHECK(nl_checked_copy_unit_view(o->caller, o->call, &u));
        CHECK(u.body == o->unit.body && u.function == o->unit.function &&
              u.normal_end == o->unit.normal_end &&
              u.postcall == o->unit.postcall);
    }
    return true;
}
static bool p974_validate(const P974Observation *o, bool unit, bool accept)
{
    NLCheckStatus expected =
        accept ? NL_CHECK_OK : NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    CHECK((unit ? nl_checked_copy_unit_validate(o->caller, o->call, &o->unit)
                : nl_checked_copy_body_validate(o->caller, o->call,
                                                &o->view)) == expected);
    CHECK(nl_checked_copy_body_parameters_validate(o->caller, o->call,
                                                   &o->parameters) == expected);
    if (unit)
        CHECK(nl_copy_unit_entry_validate(o->body) == expected);
    else if (o->view.event_count) {
        NLCopyBodyEventView event;
        CHECK(nl_checked_copy_body_event(o->caller, o->call, 0, &event));
        CHECK(nl_checked_copy_body_event_validate(o->caller, o->call, 0,
                                                  &event) == expected);
    }
    return true;
}
#endif
