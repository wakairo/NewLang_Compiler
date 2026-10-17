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
#include <stdint.h>
#include <stdio.h>

static bool describe(const NLCheckedFragment *, NLCheckedNodeId,
                     const NLSemanticContext *, NLArray4Construction *);

size_t nl_checked_array4_count(const NLCheckedFragment *f)
{
    if (!f || !f->array4_world || !f->array4_binding || !f->array4_issued ||
        f->array4_binding != f->array4_issued->binding)
        return 0;
    size_t count = 1;
    for (const NLArray4Capture *p = f->array4_more; p; p = p->next) {
        if (count >= NL_SEMANTIC_MAX_ENTRIES || !p->world || !p->binding ||
            !p->issued || p->issued->binding != p->binding)
            return 0;
        ++count;
    }
    return count;
}

/* Selection is always qualified by the owning actual source fragment and
 * construction ordinal. It never looks up a guessed root in a host world. */
static const NLArray4Construction *record_at(const NLCheckedFragment *f,
                                             size_t index,
                                             const NLSemanticContext **world)
{
    if (!index) {
        *world = f->array4_world;
        return f->array4_issued;
    }
    const NLArray4Capture *p = f->array4_more;
    for (size_t i = 1; p && i < index; ++i)
        p = p->next;
    if (!p)
        return NULL;
    *world = p->world;
    return p->issued;
}

NLCheckStatus nl_array4_capture(NLCheckedFragment *f, NLCheckedNodeId binding,
                                const NLSemanticContext *c)
{
    const size_t count = nl_checked_array4_count(f);
    if ((!count && (f->array4_world || f->array4_binding || f->array4_issued ||
                    f->array4_more)) ||
        count >= NL_SEMANTIC_MAX_ENTRIES)
        return count ? NL_CHECK_RESOURCE_LIMIT : NL_CHECK_INTERNAL_ERROR;
    NLSemanticContext *snapshot = NULL;
    NLCheckStatus status = nl_sem_clone(c, &snapshot);
    if (status != NL_CHECK_OK)
        return status;
    NLArray4Construction description;
    if (!describe(f, binding, snapshot, &description)) {
        nl_semantic_destroy(snapshot);
        return NL_CHECK_INTERNAL_ERROR;
    }
    NLArray4Construction *issued = malloc(sizeof(*issued));
    if (!issued) {
        nl_semantic_destroy(snapshot);
        return NL_CHECK_OUT_OF_MEMORY;
    }
    *issued = description;
    if (!count) {
        f->array4_world = snapshot;
        f->array4_binding = binding;
        f->array4_issued = issued;
    } else {
        NLArray4Capture *event = malloc(sizeof(*event));
        if (!event) {
            free(issued);
            nl_semantic_destroy(snapshot);
            return NL_CHECK_OUT_OF_MEMORY;
        }
        *event = (NLArray4Capture){snapshot, binding, issued, NULL};
        NLArray4Capture **last = &f->array4_more;
        while (*last)
            last = &(*last)->next;
        *last = event;
    }
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

static bool describe(const NLCheckedFragment *f, NLCheckedNodeId binding,
                     const NLSemanticContext *c, NLArray4Construction *out)
{
    if (!f || !out || !c || !binding)
        return false;
    const NLCheckedNodeView *b = nl_checked_node_view(f, binding);
    if (!b || b->kind != NL_CHECKED_BINDING)
        return false;
    const NLCheckedNodeView *a = nl_checked_node_view(f, b->initializer);
    const NLCheckedNodeView *receiver =
        nl_checked_node_view(f, b->first_argument);
    if (!a || !receiver || b->symbol == 0 || b->symbol > c->binding_count)
        return false;
    const NLPlaceId root = c->bindings[b->symbol - 1].view.place;
    if (!root || root > c->place_count)
        return false;
    const NLSemanticPlaceView p = c->places[root - 1];
    if (!p.type || p.type > c->type_count || p.fixed_field_count != 4)
        return false;
    NLArray4Construction v = {.origin = f,
                              .binding = binding,
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

bool nl_checked_array4_view_at(const NLCheckedFragment *f, size_t index,
                               NLArray4Construction *out)
{
    if (!out || index >= nl_checked_array4_count(f))
        return false;
    const NLSemanticContext *world;
    const NLArray4Construction *v = record_at(f, index, &world);
    if (!v || !world)
        return false;
    *out = *v;
    return true;
}

const NLSemanticContext *nl_checked_array4_world_at(const NLCheckedFragment *f,
                                                    size_t index)
{
    if (index >= nl_checked_array4_count(f))
        return NULL;
    const NLSemanticContext *world = NULL;
    (void)record_at(f, index, &world);
    return world;
}

bool nl_checked_array4_view(const NLCheckedFragment *f,
                            NLArray4Construction *out)
{
    return nl_checked_array4_count(f) == 1 &&
           nl_checked_array4_view_at(f, 0, out);
}
const NLSemanticContext *nl_checked_array4_world(const NLCheckedFragment *f)
{
    return nl_checked_array4_count(f) == 1 ? nl_checked_array4_world_at(f, 0)
                                           : NULL;
}
static bool same_span(NLSourceSpan a, NLSourceSpan b)
{
    return a.start_byte == b.start_byte && a.end_byte == b.end_byte;
}

/* Authenticate the COMPLETE closed source/checked body, not a selected subset
 * or coincident numeric IDs. Retained immutable AST belongs to the registered
 * body, independently of the destroyed input unit AST/source. */
static bool body_graph(const NLCheckedFragment *f, size_t count)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    const NLSyntaxView *syntax =
        f->body_owner && f->body_owner->syntax
            ? nl_syntax_node_view(nl_syntax_tree_root(f->body_owner->syntax))
            : NULL;
    if (!count || count > (NL_SEMANTIC_MAX_ENTRIES - 2) / 7 || !root ||
        !syntax || syntax->kind != NL_SYNTAX_BLOCK ||
        root->kind != NL_CHECKED_BLOCK || root->terminates || root->type != 1 ||
        root->result_count || root->item_count != count ||
        syntax->data.block.item_count != count ||
        !same_span(root->span, syntax->span) ||
        nl_checked_node_count(f) != 7 * count + 2 ||
        nl_control_exits_count(nl_checked_control_exits(f)))
        return false;
    const NLCheckedNodeView *tail = nl_checked_node_view(f, root->tail);
    const NLSyntaxView *stail = nl_syntax_node_view(syntax->data.block.tail);
    if (!tail || !stail || tail->kind != NL_CHECKED_UNIT || tail->type != 1 ||
        tail->result_count || tail->terminates ||
        stail->kind != NL_SYNTAX_EXPR_NAME ||
        !same_span(tail->span, stail->span) ||
        !token_is(f->source, (NLToken){.span = stail->data.name}, "unit"))
        return false;
    NLCheckedNodeId next = root->first_item;
    const NLSyntaxNode *item = syntax->data.block.items;
    for (size_t i = 0; i < count; ++i) {
        const NLSemanticContext *world;
        const NLArray4Construction *p = record_at(f, i, &world);
        const NLSyntaxView *s = nl_syntax_node_view(item);
        if (!p || !s || s->kind != NL_SYNTAX_BINDING || p->binding != next)
            return false;
        const NLSyntaxView *a =
            nl_syntax_node_view(s->data.binding.initializer);
        const NLCheckedNodeView *b = nl_checked_node_view(f, next),
                                *r = nl_checked_node_view(f, p->receiver),
                                *constructor =
                                    nl_checked_node_view(f, p->constructor);
        if (!a || a->kind != NL_SYNTAX_ARRAY4_CONSTRUCTOR ||
            a->data.array4.count != 4 || !b || !r || !constructor ||
            b->terminates || b->result_count || !same_span(b->span, s->span) ||
            !same_span(b->name, s->data.binding.name) ||
            !same_span(r->span, s->data.binding.name) ||
            !same_span(constructor->span, a->span))
            return false;
        const NLSyntaxNode *element = a->data.array4.elements;
        for (size_t e = 0; e < 4; ++e) {
            const NLSyntaxView *se = nl_syntax_node_view(element);
            const NLCheckedNodeView *ce =
                nl_checked_node_view(f, p->elements[e].initializer);
            if (!se || !ce || se->kind != NL_SYNTAX_SUM_CONSTRUCTOR ||
                !same_span(ce->span, se->span))
                return false;
            element = nl_syntax_next_argument(element);
        }
        if (element)
            return false;
        next = b->next_item;
        item = nl_syntax_next_argument(item);
    }
    return next == 0 && item == NULL;
}

static bool same_record(const NLArray4Construction *v,
                        const NLArray4Construction *issued)
{
    if (v->origin != issued->origin || v->binding != issued->binding ||
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
    return true;
}
static bool plain_package(NLSemanticValueView v)
{
    return v.dependencies == NL_DEPENDENCY_FREE && !v.value_dependency_count &&
           !v.domain && !v.slot_place && !v.allocation_region &&
           !v.occupancy.region && !v.occupancy.start && !v.occupancy.length &&
           !v.reference_count && !v.reference.place &&
           !v.reference.incarnation && !v.reference.scope &&
           v.reference.provenance == NL_PROVENANCE_UNKNOWN &&
           !v.reference.readable && !v.reference.writable &&
           !v.reference.occurrence_dependency && !v.scalar_known &&
           !v.scalar_value && !v.sum_owner;
}
static bool event_valid(const NLCheckedFragment *f,
                        const NLArray4Construction *v,
                        const NLSemanticContext *c)
{
    const NLSemanticContext *final = nl_checked_context(f);
    if (v->origin != f || !v->incarnation || !v->fact || !final ||
        final->place_count < c->place_count ||
        final->value_count < c->value_count ||
        final->binding_count < c->binding_count)
        return false;
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
        root.placement.region || root.placement.start ||
        root.placement.length || value.type != v->array_type ||
        value.field_count != 4 || value.carrier != NL_CARRIER_PLACE ||
        value.owner_place != v->root || value.aggregate_owner || value.domain ||
        value.allocation_region || value.dependencies != NL_DEPENDENCY_FREE ||
        value.value_dependency_count || !plain_package(value) ||
        value.variant || value.sum_payload || root.payload_occurrence ||
        c->region_count || c->domain_count || c->scope_count ||
        c->occurrence_count || final->place_count < c->place_count ||
        final->places[v->root - 1].incarnation != root.incarnation ||
        final->places[v->root - 1].live ||
        final->places[v->root - 1].type != v->array_type ||
        final->bindings[v->symbol - 1].view.place != v->root ||
        final->bindings[v->symbol - 1].view.availability != NL_CONSUMED)
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
            !plain_package(x) || x.field_count || p.fixed_field_count ||
            !e.fact || !e.incarnation || p.implicit_local ||
            p.placement.region || p.placement.start || p.placement.length ||
            final->places[e.place - 1].live ||
            final->places[e.place - 1].type != v->element_type ||
            final->places[e.place - 1].parent_aggregate != v->root ||
            final->places[e.place - 1].parent_incarnation != v->incarnation ||
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
    return next == 0 && source_vector(f, a, elements);
}

bool nl_checked_array4_valid_at(const NLCheckedFragment *f, size_t index,
                                const NLArray4Construction *v)
{
    const size_t count = nl_checked_array4_count(f);
    if (!f || !v || index >= count || v->origin != f || !f->body_owner ||
        f->source != f->body_owner->source)
        return false;
    const NLSemanticContext *c;
    const NLArray4Construction *issued = record_at(f, index, &c);
    if (!issued || !c || !same_record(v, issued) || !body_graph(f, count) ||
        c->place_count != 5 * (index + 1) ||
        c->value_count != 5 * (index + 1) || c->binding_count != index + 1 ||
        c->place_count > NL_SEMANTIC_MAX_ENTRIES || c->region_count ||
        c->domain_count || c->scope_count || c->occurrence_count ||
        c->raw_interval_count)
        return false;
    bool places[NL_SEMANTIC_MAX_ENTRIES + 1] = {false};
    bool values[NL_SEMANTIC_MAX_ENTRIES + 1] = {false};
    bool bindings[NL_SEMANTIC_MAX_ENTRIES + 1] = {false};
    /* Revalidate EVERY earlier source-issued original in the selected actual
     * checkpoint. Unique coverage + exact counts closes its whole frame
     * before the invariant kernel can index any other place/package. */
    for (size_t i = 0; i <= index; ++i) {
        const NLSemanticContext *origin;
        const NLArray4Construction *p = record_at(f, i, &origin);
        if (!p || !event_valid(f, p, c) || bindings[p->symbol] ||
            places[p->root] || values[p->value])
            return false;
        bindings[p->symbol] = places[p->root] = values[p->value] = true;
        for (size_t e = 0; e < 4; ++e) {
            const NLArray4Element child = p->elements[e];
            if (places[child.place] || values[child.value])
                return false;
            places[child.place] = values[child.value] = true;
        }
    }
    return nl_sem_validate(c) == NL_CHECK_OK;
}
bool nl_checked_array4_valid(const NLCheckedFragment *f,
                             const NLArray4Construction *v)
{
    return nl_checked_array4_count(f) == 1 &&
           nl_checked_array4_valid_at(f, 0, v);
}

/* Two passes over an authenticated source body: first size, then one staged
 * allocation. No guest array or authority is constructed by this host helper.
 */
static bool c_text(const NLCheckedFragment *f, size_t count, size_t function,
                   char *text, size_t capacity, size_t *written)
{
    const NLSemanticContext *world;
    const NLArray4Construction *first = record_at(f, 0, &world);
    size_t used = 0;
#define WRITE(...)                                                             \
    do {                                                                       \
        const int n = snprintf(text ? text + used : NULL,                      \
                               text ? capacity - used : 0, __VA_ARGS__);       \
        if (n < 0 || (size_t)n > SIZE_MAX - used ||                            \
            (text && (size_t)n >= capacity - used))                            \
            return false;                                                      \
        used += (size_t)n;                                                     \
    } while (0)
    WRITE("/* SPECULATIVE PR533 original Array4, checked source only. */\n"
          "#include <stdint.h>\n"
          "enum { NL_ARRAY4_NONE = 37 };\n"
          "typedef struct { uint8_t tag; uint8_t payload; } nl_option_%zu;\n"
          "typedef nl_option_%zu nl_array_%zu[4];\n"
          "static void nl_fn_%zu(void)\n{\n",
          first->element_type, first->element_type, first->array_type,
          function);
    for (size_t i = 0; i < count; ++i) {
        const NLArray4Construction *p = record_at(f, i, &world);
        WRITE("    nl_array_%zu nl_local_%zu = {\n"
              "        { .tag = NL_ARRAY4_NONE },\n"
              "        { .tag = NL_ARRAY4_NONE },\n"
              "        { .tag = NL_ARRAY4_NONE },\n"
              "        { .tag = NL_ARRAY4_NONE }\n"
              "    };\n"
              "    (void)nl_local_%zu;\n",
              p->array_type, p->symbol, p->symbol);
    }
    WRITE("}\nint main(void) { nl_fn_%zu(); return 0; }\n", function);
#undef WRITE
    *written = used;
    return true;
}

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
    const size_t count = nl_checked_array4_count(f);
    if (!count)
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    NLArray4Construction first, p;
    if (!nl_checked_array4_view_at(f, 0, &first))
        return NL_CHECK_SEMANTIC_UNSUPPORTED;
    for (size_t i = 0; i < count; ++i)
        if (!nl_checked_array4_view_at(f, i, &p) ||
            p.array_type != first.array_type ||
            p.element_type != first.element_type ||
            !nl_checked_array4_valid_at(f, i, &p))
            return NL_CHECK_SEMANTIC_UNSUPPORTED;
    size_t n = 0, actual = 0;
    if (!c_text(f, count, call->function, NULL, 0, &n) || n == SIZE_MAX)
        return NL_CHECK_RESOURCE_LIMIT;
    char *text = malloc(n + 1);
    if (!text)
        return NL_CHECK_OUT_OF_MEMORY;
    if (!c_text(f, count, call->function, text, n + 1, &actual) ||
        actual != n) {
        free(text);
        return NL_CHECK_INTERNAL_ERROR;
    }
    *out = text;
    *length = n;
    return NL_CHECK_OK;
}
