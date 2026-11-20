/* P-owned observations and corruption tests. No semantic-value seeding.
 * Private access below is restricted to adversarial mutation and reached OOM;
 * positive numerical evidence uses the public read-only division API. */
#include "../../src/division_usize_internal.h"
#include "../../src/h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_addition.h"
#include "newlang/usize_division.h"
#include "newlang/usize_subtraction.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEED(x)                                                                \
    do {                                                                       \
        if (!(x)) {                                                            \
            fprintf(stderr, "P1544_ASSERT line=%d %s\n", __LINE__, #x);        \
            return false;                                                      \
        }                                                                      \
    } while (0)
static unsigned fail_site, hits, unknown_seen, unknown_valid, forged_rejected;
static bool armed, receipt_window, result_window, emitter_window, observer_bad;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
NLCheckStatus __real_nl_usize_arithmetic_capture(NLCheckedFragment *,
                                                 NLCheckedNodeId,
                                                 const NLSyntaxNode *,
                                                 const NLSemanticContext *);
NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
void *__wrap_malloc(size_t size)
{
    if (armed && (result_window || emitter_window)) {
        armed = false;
        ++hits;
        return NULL;
    }
    return __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    if (armed && receipt_window) {
        armed = false;
        ++hits;
        return NULL;
    }
    return __real_calloc(count, size);
}
NLCheckStatus __wrap_nl_usize_arithmetic_capture(NLCheckedFragment *f,
                                                 NLCheckedNodeId id,
                                                 const NLSyntaxNode *s,
                                                 const NLSemanticContext *c)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    const bool division = n && n->kind == NL_CHECKED_USIZE_DIVISION;
    receipt_window = armed && fail_site == 1 && division;
    const NLCheckStatus status =
        __real_nl_usize_arithmetic_capture(f, id, s, c);
    receipt_window = false;
    if (status == NL_CHECK_OK && division && !n->scalar_result.known) {
        NLUsizeDivisionView v;
        ++unknown_seen;
        if (!nl_checked_usize_division_view(f, id, &v) || v.numerator_known ||
            v.quotient_known || v.numbers[0] || v.quotient || !v.numbers[1]) {
            observer_bad = true;
            return status;
        }
        /* A nested source call during independent definition checking has a
         * real registered body and current destination, unlike the temporary
         * unregistered independent root. It supplies genuine Unknown evidence.
         */
        if (nl_checked_usize_division_valid(f, id, &v)) {
            ++unknown_valid;
            v.numerator_known = v.quotient_known = true;
            v.numbers[0] = 7;
            v.quotient = 1;
            if (nl_checked_usize_division_valid(f, id, &v))
                observer_bad = true;
            else
                ++forged_rejected;
        }
    }
    return status;
}
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    result_window = armed && fail_site == 2;
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, call, world);
    result_window = false;
    return status;
}

typedef struct {
    NLSource *source, *entry_source;
    NLParser *parser, *entry_parser;
    NLSyntaxTree *unit, *entry;
    NLSemanticContext *context;
    NLCheckedFragment *artifact;
} Program;
static void inputs_destroy(Program *p)
{
    nl_syntax_tree_destroy(p->unit);
    p->unit = NULL;
    nl_parser_destroy(p->parser);
    p->parser = NULL;
    nl_source_destroy(p->source);
    p->source = NULL;
}
static void destroy(Program *p)
{
    nl_checked_destroy(p->artifact);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->entry_parser);
    nl_source_destroy(p->entry_source);
    nl_semantic_destroy(p->context);
    inputs_destroy(p);
}
static bool load(const char *path, Program *p)
{
    NEED(nl_source_load(path, &p->source) == NL_SOURCE_OK);
    NEED(nl_parser_create(p->source, &p->parser) == NL_PARSE_OK);
    NEED(nl_parser_parse_function_unit(p->parser, &p->unit, NULL) ==
         NL_PARSE_OK);
    NEED(nl_semantic_create(&p->context) == NL_CHECK_OK);
    return true;
}
static NLCheckStatus registration(Program *p)
{
    const NLSyntaxTree *units[] = {p->unit};
    NLFunctionUnitDiagnostic d = {0};
    const NLCheckStatus status =
        nl_semantic_register_function_unit(p->context, units, 1, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "REGISTER status=%d code=%s\n", status,
                d.diagnostic.diagnostic.code ? d.diagnostic.diagnostic.code
                                             : "none");
    return status;
}
static bool ready(const char *path, Program *p)
{
    NEED(load(path, p));
    NEED(registration(p) == NL_CHECK_OK && !observer_bad);
    return true;
}
static NLCheckStatus actual(Program *p)
{
    if (!p->entry_source) {
        if (nl_source_create("main()", 6, "<p1544-entry>", &p->entry_source) !=
                NL_SOURCE_OK ||
            nl_parser_create(p->entry_source, &p->entry_parser) !=
                NL_PARSE_OK ||
            nl_parser_parse_expression_fragment(p->entry_parser, &p->entry,
                                                NULL) != NL_PARSE_OK)
            return NL_CHECK_INTERNAL_ERROR;
    }
    NLCheckDiagnostic d = {0};
    const NLCheckStatus status =
        nl_semantic_check_expression(p->context, p->entry, &p->artifact, &d);
    if (status != NL_CHECK_OK)
        fprintf(stderr, "ACTUAL status=%d code=%s\n", status,
                d.diagnostic.code ? d.diagnostic.code : "none");
    return status;
}
static bool scalar(const NLSemanticContext *c, NLValueId id, size_t number)
{
    NLSemanticValueView v;
    return nl_semantic_value_view(c, id, &v) && v.scalar_known &&
           v.scalar_value == number && v.dependencies == NL_DEPENDENCY_FREE &&
           !v.value_dependency_count && !v.domain && !v.reference_count &&
           !v.reference.place && !v.allocation_region;
}
static bool positive(const char *path, size_t numerator, size_t divisor)
{
    Program p = {0};
    NEED(ready(path, &p));
    inputs_destroy(&p); /* the registered plan must own its actual source */
    NEED(actual(&p) == NL_CHECK_OK && !observer_bad && unknown_seen);
    NLDivisionUsizePlan main, helper;
    NEED(nl_division_usize_call_valid(p.artifact, p.artifact->root, &main));
    NEED(nl_division_usize_call_valid(main.body, main.call, &helper));
    NLUsizeDivisionView v;
    NEED(nl_checked_usize_division_view(helper.body, helper.division, &v));
    NEED(nl_checked_usize_division_valid(helper.body, helper.division, &v));
    NEED(v.numerator_known && v.quotient_known && v.target_max == SIZE_MAX &&
         v.numbers[0] == numerator && v.numbers[1] == divisor &&
         v.quotient == numerator / divisor && v.result != v.values[0] &&
         v.result != v.values[1] && v.operands[0] < v.operands[1]);
    NEED(!nl_checked_usize_addition_valid(helper.body, helper.division, NULL));
    NEED(!nl_checked_usize_subtraction_valid(helper.body, helper.division,
                                             NULL));
    const NLCheckedNodeView *root =
        nl_checked_node_view(main.body, main.body->root);
    size_t symbols[3] = {0};
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *b = nl_checked_node_view(main.body, id);
        const char *name = p.context->bindings[b->symbol - 1].name;
        const size_t q = numerator / divisor;
        if (!strcmp(name, "groups")) {
            NEED(scalar(p.context,
                        p.context->bindings[b->symbol - 1].view.value, q));
            symbols[0] = b->symbol;
        }
        if (!strcmp(name, "successor")) {
            NEED(scalar(p.context,
                        p.context->bindings[b->symbol - 1].view.value, q + 1));
            NEED(nl_checked_usize_addition_valid(main.body, b->initializer,
                                                 NULL));
            symbols[1] = b->symbol;
        }
        if (!strcmp(name, "retained_quotient")) {
            NEED(scalar(p.context,
                        p.context->bindings[b->symbol - 1].view.value, q));
            symbols[2] = b->symbol;
        }
        id = b->next_item;
    }
    NEED(symbols[0] && symbols[1] && symbols[2]);
    char *text = NULL;
    size_t length = 0;
    NEED(nl_division_usize_c17(p.artifact, &text, &length) == NL_CHECK_OK);
    NEED(length && strstr(text, " / ") && strstr(text, " + ") &&
         !strstr(text, "abort"));
    free(text);
    printf("{\"groups\":%zu,\"successor\":%zu,\"retained\":%zu,\"unknown_"
           "seen\":%u,\"unknown_valid\":%u,\"forged_rejected\":%u}\n",
           symbols[0], symbols[1], symbols[2], unknown_seen, unknown_valid,
           forged_rejected);
    destroy(&p);
    return true;
}
static bool refused(const Program *p)
{
    char sentinel = 0, *out = &sentinel;
    size_t length = 127;
    NLDivisionUsizePlan plan;
    const NLCheckStatus status =
        nl_division_usize_c17(p->artifact, &out, &length);
    if (status == NL_CHECK_OK)
        free(out);
    NEED(status == NL_CHECK_SEMANTIC_UNSUPPORTED && out == &sentinel &&
         length == 127);
    NEED(!nl_division_usize_call_valid(p->artifact, p->artifact->root, &plan));
    return true;
}
static bool attacks(const char *path)
{
    Program p = {0}, foreign = {0};
    NEED(ready(path, &p) && ready(path, &foreign));
    NEED(actual(&p) == NL_CHECK_OK && actual(&foreign) == NL_CHECK_OK);
    NLDivisionUsizePlan main, helper, other_main, other;
    NEED(nl_division_usize_call_valid(p.artifact, p.artifact->root, &main));
    NEED(nl_division_usize_call_valid(main.body, main.call, &helper));
    NEED(nl_division_usize_call_valid(foreign.artifact, foreign.artifact->root,
                                      &other_main));
    NEED(
        nl_division_usize_call_valid(other_main.body, other_main.call, &other));
    NLUsizeDivisionView v;
    NEED(nl_checked_usize_division_view(helper.body, helper.division, &v));
    NLUsizeDivisionView unchanged;
    memset(&unchanged, 0xa5, sizeof unchanged);
    const NLUsizeDivisionView sentinel_view = unchanged;
    NEED(!nl_checked_usize_division_view(helper.body, helper.body->root,
                                         &unchanged));
    NEED(!memcmp(&unchanged, &sentinel_view, sizeof unchanged));
    NLUsizeAdditionView addition = {0};
    NLUsizeSubtractionView subtraction = {0};
    NEED(!nl_checked_usize_addition_view(helper.body, helper.division,
                                         &addition));
    NEED(!nl_checked_usize_subtraction_view(helper.body, helper.division,
                                            &subtraction));
    for (size_t i = 0; i < 14; ++i) {
        NLUsizeDivisionView bad = v;
        switch (i) {
        case 0:
            bad.origin = other.body;
            break;
        case 1:
            ++bad.result;
            break;
        case 2:
            ++bad.type;
            break;
        case 3:
            bad.numbers[1] = 0;
            break;
        case 4:
            ++bad.numbers[0];
            break;
        case 5:
            ++bad.quotient;
            break;
        case 6:
            bad.numerator_known = false;
            break;
        case 7:
            bad.quotient_known = false;
            break;
        case 8:
            --bad.target_max;
            break;
        case 9:
            ++bad.operator_span.start_byte;
            break;
        case 10:
            ++bad.operands[0];
            break;
        case 11:
            ++bad.symbols[0];
            break;
        case 12:
            ++bad.facts[0];
            break;
        case 13:
            ++bad.operand_spans[1].end_byte;
            break;
        }
        NEED(!nl_checked_usize_division_valid(helper.body, helper.division,
                                              &bad));
    }
    NLCheckedFragment *body = (NLCheckedFragment *)helper.body;
    NLCheckedNodeView *div = &body->nodes[helper.division - 1];
    const NLCheckedNodeView original = *div;
    for (size_t i = 0; i < 4; ++i) {
        if (i == 0)
            div->kind = NL_CHECKED_USIZE_SUBTRACTION;
        if (i == 1)
            ++div->name.start_byte;
        if (i == 2)
            div->scalar_result.known = false;
        if (i == 3)
            div->results[0].value = v.values[0];
        const bool bad = refused(&p);
        *div = original;
        NEED(bad);
    }
    NLCheckedFragment *caller = (NLCheckedFragment *)main.body;
    NLCheckedNodeView *call = &caller->nodes[main.call - 1];
    const NLCheckedNodeView saved_call = *call;
    ++call->results[0].value;
    bool bad = refused(&p);
    *call = saved_call;
    NEED(bad);
    NLSemanticBindingView *formal =
        &p.context->bindings[helper.formal - 1].view;
    const NLSemanticBindingView saved_formal = *formal;
    ++formal->value;
    bad = refused(&p);
    *formal = saved_formal;
    NEED(bad);
    NLSemanticValueView *value = &p.context->values[v.result - 1];
    const NLSemanticValueView saved_value = *value;
    value->dependencies = NL_DEPENDENCIES_UNKNOWN;
    bad = refused(&p);
    *value = saved_value;
    NEED(bad);
    ++value->scalar_value;
    bad = refused(&p);
    *value = saved_value;
    NEED(bad);
    const NLSource *saved_source = body->source;
    body->source = other.body->source;
    bad = refused(&p);
    body->source = saved_source;
    NEED(bad);
    const NLSemanticContext *world = body->context;
    body->context = foreign.context;
    bad = refused(&p);
    body->context = world;
    NEED(bad);
    const NLCheckedNodeId edge = caller->body_calls[0];
    ++caller->body_calls[0];
    bad = refused(&p);
    caller->body_calls[0] = edge;
    NEED(bad);
    NEED(nl_division_usize_call_valid(p.artifact, p.artifact->root, &main));
    printf("AUTHENTICITY public14 graph11 rejected; pristine accepted\n");
    destroy(&p);
    destroy(&foreign);
    return true;
}
static uint64_t digest(const NLSource *source)
{
    NLSourceView v;
    if (!nl_source_view(source, (NLSourceSpan){0, nl_source_length(source)},
                        &v))
        return 0;
    uint64_t hash = UINT64_C(1469598103934665603);
    for (size_t i = 0; i < v.length; ++i)
        hash = (hash ^ v.bytes[i]) * UINT64_C(1099511628211);
    return hash;
}
static bool oom(const char *path, unsigned site)
{
    Program p = {0};
    NEED(load(path, &p));
    const uint64_t source_before = digest(p.source);
    fail_site = site;
    if (site != 1)
        NEED(registration(&p) == NL_CHECK_OK);
    NLSemanticContext before = *p.context;
    if (site == 1) {
        armed = true;
        NEED(registration(&p) == NL_CHECK_OUT_OF_MEMORY);
    } else if (site == 2) {
        armed = true;
        /* An occupied owner slot is refused before CHECK. Start with NULL
         * so this injection reaches result sealing and must preserve NULL. */
        NEED(p.artifact == NULL);
        NEED(actual(&p) == NL_CHECK_OUT_OF_MEMORY && p.artifact == NULL);
    } else {
        NEED(actual(&p) == NL_CHECK_OK);
        before = *p.context;
        char sentinel = 0, *out = &sentinel;
        size_t length = 127;
        emitter_window = armed = true;
        const NLCheckStatus status =
            nl_division_usize_c17(p.artifact, &out, &length);
        emitter_window = false;
        NEED(status == NL_CHECK_OUT_OF_MEMORY && out == &sentinel &&
             length == 127);
    }
    NEED(hits == 1 && !armed && digest(p.source) == source_before);
    NEED(!memcmp(&before, p.context, sizeof before));
    NEED(nl_sem_validate(p.context) == NL_CHECK_OK);
    if (site == 1)
        NEED(registration(&p) == NL_CHECK_OK);
    if (site != 3)
        NEED(actual(&p) == NL_CHECK_OK);
    char *out = NULL;
    size_t length = 0;
    NEED(nl_division_usize_c17(p.artifact, &out, &length) == NL_CHECK_OK &&
         length);
    free(out);
    printf("OOM site=%u hits=%u no_clobber=PASS pristine_retry=PASS\n", site,
           hits);
    destroy(&p);
    return true;
}
int main(int argc, char **argv)
{
    if (argc == 5 && !strcmp(argv[1], "positive"))
        return positive(argv[2], (size_t)strtoull(argv[3], NULL, 10),
                        (size_t)strtoull(argv[4], NULL, 10))
                   ? 0
                   : 1;
    if (argc == 3 && !strcmp(argv[1], "attack"))
        return attacks(argv[2]) ? 0 : 1;
    if (argc == 4 && !strcmp(argv[1], "oom"))
        return oom(argv[2], (unsigned)strtoul(argv[3], NULL, 10)) ? 0 : 1;
    return 2;
}
