#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"
#include "newlang/usize_division.h"
#include "newlang/usize_remainder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                     \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "line %d: %s\n", __LINE__, #condition);            \
            return false;                                                      \
        }                                                                      \
    } while (0)

typedef struct {
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *expression;
    NLSemanticContext *context;
    NLCheckedFragment *root;
} Program;

static bool prepare(Program *p, const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    bool ok =
        nl_source_load(file, &source) == NL_SOURCE_OK &&
        nl_parser_create(source, &parser) == NL_PARSE_OK &&
        nl_parser_parse_function_unit(parser, &tree, NULL) == NL_PARSE_OK &&
        nl_semantic_create(&p->context) == NL_CHECK_OK;
    const NLSyntaxTree *units[] = {tree};
    ok = ok && nl_semantic_register_function_unit(p->context, units, 1, NULL) ==
                   NL_CHECK_OK;
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return ok &&
           nl_source_create("main()", 6, "p1605-entry", &p->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->expression,
                                               NULL) == NL_PARSE_OK;
}

static void dispose(Program *p)
{
    nl_checked_destroy(p->root);
    nl_syntax_tree_destroy(p->expression);
    nl_parser_destroy(p->parser);
    nl_source_destroy(p->source);
    nl_semantic_destroy(p->context);
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->expression, &p->root, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

static bool arithmetic(NLCheckedKind kind)
{
    return kind == NL_CHECKED_USIZE_DIVISION ||
           kind == NL_CHECKED_USIZE_REMAINDER;
}

static bool event_valid(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (n && n->kind == NL_CHECKED_USIZE_DIVISION) {
        NLUsizeDivisionView v;
        return nl_checked_usize_division_view(f, id, &v) &&
               nl_checked_usize_division_valid(f, id, &v) &&
               nl_checked_usize_division_valid(f, id, NULL);
    }
    NLUsizeRemainderView v;
    return n && n->kind == NL_CHECKED_USIZE_REMAINDER &&
           nl_checked_usize_remainder_view(f, id, &v) &&
           nl_checked_usize_remainder_valid(f, id, &v) &&
           nl_checked_usize_remainder_valid(f, id, NULL);
}

typedef enum {
    NO_FAULT,
    ARM_RESERVATION,
    HELPER_SEAL,
    ARM_WORLD,
    COPY_ENTRY_WORLD
} Fault;
static Fault fault;
static size_t fail_allocation, visits, hits, bytes, pre_events;
static bool pre_ok = true;

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);

static bool allocation_fails(size_t size)
{
    if (!fail_allocation)
        return false;
    ++visits;
    if (--fail_allocation)
        return false;
    bytes = size;
    ++hits;
    return true;
}

void *__wrap_malloc(size_t size)
{
    return allocation_fails(size) ? NULL : __real_malloc(size);
}

void *__wrap_realloc(void *p, size_t size)
{
    return allocation_fails(size) ? NULL : __real_realloc(p, size);
}

NLCheckStatus __real_nl_h_custody_reserve(NLCheckedFragment *,
                                          NLCheckedFragment *, NLCustodyRole,
                                          NLCheckedNodeId, size_t);
NLCheckStatus __wrap_nl_h_custody_reserve(NLCheckedFragment *parent,
                                          NLCheckedFragment *child,
                                          NLCustodyRole role,
                                          NLCheckedNodeId site, size_t slot)
{
    if (fault == ARM_RESERVATION && role == NL_CUSTODY_ARM && slot == 1)
        fail_allocation = 1;
    const NLCheckStatus result =
        __real_nl_h_custody_reserve(parent, child, role, site, slot);
    fail_allocation = 0;
    return result;
}

NLCheckStatus __real_nl_h_custody_seal_result(NLCheckedFragment *,
                                              NLCheckedFragment *,
                                              NLCheckedNodeId,
                                              const NLSemanticContext *);
NLCheckStatus __wrap_nl_h_custody_seal_result(NLCheckedFragment *parent,
                                              NLCheckedFragment *child,
                                              NLCheckedNodeId id,
                                              const NLSemanticContext *world)
{
    size_t events = 0;
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    for (size_t i = 0; i < child->arm_count; ++i) {
        const NLCheckedFragment *arm = child->arms[i].artifact;
        for (NLCheckedNodeId j = 1; j <= arm->count; ++j) {
            if (!arithmetic(nl_checked_node_view(arm, j)->kind))
                continue;
            size_t function = 0;
            ++events;
            pre_ok &= call && !nl_h_custody_result_valid(parent, id) &&
                      nl_h_custody_if_body(arm, child, &function) == child &&
                      function == call->function &&
                      !nl_h_custody_if_body(arm, NULL, &function) &&
                      !event_valid(arm, j);
        }
    }
    pre_events += events;
    if (events && !world && fault == HELPER_SEAL)
        fail_allocation = 1;
    if (events && world && fault == ARM_WORLD)
        /* Body nodes/world, then the next arm's nodes/world. */
        fail_allocation = 4;
    if (world && fault == COPY_ENTRY_WORLD) {
        const NLCheckedNodeView *root =
            nl_checked_node_view(child, child->root);
        for (NLCheckedNodeId item = root->first_item; item;) {
            const NLCheckedNodeView *n = nl_checked_node_view(child, item);
            if (n->kind == NL_CHECKED_STATEMENT) {
                /* Entry body nodes, then the actual published-world bytes. */
                fail_allocation = 2;
                break;
            }
            item = n->next_item;
        }
    }
    const NLCheckStatus result =
        __real_nl_h_custody_seal_result(parent, child, id, world);
    fail_allocation = 0;
    return result;
}

static bool trace(const NLCheckedFragment *parent, bool entry, size_t *events)
{
    for (size_t i = 0; i < parent->body_count; ++i) {
        const NLCheckedNodeId id = parent->body_calls[i];
        NLDivisionUsizePlan plan;
        REQUIRE(nl_h_custody_result_valid(parent, id) &&
                nl_quantity_usize_call_valid(parent, id, &plan) &&
                (!entry || nl_h_custody_result_world_present(parent, id)));
        const NLCheckedFragment *body = plan.body;
        const NLSemanticContext *c = body->context;
        printf("FUNCTION %zu %s\n", plan.function,
               c->functions[plan.function - 1].name);
        for (NLCheckedNodeId j = 1; j <= body->count; ++j) {
            const NLCheckedNodeView *n = nl_checked_node_view(body, j);
            if (entry && n->kind == NL_CHECKED_BINDING)
                printf("BIND %zu %s\n", n->symbol,
                       c->bindings[n->symbol - 1].name);
            if (n->kind == NL_CHECKED_IF) {
                NLSemanticValueView joined;
                REQUIRE(
                    nl_semantic_value_view(c, n->results[0].value, &joined) &&
                    !joined.scalar_known && !joined.scalar_value);
                printf("JOIN %zu Unknown\n", j);
            }
        }
        for (size_t a = 0; a < body->arm_count; ++a) {
            const NLCheckedFragment *arm = body->arms[a].artifact;
            for (NLCheckedNodeId j = 1; j <= arm->count; ++j) {
                const NLCheckedNodeView *n = nl_checked_node_view(arm, j);
                if (!arithmetic(n->kind))
                    continue;
                size_t function = 0;
                REQUIRE(event_valid(arm, j) &&
                        nl_h_custody_if_body(arm, NULL, &function) == body &&
                        function == plan.function);
                ++*events;
                printf("EVENT %c %d %zu\n",
                       n->kind == NL_CHECKED_USIZE_DIVISION ? '/' : '%',
                       n->scalar_result.known, n->scalar_result.value);
            }
        }
        REQUIRE(trace(body, false, events));
    }
    for (size_t a = 0; a < parent->arm_count; ++a)
        REQUIRE(trace(parent->arms[a].artifact, false, events));
    return true;
}

static bool withheld(const NLCheckedFragment *root, NLCheckStatus expected)
{
    char sentinel[] = "keep\0arm";
    char original[sizeof(sentinel)];
    memcpy(original, sentinel, sizeof(sentinel));
    char *output = sentinel;
    size_t length = sizeof(sentinel);
    const NLCheckStatus status = nl_quantity_usize_c17(root, &output, &length);
    const bool unchanged = output == sentinel && length == sizeof(sentinel) &&
                           !memcmp(original, sentinel, sizeof(sentinel));
    if (output != sentinel)
        free(output);
    return status == expected && unchanged;
}

static bool retry(const NLCheckedFragment *root, const char *pristine,
                  size_t size)
{
    char *out = NULL;
    size_t length = 0;
    const NLCheckStatus status = nl_quantity_usize_c17(root, &out, &length);
    const bool ok =
        status == NL_CHECK_OK && out && length &&
        (!pristine || (length == size && !memcmp(pristine, out, size)));
    free(out);
    return ok;
}

static bool rejected(const NLCheckedFragment *root,
                     const NLCheckedFragment *arm, NLCheckedNodeId event)
{
    NLDivisionUsizePlan plan;
    return !nl_checked_usize_division_valid(arm, event, NULL) &&
           !nl_quantity_usize_call_valid(root, root->root, &plan) &&
           withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
}

static NLCheckedNodeId division_after(const NLCheckedFragment *f,
                                      NLCheckedNodeId after)
{
    for (NLCheckedNodeId id = after + 1; id <= f->count; ++id)
        if (nl_checked_node_view(f, id)->kind == NL_CHECKED_USIZE_DIVISION)
            return id;
    return 0;
}

static bool identity(const Program *p, const Program *other)
{
    REQUIRE(p->root->body_count == 1 && other->root->body_count == 1 &&
            p->root->bodies[0]->body_count &&
            other->root->bodies[0]->body_count);
    NLCheckedFragment *main = p->root->bodies[0];
    NLCheckedFragment *body = main->bodies[0];
    NLCheckedFragment *alien_body = other->root->bodies[0]->bodies[0];
    REQUIRE(body->arm_count == 4 && alien_body->arm_count == 4);
    NLCheckedFragment *arm = body->arms[2].artifact;
    NLCheckedFragment *sibling = body->arms[3].artifact;
    const NLCheckedFragment *alien_arm = alien_body->arms[2].artifact;
    const NLCheckedNodeId event = division_after(arm, 0);
    const NLCheckedNodeId second = division_after(arm, event);
    NLUsizeDivisionView actual, foreign, unknown;
    REQUIRE(event && second && event_valid(arm, event) &&
            event_valid(arm, second) && event_valid(alien_arm, event) &&
            nl_checked_usize_division_view(arm, event, &actual) &&
            nl_checked_usize_division_view(arm, second, &unknown) &&
            nl_checked_usize_division_view(alien_arm, event, &foreign) &&
            actual.quotient_known && !unknown.quotient_known &&
            foreign.quotient_known && !actual.quotient && !foreign.quotient &&
            !unknown.quotient && actual.result == foreign.result &&
            arm->source != alien_arm->source &&
            body->body_owner != alien_body->body_owner &&
            !nl_checked_usize_division_valid(arm, event, &foreign) &&
            !nl_checked_usize_division_valid(arm, event, &unknown));
    char *pristine = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(p->root, &pristine, &length) == NL_CHECK_OK);

    const NLSource *source = arm->source;
    arm->source = alien_arm->source;
    const bool source_rejected = rejected(p->root, arm, event);
    arm->source = source;

    NLFunctionBody *owner = body->body_owner;
    body->body_owner = alien_body->body_owner;
    const bool owner_rejected = rejected(p->root, arm, event);
    body->body_owner = owner;

    const NLSemanticContext *world = arm->context;
    arm->context = sibling->context;
    const bool world_rejected = rejected(p->root, arm, event);
    arm->context = world;

    const NLCheckedNodeId site = arm->if_parent_node;
    arm->if_parent_node = body->arms[0].match;
    const bool site_rejected = rejected(p->root, arm, event);
    arm->if_parent_node = site;

    arm->if_arm_index = 1;
    const bool index_rejected = rejected(p->root, arm, event);
    arm->if_arm_index = 0;

    NLCheckedArm slot = body->arms[2];
    body->arms[2] = body->arms[3];
    body->arms[3] = slot;
    const bool slots_rejected = rejected(p->root, arm, event);
    body->arms[3] = body->arms[2];
    body->arms[2] = slot;

    NLCheckedFragment borrowed = *arm;
    body->arms[2].artifact = &borrowed;
    const bool observation_rejected = nl_h_custody_borrow_valid(&borrowed) &&
                                      rejected(p->root, &borrowed, event) &&
                                      rejected(p->root, arm, event);
    body->arms[2] = slot;

    main->bodies[0] = other->root->bodies[0]->bodies[0];
    const bool body_rejected = rejected(p->root, arm, event);
    main->bodies[0] = body;

    arm->body_owner = owner;
    arm->if_parent = NULL;
    const bool disguise_rejected = rejected(p->root, arm, event);
    arm->body_owner = NULL;
    arm->if_parent = body;

    NLValueId *result = &arm->nodes[event - 1].results[0].value;
    *result = unknown.result;
    const bool event_rejected = rejected(p->root, arm, event);
    *result = actual.result;

    const bool restored =
        event_valid(arm, event) && retry(p->root, pristine, length);
    free(pristine);
    printf("IDENTITY source=%d owner=%d sibling_world=%d site=%d index=%d "
           "slots=%d "
           "observation=%d foreign_body=%d disguise=%d event=%d retry=%d\n",
           source_rejected, owner_rejected, world_rejected, site_rejected,
           index_rejected, slots_rejected, observation_rejected, body_rejected,
           disguise_rejected, event_rejected, restored);
    return source_rejected && owner_rejected && world_rejected &&
           site_rejected && index_rejected && slots_rejected &&
           observation_rejected && body_rejected && disguise_rejected &&
           event_rejected && restored;
}

static bool check_fault(Program *p, Fault at)
{
    NLSemanticContext *c = p->context;
    NLSymbolId symbol = 0;
    const NLScalarValue seed = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                123};
    REQUIRE(nl_semantic_seed_scalar(c, "p1605_anchor", seed, &symbol) ==
                NL_CHECK_OK &&
            symbol);
    const NLSemanticBindingView anchor = c->bindings[symbol - 1].view;
    REQUIRE(anchor.place && anchor.value);
    unsigned char context[sizeof(*c)], binding[sizeof(*c->bindings)],
        place[sizeof(*c->places)], value[sizeof(*c->values)];
    memcpy(context, c, sizeof(context));
    memcpy(binding, c->bindings + symbol - 1, sizeof(binding));
    memcpy(place, c->places + anchor.place - 1, sizeof(place));
    memcpy(value, c->values + anchor.value - 1, sizeof(value));

    fault = at;
    const NLCheckStatus status = check(p);
    fault = NO_FAULT;
    printf("OOM allocations=%zu hits=%zu bytes=%zu status=%d\n", visits, hits,
           bytes, status);
    const size_t allocations = at == ARM_WORLD          ? 4
                               : at == COPY_ENTRY_WORLD ? 2
                                                        : 1;
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && !p->root && hits == 1 &&
            bytes && visits == allocations &&
            !memcmp(context, c, sizeof(context)) &&
            !memcmp(binding, c->bindings + symbol - 1, sizeof(binding)) &&
            !memcmp(place, c->places + anchor.place - 1, sizeof(place)) &&
            !memcmp(value, c->values + anchor.value - 1, sizeof(value)) &&
            nl_sem_validate(c) == NL_CHECK_OK);
    pre_events = 0;
    size_t events = 0;
    REQUIRE(check(p) == NL_CHECK_OK && nl_h_custody_validate(p->root) &&
            trace(p->root, true, &events) && pre_ok && events &&
            pre_events == events && retry(p->root, NULL, 0));
    puts("OOM checked output and caller state preserved; normal CHECK/C17 "
         "retry");
    return true;
}

static bool stage_fault(const NLCheckedFragment *root)
{
    char *pristine = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(root, &pristine, &length) == NL_CHECK_OK);
    fail_allocation = 1;
    const bool preserved = withheld(root, NL_CHECK_OUT_OF_MEMORY);
    fail_allocation = 0;
    const bool restored = retry(root, pristine, length);
    free(pristine);
    printf("OOM staging allocations=%zu hits=%zu bytes=%zu\n", visits, hits,
           bytes);
    return preserved && visits == 1 && hits == 1 && bytes > length && restored;
}

static bool copy_layout(const NLCheckedFragment *, NLCheckedNodeId);

static bool copy_choice(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    const NLCheckedNodeView *n = nl_checked_node_view(f, id);
    if (!n || n->kind != NL_CHECKED_IF)
        return true;
    for (size_t i = 0; i < n->normal_arms; ++i) {
        const NLCheckedFragment *arm = nl_checked_if_arm(f, id, i);
        const NLCheckedNodeView *r =
            arm ? nl_checked_node_view(arm, arm->root) : NULL;
        REQUIRE(r && r->kind == NL_CHECKED_IF_ARM &&
                arm->context != f->context && copy_layout(arm, r->initializer));
    }
    return true;
}

static bool copy_layout(const NLCheckedFragment *f, NLCheckedNodeId block)
{
    const NLSemanticContext *c = f->context;
    const NLCheckedNodeView *root = nl_checked_node_view(f, block);
    REQUIRE(root && root->kind == NL_CHECKED_BLOCK);
    for (NLCheckedNodeId id = root->first_item; id;) {
        const NLCheckedNodeView *item = nl_checked_node_view(f, id);
        REQUIRE(item && (!item->next_item || item->next_item > id));
        if (item->kind == NL_CHECKED_STATEMENT) {
            const NLCheckedNodeView *read =
                nl_checked_node_view(f, item->initializer);
            REQUIRE(
                !item->result_count && !item->results[0].value &&
                !item->terminates && item->type == nl_semantic_unit_type(c) &&
                item->initializer < id && read &&
                read->kind == NL_CHECKED_IDENTIFIER &&
                read->value_use == NL_VALUE_COPIED && read->result_count == 1 &&
                read->symbol && read->symbol <= c->binding_count);
            const NLBindingEntry *binding = &c->bindings[read->symbol - 1];
            const NLValueId copied = read->results[0].value;
            NLSemanticValueView original, discarded;
            REQUIRE(read->type == nl_semantic_core_type(c, NL_TYPE_USIZE) &&
                    binding->view.value < copied &&
                    nl_semantic_value_view(c, binding->view.value, &original) &&
                    nl_semantic_value_view(c, copied, &discarded) &&
                    !discarded.owner_place &&
                    discarded.carrier == NL_CARRIER_ENDED &&
                    original.scalar_known == discarded.scalar_known &&
                    original.scalar_value == discarded.scalar_value);
            printf("READ %zu %s %d %zu %zu %zu\n", read->symbol, binding->name,
                   discarded.scalar_known, discarded.scalar_value,
                   binding->view.value, copied);
        } else {
            REQUIRE(copy_choice(f, item->initializer));
        }
        id = item->next_item;
    }
    return copy_choice(f, root->tail);
}

static bool copy_layout_calls(const NLCheckedFragment *f)
{
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedNodeView *call =
            nl_checked_node_view(f, f->body_calls[i]);
        const NLCheckedFragment *body = f->bodies[i];
        printf("READS %zu\n", call->function);
        REQUIRE(copy_layout(body, body->root) && copy_layout_calls(body));
    }
    for (size_t i = 0; i < f->arm_count; ++i)
        REQUIRE(copy_layout_calls(f->arms[i].artifact));
    return true;
}

static bool copy_identity(const NLCheckedFragment *root)
{
    REQUIRE(root->body_count == 1 && root->bodies[0]->body_count == 2);
    NLCheckedFragment *body = root->bodies[0]->bodies[0];
    REQUIRE(body->arm_count == 2);
    NLCheckedFragment *arm = body->arms[0].artifact;
    const NLCheckedNodeView *arm_root = nl_checked_node_view(arm, arm->root);
    REQUIRE(arm_root && arm_root->kind == NL_CHECKED_IF_ARM);
    const NLCheckedNodeView *block =
        nl_checked_node_view(arm, arm_root->initializer);
    REQUIRE(block && block->kind == NL_CHECKED_BLOCK);
    const NLCheckedNodeView *first =
        nl_checked_node_view(arm, block->first_item);
    REQUIRE(first && nl_checked_node_view(arm, first->next_item));
    const NLCheckedNodeId statement = first->next_item;
    NLCheckedNodeView *item = &arm->nodes[statement - 1];
    const NLCheckedNodeId initializer = item->initializer;
    REQUIRE(item->kind == NL_CHECKED_STATEMENT &&
            nl_checked_node_view(arm, initializer));
    NLCheckedNodeView *read = &arm->nodes[initializer - 1];
    const NLCheckedNodeView *tail = nl_checked_node_view(arm, block->tail);
    const NLValueId value = read->results[0].value;
    NLSemanticValueView discarded, forwarded;
    const NLSemanticContext *world = arm->context;
    REQUIRE(tail && nl_semantic_value_view(world, value, &discarded) &&
            nl_semantic_value_view(world, tail->results[0].value, &forwarded) &&
            value != tail->results[0].value && discarded.scalar_known &&
            forwarded.scalar_known &&
            discarded.scalar_value == forwarded.scalar_value);
    char *pristine = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(root, &pristine, &length) == NL_CHECK_OK);

    item->initializer = block->tail;
    const bool occurrence = withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
    item->initializer = initializer;
    read->results[0].value = tail->results[0].value;
    const bool package = withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
    read->results[0].value = value;

    const NLCheckedNodeId next = item->next_item;
    item->next_item = block->first_item;
    const bool order = withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
    item->next_item = next;
    arm->context = body->arms[1].artifact->context;
    const bool branch = withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
    arm->context = world;

    NLSemanticContext *clone = NULL;
    const bool cloned = nl_sem_clone(world, &clone) == NL_CHECK_OK;
    if (cloned)
        arm->context = clone;
    const bool foreign =
        cloned && withheld(root, NL_CHECK_SEMANTIC_UNSUPPORTED);
    arm->context = world;
    nl_semantic_destroy(clone);

    const bool restored = retry(root, pristine, length);
    free(pristine);
    printf("COPY IDENTITY occurrence=%d package=%d order=%d sibling=%d "
           "clone=%d byte_retry=%d\n",
           occurrence, package, order, branch, foreign, restored);
    return occurrence && package && order && branch && foreign && restored;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    Program p = {0}, other = {0};
    const char *mode = argv[1];
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(mode, "arm-reservation-oom"))
        ok = check_fault(&p, ARM_RESERVATION);
    else if (ok && !strcmp(mode, "helper-seal-oom"))
        ok = check_fault(&p, HELPER_SEAL);
    else if (ok && !strcmp(mode, "arm-world-oom"))
        ok = check_fault(&p, ARM_WORLD);
    else if (ok && !strcmp(mode, "copy-world-oom"))
        ok = check_fault(&p, COPY_ENTRY_WORLD);
    else if (ok) {
        ok = check(&p) == NL_CHECK_OK && nl_h_custody_validate(p.root);
        if (ok && !strcmp(mode, "trace")) {
            size_t events = 0;
            ok = trace(p.root, true, &events) && pre_ok && pre_events == events;
            printf("PRE events=%zu POST events=%zu\n", pre_events, events);
        } else if (ok && !strcmp(mode, "copy-trace")) {
            size_t events = 0;
            ok = trace(p.root, true, &events) && pre_ok &&
                 pre_events == events && copy_layout_calls(p.root);
        } else if (ok && !strcmp(mode, "copy-identity")) {
            ok = copy_identity(p.root);
        } else if (ok && !strcmp(mode, "identity"))
            ok = prepare(&other, argv[2]) && check(&other) == NL_CHECK_OK &&
                 pre_ok && identity(&p, &other);
        else if (ok && !strcmp(mode, "c17-staging-oom"))
            ok = stage_fault(p.root);
        else
            ok = false;
    }
    dispose(&other);
    dispose(&p);
    return ok ? 0 : 1;
}
