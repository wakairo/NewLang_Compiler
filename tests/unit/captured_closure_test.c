#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
    s = nl_captured_closure_probe(unit, out, diagnostic);
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
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        c->view = good;
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_OK);
    }
    /* Same numeric pre-grant IDs/contents, distinct nominal owned forks.
     * Even a valid clone cannot replace the recorded exact arm or fork. */
    CHECK(nl_packet_same_entry(c->branches[0].entry, c->branches[1].entry));
    NLSemanticContext *saved_entry = c->branches[0].entry;
    c->branches[0].entry = c->branches[1].entry;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    c->branches[0].entry = saved_entry;
    NLSemanticContext *foreign_ancestor = NULL;
    CHECK(nl_sem_clone(c->view.ancestor, &foreign_ancestor) == NL_CHECK_OK);
    c->view.ancestor = foreign_ancestor;
    for (size_t r = 0; r < c->view.count; ++r)
        c->view.originals[r].origin = foreign_ancestor;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    c->view = good;
    nl_semantic_destroy(foreign_ancestor);
    NLCheckedFragment *none = (NLCheckedFragment *)c->branches[0].arm;
    NLSemanticContext *foreign = NULL;
    CHECK(nl_sem_clone(none->context, &foreign) == NL_CHECK_OK);
    const NLSemanticContext *saved_world = none->context;
    none->context = foreign;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    none->context = saved_world;
    nl_semantic_destroy(foreign);
    c->view.closed_post = c->branches[1].world;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    c->view = good;
    NLSemanticContext *post = (NLSemanticContext *)c->view.closed_post;
    post->regions[0].view.live = true;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    post->regions[0].view.live = false;
    NLSemanticContext *world = (NLSemanticContext *)none->context;
    world->scopes[0].active = true;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    world->scopes[0].active = false;
    world->places[good.originals[0].root - 1].live = true;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    world->places[good.originals[0].root - 1].live = false;
    NLCheckedFragment *some = (NLCheckedFragment *)c->branches[1].arm;
    const NLSemanticContext *some_world = some->context;
    some->context = none->context;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    some->context = some_world;
    NLSemanticBindingView *owner =
        &world->bindings[good.originals[0].allocation_binding - 1].view;
    owner->availability = NL_AVAILABLE;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    owner->availability = NL_CONSUMED;
    NLValueId original = good.originals[0].allocation_value;
    world->values[original - 1].allocation_region =
        good.originals[1].extent.region;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    world->values[original - 1].allocation_region =
        good.originals[0].extent.region;
    const NLSemanticValueView old = world->values[world->value_count - 1];
    world->values[world->value_count - 1].dependencies =
        NL_DEPENDENCIES_UNKNOWN;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    world->values[world->value_count - 1] = old;
    world->values[world->value_count - 1].value_dependency_count = 1;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
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
        CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
              NL_CHECK_ANALYSIS_PRECISION_LIMIT);
        *v = saved;
    }
    NLCheckedNodeView *end = operation(none, NL_CHECKED_DESTROY);
    NLCheckedNodeView *unit = operation(none, NL_CHECKED_UNIT);
    CHECK(end != NULL && unit != NULL);
    NLCheckedNodeView saved_unit = *unit;
    *unit = *end; /* duplicate EndRoot after actual EndRoot */
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    *unit = saved_unit;
    NLCheckedNodeView *free_node = operation(none, NL_CHECKED_DEALLOCATE);
    NLCheckedNodeView *argument = &none->nodes[free_node->first_argument - 1];
    const NLValueUse use = argument->value_use;
    argument->value_use = NL_VALUE_COPIED;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    argument->value_use = use;
    NLCheckedNodeView *loan = operation(none, NL_CHECKED_LOAN_HEADER);
    CHECK(loan != NULL);
    loan->loan.body_nonescape_proved = false;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    loan->loan.body_nonescape_proved = true;
    NLCheckedNodeView *grant =
        &none->nodes[nl_checked_node_view(none, none->root)->initializer - 1];
    grant->allocation_success = true; /* None must mint no original */
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    grant->allocation_success = false;
    CHECK(nl_checked_captured_closure_validate(top, match(top)) == NL_CHECK_OK);
    nl_checked_destroy(entry);
    puts("tuple/order/world/post/trace/dependency/scope poison rejected");
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
    CHECK(nl_checked_captured_closure_validate(top, match(top)) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
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
    NLCheckStatus s = nl_captured_closure_probe(unit, &out, NULL);
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
        s = nl_captured_closure_probe(unit, &out, NULL);
        injecting = false;
        CHECK(s == NL_CHECK_OUT_OF_MEMORY && out == NULL);
        ++attacked;
        CHECK(nl_captured_closure_probe(unit, &out, NULL) == NL_CHECK_OK);
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
        puts("; NOT public source admission or native evidence");
    } else {
        printf("status=%d category=%s code=%s\n", s,
               d.diagnostic.category == NULL ? "profile"
                                             : d.diagnostic.category,
               d.diagnostic.code == NULL ? "PROBE-PROFILE" : d.diagnostic.code);
    }
    nl_checked_destroy(out);
    return s == NL_CHECK_OK ? 0 : 3;
}

int main(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[1], "check") == 0)
        return inspect(argv[2]);
    if (argc == 4 && strcmp(argv[1], "evidence") == 0)
        return evidence(argv[2], (size_t)strtoul(argv[3], NULL, 10)) ? 0 : 1;
    if (argc != 3)
        return EXIT_FAILURE;
    bool ok = strcmp(argv[1], "poison") == 0        ? poison(argv[2])
              : strcmp(argv[1], "oom") == 0         ? oom(argv[2])
              : strcmp(argv[1], "checker-oom") == 0 ? checker_oom(argv[2])
                                                    : false;
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
