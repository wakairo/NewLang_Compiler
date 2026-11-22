#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "p1645:%d: %s\n", __LINE__, #condition);           \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef struct {
    NLSemanticContext *context;
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *entry;
    NLCheckedFragment *root;
} Program;

static bool prepare(Program *p, const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    bool ok =
        nl_source_load(path, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &unit, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&p->context) == NL_CHECK_OK;
    const NLSyntaxTree *units[] = {unit};
    ok = ok && nl_semantic_register_function_unit(p->context, units, 1, NULL) ==
                   NL_CHECK_OK;
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok &&
           nl_source_create("main()", sizeof("main()") - 1, "<p1645-entry>",
                            &p->source) == NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->entry, NULL) ==
               NL_PARSE_OK;
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->entry, &p->root, &diagnostic);
    printf("CHECK status=%d diagnostic=%s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

static void dispose(Program *p)
{
    nl_checked_destroy(p->root);
    nl_syntax_tree_destroy(p->entry);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    nl_semantic_destroy(p->context);
}

static NLCheckedNodeId normal_if(const NLCheckedFragment *body)
{
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *n = nl_checked_node_view(body, id);
        if (n && n->kind == NL_CHECKED_IF && n->normal_arms == 2)
            return id;
    }
    return 0;
}

static const NLCheckedNodeView *arm_block(const NLCheckedFragment *arm)
{
    const NLCheckedNodeView *root =
        nl_checked_node_view(arm, nl_checked_root(arm));
    return root ? nl_checked_node_view(arm, root->initializer) : NULL;
}

static NLCheckedNodeId division_actual(const NLCheckedFragment *arm,
                                       NLCheckedNodeId call_id)
{
    const NLCheckedNodeView *call = nl_checked_node_view(arm, call_id);
    const NLCheckedNodeView *actual =
        call ? nl_checked_node_view(arm, call->first_argument) : NULL;
    return call && call->kind == NL_CHECKED_REGISTERED_CALL &&
                   call->argument_count == 1 && actual &&
                   actual->kind == NL_CHECKED_USIZE_DIVISION
               ? call->first_argument
               : 0;
}

typedef struct {
    const NLCheckedFragment *arm;
    NLCheckedNodeId id;
    NLUsizeDivisionView view;
} Event;

static bool obtain(Event *e, const NLCheckedFragment *arm, NLCheckedNodeId id)
{
    e->arm = arm;
    e->id = id;
    return id && nl_checked_usize_division_view(arm, id, &e->view) &&
           e->view.origin == arm && e->view.node == id &&
           nl_checked_usize_division_valid(arm, id, NULL) &&
           nl_checked_usize_division_valid(arm, id, &e->view);
}

static bool samples(Program *p, Event out[2][2])
{
    REQUIRE(check(p) == NL_CHECK_OK && p->root);
    REQUIRE(nl_checked_proof_class(p->root) == NL_CHECKED_MACHINE_PROVED);
    NLDivisionUsizePlan plan;
    REQUIRE(
        nl_quantity_usize_call_valid(p->root, nl_checked_root(p->root), &plan));
    const size_t expected[2][2] = {{11, 4}, {28, 12}};
    const NLCheckedFragment *body = plan.body;
    size_t count = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *call = nl_checked_node_view(body, id);
        if (!call || call->kind != NL_CHECKED_REGISTERED_CALL)
            continue;
        REQUIRE(count < 2 && nl_h_custody_result_valid(body, id));
        const NLCheckedFragment *route = nl_checked_call_body(body, id);
        const NLCheckedNodeId site = normal_if(route);
        REQUIRE(site);
        for (size_t a = 0; a < 2; ++a) {
            Event *e = &out[count][a];
            const NLCheckedFragment *arm = nl_checked_if_arm(route, site, a);
            const NLCheckedNodeView *block = arm_block(arm);
            REQUIRE(block && block->kind == NL_CHECKED_BLOCK &&
                    nl_h_custody_result_valid(arm, block->tail));
            REQUIRE(obtain(e, arm, division_actual(arm, block->tail)));
            const NLSemanticContext *world = nl_checked_context(arm);
            const NLCheckedNodeView *actual = nl_checked_node_view(arm, e->id);
            const NLCheckedFragment *callee =
                nl_checked_call_body(arm, block->tail);
            REQUIRE(world != nl_checked_context(route) &&
                    world == nl_checked_context(callee) && actual &&
                    actual->parameter_type ==
                        nl_semantic_core_type(world, NL_TYPE_USIZE) &&
                    actual->result_count == 1 &&
                    actual->results[0].value == e->view.result &&
                    e->view.numerator_known && e->view.quotient_known &&
                    e->view.quotient == expected[count][a] &&
                    e->view.numbers[1] == (a ? 9 : 4));
        }
        ++count;
    }
    return count == 2;
}

static bool rejects(const Event *target, const Event *other)
{
    return !nl_checked_usize_division_valid(target->arm, target->id,
                                            &other->view);
}

static bool identity(Program *p, const char *path)
{
    Event local[2][2] = {0}, foreign[2][2] = {0};
    REQUIRE(samples(p, local));
    const Event *target = &local[0][0];
    const Event *sibling = &local[0][1];
    const Event *later = &local[1][0];
    const NLCheckedNodeView *block = arm_block(target->arm);
    const NLCheckedNodeView *binding =
        nl_checked_node_view(target->arm, block->first_item);
    Event initializer;
    REQUIRE(binding && binding->kind == NL_CHECKED_BINDING &&
            obtain(&initializer, target->arm, binding->initializer) &&
            initializer.view.quotient == target->view.quotient &&
            initializer.view.result != target->view.result);
    REQUIRE(rejects(target, &initializer) && rejects(&initializer, target));
    REQUIRE(nl_checked_source(target->arm) == nl_checked_source(sibling->arm) &&
            nl_checked_source(target->arm) == nl_checked_source(later->arm) &&
            nl_checked_context(target->arm) !=
                nl_checked_context(sibling->arm) &&
            nl_checked_context(target->arm) != nl_checked_context(later->arm) &&
            rejects(target, sibling) && rejects(target, later));

    Program second = {0};
    const bool independent =
        prepare(&second, path) && samples(&second, foreign) &&
        target->id == foreign[0][0].id &&
        target->view.result == foreign[0][0].view.result &&
        target->view.quotient == foreign[0][0].view.quotient &&
        nl_checked_source(target->arm) !=
            nl_checked_source(foreign[0][0].arm) &&
        nl_checked_context(target->arm) !=
            nl_checked_context(foreign[0][0].arm) &&
        rejects(target, &foreign[0][0]) && rejects(&foreign[0][0], target);
    dispose(&second);
    REQUIRE(independent);
    REQUIRE(nl_checked_usize_division_valid(target->arm, target->id,
                                            &target->view));
    puts("IDENTITY actual-slot sibling-arm later-body independent-source/world"
         " rejected; original views valid");
    return true;
}

typedef enum {
    NO_FAULT,
    INNER_SEAL,
    ENTRY_SEAL
} Fault;
static Fault fault;
static size_t fail_malloc, visits, hits, failed_bytes, preseal_visits;
static bool preseal_ok = true;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    if (!fail_malloc || --fail_malloc)
        return __real_malloc(size);
    ++hits;
    failed_bytes = size;
    return NULL;
}

NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId call,
                                              const NLSemanticContext *world)
{
    const NLCheckedNodeId site = normal_if(child);
    for (size_t a = 0; site && a < 2; ++a) {
        const NLCheckedFragment *arm = nl_checked_if_arm(child, site, a);
        const NLCheckedNodeView *block = arm_block(arm);
        const NLCheckedNodeId div =
            block ? division_actual(arm, block->tail) : 0;
        if (!div)
            continue;
        ++preseal_visits;
        size_t function = 0;
        preseal_ok &= !nl_h_custody_result_valid(parent, call) &&
                      nl_h_custody_if_body(arm, child, &function) == child &&
                      function != 0 &&
                      nl_h_custody_result_valid(arm, block->tail) &&
                      !nl_checked_usize_division_valid(arm, div, NULL);
    }
    const NLCheckedNodeView *node = nl_checked_node_view(parent, call);
    if (fault == INNER_SEAL && !world && division_actual(parent, call)) {
        ++visits;
        fail_malloc = 1;
    } else if (fault == ENTRY_SEAL && world && node && !node->argument_count) {
        ++visits;
        /* The entry path snapshots nodes before the owned world. */
        fail_malloc = 2;
    }
    const NLCheckStatus status =
        __real_nl_h_custody_seal_result(parent, child, call, world);
    fail_malloc = 0;
    return status;
}

static bool rollback(Program *p)
{
    NLSemanticContext *c = p->context;
    NLSymbolId symbol = 0;
    const NLScalarValue seed = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                1645};
    REQUIRE(nl_semantic_seed_scalar(c, "p1645_anchor", seed, &symbol) ==
                NL_CHECK_OK &&
            symbol);
    const NLSemanticBindingView b = c->bindings[symbol - 1].view;
    unsigned char context[sizeof(*c)], binding[sizeof(*c->bindings)],
        place[sizeof(*c->places)], value[sizeof(*c->values)];
    memcpy(context, c, sizeof(context));
    memcpy(binding, &c->bindings[symbol - 1], sizeof(binding));
    memcpy(place, &c->places[b.place - 1], sizeof(place));
    memcpy(value, &c->values[b.value - 1], sizeof(value));
    for (Fault site = INNER_SEAL; site <= ENTRY_SEAL; ++site) {
        fault = site;
        visits = hits = failed_bytes = 0;
        const NLCheckStatus status = check(p);
        fault = NO_FAULT;
        REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && !p->root && visits == 1 &&
                hits == 1 && failed_bytes &&
                !memcmp(context, c, sizeof(context)) &&
                !memcmp(binding, &c->bindings[symbol - 1], sizeof(binding)) &&
                !memcmp(place, &c->places[b.place - 1], sizeof(place)) &&
                !memcmp(value, &c->values[b.value - 1], sizeof(value)) &&
                nl_sem_validate(c) == NL_CHECK_OK);
        printf("OOM %s reached=%zu hit=%zu bytes=%zu rollback=PASS\n",
               site == INNER_SEAL ? "inner-callee-seal" : "entry-result-world",
               visits, hits, failed_bytes);
    }
    preseal_visits = 0;
    preseal_ok = true;
    REQUIRE(check(p) == NL_CHECK_OK && p->root && preseal_ok &&
            preseal_visits == 2 &&
            nl_checked_proof_class(p->root) == NL_CHECKED_MACHINE_PROVED);
    char *text = NULL;
    size_t length = 1645;
    hits = failed_bytes = 0;
    fail_malloc = 1;
    const NLCheckStatus status = nl_quantity_usize_c17(p->root, &text, &length);
    fail_malloc = 0;
    const bool preserved = !text && length == 1645 && hits == 1 && failed_bytes;
    free(text);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && preserved);
    printf("OOM C17 staging hit=%zu bytes=%zu output-preserved=PASS\n", hits,
           failed_bytes);
    text = NULL;
    const bool emitted =
        nl_quantity_usize_c17(p->root, &text, &length) == NL_CHECK_OK && text &&
        length && strstr(text, " / ((size_t)UINTMAX_C(3)))");
    free(text);
    REQUIRE(emitted);
    puts("PRESEAL inner valid, enclosing seal pending, public unpublished;"
         " CHECK/C17 retry PASS");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0};
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(argv[1], "identity"))
        ok = identity(&p, argv[2]);
    else if (ok && !strcmp(argv[1], "rollback"))
        ok = rollback(&p);
    else
        ok = false;
    dispose(&p);
    return ok ? 0 : 1;
}
