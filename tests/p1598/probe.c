#include "division_usize_internal.h"
#include "h_origin_internal.h"
#include "newlang/parser.h"

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
    NLSemanticContext *context;
    NLSource *source;
    NLParser *parser;
    NLSyntaxTree *expression;
    NLCheckedFragment *entry;
} Program;

static bool prepare(Program *p, const char *file)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    bool ok =
        nl_source_load(file, &source) == NL_SOURCE_OK &&
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
           nl_source_create("main()", 6, "p1598-entry", &p->source) ==
               NL_SOURCE_OK &&
           nl_parser_create(p->source, &p->parser) == NL_PARSE_OK &&
           nl_parser_parse_expression_fragment(p->parser, &p->expression,
                                               NULL) == NL_PARSE_OK;
}

static NLCheckStatus check(Program *p)
{
    NLCheckDiagnostic diagnostic = {0};
    const NLCheckStatus status = nl_semantic_check_expression(
        p->context, p->expression, &p->entry, &diagnostic);
    printf("CHECK %d %s\n", status,
           diagnostic.diagnostic.code ? diagnostic.diagnostic.code : "-");
    return status;
}

typedef struct {
    NLSymbolId formal, binding;
    NLValueId value;
    size_t copies;
} CopyFlow;

static bool copy_at(const NLCheckedFragment *body, NLCheckedNodeId id,
                    const NLSyntaxNode *syntax, const char *parameter,
                    CopyFlow *flow)
{
    const NLSemanticContext *c = body->context;
    const NLSyntaxView *s = nl_syntax_node_view(syntax);
    const NLCheckedNodeView *use = nl_checked_node_view(body, id);
    NLSourceView name;
    NLSemanticValueView from, produced;
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    REQUIRE(s && s->kind == NL_SYNTAX_EXPR_NAME && use &&
            use->kind == NL_CHECKED_IDENTIFIER &&
            use->value_use == NL_VALUE_COPIED && use->type == usize &&
            use->result_count == 1 && use->results[0].type == usize &&
            use->symbol && use->symbol <= c->binding_count &&
            use->span.start_byte == s->span.start_byte &&
            use->span.end_byte == s->span.end_byte &&
            nl_source_view(body->source, s->data.name, &name));
    const NLBindingEntry *binding = &c->bindings[use->symbol - 1];
    REQUIRE(strlen(binding->name) == name.length &&
            !memcmp(binding->name, name.bytes, name.length));
    if (!flow->formal) {
        REQUIRE(!strcmp(binding->name, parameter));
        flow->formal = use->symbol;
        flow->binding = use->symbol;
    }
    REQUIRE(flow->binding == use->symbol &&
            binding->view.value == flow->value &&
            flow->value < use->results[0].value &&
            nl_semantic_value_view(c, flow->value, &from) &&
            nl_semantic_value_view(c, use->results[0].value, &produced) &&
            from.type == usize && produced.type == usize &&
            from.dependencies == NL_DEPENDENCY_FREE &&
            produced.dependencies == NL_DEPENDENCY_FREE &&
            !from.value_dependency_count && !produced.value_dependency_count &&
            from.scalar_known == produced.scalar_known &&
            from.scalar_value == produced.scalar_value);
    flow->value = use->results[0].value;
    ++flow->copies;
    return true;
}

static bool copy_result(const NLCheckedFragment *parent, NLCheckedNodeId id,
                        const NLCheckedFragment *body,
                        const NLFunctionEntry *fn,
                        const NLDivisionUsizePlan *plan)
{
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    const NLCheckedNodeView *arg =
        nl_checked_node_view(parent, call->first_argument);
    const NLCheckedNodeView *block = nl_checked_node_view(body, body->root);
    const NLSyntaxView *source =
        nl_syntax_node_view(nl_syntax_tree_root(fn->body->syntax));
    const NLSemanticContext *c = parent->context;
    const NLTypeId usize = nl_semantic_core_type(c, NL_TYPE_USIZE);
    REQUIRE(fn->count == 1 && fn->result == usize && body->context == c &&
            !body->arm_count && !body->body_count && block &&
            block->kind == NL_CHECKED_BLOCK && source &&
            source->kind == NL_SYNTAX_BLOCK && arg &&
            call->argument_count == 1 && !arg->next_argument &&
            arg->type == usize && arg->parameter_type == usize &&
            arg->result_count == 1 && call->result_count == 1 &&
            call->results[0].type == usize);
    CopyFlow flow = {.value = arg->results[0].value};
    const NLSyntaxNode *item = source->data.block.items;
    const NLSyntaxNode *result_source = source->data.block.tail;
    NLCheckedNodeId result_node = block->tail;
    for (NLCheckedNodeId at = block->first_item; at;) {
        const NLCheckedNodeView *node = nl_checked_node_view(body, at);
        const NLSyntaxView *s = nl_syntax_node_view(item);
        REQUIRE(node && s && (!node->next_item || node->next_item > at));
        item = nl_syntax_next_argument(item);
        if (node->kind == NL_CHECKED_RETURN) {
            REQUIRE(s->kind == NL_SYNTAX_RETURN && !result_source && !item &&
                    !node->next_item && block->terminates &&
                    nl_control_exits_count(body->exits) == 1 &&
                    nl_control_exits_only_return(body->exits,
                                                 body->function_target) &&
                    node->returned.value == block->returned.value);
            result_source = s->data.statement.expression;
            result_node = node->initializer;
        } else {
            REQUIRE(node->kind == NL_CHECKED_BINDING &&
                    s->kind == NL_SYNTAX_BINDING && node->symbol &&
                    node->symbol <= c->binding_count &&
                    copy_at(body, node->initializer,
                            s->data.binding.initializer,
                            fn->body->parameter_names[0], &flow) &&
                    c->bindings[node->symbol - 1].view.value == flow.value);
            flow.binding = node->symbol;
        }
        at = node->next_item;
    }
    REQUIRE(!item && copy_at(body, result_node, result_source,
                             fn->body->parameter_names[0], &flow));
    const NLCheckedResult result =
        block->terminates ? block->returned : block->results[0];
    REQUIRE(result.type == usize && result.value == flow.value &&
            result.value == call->results[0].value &&
            (!plan ||
             (plan->result == result_node && plan->formal == flow.formal)));
    const NLSemanticValueView facts = c->values[result.value - 1];
    printf("COPY %s actual=%zu formal=%zu result=%zu copies=%zu known=%d "
           "number=%zu\n",
           fn->name, arg->results[0].value, flow.formal, result.value,
           flow.copies, facts.scalar_known, facts.scalar_value);
    return true;
}

static bool trace(const NLCheckedFragment *f, const char *leaf, bool strict,
                  size_t depth, size_t *leaves)
{
    REQUIRE(f && depth < NL_SEMANTIC_MAX_DEPTH);
    for (size_t i = 0; i < f->body_count; ++i) {
        const NLCheckedNodeId id = f->body_calls[i];
        const NLCheckedNodeView *call = nl_checked_node_view(f, id);
        const NLCheckedFragment *body = nl_checked_call_body(f, id);
        REQUIRE(call && body && body == f->bodies[i] && call->body_backed &&
                call->function && call->function <= f->context->function_count);
        for (size_t earlier = 0; earlier < i; ++earlier)
            REQUIRE(f->bodies[earlier] != body && f->body_calls[earlier] != id);
        const NLFunctionEntry *fn = &f->context->functions[call->function - 1];
        REQUIRE(body->body_owner == fn->body &&
                body->source == fn->body->source);
        NLDivisionUsizePlan plan = {0};
        const bool sealed = nl_h_custody_result_valid(f, id);
        const bool valid = nl_quantity_usize_call_valid(f, id, &plan);
        printf("CALL %s depth=%zu ordinal=%zu node=%zu sealed=%d plan=%d\n",
               fn->name, depth, i, id, sealed, valid);
        REQUIRE(!strict ||
                (sealed && valid && plan.body == body &&
                 (depth || nl_h_custody_result_world_present(f, id))));
        if (!strcmp(fn->name, leaf)) {
            REQUIRE(copy_result(f, id, body, fn, valid ? &plan : NULL));
            ++*leaves;
        }
        REQUIRE(trace(body, leaf, strict, depth + 1, leaves));
    }
    return true;
}

static bool withheld(const NLCheckedFragment *entry, NLCheckStatus expected)
{
    char sentinel[] = "p1598 output";
    char *out = sentinel;
    size_t length = 47;
    const NLCheckStatus status = nl_quantity_usize_c17(entry, &out, &length);
    bool preserved =
        out == sentinel && length == 47 && !strcmp(sentinel, "p1598 output");
    if (out != sentinel)
        free(out);
    return status == expected && preserved;
}

static bool emitted(const NLCheckedFragment *entry, const char *expected,
                    size_t size)
{
    char *out = NULL;
    size_t bytes = 0;
    const bool ok =
        nl_quantity_usize_c17(entry, &out, &bytes) == NL_CHECK_OK && out &&
        bytes && (!expected || (size == bytes && !memcmp(out, expected, size)));
    free(out);
    return ok;
}

static bool identity(Program *p)
{
    NLCheckedFragment *main = (NLCheckedFragment *)nl_checked_call_body(
        p->entry, nl_checked_root(p->entry));
    REQUIRE(main && main->body_count == 2);
    const NLCheckedNodeId first = main->body_calls[0],
                          other = main->body_calls[1];
    NLCheckedNodeView *call = &main->nodes[first - 1];
    const NLCheckedNodeView *second = &main->nodes[other - 1];
    const NLCheckedNodeId actual = call->first_argument;
    const NLCheckedNodeId foreign = second->first_argument;
    const NLCheckedNodeView *a = nl_checked_node_view(main, actual);
    const NLCheckedNodeView *b = nl_checked_node_view(main, foreign);
    NLDivisionUsizePlan plan, second_plan;
    REQUIRE(actual != foreign && call->function == second->function &&
            nl_quantity_usize_call_valid(main, first, &plan) &&
            nl_quantity_usize_call_valid(main, other, &second_plan) &&
            plan.body != second_plan.body &&
            plan.formal != second_plan.formal &&
            a->results[0].value != b->results[0].value);
    NLSemanticValueView av, bv;
    REQUIRE(nl_semantic_value_view(p->context, a->results[0].value, &av) &&
            nl_semantic_value_view(p->context, b->results[0].value, &bv) &&
            av.scalar_known && bv.scalar_known &&
            av.scalar_value == bv.scalar_value);
    char *pristine = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(p->entry, &pristine, &length) == NL_CHECK_OK);

    call->first_argument = foreign;
    const bool rejects_actual =
        withheld(p->entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    call->first_argument = actual;
    NLSemanticBindingView *formal = &p->context->bindings[plan.formal - 1].view;
    const NLValueId formal_value = formal->value;
    formal->value = b->results[0].value;
    const bool rejects_formal =
        withheld(p->entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    formal->value = formal_value;
    NLCheckedFragment *body = main->bodies[0];
    NLCheckedNodeView *copy = &body->nodes[plan.result - 1];
    const NLValueId copy_value = copy->results[0].value;
    copy->results[0].value = second->results[0].value;
    const bool rejects_copy = withheld(p->entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    copy->results[0].value = copy_value;
    main->bodies[0] = main->bodies[1];
    const bool rejects_body = withheld(p->entry, NL_CHECK_SEMANTIC_UNSUPPORTED);
    main->bodies[0] = body;

    const bool retry = emitted(p->entry, pristine, length);
    free(pristine);
    REQUIRE(rejects_actual && rejects_formal && rejects_copy && rejects_body &&
            retry);
    puts("IDENTITY same-valued other actual/formal/Copy result/body refused, "
         "output held, byte-exact restore");
    return true;
}

typedef enum {
    NONE,
    LEAF_SEAL,
    ENTRY_SEAL
} Site;
static Site fault;
static bool fail_malloc;
static size_t visits, hits, size_requested;

void *__real_malloc(size_t);
void *__wrap_malloc(size_t size)
{
    if (!fail_malloc)
        return __real_malloc(size);
    ++hits;
    size_requested = size;
    return NULL;
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
    const NLCheckedNodeView *call = nl_checked_node_view(parent, id);
    if (call && ((fault == LEAF_SEAL && !world && call->argument_count == 1) ||
                 (fault == ENTRY_SEAL && world && !call->argument_count))) {
        ++visits;
        fail_malloc = true;
        const NLCheckStatus status =
            __real_nl_h_custody_seal_result(parent, child, id, world);
        fail_malloc = false;
        return status;
    }
    return __real_nl_h_custody_seal_result(parent, child, id, world);
}

static bool seal_fault(Program *p, Site site)
{
    NLSemanticContext *c = p->context;
    NLSymbolId anchor = 0;
    const NLScalarValue scalar = {nl_semantic_core_type(c, NL_TYPE_USIZE), true,
                                  79};
    REQUIRE(nl_semantic_seed_scalar(c, "p1598_anchor", scalar, &anchor) ==
                NL_CHECK_OK &&
            anchor);
    const NLSemanticBindingView before = c->bindings[anchor - 1].view;
    REQUIRE(before.place && before.value);
    unsigned char context[sizeof(*c)], binding[sizeof(*c->bindings)],
        place[sizeof(*c->places)], value[sizeof(*c->values)];
    memcpy(context, c, sizeof(context));
    memcpy(binding, c->bindings + anchor - 1, sizeof(binding));
    memcpy(place, c->places + before.place - 1, sizeof(place));
    memcpy(value, c->values + before.value - 1, sizeof(value));
    fault = site;
    const NLCheckStatus status = check(p);
    fault = NONE;
    printf("OOM %s visits=%zu hits=%zu bytes=%zu status=%d\n",
           site == LEAF_SEAL ? "leaf-seal" : "entry-seal", visits, hits,
           size_requested, status);
    REQUIRE(status == NL_CHECK_OUT_OF_MEMORY && visits == 1 && hits == 1 &&
            size_requested && !p->entry &&
            !memcmp(context, c, sizeof(context)) &&
            !memcmp(binding, c->bindings + anchor - 1, sizeof(binding)) &&
            !memcmp(place, c->places + before.place - 1, sizeof(place)) &&
            !memcmp(value, c->values + before.value - 1, sizeof(value)) &&
            nl_sem_validate(c) == NL_CHECK_OK);
    size_t leaves = 0;
    REQUIRE(check(p) == NL_CHECK_OK && nl_h_custody_validate(p->entry) &&
            trace(p->entry, "carry", true, 0, &leaves) && leaves == 2 &&
            emitted(p->entry, NULL, 0));
    puts("OOM no published artifact, caller bytes held, normal retry");
    return true;
}

static bool stage_fault(const NLCheckedFragment *entry)
{
    char *pristine = NULL;
    size_t length = 0;
    REQUIRE(nl_quantity_usize_c17(entry, &pristine, &length) == NL_CHECK_OK);
    fail_malloc = true;
    const bool preserved = withheld(entry, NL_CHECK_OUT_OF_MEMORY);
    fail_malloc = false;
    const bool retry = emitted(entry, pristine, length);
    free(pristine);
    printf("OOM staging hits=%zu bytes=%zu\n", hits, size_requested);
    REQUIRE(preserved && hits == 1 && size_requested > length && retry);
    puts("OOM pointer, length and bytes held, byte-exact retry");
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    Program p = {0};
    const char *mode = argv[1];
    bool ok = prepare(&p, argv[2]);
    if (ok && !strcmp(mode, "leaf-oom"))
        ok = seal_fault(&p, LEAF_SEAL);
    else if (ok && !strcmp(mode, "entry-oom"))
        ok = seal_fault(&p, ENTRY_SEAL);
    else if (ok) {
        ok = check(&p) == NL_CHECK_OK && nl_h_custody_validate(p.entry);
        if (ok && (!strcmp(mode, "baseline") || !strcmp(mode, "strict"))) {
            size_t leaves = 0;
            ok = trace(p.entry, argv[3], !strcmp(mode, "strict"), 0, &leaves) &&
                 leaves != 0;
        } else if (ok && !strcmp(mode, "identity"))
            ok = identity(&p);
        else if (ok && !strcmp(mode, "staging-oom"))
            ok = stage_fault(p.entry);
        else
            ok = false;
    }
    nl_checked_destroy(p.entry);
    nl_syntax_tree_destroy(p.expression);
    nl_parser_destroy(p.parser);
    nl_source_destroy(p.source);
    nl_semantic_destroy(p.context);
    return ok ? 0 : 1;
}
