/* SPECULATIVE PR533: concrete structural builtins, no user nominal/generic
 * machinery and no value/place/owner issuance in the type registry. */
#include "semantic_internal.h"
#include <stdlib.h>
#include <string.h>
static char *owned(const char *s)
{
    char *p = malloc(strlen(s) + 1);
    if (p)
        memcpy(p, s, strlen(s) + 1);
    return p;
}
NLCheckStatus nl_array4_registry(NLSemanticContext *c, bool array,
                                 NLTypeId *out)
{
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    NLTypeId target = u8;
    if (array) {
        NLCheckStatus s = nl_array4_registry(c, false, &target);
        if (s != NL_CHECK_OK)
            return s;
    }
    for (size_t i = 0; i < c->type_count; ++i)
        if (array ? (c->types[i].view.kind == NL_TYPE_ARRAY &&
                     c->types[i].view.target == target &&
                     c->types[i].view.element_count == 4)
                  : c->types[i].option_target == u8) {
            *out = i + 1;
            return NL_CHECK_OK;
        }
    if (c->type_count >= NL_SEMANTIC_MAX_ENTRIES)
        return NL_CHECK_RESOURCE_LIMIT;
    NLTypeEntry t = {
        .option_target = array ? 0 : u8,
        .view = {.kind = array ? NL_TYPE_ARRAY : NL_TYPE_SUM,
                 .is_copy = c->types[target - 1].view.is_copy,
                 .is_discardable = c->types[target - 1].view.is_discardable,
                 .target = array ? target : 0,
                 .variant_count = array ? 0 : 2,
                 .field_count = array ? 4 : 0,
                 .element_count = array ? 4 : 0}};
    if (array) {
        const char *indices[] = {"0", "1", "2", "3"};
        for (size_t i = 0; i < 4; ++i) {
            t.field_names[i] = owned(indices[i]);
            t.field_types[i] = target;
            if (!t.field_names[i])
                goto oom;
        }
    } else {
        t.variant_names[0] = owned("None");
        t.variant_names[1] = owned("Some");
        t.variant_types[1] = u8;
        if (!t.variant_names[0] || !t.variant_names[1])
            goto oom;
    }
    NLTypeEntry *table =
        realloc(c->types, (c->type_count + 1) * sizeof(*table));
    if (!table)
        goto oom;
    c->types = table;
    c->types[c->type_count++] = t;
    *out = c->type_count;
    return NL_CHECK_OK;
oom:
    for (size_t i = 0; i < 4; ++i)
        free(t.field_names[i]);
    free(t.variant_names[0]);
    free(t.variant_names[1]);
    return NL_CHECK_OUT_OF_MEMORY;
}
bool nl_array4_type(const NLSemanticContext *c, NLTypeId id)
{
    if (!c || !id || id > c->type_count)
        return false;
    const NLTypeEntry *t = &c->types[id - 1];
    if (t->view.kind != NL_TYPE_ARRAY || t->view.element_count != 4 ||
        t->view.field_count != 4 || !t->view.target ||
        t->view.target > c->type_count)
        return false;
    const NLTypeEntry *o = &c->types[t->view.target - 1];
    const NLTypeId u8 = nl_semantic_core_type(c, NL_TYPE_U8);
    if (t->name || t->incomplete || t->recursive_header ||
        t->view.variant_count || o->name || o->view.kind != NL_TYPE_SUM ||
        o->option_target != u8 || o->view.variant_count != 2 ||
        o->variant_types[0] || o->variant_types[1] != u8 ||
        !o->variant_names[0] || !o->variant_names[1] ||
        strcmp(o->variant_names[0], "None") ||
        strcmp(o->variant_names[1], "Some") ||
        o->view.is_copy != c->types[u8 - 1].view.is_copy ||
        o->view.is_discardable != c->types[u8 - 1].view.is_discardable ||
        t->view.is_copy != o->view.is_copy ||
        t->view.is_discardable != o->view.is_discardable)
        return false;
    for (size_t i = 0; i < 4; ++i)
        if (t->field_types[i] != t->view.target)
            return false;
    return true;
}

#include "newlang/array4.h"
#include "newlang/lexer.h"

static bool describe(const NLCheckedFragment *, NLArray4Construction *);

NLCheckStatus nl_array4_capture(NLCheckedFragment *f, NLCheckedNodeId binding,
                                const NLSemanticContext *c)
{
    if (f->array4_world != NULL)
        return NL_CHECK_ANALYSIS_PRECISION_LIMIT;
    NLSemanticContext *snapshot = NULL;
    NLCheckStatus s = nl_sem_clone(c, &snapshot);
    if (s != NL_CHECK_OK)
        return s;
    f->array4_world = snapshot;
    f->array4_binding = binding;
    f->array4_issued = malloc(sizeof(*f->array4_issued));
    if (f->array4_issued == NULL)
        return NL_CHECK_OUT_OF_MEMORY;
    if (!describe(f, f->array4_issued))
        return NL_CHECK_INTERNAL_ERROR;
    return NL_CHECK_OK;
}
static bool token_is(const NLSource *s, NLToken t, const char *word)
{
    NLSourceView bytes;
    return nl_source_view(s, t.span, &bytes) && bytes.length == strlen(word) &&
           memcmp(bytes.bytes, word, bytes.length) == 0;
}
/* Read actual source tokens, including source-order positions for each
 * constructor. Comments are handled by the existing source lexer, not by
 * deleting arbitrary source bytes. This never grants semantic rights. */
static bool source_vector(const NLCheckedFragment *f,
                          const NLCheckedNodeView *a,
                          const NLCheckedNodeView *elements[4])
{
    static const char *prefix[] = {"Array", "<", "Option", "<", "u8",
                                   ">",     ",", "4",      ">", "{"};
    static const char *none[] = {"Option", "<", "u8", ">", ":", ":", "None"};
    NLLexer lexer;
    if (!nl_lexer_init_source(&lexer, nl_checked_source(f)))
        return false;
    NLToken t;
    NLLexResult status;
    do {
        status = nl_lexer_next(&lexer, &t);
    } while (status == NL_LEX_TOKEN && t.span.start_byte < a->span.start_byte);
    if (status != NL_LEX_TOKEN || t.span.start_byte != a->span.start_byte)
        return false;
    for (size_t i = 0; i < sizeof(prefix) / sizeof(prefix[0]); ++i) {
        if (!token_is(f->source, t, prefix[i]))
            return false;
        if (nl_lexer_next(&lexer, &t) != NL_LEX_TOKEN)
            return false;
    }
    for (size_t i = 0; i < 4; ++i) {
        if (t.span.start_byte != elements[i]->span.start_byte)
            return false;
        for (size_t j = 0; j < sizeof(none) / sizeof(none[0]); ++j) {
            if (!token_is(f->source, t, none[j]))
                return false;
            if (j == 6 && t.span.end_byte != elements[i]->span.end_byte)
                return false;
            if (nl_lexer_next(&lexer, &t) != NL_LEX_TOKEN)
                return false;
        }
        if (i < 3 || token_is(f->source, t, ",")) {
            if (!token_is(f->source, t, ",") ||
                nl_lexer_next(&lexer, &t) != NL_LEX_TOKEN)
                return false;
        }
    }
    return token_is(f->source, t, "}") && t.span.end_byte == a->span.end_byte;
}

const NLSemanticContext *nl_checked_array4_world(const NLCheckedFragment *f)
{
    return f ? f->array4_world : NULL;
}

static bool describe(const NLCheckedFragment *f, NLArray4Construction *out)
{
    if (!f || !out || !f->array4_world || !f->array4_binding)
        return false;
    const NLCheckedNodeView *b = nl_checked_node_view(f, f->array4_binding);
    if (!b || b->kind != NL_CHECKED_BINDING)
        return false;
    const NLCheckedNodeView *a = nl_checked_node_view(f, b->initializer);
    const NLCheckedNodeView *receiver =
        nl_checked_node_view(f, b->first_argument);
    const NLSemanticContext *c = f->array4_world;
    if (!a || !receiver || b->symbol == 0 || b->symbol > c->binding_count)
        return false;
    const NLPlaceId root = c->bindings[b->symbol - 1].view.place;
    if (!root || root > c->place_count)
        return false;
    const NLSemanticPlaceView p = c->places[root - 1];
    if (!p.type || p.type > c->type_count || p.fixed_field_count != 4)
        return false;
    NLArray4Construction v = {.origin = f,
                              .binding = f->array4_binding,
                              .constructor = b->initializer,
                              .receiver = b->first_argument,
                              .symbol = b->symbol,
                              .array_type = p.type,
                              .element_type = c->types[p.type - 1].view.target,
                              .root = root,
                              .incarnation = p.incarnation,
                              .fact = p.current_fact,
                              .value = p.current_value};
    NLCheckedNodeId id = a->first_argument;
    for (size_t i = 0; i < 4; ++i) {
        const NLPlaceId child = p.fixed_fields[i];
        const NLCheckedNodeView *n = nl_checked_node_view(f, id);
        if (!child || child > c->place_count || !n)
            return false;
        const NLSemanticPlaceView e = c->places[child - 1];
        v.elements[i] = (NLArray4Element){
            i, id, child, e.incarnation, e.current_fact, e.current_value};
        id = n->next_argument;
    }
    *out = v;
    return true;
}

bool nl_checked_array4_view(const NLCheckedFragment *f,
                            NLArray4Construction *out)
{
    if (!f || !out || !f->array4_issued)
        return false;
    *out = *f->array4_issued;
    return true;
}

bool nl_checked_array4_valid(const NLCheckedFragment *f,
                             const NLArray4Construction *v)
{
    if (!f || !v || !f->array4_issued || v->origin != f || !f->array4_world ||
        v->binding != f->array4_binding || !f->body_owner ||
        f->source != f->body_owner->source)
        return false;
    const NLArray4Construction *issued = f->array4_issued;
    if (v->binding != issued->binding ||
        v->constructor != issued->constructor ||
        v->receiver != issued->receiver || v->symbol != issued->symbol ||
        v->array_type != issued->array_type ||
        v->element_type != issued->element_type || v->root != issued->root ||
        v->incarnation != issued->incarnation || v->fact != issued->fact ||
        v->value != issued->value)
        return false;
    for (size_t i = 0; i < 4; ++i) {
        const NLArray4Element a = v->elements[i], b = issued->elements[i];
        if (a.index != b.index || a.initializer != b.initializer ||
            a.place != b.place || a.incarnation != b.incarnation ||
            a.fact != b.fact || a.value != b.value)
            return false;
    }
    const NLSemanticContext *c = f->array4_world,
                            *final = nl_checked_context(f);
    bool registered = false;
    for (size_t i = 0; i < final->function_count && i < c->function_count;
         ++i) {
        const NLFunctionEntry a = final->functions[i], b = c->functions[i];
        if (a.body == f->body_owner && b.body == f->body_owner &&
            a.kind == NL_CHECKED_REGISTERED_CALL && b.kind == a.kind &&
            a.result == 1 && b.result == 1 && a.count == 0 && b.count == 0 &&
            !a.caller_effects && !a.hidden_dependencies)
            registered = true;
    }
    if (!registered)
        return false;
    const NLCheckedNodeView *b = nl_checked_node_view(f, v->binding),
                            *a = nl_checked_node_view(f, v->constructor),
                            *r = nl_checked_node_view(f, v->receiver);
    if (!b || !a || !r || b->kind != NL_CHECKED_BINDING || b->type != 1 ||
        b->argument_count != 1 || b->initializer != v->constructor ||
        b->first_argument != v->receiver || b->symbol != v->symbol ||
        a->kind != NL_CHECKED_ARRAY4_CONSTRUCTOR || a->argument_count != 4 ||
        a->result_count != 1 || a->terminates || a->type != v->array_type ||
        a->results[0].type != v->array_type ||
        a->results[0].value != v->value || r->kind != NL_CHECKED_RECEIVER ||
        r->symbol != v->symbol || r->type != v->array_type ||
        r->value_use != NL_VALUE_RECEIVED || r->next_argument ||
        !nl_array4_type(c, v->array_type) ||
        !nl_array4_type(final, v->array_type) ||
        c->types[v->array_type - 1].view.target != v->element_type ||
        final->types[v->array_type - 1].view.target != v->element_type ||
        !c->types[v->array_type - 1].view.is_copy ||
        !c->types[v->array_type - 1].view.is_discardable || v->symbol == 0 ||
        v->symbol > c->binding_count || !v->root || v->root > c->place_count ||
        !v->value || v->value > c->value_count)
        return false;
    const NLSemanticBindingView binding = c->bindings[v->symbol - 1].view;
    const NLSemanticPlaceView root = c->places[v->root - 1];
    const NLSemanticValueView value = c->values[v->value - 1];
    if (binding.place != v->root || binding.value != v->value ||
        binding.type != v->array_type || binding.availability != NL_AVAILABLE ||
        !root.live || !root.implicit_local || !root.independent_root ||
        root.parent_sum || root.parent_aggregate ||
        root.type != v->array_type || root.incarnation != v->incarnation ||
        root.current_fact != v->fact || root.current_value != v->value ||
        root.fixed_field_count != 4 || root.governing_domain ||
        root.placement.region || value.type != v->array_type ||
        value.field_count != 4 || value.carrier != NL_CARRIER_PLACE ||
        value.owner_place != v->root || value.domain ||
        value.allocation_region || value.dependencies != NL_DEPENDENCY_FREE ||
        value.value_dependency_count || c->place_count != 5 ||
        c->value_count != 5 || c->binding_count != 1 || c->region_count ||
        c->domain_count || c->scope_count || c->occurrence_count ||
        final->place_count < c->place_count ||
        final->places[v->root - 1].incarnation != root.incarnation)
        return false;
    const NLCheckedNodeView *elements[4];
    NLCheckedNodeId next = a->first_argument;
    for (size_t i = 0; i < 4; ++i) {
        const NLArray4Element e = v->elements[i];
        const NLCheckedNodeView *n = nl_checked_node_view(f, e.initializer);
        if (!n || e.index != i || e.initializer != next ||
            e.place != root.fixed_fields[i] || !e.place ||
            e.place > c->place_count || !e.value || e.value > c->value_count ||
            n->kind != NL_CHECKED_SUM_CONSTRUCTOR ||
            n->type != v->element_type || n->variant != 1 || n->initializer ||
            n->argument_count || n->result_count != 1 ||
            n->results[0].type != v->element_type ||
            n->results[0].value != e.value || n->terminates)
            return false;
        const NLSemanticPlaceView p = c->places[e.place - 1];
        const NLSemanticValueView x = c->values[e.value - 1];
        if (!p.live || p.independent_root || p.type != v->element_type ||
            p.parent_aggregate != v->root ||
            p.parent_incarnation != v->incarnation ||
            p.parent_field_index != i || p.incarnation != e.incarnation ||
            p.current_fact != e.fact || p.current_value != e.value ||
            p.payload_occurrence || p.parent_sum || p.governing_domain ||
            x.type != v->element_type || x.variant != 1 || x.sum_payload ||
            x.carrier != NL_CARRIER_AGGREGATE ||
            x.aggregate_owner != v->value || x.owner_place ||
            value.fields[i] != e.value || x.domain || x.allocation_region ||
            x.dependencies != NL_DEPENDENCY_FREE || x.value_dependency_count ||
            final->places[e.place - 1].incarnation != e.incarnation)
            return false;
        for (size_t j = 0; j < i; ++j)
            if (v->elements[j].place == e.place ||
                v->elements[j].value == e.value ||
                v->elements[j].initializer == e.initializer)
                return false;
        elements[i] = n;
        next = n->next_argument;
    }
    /* All five places/packages and type/parent/value bounds are checked
     * before invoking the committed-world kernel on this captured event. */
    return next == 0 && source_vector(f, a, elements) &&
           nl_sem_validate(c) == NL_CHECK_OK;
}

#include <stdio.h>
NLCheckStatus nl_checked_array4_c17(const NLCheckedFragment *entry, char **out,
                                    size_t *length)
{
    if (!entry || !out || !length || *out)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const NLCheckedNodeView *call =
        nl_checked_node_view(entry, nl_checked_root(entry));
    const NLCheckedFragment *f =
        nl_checked_call_body(entry, nl_checked_root(entry));
    if (!call || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->argument_count || call->result_count ||
        call->type != 1 || call->terminates || !f ||
        nl_checked_context(entry) != nl_checked_context(f))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const NLSemanticContext *c = nl_checked_context(f);
    if (!call->function || call->function > c->function_count)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const NLFunctionEntry fn = c->functions[call->function - 1];
    if (fn.body != f->body_owner || fn.kind != NL_CHECKED_REGISTERED_CALL ||
        fn.count || fn.result != 1 || fn.caller_effects ||
        fn.hidden_dependencies || fn.owner_receiver || fn.owner_producer ||
        fn.custody_recipient)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    NLArray4Construction p;
    if (!nl_checked_array4_view(f, &p) || !nl_checked_array4_valid(f, &p))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f)),
                            *binding = nl_checked_node_view(f, p.binding);
    const NLCheckedNodeView *tail =
        root ? nl_checked_node_view(f, root->tail) : NULL;
    if (!root || root->kind != NL_CHECKED_BLOCK || root->terminates ||
        root->type != 1 || root->result_count || root->item_count != 1 ||
        root->first_item != p.binding || !binding || binding->next_item ||
        !tail || tail->kind != NL_CHECKED_UNIT || tail->type != 1 ||
        tail->result_count || tail->terminates ||
        nl_checked_node_count(f) != 9 ||
        nl_control_exits_count(nl_checked_control_exits(f)))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    /* Each source-issued initializer was independently checked as typed None.
     * The opaque envelope/tag is this compiler's private C17 choice; Array is
     * a C array of complete envelopes, not a record of four fields. */
    static const char format[] =
        "/* SPECULATIVE PR533 original Array4, checked source only. */\n"
        "#include <stdint.h>\n"
        "enum { NL_ARRAY4_NONE = 37 };\n"
        "typedef struct { uint8_t tag; uint8_t payload; } nl_option_%zu;\n"
        "typedef nl_option_%zu nl_array_%zu[4];\n"
        "static void nl_fn_%zu(void)\n{\n"
        "    nl_array_%zu nl_local_%zu = {\n"
        "        { .tag = NL_ARRAY4_NONE },\n"
        "        { .tag = NL_ARRAY4_NONE },\n"
        "        { .tag = NL_ARRAY4_NONE },\n"
        "        { .tag = NL_ARRAY4_NONE }\n"
        "    };\n"
        "    (void)nl_local_%zu;\n}\n"
        "int main(void) { nl_fn_%zu(); return 0; }\n";
    const int n = snprintf(NULL, 0, format, p.element_type, p.element_type,
                           p.array_type, call->function, p.array_type, p.symbol,
                           p.symbol, call->function);
    if (n < 0)
        return NL_CHECK_RESOURCE_LIMIT;
    char *text = malloc((size_t)n + 1);
    if (!text)
        return NL_CHECK_OUT_OF_MEMORY;
    if (snprintf(text, (size_t)n + 1, format, p.element_type, p.element_type,
                 p.array_type, call->function, p.array_type, p.symbol, p.symbol,
                 call->function) != n) {
        free(text);
        return NL_CHECK_INTERNAL_ERROR;
    }
    *out = text;
    *length = (size_t)n;
    return NL_CHECK_OK;
}
