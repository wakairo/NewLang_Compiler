#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"
#include "newlang/checked_c_node.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static bool public_source;
static NLCheckedNodeId match(const NLCheckedFragment *f);
static size_t backend_poison_count;
static bool rejected_certificate(const NLCheckedFragment *top,
                                 const NLCheckedFragment *entry)
{
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    if (public_source) {
        char *output = NULL;
        size_t length = 777;
        CHECK(nl_checked_c_node(entry, &output, &length) ==
              NL_NODE_C_UNSUPPORTED);
        CHECK(output == NULL && length == 777);
        ++backend_poison_count;
    }
    return true;
}
/* Actual production registration + actual known main() check, no probe flag.
 * On either public transaction failure the caller snapshot must be unchanged.
 */
static NLCheckStatus public_unit(const NLSyntaxTree *unit,
                                 NLCheckedFragment **out,
                                 NLCheckDiagnostic *diagnostic)
{
    NLSemanticContext *context = NULL;
    NLSource *entry_source = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *entry_syntax = NULL;
    NLCheckStatus s = nl_semantic_create(&context);
    if (s != NL_CHECK_OK)
        goto done;
    NLSemanticSnapshot before, after;
    if (!nl_semantic_snapshot(context, &before)) {
        s = NL_CHECK_INTERNAL_ERROR;
        goto done;
    }
    const NLSyntaxTree *units[] = {unit};
    NLFunctionUnitDiagnostic d = {0};
    s = nl_semantic_register_function_unit(context, units, 1, &d);
    if (s != NL_CHECK_OK) {
        if (diagnostic != NULL)
            *diagnostic = d.diagnostic;
        if (!nl_semantic_snapshot(context, &after) ||
            memcmp(&before, &after, sizeof(before)) != 0)
            s = NL_CHECK_INTERNAL_ERROR;
        goto done;
    }
    if (nl_source_create("main()", 6, "actual-main", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_syntax,
                                            NULL) != NL_PARSE_OK) {
        s = NL_CHECK_OUT_OF_MEMORY;
        goto done;
    }
    if (!nl_semantic_snapshot(context, &before)) {
        s = NL_CHECK_INTERNAL_ERROR;
        goto done;
    }
    s = nl_semantic_check_expression(context, entry_syntax, out, diagnostic);
    if (s == NL_CHECK_OK) {
        (*out)->destroy_context = nl_semantic_destroy;
        (*out)->source = NULL;
        context = NULL;
    } else if (!nl_semantic_snapshot(context, &after) ||
               memcmp(&before, &after, sizeof(before)) != 0)
        s = NL_CHECK_INTERNAL_ERROR;
done:
    nl_syntax_tree_destroy(entry_syntax);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    return s;
}
static NLCheckStatus unit_check(const NLSyntaxTree *unit,
                                NLCheckedFragment **out,
                                NLCheckDiagnostic *diagnostic)
{
    return public_source ? public_unit(unit, out, diagnostic)
                         : nl_captured_closure_probe(unit, out, diagnostic);
}
static NLCheckStatus load(const char *path, NLCheckedFragment **out,
                          NLCheckDiagnostic *diagnostic)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLCheckStatus s = NL_CHECK_OUT_OF_MEMORY;
    if (nl_source_load(path, &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK)
        goto done;
    NLParseDiagnostic parsed = {0};
    if (nl_parser_parse_function_unit(parser, &unit, &parsed) != NL_PARSE_OK) {
        *diagnostic = (NLCheckDiagnostic){parsed.diagnostic, parsed.span};
        s = NL_CHECK_SEMANTIC_UNSUPPORTED;
        goto done;
    }
    s = unit_check(unit, out, diagnostic);
done:
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return s;
}
static const NLCheckedFragment *body(const NLCheckedFragment *entry)
{
    return nl_checked_call_body(entry, nl_checked_root(entry));
}
static NLCheckedNodeId match(const NLCheckedFragment *f)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i)
        if (nl_checked_node_view(f, i)->kind == NL_CHECKED_MATCH)
            return i;
    return 0;
}
static bool full_topology(const NLCheckedFragment *entry)
{
    const NLCheckedFragment *parent = body(entry);
    for (size_t i = 0; i < 4; ++i)
        parent = nl_checked_match_arm(parent, match(parent), 1);
    NLCapturedClosureView owners;
    CHECK(nl_checked_captured_closure_view(parent, match(parent), &owners) &&
          owners.count == 4);
    const NLCheckedFragment *leaf =
        nl_checked_match_arm(parent, match(parent), 1);
    CHECK(nl_checked_captured_change_count(leaf) == 6);
    NLCapturedChangeView last;
    CHECK(nl_checked_captured_change_view(leaf, 5, &last));
    const NLSemanticContext *checkpoint = last.after;
    NLPlaceId roots[5];
    for (size_t i = 0; i < 4; ++i)
        roots[i] = owners.originals[i].root;
    roots[4] = 0;
    for (size_t i = 1; i <= nl_checked_node_count(leaf); ++i)
        if (nl_checked_node_view(leaf, i)->kind == NL_CHECKED_INITIALIZE)
            roots[4] = nl_checked_node_view(leaf, i)->lifetime_place;
    CHECK(roots[4] != 0 && checkpoint->region_count == 5 &&
          checkpoint->domain_count == 5);
    for (size_t i = 0; i < 5; ++i) {
        const NLSemanticPlaceView r = checkpoint->places[roots[i] - 1];
        CHECK(r.live && r.independent_root && r.fixed_field_count == 4 &&
              checkpoint->regions[r.placement.region - 1].view.live &&
              checkpoint->domains[r.governing_domain - 1].live);
        for (size_t j = 0; j < i; ++j) {
            const NLSemanticPlaceView old = checkpoint->places[roots[j] - 1];
            CHECK(r.placement.region != old.placement.region &&
                  r.incarnation != old.incarnation &&
                  r.governing_domain != old.governing_domain);
        }
        for (size_t j = 0; j < 3; ++j) {
            const NLSemanticPlaceView child =
                checkpoint->places[r.fixed_fields[j] - 1];
            CHECK(child.live && !child.independent_root &&
                  child.parent_aggregate == roots[i] &&
                  child.parent_incarnation == r.incarnation &&
                  child.parent_field_index == j &&
                  child.governing_domain == r.governing_domain &&
                  child.placement.region == 0);
            for (size_t k = 0; k < j; ++k)
                CHECK(r.fixed_fields[j] != r.fixed_fields[k] &&
                      !nl_fixed_overlap(checkpoint, r.fixed_fields[j],
                                        r.fixed_fields[k]));
        }
    }
    const size_t facts[][3] = {{0, 2, 1}, {1, 1, 3}, {1, 0, 2},
                               {2, 1, 1}, {2, 0, 3}, {3, 1, 2}};
    for (size_t i = 0; i < 6; ++i) {
        const NLSemanticPlaceView r =
            checkpoint->places[roots[facts[i][0]] - 1];
        const NLSemanticPlaceView field =
            checkpoint->places[r.fixed_fields[facts[i][1]] - 1];
        const NLSemanticValueView option =
            checkpoint->values[field.current_value - 1];
        CHECK(option.variant == 2 && field.payload_occurrence != 0);
        const NLSemanticValueView pointer =
            checkpoint->values[option.sum_payload - 1];
        CHECK(pointer.reference.place == roots[facts[i][2]] &&
              pointer.reference.incarnation ==
                  checkpoint->places[roots[facts[i][2]] - 1].incarnation &&
              pointer.reference.provenance == NL_PROVENANCE_VALID &&
              pointer.reference.scope == 0);
        NLCapturedChangeView change;
        CHECK(nl_checked_captured_change_view(leaf, i, &change));
        const NLCheckedField e =
            nl_checked_node_view(leaf, change.operation)->field;
        CHECK(e.parent == roots[facts[i][0]] && e.index == facts[i][1] &&
              e.post_payload_occurrence != 0 &&
              e.parent_fact != e.parent_post_fact &&
              e.child_fact != e.child_post_fact);
        const NLSemanticPlaceView before = change.before->places[e.parent - 1];
        const NLSemanticPlaceView after = change.after->places[e.parent - 1];
        CHECK(before.incarnation == after.incarnation);
        for (size_t j = 0; j < 3; ++j) {
            CHECK(before.fixed_fields[j] == after.fixed_fields[j]);
            if (j == e.index)
                continue;
            const NLSemanticPlaceView a =
                change.before->places[before.fixed_fields[j] - 1];
            const NLSemanticPlaceView b =
                change.after->places[after.fixed_fields[j] - 1];
            CHECK(a.current_value == b.current_value &&
                  a.current_fact == b.current_fact &&
                  a.payload_occurrence == b.payload_occurrence &&
                  a.incarnation == b.incarnation);
        }
    }
    const NLValueId dst_child =
        checkpoint->places[checkpoint->places[roots[4] - 1].fixed_fields[2] - 1]
            .current_value;
    CHECK(checkpoint->values[dst_child - 1].variant == 1 &&
          checkpoint->values[dst_child - 1].sum_payload == 0);
    puts("public full source: five original live R/O/D; six changes; seven "
         "actual field facts; sibling preservation");
    return true;
}
static bool evidence(const char *path, size_t sites)
{
    NLCheckedFragment *entry = NULL;
    NLCheckDiagnostic d = {0};
    NLCheckStatus s = load(path, &entry, &d);
    if (s != NL_CHECK_OK) {
        fprintf(stderr, "status=%d code=%s message=%s\n", s, d.diagnostic.code,
                d.diagnostic.message);
        return false;
    }
    const NLCheckedFragment *f = body(entry);
    NLCapturedClosureView view;
    CHECK(f != NULL && nl_checked_captured_closure_view(f, match(f), &view));
    CHECK(nl_checked_captured_closure_validate(f, match(f)) == NL_CHECK_OK);
    CHECK(view.count == 0);
    CHECK(nl_checked_context(entry)->region_count == 0 &&
          nl_checked_context(entry)->domain_count == 0);
    for (size_t i = 0; i <= NL_CAPTURED_MAX_RELEASES; ++i)
        CHECK(view.release_worlds[i] == (i <= sites));
    if (public_source)
        CHECK(full_topology(entry));
    printf("owned certificate: sites=%zu, release worlds=0..%zu\n", sites,
           sites);
    nl_checked_destroy(entry);
    return true;
}
static NLCheckedFragment *at_count(NLCheckedFragment *entry, size_t count)
{
    NLCheckedFragment *f = (NLCheckedFragment *)body(entry);
    for (size_t i = 0; f != NULL && i <= NL_CAPTURED_MAX_ORIGINALS; ++i) {
        NLCapturedClosureView v;
        if (!nl_checked_captured_closure_view(f, match(f), &v))
            return NULL;
        if (v.count == count)
            return f;
        f = (NLCheckedFragment *)nl_checked_match_arm(f, match(f), 1);
    }
    return NULL;
}
static NLCheckedNodeView *operation(NLCheckedFragment *f, NLCheckedKind kind)
{
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == kind)
            return &f->nodes[i];
    return NULL;
}
static bool poison(const char *path)
{
    NLCheckedFragment *entry = NULL;
    NLCheckDiagnostic d = {0};
    CHECK(load(path, &entry, &d) == NL_CHECK_OK);
    NLCheckedFragment *top = (NLCheckedFragment *)body(entry);
    NLCheckedFragment *f = at_count(entry, 2);
    CHECK(f != NULL);
    NLCapturedClosure *c = f->captured_closure;
    const NLCapturedClosureView good = c->view;
    for (size_t attack = 0; attack < 14; ++attack) {
        switch (attack) {
        case 0:
            --c->view.count;
            break;
        case 1:
            ++c->view.count;
            break;
        case 2:
            c->view.originals[1] = c->view.originals[0];
            break;
        case 3:
            c->view.originals[0] = good.originals[1];
            c->view.originals[1] = good.originals[0];
            break;
        case 4:
            ++c->view.originals[0].incarnation;
            break;
        case 5:
            ++c->view.originals[0].extent.start;
            break;
        case 6:
            --c->view.originals[0].extent.length;
            break;
        case 7:
            c->view.originals[0].extent.region =
                good.originals[1].extent.region;
            break;
        case 8:
            c->view.originals[0].domain = good.originals[1].domain;
            break;
        case 9:
            c->view.originals[0].allocation_binding =
                good.originals[1].allocation_binding;
            break;
        case 10:
            c->view.originals[0].domain_value = good.originals[1].domain_value;
            break;
        case 11:
            c->view.originals[0].origin = c->branches[1].world;
            break;
        case 12:
            c->view.originals[0].allocation_availability = NL_CONSUMED;
            break;
        case 13:
            ++c->view.release_worlds[2];
            break;
        }
        CHECK(rejected_certificate(top, entry));
        c->view = good;
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
    }
    /* Same numeric pre-grant IDs/contents, distinct nominal owned forks.
     * Even a valid clone cannot replace the recorded exact arm or fork. */
    CHECK(nl_packet_same_entry(c->branches[0].entry, c->branches[1].entry));
    NLSemanticContext *saved_entry = c->branches[0].entry;
    c->branches[0].entry = c->branches[1].entry;
    CHECK(rejected_certificate(top, entry));
    c->branches[0].entry = saved_entry;
    NLSemanticContext *foreign_ancestor = NULL;
    CHECK(nl_sem_clone(c->view.ancestor, &foreign_ancestor) == NL_CHECK_OK);
    c->view.ancestor = foreign_ancestor;
    for (size_t r = 0; r < c->view.count; ++r)
        c->view.originals[r].origin = foreign_ancestor;
    CHECK(rejected_certificate(top, entry));
    c->view = good;
    nl_semantic_destroy(foreign_ancestor);
    NLCheckedFragment *none = (NLCheckedFragment *)c->branches[0].arm;
    NLSemanticContext *foreign = NULL;
    CHECK(nl_sem_clone(none->context, &foreign) == NL_CHECK_OK);
    const NLSemanticContext *saved_world = none->context;
    none->context = foreign;
    CHECK(rejected_certificate(top, entry));
    none->context = saved_world;
    nl_semantic_destroy(foreign);
    c->view.closed_post = c->branches[1].world;
    CHECK(rejected_certificate(top, entry));
    c->view = good;
    NLSemanticContext *post = (NLSemanticContext *)c->view.closed_post;
    post->regions[0].view.live = true;
    CHECK(rejected_certificate(top, entry));
    post->regions[0].view.live = false;
    NLSemanticContext *world = (NLSemanticContext *)none->context;
    world->scopes[0].active = true;
    CHECK(rejected_certificate(top, entry));
    world->scopes[0].active = false;
    world->places[good.originals[0].root - 1].live = true;
    CHECK(rejected_certificate(top, entry));
    world->places[good.originals[0].root - 1].live = false;
    NLCheckedFragment *some = (NLCheckedFragment *)c->branches[1].arm;
    const NLSemanticContext *some_world = some->context;
    some->context = none->context;
    CHECK(rejected_certificate(top, entry));
    some->context = some_world;
    NLSemanticBindingView *owner =
        &world->bindings[good.originals[0].allocation_binding - 1].view;
    owner->availability = NL_AVAILABLE;
    CHECK(rejected_certificate(top, entry));
    owner->availability = NL_CONSUMED;
    NLValueId original = good.originals[0].allocation_value;
    world->values[original - 1].allocation_region =
        good.originals[1].extent.region;
    CHECK(rejected_certificate(top, entry));
    world->values[original - 1].allocation_region =
        good.originals[0].extent.region;
    const NLSemanticValueView old = world->values[world->value_count - 1];
    world->values[world->value_count - 1].dependencies =
        NL_DEPENDENCIES_UNKNOWN;
    CHECK(rejected_certificate(top, entry));
    world->values[world->value_count - 1] = old;
    world->values[world->value_count - 1].value_dependency_count = 1;
    CHECK(rejected_certificate(top, entry));
    world->values[world->value_count - 1] = old;
    const NLCheckedKind kinds[] = {NL_CHECKED_DESTROY, NL_CHECKED_ERASE_SLOT,
                                   NL_CHECKED_DOMAIN_FINALIZE,
                                   NL_CHECKED_DEALLOCATE};
    for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); ++i) {
        NLCheckedNodeView *v = operation(none, kinds[i]);
        CHECK(v != NULL);
        NLCheckedNodeView saved = *v;
        v->kind =
            NL_CHECKED_UNIT; /* final state still closed: trace must fail */
        CHECK(rejected_certificate(top, entry));
        *v = saved;
    }
    NLCheckedNodeView *end = operation(none, NL_CHECKED_DESTROY);
    NLCheckedNodeView *unit = operation(none, NL_CHECKED_UNIT);
    CHECK(end != NULL && unit != NULL);
    NLCheckedNodeView saved_unit = *unit;
    *unit = *end; /* duplicate EndRoot after actual EndRoot */
    CHECK(rejected_certificate(top, entry));
    *unit = saved_unit;
    NLCheckedNodeView *free_node = operation(none, NL_CHECKED_DEALLOCATE);
    NLCheckedNodeView *argument = &none->nodes[free_node->first_argument - 1];
    const NLValueUse use = argument->value_use;
    argument->value_use = NL_VALUE_COPIED;
    CHECK(rejected_certificate(top, entry));
    argument->value_use = use;
    NLCheckedNodeView *loan = operation(none, NL_CHECKED_LOAN_HEADER);
    CHECK(loan != NULL);
    loan->loan.body_nonescape_proved = false;
    CHECK(rejected_certificate(top, entry));
    loan->loan.body_nonescape_proved = true;
    NLCheckedNodeView *grant =
        &none->nodes[nl_checked_node_view(none, none->root)->initializer - 1];
    grant->allocation_success = true; /* None must mint no original */
    CHECK(rejected_certificate(top, entry));
    grant->allocation_success = false;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    if (public_source) {
        NLCheckedFragment *last_parent = at_count(entry, 4);
        NLCheckedFragment *leaf = (NLCheckedFragment *)nl_checked_match_arm(
            last_parent, match(last_parent), 1);
        CHECK(leaf->field_change_count == 6);
        NLCapturedChange *change = leaf->field_changes[1];
        NLCheckedNodeView *write = &leaf->nodes[change->node - 1];
        const NLCheckedNodeView saved = *write;
        for (size_t attack = 0; attack < 8; ++attack) {
            switch (attack) {
            case 0:
                write->field.index = 0;
                break;
            case 1:
                ++write->field.parent_incarnation;
                break;
            case 2:
                ++write->field.child_post_fact;
                break;
            case 3:
                write->field.post_payload_occurrence =
                    write->field.payload_occurrence;
                break;
            case 4:
                write->field.new_value = write->field.old_value;
                break;
            case 5:
                write->field.access = NL_ACCESS_READ;
                break;
            case 6:
                write->field.dependency_compatible = false;
                break;
            case 7:
                write->kind = NL_CHECKED_UNIT;
                break;
            }
            CHECK(rejected_certificate(top, entry));
            *write = saved;
        }
        NLCheckedNodeView *projection = &leaf->nodes[write->first_argument - 1];
        const NLCheckedNodeView original_projection = *projection;
        projection->field = leaf->nodes[leaf->field_changes[0]->node - 1].field;
        CHECK(rejected_certificate(top, entry));
        *projection = original_projection;
        projection->reference_result.writable = false;
        CHECK(rejected_certificate(top, entry));
        *projection = original_projection;
        NLSemanticContext *before_change = change->before;
        change->before = leaf->field_changes[0]
                             ->before; /* valid other epoch, same world IDs */
        CHECK(rejected_certificate(top, entry));
        change->before = before_change;
        change->before = NULL;
        CHECK(rejected_certificate(top, entry));
        change->before = before_change;
        NLSemanticContext *after_change = change->after;
        change->after = NULL;
        CHECK(rejected_certificate(top, entry));
        change->after = after_change;
        leaf->field_change_count = 7;
        CHECK(rejected_certificate(top, entry));
        leaf->field_change_count = 6;
        const NLPlaceId root = write->field.parent;
        const NLPlaceId sibling =
            before_change->places[root - 1].fixed_fields[0];
        before_change->places[root - 1].fixed_fields[1] = sibling;
        CHECK(rejected_certificate(top, entry));
        before_change->places[root - 1].fixed_fields[1] = write->field.child;
        const NLValueId ref_id =
            before_change->bindings[write->field.base - 1].view.value;
        before_change->values[ref_id - 1].reference.writable = false;
        CHECK(rejected_certificate(top, entry));
        before_change->values[ref_id - 1].reference.writable = true;
        NLCheckedNodeView *acquisition = NULL;
        for (size_t i = 0; i < change->node - 1; ++i)
            if (leaf->nodes[i].kind == NL_CHECKED_REF_FROM_PTR &&
                leaf->nodes[i].results[0].value == ref_id)
                acquisition = &leaf->nodes[i];
        CHECK(acquisition != NULL);
        NLCheckedNodeView *pointer_input =
            &leaf->nodes[acquisition->first_argument - 1];
        NLCheckedNodeView *stability_input =
            &leaf->nodes[pointer_input->next_argument - 1];
        NLCheckedNodeView *stability_loan = NULL;
        for (size_t i = 0; i < change->node - 1; ++i)
            if (leaf->nodes[i].kind == NL_CHECKED_LOAN_HEADER &&
                leaf->nodes[i].loan.ref_symbol == stability_input->symbol)
                stability_loan = &leaf->nodes[i];
        CHECK(stability_loan != NULL);
        const NLCheckedNodeView original_loan = *stability_loan;
        for (size_t attack = 0; attack < 4; ++attack) {
            switch (attack) {
            case 0:
                ++stability_loan->loan.domain;
                break;
            case 1:
                stability_loan->loan.source = write->field.base;
                break;
            case 2:
                stability_loan->loan.prevent_lifetime_end = false;
                break;
            case 3:
                stability_loan->loan.is_exclusive = true;
                break;
            }
            CHECK(rejected_certificate(top, entry));
            *stability_loan = original_loan;
        }
        ++stability_input->symbol;
        CHECK(rejected_certificate(top, entry));
        --stability_input->symbol;
        pointer_input->value_use = NL_VALUE_USE_NONE;
        CHECK(rejected_certificate(top, entry));
        pointer_input->value_use = NL_VALUE_COPIED;
        ++change->after->places[write->field.child - 1].current_fact;
        CHECK(rejected_certificate(top, entry));
        --change->after->places[write->field.child - 1].current_fact;
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
        CHECK(full_topology(entry));
    }
    nl_checked_destroy(entry);
    printf("tuple/order/world/post/trace/dependency/scope poison rejected; "
           "backend refusals=%zu\n",
           backend_poison_count);
    return true;
}

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
void *__real_calloc(size_t, size_t);
static bool injecting;
static size_t index_at, failure_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && index_at++ == failure_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && index_at++ == failure_at)
        return NULL;
    return __real_realloc(p, n);
}
void *__wrap_calloc(size_t n, size_t size)
{
    if (injecting && index_at++ == failure_at)
        return NULL;
    return __real_calloc(n, size);
}
static bool oom(const char *path)
{
    NLCheckedFragment *entry = NULL;
    NLCheckDiagnostic d = {0};
    CHECK(load(path, &entry, &d) == NL_CHECK_OK);
    NLCheckedFragment *top = (NLCheckedFragment *)body(entry);
    const NLSemanticContext *ancestor =
        at_count(entry, 4)->captured_closure->view.ancestor;
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(ancestor, &before));
    size_t created = 0, validated = 0;
    for (failure_at = 0; failure_at < 30000; ++failure_at) {
        NLCapturedClosure *c = NULL;
        index_at = 0;
        injecting = true;
        NLCheckStatus s = nl_captured_closure_create(ancestor, &c);
        injecting = false;
        if (s == NL_CHECK_OK) {
            created = failure_at;
            nl_captured_closure_destroy(c);
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && c == NULL);
        CHECK(nl_semantic_snapshot(ancestor, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        CHECK(nl_captured_closure_create(ancestor, &c) == NL_CHECK_OK);
        nl_captured_closure_destroy(c);
    }
    CHECK(created != 0);
    NLCapturedClosureView certificate;
    CHECK(nl_checked_captured_closure_view(top, match(top), &certificate));
    for (failure_at = 0; failure_at < 30000; ++failure_at) {
        index_at = 0;
        injecting = true;
        NLCheckStatus s = nl_checked_captured_closure_validate(top, match(top));
        injecting = false;
        if (s == NL_CHECK_OK) {
            validated = failure_at;
            break;
        }
        CHECK(s == NL_CHECK_OUT_OF_MEMORY);
        NLCapturedClosureView current;
        CHECK(nl_checked_captured_closure_view(top, match(top), &current));
        CHECK(memcmp(&certificate, &current, sizeof(current)) == 0);
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
    }
    CHECK(validated != 0);
    /* Negative proof validation must also fail closed under allocation loss. */
    NLCheckedFragment *bad = at_count(entry, 4);
    ++bad->captured_closure->view.originals[0].incarnation;
    failure_at = 0;
    index_at = 0;
    injecting = true;
    NLCheckStatus refused =
        nl_checked_captured_closure_validate(top, match(top));
    injecting = false;
    CHECK(refused == NL_CHECK_OUT_OF_MEMORY);
    CHECK(rejected_certificate(top, entry));
    --bad->captured_closure->view.originals[0].incarnation;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    nl_checked_destroy(entry);
    printf("exhaustive OOM: constructor=%zu validator=%zu; snapshots and "
           "retries preserved\n",
           created, validated);
    return true;
}
static bool checker_oom(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK);
    NLCheckedFragment *out = NULL;
    failure_at = SIZE_MAX;
    index_at = 0;
    injecting = true;
    NLCheckStatus s = unit_check(unit, &out, NULL);
    injecting = false;
    CHECK(s == NL_CHECK_OK && out != NULL);
    const size_t allocations = index_at;
    nl_checked_destroy(out);
    size_t attacked = 0;
    for (size_t i = 0; i < allocations; ++i) {
        if (i >= 64 && i + 64 < allocations && i % 128 != 0)
            continue;
        out = NULL;
        failure_at = i;
        index_at = 0;
        injecting = true;
        s = unit_check(unit, &out, NULL);
        injecting = false;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && out == NULL);
        ++attacked;
        CHECK(unit_check(unit, &out, NULL) == NL_CHECK_OK);
        CHECK(nl_checked_captured_closure_validate(
                  body(out), match(body(out))) == NL_CHECK_OK);
        nl_checked_destroy(out);
    }
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    printf("checker OOM: %zu distributed failures / %zu allocation "
           "opportunities; no publication and clean retry\n",
           attacked, allocations);
    return true;
}
static bool same_address_reset(const char *path)
{
    NLCheckedFragment *entry = NULL;
    NLCheckDiagnostic d = {0};
    CHECK(load(path, &entry, &d) == NL_CHECK_OK);
    const NLCheckedFragment *top = body(entry);
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    NLCheckedFragment *leaf = (NLCheckedFragment *)nl_checked_match_arm(
        at_count(entry, 4), match(at_count(entry, 4)), 1);
    const NLCheckedField previous =
        leaf->nodes[leaf->field_changes[1]->node - 1].field;
    NLCheckedNodeView *write = &leaf->nodes[leaf->field_changes[2]->node - 1];
    CHECK(write->field.child == previous.child &&
          write->field.payload_occurrence == previous.post_payload_occurrence &&
          write->field.post_payload_occurrence !=
              write->field.payload_occurrence);
    const NLSemanticContext *before = leaf->field_changes[2]->before;
    const NLSemanticValueView old = before->values[write->field.old_value - 1];
    const NLSemanticValueView next = before->values[write->field.new_value - 1];
    CHECK(old.variant == 2 && next.variant == 2 &&
          old.sum_payload != next.sum_payload &&
          before->values[old.sum_payload - 1].reference.place ==
              before->values[next.sum_payload - 1].reference.place);
    const NLOccurrenceId saved = write->field.payload_occurrence;
    write->field.payload_occurrence = 0;
    CHECK(rejected_certificate(top, entry));
    write->field.payload_occurrence = saved;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    nl_checked_destroy(entry);
    puts("same pointer target; distinct copied packages and fresh Some "
         "occurrence; stale evidence rejected");
    return true;
}

#ifdef NEWLANG_EXPERIMENTAL_NESTED_CALLER
/* Poison owned actual-source call evidence only AFTER ordinary public
 * registration/checking. These tests never seed a source grant. */
static bool nested_poison(const char *path)
{
    NLCheckedFragment *entry = NULL;
    NLCheckDiagnostic diagnostic = {0};
    CHECK(load(path, &entry, &diagnostic) == NL_CHECK_OK);
    NLCheckedFragment *top = (NLCheckedFragment *)body(entry);
    NLCheckedFragment *parent = at_count(entry, 4);
    NLCheckedFragment *leaf =
        (NLCheckedFragment *)nl_checked_match_arm(parent, match(parent), 1);
    CHECK(leaf && leaf->whole_call &&
          nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    const NLWholeValueCallView good = leaf->whole_value;
    NLWholeValueCallView untouched = good;
    CHECK(!nl_checked_whole_value_call_view(leaf, 0, &untouched) &&
          !memcmp(&good, &untouched, sizeof(good)));
    NLSemanticContext *equal = NULL;
    CHECK(nl_sem_clone(good.entry, &equal) == NL_CHECK_OK);
    leaf->whole_value.entry = equal;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    nl_semantic_destroy(equal);
    leaf->whole_value.returned = good.received;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.received = good.returned;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.inputs[1] = good.inputs[0];
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.donors[1] = good.donors[0];
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.parameters[1] = good.parameters[0];
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.result = 0;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.receiver = good.callee_result;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    NLSemanticContext *returned = (NLSemanticContext *)good.returned;
    NLSemanticContext *received = (NLSemanticContext *)good.received;
    const NLAvailability availability =
        returned->bindings[good.donors[0] - 1].view.availability;
    returned->bindings[good.donors[0] - 1].view.availability = NL_AVAILABLE;
    CHECK(rejected_certificate(top, entry));
    returned->bindings[good.donors[0] - 1].view.availability = availability;
    const NLSemanticValueView packet = received->values[good.result - 1];
    received->values[good.result - 1].fields[1] = packet.fields[0];
    CHECK(rejected_certificate(top, entry));
    received->values[good.result - 1] = packet;
    received->values[good.result - 1].carrier = NL_CARRIER_LOOSE;
    CHECK(rejected_certificate(top, entry));
    received->values[good.result - 1] = packet;
    NLSemanticBindingView *receiver =
        &received->bindings[good.receiver - 1].view;
    const NLPlaceId place = receiver->place;
    receiver->place = SIZE_MAX;
    CHECK(rejected_certificate(top, entry));
    receiver->place = place;
    NLSemanticBindingView *local =
        &returned->bindings[good.callee_result - 1].view;
    const NLPlaceId local_place = local->place;
    local->place = 0;
    CHECK(rejected_certificate(top, entry));
    local->place = local_place;
    NLCheckedFragment *callee =
        (NLCheckedFragment *)nl_checked_call_body(leaf, leaf->whole_call);
    CHECK(callee);
    NLCheckedNodeView *ret = operation(callee, NL_CHECKED_RETURN);
    NLCheckedNodeView *value = operation(callee, NL_CHECKED_IDENTIFIER);
    NLCheckedNodeView *aggregate = operation(callee, NL_CHECKED_AGGREGATE);
    CHECK(ret && value && aggregate);
    const NLCheckedNodeView saved_return = *ret, saved_value = *value,
                            saved_aggregate = *aggregate;
    ret->returned.value = 0;
    CHECK(rejected_certificate(top, entry));
    *ret = saved_return;
    ret->initializer = 0;
    CHECK(rejected_certificate(top, entry));
    *ret = saved_return;
    value->value_use = NL_VALUE_COPIED;
    CHECK(rejected_certificate(top, entry));
    *value = saved_value;
    value->results[0].value = good.result;
    CHECK(rejected_certificate(top, entry));
    *value = saved_value;
    aggregate->first_argument = 0;
    CHECK(rejected_certificate(top, entry));
    *aggregate = saved_aggregate;
    aggregate->argument_count = 1;
    CHECK(rejected_certificate(top, entry));
    *aggregate = saved_aggregate;
    const NLCheckedNodeId root = callee->root;
    callee->root = 0;
    CHECK(rejected_certificate(top, entry));
    callee->root = root;
    NLCheckedNodeView *block = &callee->nodes[root - 1];
    const NLCheckedNodeView saved_block = *block;
    block->first_item = 0;
    CHECK(rejected_certificate(top, entry));
    *block = saved_block;
    block->item_count = 0;
    CHECK(rejected_certificate(top, entry));
    *block = saved_block;
    /* A closed final world cannot replace the original callee release trace. */
    size_t terminals = 0;
    for (size_t i = 1; i <= leaf->count; ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(leaf, i);
        if (n->kind != NL_CHECKED_REGISTERED_CALL || i == leaf->whole_call)
            continue;
        NLCheckedFragment *terminal =
            (NLCheckedFragment *)nl_checked_call_body(leaf, i);
        CHECK(terminal);
        NLCheckedNodeView *deallocate =
            operation(terminal, NL_CHECKED_DEALLOCATE);
        NLCheckedNodeView *pattern =
            operation(terminal, NL_CHECKED_AGGREGATE_BINDING);
        CHECK(deallocate && pattern);
        const NLCheckedNodeView saved = *deallocate, saved_pattern = *pattern;
        deallocate->kind = NL_CHECKED_UNIT;
        CHECK(rejected_certificate(top, entry));
        *deallocate = saved;
        pattern->argument_count = 2;
        CHECK(rejected_certificate(top, entry));
        *pattern = saved_pattern;
        NLCheckedNodeView *field =
            &terminal->nodes[pattern->first_argument - 1];
        const NLCheckedNodeView saved_field = *field;
        field->field_index = 3;
        CHECK(rejected_certificate(top, entry));
        *field = saved_field;
        ++terminals;
    }
    CHECK(terminals == 2 &&
          nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    printf("actual owned whole/result/return/primitive call poison rejected: "
           "%zu; no backend artifact\n",
           backend_poison_count);
    nl_checked_destroy(entry);
    return true;
}
#endif
static int inspect(const char *path)
{
    NLCheckedFragment *out = NULL;
    NLCheckDiagnostic d = {0};
    NLCheckStatus s = load(path, &out, &d);
    if (s == NL_CHECK_OK) {
        const NLCheckedFragment *f = body(out);
        NLCapturedClosureView v;
        if (!nl_checked_captured_closure_view(f, match(f), &v) ||
            nl_checked_captured_closure_validate(f, match(f)) != NL_CHECK_OK) {
            nl_checked_destroy(out);
            return EXIT_FAILURE;
        }
        printf("certificate-substrate accepted; release worlds:");
        for (size_t i = 0; i <= NL_CAPTURED_MAX_RELEASES; ++i)
            printf(" %zu", v.release_worlds[i]);
        puts(public_source
                 ? "; public source semantic admission; no native claim"
                 : "; NOT public source admission or native evidence");
    } else {
        printf("status=%d category=%s code=%s\n", s,
               d.diagnostic.category == NULL ? "profile"
                                             : d.diagnostic.category,
               d.diagnostic.code == NULL ? "PROBE-PROFILE" : d.diagnostic.code);
    }
    nl_checked_destroy(out);
    return s == NL_CHECK_OK ? 0 : 3;
}

#ifdef NEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL
static bool transitive_poison(const char *path)
{
    NLCheckedFragment *entry = NULL;
    NLCheckDiagnostic diagnostic = {0};
    CHECK(load(path, &entry, &diagnostic) == NL_CHECK_OK);
    NLCheckedFragment *top = (NLCheckedFragment *)body(entry);
    NLCheckedFragment *parent = at_count(entry, 4);
    NLCheckedFragment *leaf =
        (NLCheckedFragment *)nl_checked_match_arm(parent, match(parent), 1);
    CHECK(leaf && leaf->two_call &&
          nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    const NLTwoRootCallView good = leaf->two_root;
    NLTwoRootCallView untouched = good;
    CHECK(!nl_checked_two_root_call_view(leaf, 0, &untouched) &&
          !memcmp(&good, &untouched, sizeof(good)));
    NLSemanticContext *equal = NULL;
    CHECK(nl_sem_clone(good.entry, &equal) == NL_CHECK_OK);
    leaf->two_root.entry = equal;
    CHECK(rejected_certificate(top, entry));
    leaf->two_root = good;
    nl_semantic_destroy(equal);
    CHECK(nl_sem_clone(good.returned, &equal) == NL_CHECK_OK);
    leaf->two_root.returned = equal;
    CHECK(rejected_certificate(top, entry));
    leaf->two_root = good;
    nl_semantic_destroy(equal);
    leaf->two_root.returned = good.entry;
    CHECK(rejected_certificate(top, entry));
    leaf->two_root = good;
    leaf->two_root.input = 0;
    CHECK(rejected_certificate(top, entry));
    leaf->two_root = good;
    leaf->two_root.donor = 0;
    CHECK(rejected_certificate(top, entry));
    leaf->two_root = good;
    leaf->two_root.parameter = good.donor;
    CHECK(rejected_certificate(top, entry));
    leaf->two_root = good;
    NLSemanticContext *before = (NLSemanticContext *)good.entry;
    NLSemanticContext *after = (NLSemanticContext *)good.returned;
    const NLSemanticValueView pair = before->values[good.input - 1];
    const NLValueId child = pair.fields[1];
    const NLSemanticValueView packet = before->values[child - 1];
    before->values[good.input - 1].fields[1] = pair.fields[0];
    CHECK(rejected_certificate(top, entry));
    before->values[good.input - 1] = pair;
    before->values[child - 1].fields[1] =
        leaf->whole_value.received->values[leaf->whole_value.result - 1]
            .fields[0];
    CHECK(rejected_certificate(top, entry));
    before->values[child - 1] = packet;
    NLSemanticValueView *allocation = &before->values[packet.fields[1] - 1];
    const NLSemanticValueView allocation_saved = *allocation;
    allocation->allocation_region = 4;
    CHECK(rejected_certificate(top, entry));
    *allocation = allocation_saved;
    NLSemanticValueView *domain = &before->values[packet.fields[2] - 1];
    const NLSemanticValueView domain_saved = *domain;
    domain->domain = 4;
    CHECK(rejected_certificate(top, entry));
    *domain = domain_saved;
    NLSemanticValueView *pointer = &before->values[packet.fields[0] - 1];
    const NLSemanticValueView pointer_saved = *pointer;
    ++pointer->reference.incarnation;
    CHECK(rejected_certificate(top, entry));
    *pointer = pointer_saved;
    before->values[good.input - 1].carrier = NL_CARRIER_LOOSE;
    CHECK(rejected_certificate(top, entry));
    before->values[good.input - 1] = pair;
    const NLAvailability availability =
        before->bindings[good.donor - 1].view.availability;
    before->bindings[good.donor - 1].view.availability = NL_CONSUMED;
    CHECK(rejected_certificate(top, entry));
    before->bindings[good.donor - 1].view.availability = availability;
    CHECK(before->scope_count);
    const bool active = before->scopes[0].active;
    before->scopes[0].active = true;
    CHECK(rejected_certificate(top, entry));
    before->scopes[0].active = active;
    const NLPlaceId root = pointer->reference.place;
    const bool live = after->places[root - 1].live;
    after->places[root - 1].live = true;
    CHECK(rejected_certificate(top, entry));
    after->places[root - 1].live = live;
    /* Unrelated original C carrier resurrected after C's legitimate release. */
    NLValueId c_allocation = 0;
    for (size_t i = 0; i < before->value_count; ++i)
        if (before->types[before->values[i].type - 1].view.kind ==
                NL_TYPE_ALLOCATION &&
            before->values[i].allocation_region == 4)
            c_allocation = i + 1;
    CHECK(c_allocation);
    const NLSemanticValueView ended_c = before->values[c_allocation - 1];
    before->values[c_allocation - 1].carrier = NL_CARRIER_LOOSE;
    CHECK(rejected_certificate(top, entry));
    before->values[c_allocation - 1] = ended_c;
    NLCheckedFragment *callee =
        (NLCheckedFragment *)nl_checked_call_body(leaf, leaf->two_call);
    CHECK(callee);
    const NLSource *source_origin = callee->source;
    callee->source = leaf->source;
    CHECK(rejected_certificate(top, entry));
    callee->source = source_origin;
    NLCheckedNodeView *pattern =
        operation(callee, NL_CHECKED_AGGREGATE_BINDING);
    NLCheckedNodeView *operand = operation(callee, NL_CHECKED_IDENTIFIER);
    NLCheckedNodeView *block = operation(callee, NL_CHECKED_BLOCK);
    CHECK(pattern && operand && block);
    const NLCheckedNodeView saved_pattern = *pattern, saved_operand = *operand,
                            saved_block = *block;
    pattern->argument_count = 1;
    CHECK(rejected_certificate(top, entry));
    *pattern = saved_pattern;
    operand->value_use = NL_VALUE_COPIED;
    CHECK(rejected_certificate(top, entry));
    *operand = saved_operand;
    block->first_item = 0;
    CHECK(rejected_certificate(top, entry));
    *block = saved_block;
    block->item_count = 0;
    CHECK(rejected_certificate(top, entry));
    *block = saved_block;
    const size_t body_count = callee->body_count;
    callee->body_count = 1;
    CHECK(rejected_certificate(top, entry));
    callee->body_count = body_count;
    NLFunctionBody *plan = callee->body_owner;
    const NLTwoRootDefinition summary = plan->two_root_definition;
    plan->two_root_definition.calls[1].member =
        plan->two_root_definition.calls[0].member;
    CHECK(rejected_certificate(top, entry));
    plan->two_root_definition = summary;
    plan->two_root_definition.calls[0].definition.requirements = 0;
    CHECK(rejected_certificate(top, entry));
    plan->two_root_definition = summary;
    plan->two_root_definition.calls[0].definition.head_link_required = true;
    CHECK(rejected_certificate(top, entry));
    plan->two_root_definition = summary;
    for (size_t i = 0; i < callee->body_count; ++i) {
        NLCheckedFragment *terminal = callee->bodies[i];
        const NLSource *terminal_source = terminal->source;
        terminal->source = callee->source;
        CHECK(rejected_certificate(top, entry));
        terminal->source = terminal_source;
        NLCheckedNodeView *free_node =
            operation(terminal, NL_CHECKED_DEALLOCATE);
        NLCheckedNodeView *split =
            operation(terminal, NL_CHECKED_AGGREGATE_BINDING);
        CHECK(free_node && split);
        const NLCheckedNodeView saved = *free_node, saved_split = *split;
        free_node->kind = NL_CHECKED_UNIT;
        CHECK(rejected_certificate(top, entry));
        *free_node = saved;
        split->argument_count = 2;
        CHECK(rejected_certificate(top, entry));
        *split = saved_split;
        NLCheckedNodeView *raw =
            &terminal->nodes[terminal->nodes[free_node->first_argument - 1]
                                 .next_argument -
                             1];
        const NLCheckedNodeView saved_raw = *raw;
        raw->results[0].value = packet.fields[1];
        CHECK(rejected_certificate(top, entry));
        *raw = saved_raw;
    }
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    printf("transitive member/world/summary/primitive poison rejected: %zu; no "
           "backend artifact\n",
           backend_poison_count);
    nl_checked_destroy(entry);
    return true;
}
#endif

#ifdef NEWLANG_EXPERIMENTAL_OWNER_AGGREGATES
static bool owner_poison(const char *path)
{
    NLCheckedFragment *entry = NULL;
    NLCheckDiagnostic d = {0};
    CHECK(load(path, &entry, &d) == NL_CHECK_OK);
    NLCheckedFragment *top = (NLCheckedFragment *)body(entry);
    NLCheckedFragment *parent = at_count(entry, 4);
    NLCheckedFragment *leaf =
        (NLCheckedFragment *)nl_checked_match_arm(parent, match(parent), 1);
    CHECK(leaf && leaf->whole_call &&
          nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    const NLWholeValueCallView good = leaf->whole_value;
    CHECK(good.count == 1);
    leaf->whole_value.count = 2;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.result = 0;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.inputs[0] = 0;
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    leaf->whole_value.receiver = good.donors[0];
    CHECK(rejected_certificate(top, entry));
    leaf->whole_value = good;
    NLSemanticContext *worlds[] = {(NLSemanticContext *)good.entry,
                                   (NLSemanticContext *)good.returned,
                                   (NLSemanticContext *)good.received};
    for (size_t j = 0; j < 3; ++j) {
        NLSemanticContext *c = worlds[j];
        for (size_t i = 0; i < c->value_count; ++i) {
            NLSemanticValueView *v = &c->values[i];
            if (v->carrier == NL_CARRIER_ENDED)
                continue;
            const NLSemanticValueView saved = *v;
            if (c->types[v->type - 1].view.kind == NL_TYPE_ALLOCATION) {
                v->allocation_region = 0;
                CHECK(rejected_certificate(top, entry));
                *v = saved;
            }
            if (v->type == nl_semantic_domain_type(c)) {
                v->domain = 0;
                CHECK(rejected_certificate(top, entry));
                *v = saved;
            }
            if (v->carrier == NL_CARRIER_AGGREGATE && v->field_count) {
                v->fields[0] = 0;
                CHECK(rejected_certificate(top, entry));
                *v = saved;
            }
        }
    }
    size_t constructors = 0, patterns = 0;
    for (size_t i = 0; i < leaf->count; ++i) {
        NLCheckedNodeView *n = &leaf->nodes[i];
        if (n->kind != NL_CHECKED_AGGREGATE &&
            n->kind != NL_CHECKED_AGGREGATE_BINDING)
            continue;
        const NLCheckedNodeView *input =
            n->kind == NL_CHECKED_AGGREGATE
                ? n
                : nl_checked_node_view(leaf, n->initializer);
        if (!input ||
            !nl_experimental_owner_aggregate_type(leaf->context, input->type))
            continue;
        const NLCheckedNodeView saved = *n;
        --n->argument_count;
        CHECK(rejected_certificate(top, entry));
        *n = saved;
        NLCheckedNodeView *member = &leaf->nodes[n->first_argument - 1];
        const NLCheckedNodeView field = *member;
        member->field_index = 4;
        CHECK(rejected_certificate(top, entry));
        *member = field;
        member->next_argument = n->first_argument;
        CHECK(rejected_certificate(top, entry));
        *member = field;
        constructors += n->kind == NL_CHECKED_AGGREGATE;
        patterns += n->kind == NL_CHECKED_AGGREGATE_BINDING;
    }
    CHECK(constructors >= 8 && patterns >= 3);
    NLCheckedFragment *callee =
        (NLCheckedFragment *)nl_checked_call_body(leaf, leaf->whole_call);
    NLCheckedNodeView *ret = operation(callee, NL_CHECKED_RETURN);
    CHECK(ret);
    const NLCheckedNodeView saved = *ret;
    ret->returned.value = 0;
    CHECK(rejected_certificate(top, entry));
    *ret = saved;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    printf("owner aggregate actual source poison rejected: %zu; "
           "constructors=%zu patterns=%zu; restored validator OK\n",
           backend_poison_count, constructors, patterns);
    nl_checked_destroy(entry);
    return true;
}
static bool owner_parser_oom(const char *path)
{
    NLSource *source = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK);
    size_t attempts = 0;
    for (failure_at = 0; failure_at < 30000; ++failure_at) {
        NLParser *parser = NULL;
        NLSyntaxTree *unit = NULL;
        index_at = 0;
        injecting = true;
        NLParseStatus status = nl_parser_create(source, &parser);
        if (status == NL_PARSE_OK)
            status = nl_parser_parse_function_unit(parser, &unit, NULL);
        injecting = false;
        if (status == NL_PARSE_OK) {
            attempts = failure_at;
            nl_syntax_tree_destroy(unit);
            nl_parser_destroy(parser);
            break;
        }
        CHECK(status == NL_PARSE_OUT_OF_MEMORY && unit == NULL);
        nl_parser_destroy(parser);
        parser = NULL;
        CHECK(nl_parser_create(source, &parser) == NL_PARSE_OK &&
              nl_parser_parse_function_unit(parser, &unit, NULL) ==
                  NL_PARSE_OK);
        nl_syntax_tree_destroy(unit);
        nl_parser_destroy(parser);
    }
    CHECK(attempts > 0);
    nl_source_destroy(source);
    printf("owner aggregate parser exhaustive OOM: %zu; no partial syntax and "
           "clean retries\n",
           attempts);
    return true;
}
#endif

int main(int argc, char **argv)
{
    if (argc >= 2 && strncmp(argv[1], "full-", 5) == 0) {
        public_source = true;
        argv[1] += 5;
    }
    if (argc == 3 && strcmp(argv[1], "reset") == 0)
        return same_address_reset(argv[2]) ? 0 : 1;
    if (argc == 3 && strcmp(argv[1], "check") == 0)
        return inspect(argv[2]);
    if (argc == 4 && strcmp(argv[1], "evidence") == 0)
        return evidence(argv[2], (size_t)strtoul(argv[3], NULL, 10)) ? 0 : 1;
    if (argc != 3)
        return EXIT_FAILURE;
#ifdef NEWLANG_EXPERIMENTAL_OWNER_AGGREGATES
    if (strcmp(argv[1], "owner-poison") == 0)
        return owner_poison(argv[2]) ? 0 : 1;
    if (strcmp(argv[1], "owner-parser-oom") == 0)
        return owner_parser_oom(argv[2]) ? 0 : 1;
#endif
#ifdef NEWLANG_EXPERIMENTAL_TRANSITIVE_TERMINAL
    if (strcmp(argv[1], "transitive-poison") == 0)
        return transitive_poison(argv[2]) ? 0 : 1;
#endif
#ifdef NEWLANG_EXPERIMENTAL_NESTED_CALLER
    if (strcmp(argv[1], "nested-poison") == 0)
        return nested_poison(argv[2]) ? 0 : 1;
#endif
    bool ok = strcmp(argv[1], "poison") == 0        ? poison(argv[2])
              : strcmp(argv[1], "oom") == 0         ? oom(argv[2])
              : strcmp(argv[1], "checker-oom") == 0 ? checker_oom(argv[2])
                                                    : false;
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
