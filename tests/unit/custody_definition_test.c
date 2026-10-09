#include "../../src/semantic_internal.h"
#include "../support/semantic_check.h"

#include <stdlib.h>

void *__real_malloc(size_t);
void *__real_realloc(void *, size_t);
static bool injecting;
static size_t at, fail_at;
void *__wrap_malloc(size_t n)
{
    if (injecting && at++ == fail_at)
        return NULL;
    return __real_malloc(n);
}
void *__wrap_realloc(void *p, size_t n)
{
    if (injecting && at++ == fail_at)
        return NULL;
    return __real_realloc(p, n);
}

static bool load(const char *path, bool definition, NLSource **source,
                 NLSyntaxTree **tree)
{
    NLSource *original = NULL;
    CHECK(nl_source_load(path, &original) == NL_SOURCE_OK);
    NLSourceView v;
    CHECK(nl_source_view(original,
                         (NLSourceSpan){0, nl_source_length(original)}, &v));
    if (definition) {
        NLParser *p = NULL;
        NLSyntaxTree *unit = NULL;
        CHECK(nl_parser_create(original, &p) == NL_PARSE_OK);
        CHECK(nl_parser_parse_function_unit(p, &unit, NULL) == NL_PARSE_OK);
        NLSourceSpan body = {0};
        const NLSyntaxView *root =
            nl_syntax_node_view(nl_syntax_tree_root(unit));
        for (const NLSyntaxNode *n = root->data.function_unit.declarations;
             n != NULL; n = nl_syntax_next_argument(n)) {
            const NLSyntaxView *f = nl_syntax_node_view(n);
            if (f->kind != NL_SYNTAX_FUNCTION)
                continue;
            NLSourceView name;
            CHECK(nl_source_view(original, f->data.function.name, &name));
            if (name.length == 4 && memcmp(name.bytes, "main", 4) == 0)
                body = nl_syntax_node_view(f->data.function.body)->span;
        }
        CHECK(body.end_byte > body.start_byte && body.end_byte <= v.length);
        const char main_body[] = "{unit}";
        const size_t length = v.length - (body.end_byte - body.start_byte) +
                              sizeof(main_body) - 1;
        char *text = malloc(length);
        CHECK(text != NULL);
        memcpy(text, v.bytes, body.start_byte);
        memcpy(text + body.start_byte, main_body, sizeof(main_body) - 1);
        memcpy(text + body.start_byte + sizeof(main_body) - 1,
               v.bytes + body.end_byte, v.length - body.end_byte);
        CHECK(nl_source_create(text, length, "independent-custody-definition",
                               source) == NL_SOURCE_OK);
        free(text);
        nl_syntax_tree_destroy(unit);
        nl_parser_destroy(p);
        nl_source_destroy(original);
    } else {
        *source = original;
    }
    NLParser *parser = NULL;
    CHECK(nl_parser_create(*source, &parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_function_unit(parser, tree, NULL) == NL_PARSE_OK);
    nl_parser_destroy(parser);
    return true;
}

static bool conditions(NLSemanticContext *c)
{
    NLSemanticSnapshot snapshot;
    CHECK(nl_semantic_snapshot(c, &snapshot));
    CHECK(snapshot.values == 0 && snapshot.places == 0 &&
          snapshot.backing_regions == 0 && snapshot.domains == 0 &&
          snapshot.scopes == 0 && snapshot.occurrences == 0);
    size_t recipients = 0;
    for (size_t i = 1; i <= snapshot.functions; ++i) {
        NLCustodyDefinition d = {0};
        if (!nl_semantic_function_custody_applicability(c, i, &d))
            continue;
        ++recipients;
        CHECK(d.definition_checked &&
              d.requirements == NL_CUSTODY_ALL_REQUIREMENTS);
        NLSemanticTypeView h, packet, option;
        CHECK(nl_semantic_type_view(c, d.target, &h) && h.field_count == 2);
        CHECK(nl_semantic_type_view(c, d.packet, &packet) &&
              packet.field_count == 3 && !packet.is_copy &&
              !packet.is_discardable);
        CHECK(nl_semantic_type_view(c, d.option, &option) &&
              option.kind == NL_TYPE_SUM && option.variant_count == 2 &&
              !option.is_copy && !option.is_discardable);
        CHECK(nl_sum_authority(c, d.packet) && nl_sum_authority(c, d.option));
        CHECK(c->types[d.option - 1].variant_types[0] == 0 &&
              c->types[d.option - 1].variant_types[1] == d.packet);
        /* Mutation of a copied descriptor does not discharge or overwrite the
         * original immutable definition. No actual transfer certificate exists.
         */
        d.requirements = 0;
        d.definition_checked = false;
        CHECK(nl_semantic_function_custody_applicability(c, i, &d) &&
              d.definition_checked &&
              d.requirements == NL_CUSTODY_ALL_REQUIREMENTS);
        NLTypedOwnerDefinition owner;
        CHECK(!nl_semantic_function_applicability(c, i, &owner));
    }
    CHECK(recipients == 1);
    return true;
}

static bool evidence(const char *path)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    CHECK(load(path, true, &source, &tree));
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    const NLSyntaxTree *inputs[] = {tree};
    CHECK(nl_semantic_register_function_unit(c, inputs, 1, NULL) ==
          NL_CHECK_OK);
    /* Evidence must survive teardown of the original AST/source. */
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    CHECK(conditions(c));
    const char *recipient = NULL;
    for (size_t i = 0; i < c->function_count; ++i)
        if (c->functions[i].custody_recipient)
            recipient = c->functions[i].name;
    CHECK(recipient != NULL);
    char call[256];
    const int written =
        snprintf(call, sizeof(call), "%s(missing,missing)", recipient);
    CHECK(written > 0 && (size_t)written < sizeof(call));
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(c, &before));
    TestChecked checked = {0};
    CHECK(test_run(c, call, TEST_SOURCE, NL_CHECK_ANALYSIS_PRECISION_LIMIT,
                   "CUSTODY-ENTRY-PRECISION", &checked));
    CHECK(checked.artifact == NULL);
    CHECK(nl_semantic_snapshot(c, &after) &&
          memcmp(&before, &after, sizeof(before)) == 0);
    test_checked_destroy(&checked);
    nl_semantic_destroy(c);
    puts("owned conditional definition, static non-Discardable Option, no "
         "minted authority, guarded call rollback: pass");
    return true;
}

static bool faults(const char *path, bool definition)
{
    NLSource *source = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *c = NULL;
    CHECK(load(path, definition, &source, &tree));
    CHECK(nl_semantic_create(&c) == NL_CHECK_OK);
    TestChecked preserved = {0};
    NLSemanticBindingView binding_before = {0};
    NLSemanticValueView value_before = {0};
    if (!definition) {
        /* A source-created existing Copy binding must survive every failure;
         * this is not a host-seeded owner/domain/topology fixture. */
        CHECK(test_run(c, "let preserved=u8(7);", TEST_SOURCE, NL_CHECK_OK,
                       NULL, &preserved));
        CHECK(nl_semantic_binding_view(c, 1, &binding_before));
        CHECK(nl_semantic_value_view(c, binding_before.value, &value_before));
        test_checked_destroy(&preserved);
    }
    NLSemanticSnapshot before, after;
    CHECK(nl_semantic_snapshot(c, &before));
    const NLSyntaxTree *inputs[] = {tree};
    NLCheckStatus last = NL_CHECK_INTERNAL_ERROR;
    size_t faults = 0;
    for (fail_at = 0; fail_at < 12000; ++fail_at) {
        NLFunctionUnitDiagnostic d = {0};
        at = 0;
        injecting = true;
        last = nl_semantic_register_function_unit(c, inputs, 1, &d);
        injecting = false;
        if (last != NL_CHECK_OUT_OF_MEMORY) {
            if (definition) {
                CHECK(last == NL_CHECK_OK && conditions(c));
            } else {
                CHECK(last == NL_CHECK_ANALYSIS_PRECISION_LIMIT &&
                      strcmp(d.diagnostic.diagnostic.code,
                             "CUSTODY-ENTRY-PRECISION") == 0);
                CHECK(nl_semantic_snapshot(c, &after) &&
                      memcmp(&before, &after, sizeof(before)) == 0);
            }
            break;
        }
        CHECK(nl_semantic_snapshot(c, &after) &&
              memcmp(&before, &after, sizeof(before)) == 0);
        ++faults;
    }
    CHECK(fail_at < 12000 && faults > 100);
    if (!definition) {
        NLSemanticBindingView binding_after;
        NLSemanticValueView value_after;
        CHECK(nl_semantic_binding_view(c, 1, &binding_after) &&
              memcmp(&binding_before, &binding_after, sizeof(binding_before)) ==
                  0);
        CHECK(nl_semantic_value_view(c, binding_before.value, &value_after) &&
              memcmp(&value_before, &value_after, sizeof(value_before)) == 0);
    }
    printf("%s: %zu staged registration allocation failures, atomic %s\n",
           definition ? "independent definition" : "primary HOLD source",
           faults, definition ? "success" : "precision rejection");
    nl_syntax_tree_destroy(tree);
    nl_source_destroy(source);
    nl_semantic_destroy(c);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        return 2;
    const bool ok = strcmp(argv[1], "oom") == 0
                        ? faults(argv[2], true) && faults(argv[2], false)
                        : evidence(argv[2]);
    return ok ? 0 : 1;
}
