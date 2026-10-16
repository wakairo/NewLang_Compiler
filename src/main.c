#include "newlang/checked.h"
#include "newlang/checked_c_node.h"
#include "newlang/diagnostic.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include "semantic_internal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char help[] =
    "Usage: newlangc [--version | --help | SOURCE]\n"
    "NewLang production compiler P0 bootstrap.\n"
    "  --version  Print the deterministic compiler version.\n"
    "  --help     Print this help.\n"
    "  SOURCE     Validate V0/V1/AVS/local-root and emit Checked-C to stdout.\n"
    "Checked-C is a bounded bootstrap/reference execution path; LLVM remains "
    "the planned primary backend.\n";

static int report_error(const char *code, const char *message,
                        const char *argument, int status)
{
    const NLDiagnosticNote note = {argument, NULL};
    const NLDiagnostic diagnostic = {NL_DIAG_ERROR, "cli", code, message,
                                     NULL,          &note, 1};
    return nl_diagnostic_render(stderr, &diagnostic) ? status : 1;
}

typedef struct {
    size_t function;
    const NLCheckedFragment *body;
    NLTypeId result;
    NLSymbolId
        parameter; /* representative body-local C formal, not caller ID */
} V0Function;

typedef struct {
    V0Function functions[NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS];
    size_t count;
    size_t entry_function;
    NLTypeId unit, u8, aggregate;
    bool uses_usize;
    const NLCheckedFragment *formal_body; /* current validation only */
    NLSymbolId formal;
    size_t scalar_depth; /* at most relay + leaf; not a general call graph */
} V0Program;

static V0Function *v0_find_function(V0Program *program, size_t function)
{
    for (size_t i = 0; i < program->count; ++i)
        if (program->functions[i].function == function)
            return &program->functions[i];
    return NULL;
}

/* Shared resolved projection predicate. Point-in-time proof is supplied by
 * the checker; final local liveness is not a replay of that proof. */
static bool bounded_field(const NLCheckedFragment *f, const NLCheckedField *e,
                          const V0Program *p)
{
    if (!e->present || !e->dependency_compatible || e->base == 0 ||
        e->type != p->u8 || e->nominal != p->aggregate || e->index >= 2 ||
        e->parent == 0 || e->child == 0 || e->parent == e->child ||
        e->parent_incarnation == 0 || e->child_incarnation == 0 ||
        e->parent_fact == 0 || e->child_fact == 0)
        return false;
    NLAggregateField field;
    if (!nl_semantic_aggregate_field_view(nl_checked_context(f), e->nominal,
                                          e->index, &field) ||
        field.type != p->u8)
        return false;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_RECEIVER && v->symbol == e->base &&
            v->type == e->nominal)
            return true;
    }
    return false;
}

/* V1 consumes only immutable checked evidence. Neither syntax trees nor
 * source bytes enter scalar lowering. Symbol IDs produce safe C local names. */
static bool v1_scalar(const NLCheckedFragment *fragment, NLCheckedNodeId id,
                      const V0Program *program)
{
    const NLCheckedNodeView *v = nl_checked_node_view(fragment, id);
    if (v == NULL || v->type != program->u8 || v->result_count != 1 ||
        v->results[0].type != program->u8 || v->terminates)
        return false;
    if (v->kind == NL_CHECKED_U8_LITERAL)
        return v->has_scalar_result && v->scalar_result.known &&
               v->scalar_result.type == program->u8 &&
               v->scalar_result.value <= 255;
    if (v->kind == NL_CHECKED_FIELD_READ)
        return v->value_use == NL_VALUE_COPIED &&
               v->field.access == NL_ACCESS_READ &&
               bounded_field(fragment, &v->field, program);
    if (v->kind != NL_CHECKED_IDENTIFIER || v->symbol == 0 ||
        v->value_use != NL_VALUE_COPIED)
        return false;
    if (fragment == program->formal_body && v->symbol == program->formal) {
        NLSemanticValueView value;
        return nl_semantic_value_view(nl_checked_context(fragment),
                                      v->results[0].value, &value) &&
               value.type == program->u8 &&
               value.dependencies == NL_DEPENDENCY_FREE &&
               value.value_dependency_count == 0;
    }
    /* Exclude host-known parameters/seeds: the C local must have a source
     * binding in this owned body. Lexical visibility is checker-established. */
    for (size_t i = 1; i <= nl_checked_node_count(fragment); ++i) {
        const NLCheckedNodeView *binding = nl_checked_node_view(fragment, i);
        if (binding != NULL && binding->kind == NL_CHECKED_RECEIVER &&
            binding->type == program->u8 && binding->symbol == v->symbol)
            return true;
    }
    return false;
}

/* Native target §6 quantity staging only. A source-connected Copy binding is
 * required; no usize call ABI, ref, arithmetic or inferred owner authority. */
static bool quantity_scalar(const NLCheckedFragment *f, NLCheckedNodeId id,
                            V0Program *p)
{
    const NLTypeId type =
        nl_semantic_core_type(nl_checked_context(f), NL_TYPE_USIZE);
    bool first = true;
    size_t expected = 0;
    /* Follow decreasing source IDs iteratively: long Copy chains cannot
     * consume the C stack or introduce a source-less scalar seed. */
    while (id != 0) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        NLSemanticValueView value;
        if (v == NULL || v->type != type || v->terminates ||
            v->result_count != 1 || v->results[0].type != type ||
            !nl_semantic_value_view(nl_checked_context(f), v->results[0].value,
                                    &value) ||
            value.type != type || !value.scalar_known ||
            value.dependencies != NL_DEPENDENCY_FREE ||
            value.value_dependency_count != 0 ||
            (!first && value.scalar_value != expected))
            return false;
        expected = value.scalar_value;
        first = false;
        if (v->kind == NL_CHECKED_USIZE_LITERAL) {
            p->uses_usize = true;
            return v->has_scalar_result && v->scalar_result.known &&
                   v->scalar_result.type == type &&
                   v->scalar_result.value == expected;
        }
        if (v->kind != NL_CHECKED_IDENTIFIER || v->symbol == 0 ||
            v->value_use != NL_VALUE_COPIED)
            return false;
        NLCheckedNodeId origin = 0;
        for (size_t i = 1; i < id; ++i) {
            const NLCheckedNodeView *b = nl_checked_node_view(f, i);
            if (b->kind == NL_CHECKED_BINDING && b->symbol == v->symbol &&
                b->initializer < i && b->initializer != 0) {
                origin = b->initializer;
                break;
            }
        }
        id = origin;
    }
    return false;
}

/* New bool lowering is restricted to the function's straight-line root block.
 * Match checked identifiers to root declarations, including retained source
 * spelling. Source bytes validate resolution only; emitted names use IDs. */
static NLCheckedNodeId bool_source_origin(const NLCheckedFragment *f,
                                          NLCheckedNodeId id)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (v == NULL || root == NULL || root->kind != NL_CHECKED_BLOCK ||
        v->kind != NL_CHECKED_IDENTIFIER || v->symbol == 0 ||
        v->value_use != NL_VALUE_COPIED)
        return 0;
    NLSourceView use;
    if (!nl_source_view(nl_checked_source(f), v->name, &use) || use.length == 0)
        return 0;
    for (NLCheckedNodeId i = root->first_item; i != 0;) {
        const NLCheckedNodeView *binding = nl_checked_node_view(f, i);
        if (binding == NULL)
            return 0;
        if (i < id && binding->kind == NL_CHECKED_BINDING &&
            binding->symbol == v->symbol && binding->initializer != 0 &&
            binding->initializer < i) {
            const NLCheckedNodeView *receiver =
                nl_checked_node_view(f, binding->first_argument);
            NLSourceView declaration;
            if (receiver != NULL && receiver->kind == NL_CHECKED_RECEIVER &&
                receiver->symbol == v->symbol && receiver->type == v->type &&
                nl_source_view(nl_checked_source(f), receiver->name,
                               &declaration) &&
                declaration.length == use.length &&
                memcmp(declaration.bytes, use.bytes, use.length) == 0)
                return binding->initializer;
            return 0;
        }
        if (binding->next_item != 0 && binding->next_item <= i)
            return 0;
        i = binding->next_item;
    }
    return 0;
}

static bool bool_root_value(const NLCheckedFragment *f, NLCheckedNodeId id)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (root == NULL || root->kind != NL_CHECKED_BLOCK)
        return false;
    if (root->tail == id)
        return true;
    for (NLCheckedNodeId i = root->first_item; i != 0;) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v == NULL)
            return false;
        if ((v->kind == NL_CHECKED_BINDING ||
             v->kind == NL_CHECKED_STATEMENT) &&
            v->initializer == id)
            return true;
        if (v->next_item != 0 && v->next_item <= i)
            return false;
        i = v->next_item;
    }
    return false;
}

static bool bool_quantity_origin(const NLCheckedFragment *f, NLCheckedNodeId id,
                                 V0Program *p)
{
    if (!quantity_scalar(f, id, p))
        return false;
    while (id != 0) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        if (v->kind == NL_CHECKED_USIZE_LITERAL)
            return true;
        id = bool_source_origin(f, id);
    }
    return false;
}

/* Boolean scalar staging: authenticate the real quantity children, not merely
 * the cached static truth. Copy locals must lead back to that same operation.
 * No IF/effect/ABI or root-identity evidence is admitted here. */
static bool boolean_scalar(const NLCheckedFragment *f, NLCheckedNodeId id,
                           V0Program *p)
{
    const NLSemanticContext *c = nl_checked_context(f);
    const NLTypeId boolean = nl_semantic_core_type(c, NL_TYPE_BOOL);
    bool first = true;
    size_t expected = 0;
    while (id != 0) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        NLSemanticValueView value;
        if (v == NULL || !bool_root_value(f, id) || v->type != boolean ||
            v->terminates || v->result_count != 1 ||
            v->results[0].type != boolean ||
            !nl_semantic_value_view(c, v->results[0].value, &value) ||
            value.type != boolean || !value.scalar_known ||
            value.scalar_value > 1 ||
            value.dependencies != NL_DEPENDENCY_FREE ||
            value.value_dependency_count != 0 || value.domain != 0 ||
            value.allocation_region != 0 || value.reference.place != 0 ||
            value.reference_count != 0 || value.slot_place != 0 ||
            value.field_count != 0 || value.sum_owner != 0 ||
            (!first && value.scalar_value != expected))
            return false;
        expected = value.scalar_value;
        first = false;
        if (v->kind == NL_CHECKED_USIZE_EQUALITY) {
            const NLCheckedNodeView *left =
                nl_checked_node_view(f, v->first_argument);
            const NLCheckedNodeView *right =
                left == NULL ? NULL
                             : nl_checked_node_view(f, left->next_argument);
            NLSemanticValueView a, b;
            if (v->argument_count != 2 || left == NULL || right == NULL ||
                v->first_argument >= left->next_argument ||
                left->next_argument >= id || right->next_argument != 0 ||
                v->span.start_byte != left->span.start_byte ||
                v->span.end_byte != right->span.end_byte ||
                v->name.end_byte < v->name.start_byte ||
                v->name.end_byte - v->name.start_byte != 2 ||
                left->span.end_byte > v->name.start_byte ||
                v->name.end_byte > right->span.start_byte ||
                !bool_quantity_origin(f, v->first_argument, p) ||
                !bool_quantity_origin(f, left->next_argument, p) ||
                !nl_semantic_value_view(c, left->results[0].value, &a) ||
                !nl_semantic_value_view(c, right->results[0].value, &b) ||
                v->results[0].value <= left->results[0].value ||
                v->results[0].value <= right->results[0].value ||
                !v->has_scalar_result || !v->scalar_result.known ||
                v->scalar_result.type != boolean ||
                v->scalar_result.value != expected)
                return false;
            /* Consistency validation only. Emission below uses lhs == rhs. */
            return expected == (a.scalar_value == b.scalar_value ? 1U : 0U);
        }
        if (v->kind != NL_CHECKED_IDENTIFIER || v->symbol == 0 ||
            v->value_use != NL_VALUE_COPIED)
            return false;
        id = bool_source_origin(f, id);
    }
    return false;
}

/* One pure, two-normal-u8 IF expression. This is a driver proof consumer,
 * not an effect/CFG join or permission to lower original-object mutation. */
static bool copy_if_plain(NLSemanticValueView v, NLTypeId type)
{
    return v.type == type && v.dependencies == NL_DEPENDENCY_FREE &&
           v.value_dependency_count == 0 && v.reference_count == 0 &&
           v.reference.place == 0 && v.reference.scope == 0 &&
           v.reference.incarnation == 0 &&
           v.reference.provenance == NL_PROVENANCE_UNKNOWN &&
           !v.reference.readable && !v.reference.writable &&
           v.reference.occurrence_dependency == 0 && v.domain == 0 &&
           v.slot_place == 0 && v.allocation_region == 0 &&
           v.occupancy.region == 0 && v.field_count == 0 &&
           v.aggregate_owner == 0 && v.sum_owner == 0 && v.sum_payload == 0 &&
           v.variant == 0;
}
static bool copy_if_gap(const NLSource *source, size_t start, size_t end,
                        const char *expected)
{
    NLSourceView bytes;
    if (!nl_source_view(source, (NLSourceSpan){start, end}, &bytes))
        return false;
    for (size_t i = 0; i < bytes.length; ++i) {
        const unsigned char ch = bytes.bytes[i];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            continue;
        if (*expected == 0 || ch != (unsigned char)*expected++)
            return false;
    }
    return *expected == 0;
}
static bool copy_if_literal(const NLCheckedFragment *f,
                            const NLCheckedNodeView *v)
{
    NLSemanticValueView value;
    NLSourceView text;
    if (v == NULL || v->kind != NL_CHECKED_U8_LITERAL || v->result_count != 1 ||
        !v->has_scalar_result || !v->scalar_result.known ||
        v->scalar_result.type != v->type || v->scalar_result.value > 255 ||
        !nl_semantic_value_view(nl_checked_context(f), v->results[0].value,
                                &value) ||
        !copy_if_plain(value, v->type) || !value.scalar_known ||
        value.scalar_value != v->scalar_result.value ||
        !nl_source_view(nl_checked_source(f), v->span, &text) ||
        text.length < 5 || text.bytes[0] != 'u' ||
        text.bytes[text.length - 1] != ')')
        return false;
    /* Read-only consistency with the already parsed source; no source
     * spelling is emitted as C and no new syntax is admitted. */
    size_t i = 0, number = 0, digits = 0;
    const char *prefix = "u8(";
    while (i < text.length && *prefix != 0) {
        unsigned char ch = text.bytes[i++];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            continue;
        if (ch != (unsigned char)*prefix++)
            return false;
    }
    if (*prefix != 0)
        return false;
    while (i < text.length) {
        unsigned char ch = text.bytes[i];
        if (ch < '0' || ch > '9')
            break;
        if (number > (255U - (ch - '0')) / 10)
            return false;
        number = number * 10 + ch - '0';
        ++i;
        ++digits;
    }
    return digits != 0 && number == value.scalar_value &&
           copy_if_gap(nl_checked_source(f), v->span.start_byte + i,
                       v->span.end_byte, ")");
}
/* Root-local u8 Copy origins only. An unknown result must lead back to THIS
 * source IF; a cached value cannot substitute for the selected C carrier. */
static bool copy_if_u8(const NLCheckedFragment *f, NLCheckedNodeId id,
                       NLCheckedNodeId selection, NLTypeId u8)
{
    NLSemanticValueView first = {0};
    bool initial = true;
    while (id != 0) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, id);
        NLSemanticValueView value;
        if (v == NULL || v->type != u8 || v->terminates ||
            v->result_count != 1 || v->results[0].type != u8 ||
            !nl_semantic_value_view(nl_checked_context(f), v->results[0].value,
                                    &value) ||
            !copy_if_plain(value, u8) || value.carrier != NL_CARRIER_ENDED ||
            value.owner_place != 0)
            return false;
        if (initial) {
            first = value;
            initial = false;
        }
        if (first.scalar_known != value.scalar_known ||
            first.scalar_value != value.scalar_value)
            return false;
        if (id == selection)
            return v->kind == NL_CHECKED_IF && !value.scalar_known &&
                   value.scalar_value == 0;
        if (v->kind == NL_CHECKED_U8_LITERAL)
            return copy_if_literal(f, v);
        id = bool_source_origin(f, id);
    }
    return false;
}
static bool copy_if_prefix(const NLCheckedFragment *f,
                           const NLCheckedFragment *arms[2],
                           const NLCheckedNodeView *conditional,
                           NLValueId prefix, size_t bindings)
{
    const NLSemanticContext *parent = nl_checked_context(f);
    const NLSemanticContext *world[2] = {nl_checked_context(arms[0]),
                                         nl_checked_context(arms[1])};
    if (world[0] == parent || world[1] == parent || world[0] == world[1])
        return false;
    for (size_t a = 0; a < 2; ++a) {
        const NLSemanticContext *c = world[a];
        if (arms[a]->branch_value_prefix != prefix ||
            c->value_count != prefix + 1 || c->binding_count != bindings ||
            c->place_count != bindings || c->scope_count != 0 ||
            c->domain_count != 0 || c->region_count != 0 ||
            c->occurrence_count != 0 ||
            c->last_value_fact != arms[a]->branch_fact_prefix ||
            arms[a]->branch_fact_prefix != bindings)
            return false;
        for (size_t j = 0; j < bindings; ++j) {
            const NLSemanticBindingView b = c->bindings[j].view,
                                        p = parent->bindings[j].view;
            const NLSemanticPlaceView place = c->places[j],
                                      original = parent->places[j];
            if (c->bindings[j].hidden || b.availability != NL_AVAILABLE ||
                b.type != p.type || b.value != p.value || b.place != p.place ||
                b.place != j + 1 || !place.live || !place.independent_root ||
                place.implicit_local != original.implicit_local ||
                place.incarnation != original.incarnation ||
                place.current_fact != j + 1 || place.current_value != b.value ||
                place.type != original.type || place.governing_domain != 0 ||
                place.placement.region != 0 || place.parent_aggregate != 0 ||
                place.parent_sum != 0 || place.fixed_field_count != 0 ||
                place.payload_occurrence != 0)
                return false;
        }
        for (size_t j = 0; j < prefix; ++j) {
            const NLSemanticValueView v = c->values[j], p = parent->values[j];
            if (!copy_if_plain(v, p.type) || v.scalar_known != p.scalar_known ||
                v.scalar_value != p.scalar_value)
                return false;
            NLPlaceId owner = 0;
            for (size_t b = 0; b < bindings; ++b)
                if (c->bindings[b].view.value == j + 1)
                    owner = c->bindings[b].view.place;
            if (v.owner_place != owner ||
                v.carrier != (owner == 0 ? NL_CARRIER_ENDED : NL_CARRIER_PLACE))
                return false;
        }
        const NLCheckedNodeView *ar =
            nl_checked_node_view(arms[a], nl_checked_root(arms[a]));
        if (ar == NULL || ar->span.start_byte <= conditional->span.start_byte ||
            ar->span.end_byte > conditional->span.end_byte)
            return false;
    }
    return true;
}
static bool copy_if_pair_frame(const NLCheckedFragment *, NLCheckedNodeId,
                               V0Program *, size_t);
static bool copy_if_selection(const NLCheckedFragment *f, NLCheckedNodeId id,
                              V0Program *p)
{
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    if (v == NULL || v->kind != NL_CHECKED_IF || v->terminates ||
        v->normal_arms != 2 || v->item_count != 2 || v->has_scalar_result ||
        v->type != p->u8 || v->result_count != 1 ||
        v->results[0].type != p->u8 || !bool_root_value(f, id) ||
        c->domain_count != 0 || c->region_count != 0 ||
        c->occurrence_count != 0 || p->count != 1 || p->scalar_depth != 0)
        return false;
    const NLCheckedNodeView *cond = nl_checked_node_view(f, v->initializer);
    NLSemanticValueView cv, origin_value, result;
    const NLCheckedNodeId origin = bool_source_origin(f, v->initializer);
    const NLCheckedNodeView *from = nl_checked_node_view(f, origin);
    if (cond == NULL || from == NULL || v->initializer >= id ||
        cond->result_count != 1 ||
        cond->type != nl_semantic_core_type(c, NL_TYPE_BOOL) ||
        !boolean_scalar(f, origin, p) ||
        !nl_semantic_value_view(c, cond->results[0].value, &cv) ||
        !nl_semantic_value_view(c, from->results[0].value, &origin_value) ||
        !copy_if_plain(cv, from->type) || !cv.scalar_known ||
        cv.scalar_value != origin_value.scalar_value ||
        cond->results[0].value <= from->results[0].value ||
        v->results[0].value != cond->results[0].value + 1 ||
        !nl_semantic_value_view(c, v->results[0].value, &result) ||
        !copy_if_plain(result, p->u8) || result.scalar_known ||
        result.carrier != NL_CARRIER_ENDED || result.owner_place != 0 ||
        result.scalar_value != 0)
        return false;
    const bool paired = p->aggregate != 0;
    size_t scalars = 0, bindings = 0;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_BINDING &&
            f->nodes[i].span.end_byte <= v->span.start_byte)
            ++bindings;
    if (paired && !copy_if_pair_frame(f, id, p, bindings))
        return false;
    if (!paired && c->scope_count != 0)
        return false;
    for (size_t i = 0; !paired && i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        switch (n->kind) {
        case NL_CHECKED_BINDING:
            break;
        case NL_CHECKED_BLOCK:
        case NL_CHECKED_STATEMENT:
        case NL_CHECKED_RECEIVER:
        case NL_CHECKED_UNIT:
            break;
        case NL_CHECKED_IF:
            if (i + 1 != id)
                return false;
            /* fall through */
        case NL_CHECKED_U8_LITERAL:
        case NL_CHECKED_USIZE_LITERAL:
        case NL_CHECKED_USIZE_EQUALITY:
        case NL_CHECKED_IDENTIFIER: {
            NLSemanticValueView value;
            if (n->result_count != 1 || n->results[0].value != ++scalars ||
                !nl_semantic_value_view(c, scalars, &value) ||
                !copy_if_plain(value, n->type) ||
                value.carrier != NL_CARRIER_ENDED || value.owner_place != 0 ||
                n->results[0].type != n->type ||
                (n->type != p->u8 &&
                 n->type != nl_semantic_core_type(c, NL_TYPE_BOOL) &&
                 n->type != nl_semantic_core_type(c, NL_TYPE_USIZE)) ||
                (n->type == p->u8 && !copy_if_u8(f, i + 1, id, p->u8)))
                return false;
            break;
        }
        default:
            return false;
        }
    }
    if (!paired && scalars != c->value_count)
        return false;
    const NLCheckedFragment *arms[2] = {nl_checked_if_arm(f, id, 0),
                                        nl_checked_if_arm(f, id, 1)};
    if (arms[0] == NULL || arms[1] == NULL || arms[0] == arms[1] ||
        (!paired &&
         !copy_if_prefix(f, arms, v, cond->results[0].value, bindings)))
        return false;
    NLSourceSpan spans[2];
    for (size_t a = 0; a < 2; ++a) {
        const NLCheckedFragment *arm = arms[a];
        const NLCheckedNodeView *ar =
            nl_checked_node_view(arm, nl_checked_root(arm));
        const NLCheckedNodeView *block =
            ar == NULL ? NULL : nl_checked_node_view(arm, ar->initializer);
        const NLCheckedNodeView *tail =
            block == NULL ? NULL : nl_checked_node_view(arm, block->tail);
        NLSemanticValueView value;
        if (arm->if_parent != f || arm->if_parent_node != id ||
            arm->if_arm_index != a ||
            nl_checked_source(arm) != nl_checked_source(f) || arm->count != 3 ||
            ar == NULL || block == NULL || tail == NULL ||
            ar->kind != NL_CHECKED_IF_ARM || ar->terminates ||
            ar->type != p->u8 || ar->result_count != 1 ||
            ar->results[0].type != p->u8 || ar->has_scalar_result ||
            block->kind != NL_CHECKED_BLOCK || block->first_item != 0 ||
            block->item_count != 0 || block->terminates ||
            block->type != p->u8 || block->result_count != 1 ||
            block->results[0].type != p->u8 || block->has_scalar_result ||
            block == tail || ar == block || ar == tail ||
            ar->results[0].value != tail->results[0].value ||
            block->results[0].value != tail->results[0].value ||
            tail->result_count != 1 || tail->type != p->u8 ||
            tail->terminates || tail->results[0].type != p->u8 ||
            tail->initializer != 0 || tail->argument_count != 0 ||
            tail->first_argument != 0 || tail->next_argument != 0 ||
            tail->results[0].value != cond->results[0].value + 1 ||
            ar->span.start_byte != block->span.start_byte ||
            ar->span.end_byte != block->span.end_byte ||
            !nl_semantic_value_view(nl_checked_context(arm),
                                    tail->results[0].value, &value) ||
            !copy_if_plain(value, p->u8) || !value.scalar_known ||
            value.scalar_value > 255 || value.carrier != NL_CARRIER_LOOSE ||
            value.owner_place != 0 ||
            !copy_if_gap(nl_checked_source(f), block->span.start_byte,
                         tail->span.start_byte, "{") ||
            !copy_if_gap(nl_checked_source(f), tail->span.end_byte,
                         block->span.end_byte, "}"))
            return false;
        spans[a] = ar->span;
        if (tail->kind == NL_CHECKED_U8_LITERAL) {
            if (!copy_if_literal(arm, tail))
                return false;
        } else {
            if (tail->kind != NL_CHECKED_IDENTIFIER ||
                tail->value_use != NL_VALUE_COPIED ||
                tail->span.start_byte != tail->name.start_byte ||
                tail->span.end_byte != tail->name.end_byte ||
                tail->symbol == 0 || tail->symbol > bindings)
                return false;
            const NLCheckedNodeView *declaration = NULL;
            for (size_t j = 0; j < f->count; ++j)
                if (f->nodes[j].kind == NL_CHECKED_BINDING &&
                    f->nodes[j].symbol == tail->symbol &&
                    f->nodes[j].span.end_byte <= v->span.start_byte)
                    declaration = &f->nodes[j];
            const NLCheckedNodeView *receiver =
                declaration == NULL
                    ? NULL
                    : nl_checked_node_view(f, declaration->first_argument);
            const NLCheckedNodeView *init =
                declaration == NULL
                    ? NULL
                    : nl_checked_node_view(f, declaration->initializer);
            NLSourceView use, name;
            NLSemanticValueView original;
            if (receiver == NULL || init == NULL ||
                receiver->kind != NL_CHECKED_RECEIVER ||
                receiver->symbol != tail->symbol || receiver->type != p->u8 ||
                !copy_if_u8(f, declaration->initializer, id, p->u8) ||
                !nl_semantic_value_view(c, init->results[0].value, &original) ||
                !original.scalar_known ||
                original.scalar_value != value.scalar_value ||
                !nl_source_view(nl_checked_source(f), tail->name, &use) ||
                !nl_source_view(nl_checked_source(f), receiver->name, &name) ||
                use.length == 0 || use.length != name.length ||
                memcmp(use.bytes, name.bytes, use.length) != 0)
                return false;
        }
    }
    return copy_if_gap(nl_checked_source(f), v->span.start_byte,
                       cond->span.start_byte, "if(") &&
           copy_if_gap(nl_checked_source(f), cond->span.end_byte,
                       spans[0].start_byte, ")") &&
           copy_if_gap(nl_checked_source(f), spans[0].end_byte,
                       spans[1].start_byte, "else") &&
           spans[1].end_byte == v->span.end_byte;
}

/* Only one flat two-u8 nominal representation. The associated checked
 * context supplies semantic shape facts; no names or layout are consulted. */
static bool avs_shape(const NLCheckedFragment *fragment, NLTypeId type,
                      V0Program *program)
{
    const NLSemanticContext *context = nl_checked_context(fragment);
    NLSemanticTypeView t;
    if (!nl_semantic_type_view(context, type, &t) ||
        t.kind != NL_TYPE_NOMINAL || t.field_count != 2 || !t.is_copy ||
        !t.is_discardable ||
        (program->aggregate != 0 && program->aggregate != type))
        return false;
    for (size_t i = 0; i < 2; ++i) {
        NLAggregateField field;
        if (!nl_semantic_aggregate_field_view(context, type, i, &field) ||
            field.type != program->u8)
            return false;
    }
    program->aggregate = type;
    return true;
}

static bool avs_construction(const NLCheckedFragment *fragment,
                             NLCheckedNodeId id, V0Program *program)
{
    const NLCheckedNodeView *aggregate = nl_checked_node_view(fragment, id);
    if (aggregate == NULL || aggregate->kind != NL_CHECKED_AGGREGATE ||
        aggregate->result_count != 1 || aggregate->argument_count != 2 ||
        !avs_shape(fragment, aggregate->type, program))
        return false;
    bool seen[2] = {false, false};
    NLCheckedNodeId field = aggregate->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(fragment, field);
        if (v == NULL || v->kind != NL_CHECKED_AGGREGATE_FIELD ||
            v->field_index >= 2 || seen[v->field_index] ||
            v->type != program->u8 ||
            !v1_scalar(fragment, v->initializer, program))
            return false;
        seen[v->field_index] = true;
        field = v->next_argument;
    }
    return field == 0;
}

static bool avs_destructuring(const NLCheckedFragment *fragment,
                              const NLCheckedNodeView *binding,
                              V0Program *program)
{
    const NLCheckedNodeView *rhs =
        nl_checked_node_view(fragment, binding->initializer);
    if (binding->argument_count != 2 || rhs == NULL ||
        rhs->kind != NL_CHECKED_IDENTIFIER ||
        rhs->value_use != NL_VALUE_COPIED || rhs->result_count != 1 ||
        rhs->symbol == 0 || !avs_shape(fragment, rhs->type, program))
        return false;
    bool local = false;
    for (size_t i = 1; i <= nl_checked_node_count(fragment); ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(fragment, i);
        const NLCheckedNodeView *r =
            v == NULL ? NULL
                      : nl_checked_node_view(fragment, v->first_argument);
        if (v != NULL && v->kind == NL_CHECKED_BINDING &&
            v->symbol == rhs->symbol && r != NULL && r->type == rhs->type)
            local = true;
    }
    if (!local)
        return false;
    bool seen[2] = {false, false};
    NLCheckedNodeId receiver = binding->first_argument;
    for (size_t i = 0; i < 2; ++i) {
        const NLCheckedNodeView *v = nl_checked_node_view(fragment, receiver);
        if (v == NULL || v->kind != NL_CHECKED_RECEIVER || v->symbol == 0 ||
            v->type != program->u8 || v->field_index >= 2 ||
            seen[v->field_index])
            return false;
        seen[v->field_index] = true;
        receiver = v->next_argument;
    }
    return receiver == 0;
}

static bool v1_emit_scalar(FILE *stream, const NLCheckedNodeView *value)
{
    if (value->kind == NL_CHECKED_U8_LITERAL)
        return fprintf(stream, "%zu", value->scalar_result.value) >= 0;
    if (value->kind == NL_CHECKED_USIZE_LITERAL)
        return fprintf(stream, "((size_t)UINTMAX_C(%zu))",
                       value->scalar_result.value) >= 0;
    if (value->kind == NL_CHECKED_IDENTIFIER)
        return fprintf(stream, "nl_local_%zu", value->symbol) >= 0;
    if (value->kind == NL_CHECKED_FIELD_READ)
        return fprintf(stream, "nl_local_%zu.f%zu", value->field.base,
                       value->field.index) >= 0;
    return false;
}

static bool emit_quantity_bool(FILE *stream, const NLCheckedFragment *f,
                               const NLCheckedNodeView *value)
{
    if (value->kind != NL_CHECKED_USIZE_EQUALITY)
        return v1_emit_scalar(stream, value);
    const NLCheckedNodeView *left =
        nl_checked_node_view(f, value->first_argument);
    const NLCheckedNodeView *right =
        nl_checked_node_view(f, left->next_argument);
    return fputc('(', stream) != EOF && v1_emit_scalar(stream, left) &&
           fputs(" == ", stream) >= 0 && v1_emit_scalar(stream, right) &&
           fputc(')', stream) != EOF;
}

static bool v0_validate_node(const NLCheckedFragment *, NLCheckedNodeId,
                             V0Program *);
static bool v0_validate_call(const NLCheckedFragment *, NLCheckedNodeId,
                             V0Program *);

/* Bounded capability lowering is authorized by completed checked loans.
 * This lookup finds C carrier names only, never proves pointer safety. */
static bool gate_ref_type(const NLCheckedFragment *f, NLTypeId id,
                          NLAccessSyntax access, const V0Program *p)
{
    NLSemanticTypeView t;
    return nl_semantic_type_view(nl_checked_context(f), id, &t) &&
           t.kind == NL_TYPE_REF && t.target == p->u8 && !t.is_exclusive &&
           t.access == access;
}
static bool gate_type(const NLCheckedFragment *f, NLTypeId id,
                      NLSemanticTypeKind kind, const V0Program *p)
{
    NLSemanticTypeView t;
    if (kind == NL_TYPE_REF)
        return gate_ref_type(f, id, NL_ACCESS_READ, p);
    return nl_semantic_type_view(nl_checked_context(f), id, &t) &&
           t.kind == kind && t.target == p->u8;
}
static bool gate_identifier(const NLCheckedFragment *f,
                            const NLCheckedNodeView *v, const V0Program *p)
{
    if (v == NULL || v->kind != NL_CHECKED_IDENTIFIER || v->symbol == 0 ||
        v->value_use != NL_VALUE_COPIED || v->result_count != 1)
        return false;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n->kind == NL_CHECKED_LOAN_HEADER && n->loan.implicit_local &&
            n->loan.body_nonescape_proved && n->loan.ref_symbol == v->symbol &&
            gate_ref_type(f, v->type, n->loan.access, p))
            return true;
        if (gate_type(f, v->type, NL_TYPE_PTR, p) &&
            n->kind == NL_CHECKED_RECEIVER && n->symbol == v->symbol &&
            n->type == v->type)
            return true;
    }
    return false;
}
static bool gate_ptr(const NLCheckedFragment *f, NLCheckedNodeId id,
                     const V0Program *p)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    if (v == NULL || !gate_type(f, v->type, NL_TYPE_PTR, p) ||
        v->result_count != 1)
        return false;
    if (v->kind == NL_CHECKED_IDENTIFIER)
        return gate_identifier(f, v, p);
    const NLCheckedNodeView *arg = nl_checked_node_view(f, v->first_argument);
    return v->kind == NL_CHECKED_PTR_FROM_REF && v->argument_count == 1 &&
           v->has_reference_result && v->reference_result.scope == 0 &&
           v->reference_result.provenance == NL_PROVENANCE_VALID &&
           gate_identifier(f, arg, p) &&
           gate_ref_type(f, arg->type, NL_ACCESS_READ, p);
}
static bool gate_replace(const NLCheckedFragment *f, NLCheckedNodeId id,
                         const V0Program *p, NLSymbolId required_ref)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    if (v == NULL || v->kind != NL_CHECKED_REPLACE)
        return false;
    if (v->argument_count != 2 || v->result_count != 1)
        return false;
    if (v->type != p->u8 || v->results[0].type != p->u8)
        return false;
    if (v->field.present &&
        (!bounded_field(f, &v->field, p) ||
         v->field.access != NL_ACCESS_WRITE ||
         v->field.old_value != v->results[0].value ||
         v->field.parent_post_fact == 0 || v->field.child_post_fact == 0 ||
         v->field.parent_post_fact == v->field.parent_fact ||
         v->field.child_post_fact == v->field.child_fact))
        return false;
    const NLCheckedNodeView *ref = nl_checked_node_view(f, v->first_argument);
    const NLCheckedNodeView *value =
        ref == NULL ? NULL : nl_checked_node_view(f, ref->next_argument);
    if (ref == NULL || value == NULL || ref->kind != NL_CHECKED_IDENTIFIER)
        return false;
    return (required_ref == 0 || ref->symbol == required_ref) &&
           gate_ref_type(f, ref->type, NL_ACCESS_WRITE, p) &&
           gate_identifier(f, ref, p) && v1_scalar(f, ref->next_argument, p);
}
static bool gate_loan(const NLCheckedFragment *f, const NLCheckedNodeView *v,
                      V0Program *p)
{
    const NLCheckedNodeView *body = nl_checked_node_view(f, v->initializer);
    const bool write = v->loan.access == NL_ACCESS_WRITE;
    if (!v->loan.implicit_local || !v->loan.body_nonescape_proved ||
        !v->loan.normal_result_forwarded || v->loan.ref_symbol == 0 ||
        v->loan.scope == 0 || v->loan.place == 0 || v->loan.incarnation == 0 ||
        (v->loan.access != NL_ACCESS_READ &&
         v->loan.access != NL_ACCESS_WRITE) ||
        v->loan.is_exclusive || (write && v->loan.from_ptr) || body == NULL ||
        body->kind != NL_CHECKED_BLOCK || body->terminates ||
        body->result_count != v->result_count || v->result_count > 1)
        return false;
    if (v->field.present &&
        (!write || !bounded_field(f, &v->field, p) ||
         v->field.access != NL_ACCESS_WRITE ||
         v->loan.source != v->field.base || v->loan.place != v->field.child ||
         v->loan.incarnation != v->field.child_incarnation))
        return false;
    bool source_local = false;
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *r = nl_checked_node_view(f, i);
        if (r->kind == NL_CHECKED_RECEIVER && r->symbol == v->loan.source &&
            (v->loan.from_ptr
                 ? gate_type(f, r->type, NL_TYPE_PTR, p)
                 : r->type == (v->field.present ? v->field.nominal : p->u8)))
            source_local = true;
    }
    if (!source_local)
        return false;
    if (v->result_count == 1) {
        const NLCheckedNodeView *tail = nl_checked_node_view(f, body->tail);
        if (v->results[0].value != body->results[0].value)
            return false;
        if (write) {
            if (tail == NULL ||
                !gate_replace(f, body->tail, p, v->loan.ref_symbol))
                return false;
        } else if (!gate_ptr(f, body->tail, p) || tail == NULL ||
                   !tail->has_reference_result ||
                   tail->reference_result.place != v->loan.place ||
                   tail->reference_result.incarnation != v->loan.incarnation) {
            return false;
        }
        /* Keep the read C profile to the loan's outer root. A valid persistent
         * token for a body-local ended root must not become an indeterminate
         * C pointer read; richer token representation is a later slice. */
    }
    if (v->result_count == 0 && v->type != p->unit)
        return false;
    return v0_validate_node(f, v->initializer, p);
}

/* No Pair memory phi: both pure arms retain the identical source-issued
 * entry frame. The only Change is a later scoped write. This read-only
 * consumer couples existing IF lineage to existing fixed-field evidence;
 * it neither replays source nor creates facts/permissions. */
static bool copy_if_pair_frame(const NLCheckedFragment *f, NLCheckedNodeId id,
                               V0Program *p, size_t bindings)
{
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedNodeView *choice = nl_checked_node_view(f, id);
    const NLCheckedNodeView *cond =
        nl_checked_node_view(f, choice->initializer);
    const NLCheckedNodeView *aggregate = NULL, *declaration = NULL;
    const NLCheckedNodeView *loan = NULL, *write = NULL, *reads[2] = {0};
    const NLCheckedFragment *arms[2] = {nl_checked_if_arm(f, id, 0),
                                        nl_checked_if_arm(f, id, 1)};
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (arms[0] == NULL || arms[1] == NULL || arms[0] == arms[1] ||
        root == NULL || root->kind != NL_CHECKED_BLOCK || c->scope_count != 1 ||
        c->scopes[0].active || c->scopes[0].parent != 0 ||
        nl_sem_validate(c) != NL_CHECK_OK)
        return false;
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        switch (n->kind) {
        case NL_CHECKED_AGGREGATE:
            if (aggregate != NULL ||
                n->span.end_byte >= choice->span.start_byte ||
                !avs_construction(f, i + 1, p))
                return false;
            aggregate = n;
            break;
        case NL_CHECKED_AGGREGATE_FIELD:
        case NL_CHECKED_U8_LITERAL:
        case NL_CHECKED_USIZE_LITERAL:
        case NL_CHECKED_USIZE_EQUALITY:
        case NL_CHECKED_IDENTIFIER:
        case NL_CHECKED_BINDING:
        case NL_CHECKED_RECEIVER:
        case NL_CHECKED_STATEMENT:
        case NL_CHECKED_UNIT:
            break;
        case NL_CHECKED_BLOCK:
            if (n != root && (loan == NULL || loan->initializer != i + 1 ||
                              n->first_item != 0 || n->item_count != 0))
                return false;
            break;
        case NL_CHECKED_IF:
            if (i + 1 != id)
                return false;
            break;
        case NL_CHECKED_LOAN_HEADER:
            if (loan != NULL || !n->field.present ||
                n->span.start_byte <= choice->span.end_byte ||
                n->loan.scope != 1 || n->loan.domain != 0 ||
                !gate_loan(f, n, p))
                return false;
            loan = n;
            break;
        case NL_CHECKED_REPLACE:
            if (write != NULL || loan == NULL ||
                !gate_replace(f, i + 1, p, loan->loan.ref_symbol))
                return false;
            write = n;
            break;
        case NL_CHECKED_FIELD_READ:
            if (write == NULL || !bounded_field(f, &n->field, p) ||
                n->field.access != NL_ACCESS_READ || n->field.index >= 2 ||
                reads[n->field.index] != NULL ||
                n->span.start_byte <= loan->span.end_byte)
                return false;
            reads[n->field.index] = n;
            break;
        default:
            return false;
        }
    }
    if (aggregate == NULL || loan == NULL || write == NULL ||
        reads[0] == NULL || reads[1] == NULL || !write->field.present ||
        loan->field.index != 0 || write->field.index != 0)
        return false;
    for (size_t i = 0; i < f->count; ++i)
        if (f->nodes[i].kind == NL_CHECKED_BINDING &&
            nl_checked_node_view(f, f->nodes[i].initializer) == aggregate) {
            if (declaration != NULL)
                return false;
            declaration = &f->nodes[i];
        }
    if (declaration == NULL || declaration->symbol != 1 ||
        root->first_item != (size_t)(declaration - f->nodes) + 1 ||
        declaration->symbol > c->binding_count)
        return false;
    const NLCheckedField *e = &write->field;
    const NLCheckedNodeView *body = nl_checked_node_view(f, loan->initializer);
    const NLCheckedNodeView *ref =
        nl_checked_node_view(f, write->first_argument);
    const NLCheckedNodeView *input =
        ref == NULL ? NULL : nl_checked_node_view(f, ref->next_argument);
    if (body == NULL || nl_checked_node_view(f, body->tail) != write ||
        input == NULL || !copy_if_u8(f, ref->next_argument, id, p->u8) ||
        e->base != declaration->symbol || e->parent != 1 || e->child != 2 ||
        loan->loan.source != e->base || loan->loan.place != e->child ||
        e->new_value != input->results[0].value ||
        e->old_value != loan->results[0].value ||
        e->parent_post_fact <= e->parent_fact ||
        e->child_post_fact <= e->child_fact)
        return false;
    NLSemanticValueView reference;
    if (!nl_semantic_value_view(c, ref->results[0].value, &reference) ||
        reference.dependencies != NL_DEPENDENCY_FREE ||
        reference.value_dependency_count != 0 || reference.domain != 0 ||
        reference.allocation_region != 0 || reference.slot_place != 0 ||
        reference.occupancy.region != 0 || reference.field_count != 0 ||
        reference.aggregate_owner != 0 || reference.sum_owner != 0 ||
        reference.sum_payload != 0 || reference.reference_count != 0 ||
        reference.reference.place != e->child ||
        reference.reference.incarnation != e->child_incarnation ||
        reference.reference.scope != loan->loan.scope ||
        reference.reference.provenance != NL_PROVENANCE_VALID ||
        !reference.reference.readable || !reference.reference.writable ||
        reference.reference.occurrence_dependency != 0 ||
        reference.carrier != NL_CARRIER_ENDED || reference.owner_place != 0)
        return false;
    const NLSemanticContext *entry = nl_checked_context(arms[0]);
    if (entry == c || entry == nl_checked_context(arms[1]) || bindings == 0 ||
        entry->binding_count != bindings ||
        entry->place_count != bindings + 2 ||
        entry->place_count > c->place_count)
        return false;
    for (size_t a = 0; a < 2; ++a) {
        const NLSemanticContext *w = nl_checked_context(arms[a]);
        if (arms[a]->branch_value_prefix != cond->results[0].value ||
            w->value_count != cond->results[0].value + 1 ||
            w->binding_count != bindings || w->place_count != bindings + 2 ||
            w->scope_count != 0 || w->domain_count != 0 ||
            w->region_count != 0 || w->occurrence_count != 0 ||
            w->last_value_fact != arms[a]->branch_fact_prefix ||
            w->last_value_fact != w->place_count)
            return false;
        /* Bit equality is intentionally conservative: these are unmodified
         * copies of the same entry, not semantic equality of distinct IDs. */
        if (memcmp(w->places, entry->places,
                   entry->place_count * sizeof(*entry->places)) != 0 ||
            memcmp(w->values, entry->values,
                   arms[a]->branch_value_prefix * sizeof(*entry->values)) != 0)
            return false;
        for (size_t j = 0; j < bindings; ++j) {
            const NLSemanticBindingView b = w->bindings[j].view;
            const NLPlaceId place = j == 0 ? 1 : j + 3;
            if (memcmp(&b, &entry->bindings[j].view, sizeof(b)) != 0 ||
                w->bindings[j].hidden || b.availability != NL_AVAILABLE ||
                b.place != place || b.type != c->bindings[j].view.type ||
                b.place != c->bindings[j].view.place ||
                !w->places[place - 1].live ||
                !w->places[place - 1].independent_root ||
                w->places[place - 1].current_value != b.value)
                return false;
        }
        for (size_t j = 0; j < w->place_count; ++j) {
            const NLSemanticPlaceView place = w->places[j];
            if (!place.live || place.incarnation != j + 1 ||
                place.current_fact != j + 1 || place.governing_domain != 0 ||
                place.placement.region != 0 || place.parent_sum != 0 ||
                place.payload_occurrence != 0 ||
                place.incarnation != c->places[j].incarnation)
                return false;
        }
        const NLSemanticPlaceView original = w->places[0];
        if (original.type != aggregate->type || !original.implicit_local ||
            original.fixed_field_count != 2 || original.fixed_fields[0] != 2 ||
            original.fixed_fields[1] != 3 ||
            original.current_value != aggregate->results[0].value ||
            e->parent_incarnation != original.incarnation ||
            e->parent_fact != original.current_fact ||
            e->child_incarnation != w->places[1].incarnation ||
            e->child_fact != w->places[1].current_fact ||
            e->old_value != w->places[1].current_value)
            return false;
        for (size_t j = 0; j < arms[a]->branch_value_prefix; ++j) {
            NLSemanticValueView value = w->values[j];
            const NLSemanticValueView final = c->values[j];
            if (value.type != final.type ||
                value.scalar_known != final.scalar_known ||
                value.scalar_value != final.scalar_value)
                return false;
            if (value.type == p->aggregate) {
                if (j + 1 != original.current_value || value.field_count != 2 ||
                    value.carrier != NL_CARRIER_PLACE || value.owner_place != 1)
                    return false;
                value.field_count = 0;
            }
            if (value.carrier == NL_CARRIER_AGGREGATE) {
                if ((j + 1 != w->places[1].current_value &&
                     j + 1 != w->places[2].current_value) ||
                    value.aggregate_owner != original.current_value)
                    return false;
                value.aggregate_owner = 0;
            }
            if (!copy_if_plain(value, final.type))
                return false;
        }
        if (nl_sem_validate(w) != NL_CHECK_OK)
            return false;
    }
    for (NLCheckedNodeId child = aggregate->first_argument; child != 0;) {
        const NLCheckedNodeView *field = nl_checked_node_view(f, child);
        const NLCheckedNodeView *literal =
            field == NULL ? NULL : nl_checked_node_view(f, field->initializer);
        if (field == NULL || field->field_index >= 2 ||
            !copy_if_literal(f, literal) ||
            literal->results[0].value !=
                entry->places[field->field_index + 1].current_value)
            return false;
        child = field->next_argument;
    }
    size_t intermediate_bindings = 0;
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        if (n->kind == NL_CHECKED_BINDING &&
            n->span.end_byte >= choice->span.end_byte &&
            n->span.end_byte < loan->span.start_byte) {
            if (!copy_if_u8(f, n->initializer, id, p->u8))
                return false;
            ++intermediate_bindings;
        }
    }
    /* Fresh sequential facts: joined Copy bindings, one ref binder, then
     * child and parent Change facts. Arm-private facts never enter parent. */
    if (intermediate_bindings == 0 ||
        e->child_post_fact !=
            entry->last_value_fact + intermediate_bindings + 2 ||
        e->parent_post_fact != e->child_post_fact + 1 ||
        e->parent_post_fact > c->last_value_fact)
        return false;
    for (size_t i = 0; i < f->count; ++i) {
        const NLCheckedNodeView *n = &f->nodes[i];
        if (n->kind == NL_CHECKED_BINDING &&
            n->span.end_byte <= choice->span.start_byte) {
            const NLCheckedNodeView *init =
                nl_checked_node_view(f, n->initializer);
            if (init == NULL || n->symbol > bindings || n->symbol == 0 ||
                entry->bindings[n->symbol - 1].view.value !=
                    init->results[0].value ||
                (init != aggregate && !quantity_scalar(f, n->initializer, p) &&
                 !boolean_scalar(f, n->initializer, p) &&
                 !copy_if_u8(f, n->initializer, id, p->u8)))
                return false;
        }
    }
    const NLCheckedField *fields[3] = {&loan->field, &reads[0]->field,
                                       &reads[1]->field};
    for (size_t i = 0; i < 3; ++i) {
        const NLCheckedField *r = fields[i];
        const size_t index = i == 2 ? 1 : 0;
        const NLSemanticPlaceView before = entry->places[index + 1];
        if (r->base != e->base || r->nominal != e->nominal ||
            r->parent_post_fact != 0 || r->child_post_fact != 0 ||
            r->index != index || r->payload_occurrence != 0 ||
            r->post_payload_occurrence != 0 || r->parent != e->parent ||
            r->parent_incarnation != e->parent_incarnation ||
            r->child != index + 2 ||
            r->child_incarnation != before.incarnation ||
            r->parent_fact != (i == 0 ? e->parent_fact : e->parent_post_fact) ||
            r->child_fact !=
                (i == 1 ? e->child_post_fact : before.current_fact) ||
            r->old_value != (i == 1 ? e->new_value : before.current_value))
            return false;
    }
    for (size_t i = 0; i < 2; ++i) {
        NLSemanticValueView observed, expected;
        if (reads[i]->result_count != 1 ||
            !nl_semantic_value_view(c, reads[i]->results[0].value, &observed) ||
            !nl_semantic_value_view(c, reads[i]->field.old_value, &expected) ||
            !copy_if_plain(observed, p->u8) ||
            observed.scalar_known != expected.scalar_known ||
            observed.scalar_value != expected.scalar_value)
            return false;
    }
    const NLSemanticBindingView retained = c->bindings[0].view;
    NLSemanticValueView recomposed;
    NLSemanticScopeView scope;
    return nl_semantic_scope_view(c, loan->loan.scope, &scope) &&
           !scope.active &&
           nl_semantic_value_view(c, retained.value, &recomposed) &&
           recomposed.type == p->aggregate && recomposed.field_count == 2 &&
           recomposed.fields[0] == e->new_value &&
           recomposed.fields[1] == entry->places[2].current_value &&
           recomposed.carrier == NL_CARRIER_ENDED;
}

/* Driver-private proof: the actual owned synchronous body must belong to
 * this exact registration. Copy arguments retain package identity when bound
 * to the fresh formal. Names locate that binding in the immutable registry;
 * neither source tokens nor scalar_known can supply runtime arguments. */
static bool v1_call_parameter(const NLCheckedFragment *f, NLCheckedNodeId id,
                              const V0Program *p, NLSymbolId *parameter)
{
    const NLCheckedNodeView *call = nl_checked_node_view(f, id);
    const NLSemanticContext *c = nl_checked_context(f);
    const NLCheckedFragment *body = nl_checked_call_body(f, id);
    if (call == NULL || c == NULL || body == NULL || call->function == 0 ||
        call->function > c->function_count || nl_checked_context(body) != c)
        return false;
    const NLFunctionEntry *fn = &c->functions[call->function - 1];
    if (fn->kind != NL_CHECKED_REGISTERED_CALL || fn->body == NULL ||
        body->body_owner != fn->body || body->source != fn->body->source ||
        fn->result != call->type || fn->count != call->argument_count ||
        fn->body->count != fn->count || fn->count > 1 || fn->caller_effects ||
        fn->hidden_dependencies || fn->owner_receiver || fn->owner_producer ||
        fn->custody_recipient)
        return false;
    *parameter = 0;
    if (fn->count == 0)
        return call->type == p->unit && call->first_argument == 0;
    const NLCheckedNodeView *arg =
        nl_checked_node_view(f, call->first_argument);
    if (fn->parameters[0] != p->u8 || call->type != p->u8 || arg == NULL ||
        arg->next_argument != 0 || arg->parameter_type != p->u8 ||
        !v1_scalar(f, call->first_argument, p))
        return false;
    for (size_t i = 0; i < c->binding_count; ++i) {
        const NLBindingEntry *binding = &c->bindings[i];
        if (binding->view.value == arg->results[0].value &&
            binding->view.type == p->u8 && binding->hidden &&
            strcmp(binding->name, fn->body->parameter_names[0]) == 0) {
            if (*parameter != 0)
                return false;
            *parameter = i + 1;
        }
    }
    return *parameter != 0;
}

/* Finite scalar callee: Copy locals and a scalar tail/return. Only the tail
 * may additionally be one authenticated u8 call, within two scalar frames.
 * No call arguments containing calls, refs, writes, branches or general ABI. */
static bool v1_copy_body(const NLCheckedFragment *f, V0Program *p,
                         NLCheckedNodeId *result)
{
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (root == NULL || root->kind != NL_CHECKED_BLOCK)
        return false;
    NLCheckedNodeId last = 0;
    size_t count = 0;
    for (NLCheckedNodeId i = root->first_item; i != 0;) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v == NULL || ++count > nl_checked_node_count(f))
            return false;
        if (v->kind == NL_CHECKED_BINDING) {
            const NLCheckedNodeView *r =
                nl_checked_node_view(f, v->first_argument);
            if (v->argument_count != 1 || r == NULL ||
                r->kind != NL_CHECKED_RECEIVER || r->next_argument != 0 ||
                r->type != p->u8 || r->symbol != v->symbol ||
                !v1_scalar(f, v->initializer, p))
                return false;
        } else if (v->kind == NL_CHECKED_STATEMENT) {
            if (!v1_scalar(f, v->initializer, p))
                return false;
        } else if (v->kind == NL_CHECKED_RETURN) {
            if (v->next_item != 0 || !v->terminates ||
                v->returned.type != p->u8 || !v1_scalar(f, v->initializer, p))
                return false;
        } else {
            return false;
        }
        last = i;
        i = v->next_item;
    }
    if (count != root->item_count)
        return false;
    const NLControlExits *exits = nl_checked_control_exits(f);
    if (root->terminates) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, last);
        if (v == NULL || v->kind != NL_CHECKED_RETURN || root->tail != 0 ||
            root->result_count != 0 || nl_control_exits_count(exits) != 1 ||
            !nl_control_exits_only_return(exits, f->function_target))
            return false;
        *result = v->initializer;
        const NLCheckedNodeView *r = nl_checked_node_view(f, *result);
        return v->returned.value == r->results[0].value;
    }
    *result = root->tail;
    const NLCheckedNodeView *tail = nl_checked_node_view(f, *result);
    return nl_control_exits_count(exits) == 0 && root->type == p->u8 &&
           root->result_count == 1 && root->results[0].type == p->u8 &&
           tail != NULL &&
           (v1_scalar(f, *result, p) ||
            (p->scalar_depth == 1 && tail->kind == NL_CHECKED_REGISTERED_CALL &&
             v0_validate_call(f, *result, p))) &&
           root->results[0].value == tail->results[0].value;
}

static bool v0_validate_call(const NLCheckedFragment *fragment,
                             NLCheckedNodeId id, V0Program *program)
{
    const NLCheckedNodeView *call = nl_checked_node_view(fragment, id);
    if (call == NULL || call->kind != NL_CHECKED_REGISTERED_CALL ||
        !call->body_backed || call->owner_call.definition.definition_checked ||
        call->terminates)
        return false;
    NLSymbolId parameter = 0;
    if (!v1_call_parameter(fragment, id, program, &parameter))
        return false;
    const bool scalar = parameter != 0;
    if (scalar
            ? (call->result_count != 1 || call->results[0].type != program->u8)
            : (call->result_count != 0))
        return false;
    const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
    if (scalar) {
        if (program->scalar_depth >= 2)
            return false;
        ++program->scalar_depth;
        const NLCheckedFragment *saved_body = program->formal_body;
        const NLSymbolId saved_formal = program->formal;
        program->formal_body = body;
        program->formal = parameter;
        NLCheckedNodeId returned = 0;
        const bool valid =
            v1_copy_body(body, program, &returned) &&
            call->results[0].value ==
                nl_checked_node_view(body, returned)->results[0].value;
        program->formal_body = saved_body;
        program->formal = saved_formal;
        --program->scalar_depth;
        if (!valid)
            return false;
    }
    V0Function *prior = v0_find_function(program, call->function);
    if (prior != NULL)
        return prior->result == call->type && (prior->parameter != 0) == scalar;
    if (program->count == NL_SEMANTIC_MAX_FUNCTION_DECLARATIONS)
        return false;
    program->functions[program->count++] =
        (V0Function){call->function, body, call->type, parameter};
    return scalar || v0_validate_node(body, nl_checked_root(body), program);
}

static bool v0_validate_block(const NLCheckedFragment *fragment,
                              const NLCheckedNodeView *block,
                              V0Program *program)
{
    size_t count = 0;
    NLCheckedNodeId item = block->first_item;
    while (item != 0) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, item);
        if (view == NULL || !v0_validate_node(fragment, item, program))
            return false;
        item = view->next_item;
        ++count;
    }
    if (count != block->item_count)
        return false;
    return block->tail == 0 || v0_validate_node(fragment, block->tail, program);
}

static bool v0_validate_node(const NLCheckedFragment *fragment,
                             NLCheckedNodeId id, V0Program *program)
{
    const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
    if (view == NULL)
        return false;

    switch (view->kind) {
    case NL_CHECKED_FIELD_READ:
    case NL_CHECKED_U8_LITERAL:
    case NL_CHECKED_USIZE_LITERAL:
    case NL_CHECKED_USIZE_EQUALITY:
    case NL_CHECKED_IDENTIFIER:
        return v1_scalar(fragment, id, program) ||
               quantity_scalar(fragment, id, program) ||
               boolean_scalar(fragment, id, program) ||
               gate_ptr(fragment, id, program);
    case NL_CHECKED_PTR_FROM_REF:
        return gate_ptr(fragment, id, program);
    case NL_CHECKED_REPLACE:
        return gate_replace(fragment, id, program, 0);
    case NL_CHECKED_LOAN_HEADER:
        return gate_loan(fragment, view, program);
    case NL_CHECKED_AGGREGATE_BINDING:
        return avs_destructuring(fragment, view, program);
    case NL_CHECKED_BINDING: {
        const NLCheckedNodeView *receiver =
            nl_checked_node_view(fragment, view->first_argument);
        const NLCheckedNodeView *initializer =
            nl_checked_node_view(fragment, view->initializer);
        return view->type == program->unit && view->argument_count == 1 &&
               view->symbol != 0 && receiver != NULL && initializer != NULL &&
               receiver->kind == NL_CHECKED_RECEIVER &&
               receiver->symbol == view->symbol &&
               receiver->type == initializer->type &&
               (quantity_scalar(fragment, view->initializer, program) ||
                boolean_scalar(fragment, view->initializer, program) ||
                (receiver->type == program->u8 &&
                 ((initializer->kind == NL_CHECKED_IF &&
                   copy_if_selection(fragment, view->initializer, program)) ||
                  v1_scalar(fragment, view->initializer, program) ||
                  (initializer->kind == NL_CHECKED_REGISTERED_CALL &&
                   initializer->type == program->u8 &&
                   v0_validate_call(fragment, view->initializer, program)) ||
                  (initializer->kind == NL_CHECKED_LOAN_HEADER &&
                   initializer->loan.access == NL_ACCESS_WRITE &&
                   v0_validate_node(fragment, view->initializer, program)))) ||
                (avs_shape(fragment, receiver->type, program) &&
                 avs_construction(fragment, view->initializer, program)) ||
                (gate_type(fragment, receiver->type, NL_TYPE_PTR, program) &&
                 v0_validate_node(fragment, view->initializer, program)));
    }
    case NL_CHECKED_BLOCK:
        return v0_validate_block(fragment, view, program);
    case NL_CHECKED_STATEMENT:
        return view->initializer != 0 &&
               v0_validate_node(fragment, view->initializer, program);
    case NL_CHECKED_REGISTERED_CALL:
        return v0_validate_call(fragment, id, program);
    case NL_CHECKED_RETURN:
        return view->terminates && view->initializer != 0 &&
               view->returned.type == program->unit &&
               v0_validate_node(fragment, view->initializer, program);
    case NL_CHECKED_UNIT:
        return view->type == program->unit && view->result_count == 0;
    default:
        return false;
    }
}

static bool v0_indent(FILE *stream, size_t depth)
{
    for (size_t i = 0; i < depth; ++i)
        if (fputs("    ", stream) < 0)
            return false;
    return true;
}

static bool v0_emit_node(FILE *, const NLCheckedFragment *, NLCheckedNodeId,
                         size_t);

static bool v0_emit_block(FILE *stream, const NLCheckedFragment *fragment,
                          const NLCheckedNodeView *block, size_t depth)
{
    if (!v0_indent(stream, depth) || fputs("{\n", stream) < 0)
        return false;

    NLCheckedNodeId item = block->first_item;
    while (item != 0) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, item);
        if (view == NULL || !v0_emit_node(stream, fragment, item, depth + 1))
            return false;
        item = view->next_item;
    }
    if (block->tail != 0 &&
        !v0_emit_node(stream, fragment, block->tail, depth + 1))
        return false;
    return v0_indent(stream, depth) && fputs("}\n", stream) >= 0;
}

static bool avs_emit_initializer(FILE *stream,
                                 const NLCheckedFragment *fragment,
                                 const NLCheckedNodeView *aggregate)
{
    if (fputs("{ ", stream) < 0)
        return false;
    for (NLCheckedNodeId id = aggregate->first_argument; id != 0;) {
        const NLCheckedNodeView *field = nl_checked_node_view(fragment, id);
        const NLCheckedNodeView *value =
            nl_checked_node_view(fragment, field->initializer);
        if (fprintf(stream, ".f%zu = ", field->field_index) < 0 ||
            !v1_emit_scalar(stream, value) || fputs(", ", stream) < 0)
            return false;
        id = field->next_argument;
    }
    return fputs("}", stream) >= 0;
}

static bool gate_emit_ptr(FILE *stream, const NLCheckedFragment *f,
                          const NLCheckedNodeView *v)
{
    if (v->kind == NL_CHECKED_LOAN_HEADER)
        return fprintf(stream, "nl_loan_result_%zu", v->loan.scope) >= 0;
    if (v->kind == NL_CHECKED_PTR_FROM_REF)
        v = nl_checked_node_view(f, v->first_argument);
    return v != NULL && v->kind == NL_CHECKED_IDENTIFIER &&
           fprintf(stream, "nl_local_%zu", v->symbol) >= 0;
}
static bool gate_emit_replace(FILE *stream, const NLCheckedFragment *f,
                              NLCheckedNodeId id, size_t depth, bool discard)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    const NLCheckedNodeView *ref =
        v == NULL ? NULL : nl_checked_node_view(f, v->first_argument);
    const NLCheckedNodeView *value =
        ref == NULL ? NULL : nl_checked_node_view(f, ref->next_argument);
    if (v == NULL || ref == NULL || value == NULL)
        return false;
    if (!v0_indent(stream, depth) ||
        fprintf(stream, "const uint8_t nl_replace_old_%zu = *nl_local_%zu;\n",
                id, ref->symbol) < 0 ||
        !v0_indent(stream, depth) ||
        fprintf(stream, "*nl_local_%zu = ", ref->symbol) < 0 ||
        !v1_emit_scalar(stream, value) || fputs(";\n", stream) < 0)
        return false;
    return !discard ||
           (v0_indent(stream, depth) &&
            fprintf(stream, "(void)nl_replace_old_%zu;\n", id) >= 0);
}
static bool gate_emit_loan(FILE *stream, const NLCheckedFragment *f,
                           const NLCheckedNodeView *v, size_t depth)
{
    const NLCheckedNodeView *body = nl_checked_node_view(f, v->initializer);
    const bool write = v->loan.access == NL_ACCESS_WRITE;
    if (v->result_count == 1) {
        if (!v0_indent(stream, depth))
            return false;
        if (write) {
            if (fprintf(stream, "uint8_t nl_loan_result_%zu;\n",
                        v->loan.scope) < 0)
                return false;
        } else if (fprintf(stream, "const uint8_t *nl_loan_result_%zu;\n",
                           v->loan.scope) < 0) {
            return false;
        }
    }
    if (!v0_indent(stream, depth) || fputs("{\n", stream) < 0 ||
        !v0_indent(stream, depth + 1))
        return false;
    if (write) {
        if (fprintf(stream, "uint8_t *const nl_local_%zu = &nl_local_%zu",
                    v->loan.ref_symbol, v->loan.source) < 0 ||
            (v->field.present &&
             fprintf(stream, ".f%zu", v->field.index) < 0) ||
            fputs(";\n", stream) < 0)
            return false;
    } else if (fprintf(stream,
                       "const uint8_t *const nl_local_%zu = %snl_local_%zu;\n",
                       v->loan.ref_symbol, v->loan.from_ptr ? "" : "&",
                       v->loan.source) < 0) {
        return false;
    }
    if (!v0_indent(stream, depth + 1) ||
        fprintf(stream, "(void)nl_local_%zu;\n", v->loan.ref_symbol) < 0)
        return false;
    for (NLCheckedNodeId i = body->first_item; i != 0;) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (!v0_emit_node(stream, f, i, depth + 1))
            return false;
        i = n->next_item;
    }
    if (v->result_count == 1) {
        if (write) {
            if (!gate_emit_replace(stream, f, body->tail, depth + 1, false) ||
                !v0_indent(stream, depth + 1) ||
                fprintf(stream, "nl_loan_result_%zu = nl_replace_old_%zu;\n",
                        v->loan.scope, body->tail) < 0)
                return false;
        } else {
            if (!v0_indent(stream, depth + 1))
                return false;
            if (fprintf(stream, "nl_loan_result_%zu = ", v->loan.scope) < 0)
                return false;
            if (!gate_emit_ptr(stream, f,
                               nl_checked_node_view(f, body->tail)) ||
                fputs(";\n", stream) < 0)
                return false;
        }
    } else if (body->tail != 0 &&
               !v0_emit_node(stream, f, body->tail, depth + 1)) {
        return false;
    }
    if (!v0_indent(stream, depth) || fputs("}\n", stream) < 0)
        return false;
    return v->result_count == 0 ||
           (v0_indent(stream, depth) &&
            fprintf(stream, "(void)nl_loan_result_%zu;\n", v->loan.scope) >= 0);
}
static bool gate_symbol_written(const NLCheckedFragment *f, NLSymbolId symbol)
{
    for (size_t i = 1; i <= nl_checked_node_count(f); ++i) {
        const NLCheckedNodeView *n = nl_checked_node_view(f, i);
        if (n != NULL && n->kind == NL_CHECKED_LOAN_HEADER &&
            n->loan.implicit_local && !n->loan.from_ptr &&
            n->loan.access == NL_ACCESS_WRITE && n->loan.source == symbol)
            return true;
    }
    return false;
}

static bool v1_emit_call(FILE *stream, const NLCheckedFragment *f,
                         NLCheckedNodeId id, size_t depth)
{
    const NLCheckedNodeView *v = nl_checked_node_view(f, id);
    if (!v0_indent(stream, depth) ||
        fprintf(stream, "const uint8_t nl_call_%zu = nl_fn_%zu(", id,
                v->function) < 0 ||
        !v1_emit_scalar(stream, nl_checked_node_view(f, v->first_argument)) ||
        fputs(");\n", stream) < 0 || !v0_indent(stream, depth))
        return false;
    return fprintf(stream, "(void)nl_call_%zu;\n", id) >= 0;
}

static bool v0_emit_node(FILE *stream, const NLCheckedFragment *fragment,
                         NLCheckedNodeId id, size_t depth)
{
    const NLCheckedNodeView *view = nl_checked_node_view(fragment, id);
    if (view == NULL)
        return false;

    switch (view->kind) {
    case NL_CHECKED_FIELD_READ:
    case NL_CHECKED_U8_LITERAL:
    case NL_CHECKED_USIZE_LITERAL:
    case NL_CHECKED_USIZE_EQUALITY:
    case NL_CHECKED_IDENTIFIER:
    case NL_CHECKED_PTR_FROM_REF:
        return v0_indent(stream, depth) && fputs("(void)", stream) >= 0 &&
               (view->kind == NL_CHECKED_PTR_FROM_REF
                    ? gate_emit_ptr(stream, fragment, view)
                    : emit_quantity_bool(stream, fragment, view)) &&
               fputs(";\n", stream) >= 0;
    case NL_CHECKED_REPLACE:
        return gate_emit_replace(stream, fragment, id, depth, true);
    case NL_CHECKED_LOAN_HEADER:
        return gate_emit_loan(stream, fragment, view, depth);
    case NL_CHECKED_AGGREGATE_BINDING: {
        const NLCheckedNodeView *rhs =
            nl_checked_node_view(fragment, view->initializer);
        if (!v0_indent(stream, depth) ||
            fprintf(stream,
                    "const nl_type_%zu nl_destructure_%zu = nl_local_%zu;\n",
                    rhs->type, id, rhs->symbol) < 0)
            return false;
        for (NLCheckedNodeId receiver = view->first_argument; receiver != 0;) {
            const NLCheckedNodeView *v =
                nl_checked_node_view(fragment, receiver);
            if (!v0_indent(stream, depth) ||
                fprintf(
                    stream,
                    "const uint8_t nl_local_%zu = nl_destructure_%zu.f%zu;\n",
                    v->symbol, id, v->field_index) < 0 ||
                !v0_indent(stream, depth) ||
                fprintf(stream, "(void)nl_local_%zu;\n", v->symbol) < 0)
                return false;
            receiver = v->next_argument;
        }
        return true;
    }
    case NL_CHECKED_BINDING: {
        const NLCheckedNodeView *initializer =
            nl_checked_node_view(fragment, view->initializer);
        const char *const scalar_qualifier =
            gate_symbol_written(fragment, view->symbol) ? "" : "const ";
        if (initializer == NULL)
            return false;
        if (initializer->kind == NL_CHECKED_REGISTERED_CALL &&
            !v1_emit_call(stream, fragment, view->initializer, depth))
            return false;
        if (initializer->kind == NL_CHECKED_LOAN_HEADER &&
            !gate_emit_loan(stream, fragment, initializer, depth))
            return false;
        if (!v0_indent(stream, depth))
            return false;
        if (initializer->kind == NL_CHECKED_AGGREGATE) {
            if (fprintf(stream,
                        "%snl_type_%zu nl_local_%zu = ", scalar_qualifier,
                        initializer->type, view->symbol) < 0 ||
                !avs_emit_initializer(stream, fragment, initializer))
                return false;
        } else if (initializer->kind == NL_CHECKED_IF) {
            const NLCheckedNodeView *condition =
                nl_checked_node_view(fragment, initializer->initializer);
            if (fprintf(stream, "const uint8_t nl_local_%zu = (",
                        view->symbol) < 0 ||
                !v1_emit_scalar(stream, condition) || fputs(" ? ", stream) < 0)
                return false;
            for (size_t a = 0; a < 2; ++a) {
                const NLCheckedFragment *arm =
                    nl_checked_if_arm(fragment, view->initializer, a);
                const NLCheckedNodeView *root =
                    nl_checked_node_view(arm, nl_checked_root(arm));
                const NLCheckedNodeView *block =
                    nl_checked_node_view(arm, root->initializer);
                if (!v1_emit_scalar(stream,
                                    nl_checked_node_view(arm, block->tail)) ||
                    (a == 0 && fputs(" : ", stream) < 0))
                    return false;
            }
            if (fputc(')', stream) == EOF)
                return false;
        } else if (initializer->type ==
                   nl_semantic_core_type(nl_checked_context(fragment),
                                         NL_TYPE_USIZE)) {
            if (fprintf(stream, "const size_t nl_local_%zu = ", view->symbol) <
                    0 ||
                !v1_emit_scalar(stream, initializer))
                return false;
        } else if (initializer->type ==
                   nl_semantic_core_type(nl_checked_context(fragment),
                                         NL_TYPE_BOOL)) {
            if (fprintf(stream, "const _Bool nl_local_%zu = ", view->symbol) <
                    0 ||
                !emit_quantity_bool(stream, fragment, initializer))
                return false;
        } else if ((initializer->kind == NL_CHECKED_LOAN_HEADER &&
                    initializer->loan.access == NL_ACCESS_READ) ||
                   initializer->kind == NL_CHECKED_PTR_FROM_REF ||
                   (initializer->kind == NL_CHECKED_IDENTIFIER &&
                    !initializer->has_scalar_result &&
                    initializer->type !=
                        nl_semantic_core_type(nl_checked_context(fragment),
                                              NL_TYPE_U8))) {
            if (fprintf(stream, "const uint8_t *const nl_local_%zu = ",
                        view->symbol) < 0 ||
                !gate_emit_ptr(stream, fragment, initializer))
                return false;
        } else if (initializer->kind == NL_CHECKED_LOAN_HEADER &&
                   initializer->loan.access == NL_ACCESS_WRITE) {
            if (fprintf(stream, "%suint8_t nl_local_%zu = nl_loan_result_%zu",
                        scalar_qualifier, view->symbol,
                        initializer->loan.scope) < 0)
                return false;
        } else if (initializer->kind == NL_CHECKED_REGISTERED_CALL) {
            if (fprintf(stream, "const uint8_t nl_local_%zu = nl_call_%zu",
                        view->symbol, view->initializer) < 0)
                return false;
        } else if (fprintf(stream, "%suint8_t nl_local_%zu = ",
                           scalar_qualifier, view->symbol) < 0 ||
                   !v1_emit_scalar(stream, initializer))
            return false;
        if (fputs(";\n", stream) < 0)
            return false;
        /* Suppress C unused-local warnings even when NewLang never reads this
         * Discardable binding. This emits no language-level operation. */
        return v0_indent(stream, depth) &&
               fprintf(stream, "(void)nl_local_%zu;\n", view->symbol) >= 0;
    }
    case NL_CHECKED_BLOCK:
        return v0_emit_block(stream, fragment, view, depth);
    case NL_CHECKED_STATEMENT:
        return v0_emit_node(stream, fragment, view->initializer, depth);
    case NL_CHECKED_REGISTERED_CALL:
        if (view->result_count == 1)
            return v1_emit_call(stream, fragment, id, depth);
        return v0_indent(stream, depth) &&
               fprintf(stream, "nl_fn_%zu();\n", view->function) >= 0;
    case NL_CHECKED_RETURN:
        if (!v0_emit_node(stream, fragment, view->initializer, depth))
            return false;
        return v0_indent(stream, depth) && fputs("return;\n", stream) >= 0;
    case NL_CHECKED_UNIT:
        return v0_indent(stream, depth) && fputs("(void)0;\n", stream) >= 0;
    default:
        return false;
    }
}

static bool v1_emit_body(FILE *stream, const V0Function *fn)
{
    const NLCheckedFragment *f = fn->body;
    const NLCheckedNodeView *root = nl_checked_node_view(f, nl_checked_root(f));
    if (fputs("{\n", stream) < 0 ||
        fprintf(stream, "    (void)nl_local_%zu;\n", fn->parameter) < 0)
        return false;
    for (NLCheckedNodeId i = root->first_item; i != 0;) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, i);
        if (v->kind == NL_CHECKED_RETURN) {
            if (fputs("    return ", stream) < 0 ||
                !v1_emit_scalar(stream,
                                nl_checked_node_view(f, v->initializer)) ||
                fputs(";\n", stream) < 0)
                return false;
        } else if (!v0_emit_node(stream, f, i, 1)) {
            return false;
        }
        i = v->next_item;
    }
    if (root->tail != 0) {
        const NLCheckedNodeView *tail = nl_checked_node_view(f, root->tail);
        if (tail->kind == NL_CHECKED_REGISTERED_CALL) {
            if (!v1_emit_call(stream, f, root->tail, 1) ||
                fprintf(stream, "    return nl_call_%zu;\n", root->tail) < 0)
                return false;
        } else if (fputs("    return ", stream) < 0 ||
                   !v1_emit_scalar(stream, tail) || fputs(";\n", stream) < 0) {
            return false;
        }
    }
    return fputs("}\n", stream) >= 0;
}

static bool v0_emit_signature(FILE *stream, const V0Function *fn)
{
    if (fn->parameter != 0)
        return fprintf(stream, "static uint8_t nl_fn_%zu(uint8_t nl_local_%zu)",
                       fn->function, fn->parameter) >= 0;
    return fprintf(stream, "static void nl_fn_%zu(void)", fn->function) >= 0;
}

static bool v0_emit_c(FILE *stream, const V0Program *program)
{
    if (fputs(
            "/* North Star V0/V1/AVS Checked-C reference output. */\n#include "
            "<stdint.h>\n\n",
            stream) < 0)
        return false;

    /* The frontend currently has a native-size_t quantity target, not a
     * cross-target integer model. Refuse mismatched C targets explicitly. */
    if (program->uses_usize &&
        fprintf(stream,
                "#include <stddef.h>\n"
                "_Static_assert(SIZE_MAX == UINTMAX_C(%zu), "
                "\"NewLang usize target mismatch\");\n\n",
                SIZE_MAX) < 0)
        return false;

    if (program->aggregate != 0 &&
        fprintf(stream,
                "typedef struct { uint8_t f0; uint8_t f1; } nl_type_%zu;\n\n",
                program->aggregate) < 0)
        return false;

    for (size_t i = 0; i < program->count; ++i)
        if (!v0_emit_signature(stream, &program->functions[i]) ||
            fputs(";\n", stream) < 0)
            return false;
    if (fputc('\n', stream) == EOF)
        return false;

    for (size_t i = 0; i < program->count; ++i) {
        const V0Function function = program->functions[i];
        const NLCheckedNodeView *root =
            nl_checked_node_view(function.body, nl_checked_root(function.body));
        if (root == NULL || !v0_emit_signature(stream, &function) ||
            fputc('\n', stream) == EOF ||
            !(function.parameter != 0
                  ? v1_emit_body(stream, &function)
                  : v0_emit_block(stream, function.body, root, 0)) ||
            fputc('\n', stream) == EOF)
            return false;
    }

    return fputs("int main(void)\n{\n", stream) >= 0 &&
           fprintf(stream, "    nl_fn_%zu();\n", program->entry_function) >=
               0 &&
           fputs("    return 0;\n}\n", stream) >= 0;
}

static int render_parse_failure(const NLSource *source,
                                const NLParseDiagnostic *diagnostic)
{
    if (diagnostic->diagnostic.code != NULL &&
        nl_parse_diagnostic_render(stderr, source, diagnostic))
        return 3;
    return report_error("V0-PARSE", "source parsing failed",
                        nl_source_name(source), 3);
}

static int render_check_failure(const NLSource *source,
                                const NLCheckDiagnostic *diagnostic)
{
    if (diagnostic->diagnostic.code != NULL &&
        nl_check_diagnostic_render(stderr, source, diagnostic))
        return 3;
    return report_error("V0-CHECK", "semantic validation failed",
                        nl_source_name(source), 3);
}

static int compile_v0(const char *path, bool check_only)
{
    static const char entry_text[] = "main()";
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    int result = 1;

    if (nl_source_load(path, &source) != NL_SOURCE_OK) {
        result = report_error("V0-SOURCE-IO", "cannot read source", path, 1);
        goto cleanup;
    }

    NLParseDiagnostic parse_diagnostic = {0};
    NLParseStatus parse = nl_parser_create(source, &parser);
    if (parse != NL_PARSE_OK) {
        result = report_error("V0-PARSER", "cannot create parser", path, 1);
        goto cleanup;
    }
    parse = nl_parser_parse_function_unit(parser, &unit, &parse_diagnostic);
    if (parse != NL_PARSE_OK) {
        result = render_parse_failure(source, &parse_diagnostic);
        goto cleanup;
    }

    if (nl_semantic_create(&context) != NL_CHECK_OK) {
        result = report_error("V0-CHECKER", "cannot create semantic context",
                              path, 1);
        goto cleanup;
    }

    const NLSyntaxTree *inputs[] = {unit};
    NLFunctionUnitDiagnostic unit_diagnostic = {0};
    const NLCheckStatus registered = nl_semantic_register_function_unit(
        context, inputs, 1, &unit_diagnostic);
    if (registered != NL_CHECK_OK) {
        result = render_check_failure(source, &unit_diagnostic.diagnostic);
        goto cleanup;
    }

    if (nl_source_create(entry_text, sizeof(entry_text) - 1, "<v0-entry>",
                         &entry_source) != NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK) {
        result = report_error("V0-ENTRY", "cannot create checked entry call",
                              path, 1);
        goto cleanup;
    }

    parse_diagnostic = (NLParseDiagnostic){0};
    parse = nl_parser_parse_expression_fragment(entry_parser, &entry_tree,
                                                &parse_diagnostic);
    if (parse != NL_PARSE_OK) {
        result = render_parse_failure(entry_source, &parse_diagnostic);
        goto cleanup;
    }

    NLCheckDiagnostic check_diagnostic = {0};
    const NLCheckStatus checked = nl_semantic_check_expression(
        context, entry_tree, &entry, &check_diagnostic);
    if (checked != NL_CHECK_OK) {
        result = render_check_failure(entry_source, &check_diagnostic);
        goto cleanup;
    }

    /* A real owned entry has been checked. No backend admission or output is
     * needed in this mode; keep the same destruction order on every exit. */
    if (check_only) {
        result = 0;
        goto cleanup;
    }

    const NLCheckedNodeId root = nl_checked_root(entry);
    const NLCheckedNodeView *root_view = nl_checked_node_view(entry, root);
    V0Program program = {.unit = nl_semantic_unit_type(context),
                         .u8 = nl_semantic_core_type(context, NL_TYPE_U8)};
    if (root_view == NULL || !v0_validate_call(entry, root, &program)) {
        char *node_c = NULL;
        size_t node_length = 0;
        const NLNodeCStatus status =
            nl_checked_c_node(entry, &node_c, &node_length);
        if (status == NL_NODE_C_OK) {
            result = fwrite(node_c, 1, node_length, stdout) == node_length &&
                             fflush(stdout) == 0
                         ? 0
                         : report_error("V0-C-OUTPUT",
                                        "failed to emit Checked-C", path, 1);
            free(node_c);
            goto cleanup;
        }
        if (status == NL_NODE_C_OUT_OF_MEMORY ||
            status == NL_NODE_C_RESOURCE_LIMIT) {
            result = report_error("NODE-C-RESOURCE",
                                  "bounded Checked-C emission resource failure",
                                  path, 1);
            goto cleanup;
        }
        result = report_error("V1-BACKEND-UNSUPPORTED",
                              "accepted program uses a construct outside the "
                              "V0/V1 Checked-C spine",
                              path, 4);
        goto cleanup;
    }
    program.entry_function = root_view->function;

    if (!v0_emit_c(stdout, &program) || fflush(stdout) != 0) {
        result =
            report_error("V0-C-OUTPUT", "failed to emit Checked-C", path, 1);
        goto cleanup;
    }
    result = 0;

cleanup:
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return result;
}

int main(int argc, char **argv)
{
    if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        return fputs(help, stdout) >= 0 && fflush(stdout) == 0 ? 0 : 1;
    }
    if (argc == 3 && strcmp(argv[1], "--check") == 0 && argv[2][0] != '-') {
        return compile_v0(argv[2], true);
    }
    if (argc != 2) {
        return report_error("P0-CLI-ARGUMENT", "expected exactly one argument",
                            "use --help for usage", 2);
    }
    if (strcmp(argv[1], "--version") == 0) {
        return fputs("newlangc 0.1.0 (P0 bootstrap)\n", stdout) >= 0 &&
                       fflush(stdout) == 0
                   ? 0
                   : 1;
    }
    if (argv[1][0] == '-') {
        return report_error("P0-CLI-OPTION", "unknown option", argv[1], 2);
    }
    return compile_v0(argv[1], false);
}
