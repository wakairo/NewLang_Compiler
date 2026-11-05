#include "newlang/checked.h"
#include "newlang/parser.h"

#include <stdio.h>
#include <string.h>

static const NLCheckedFragment *seen[128];
static size_t seen_count;
static size_t loop_count;
static size_t field_count;
static size_t refresh_count;
static size_t failures;

static void visit(const NLCheckedFragment *fragment, const char *path,
                  size_t depth)
{
    if (!fragment || depth > 24) {
        failures++;
        return;
    }
    for (size_t i = 0; i < seen_count; i++) {
        if (seen[i] == fragment)
            return;
    }
    if (seen_count == 128) {
        failures++;
        return;
    }
    seen[seen_count++] = fragment;
    const NLSemanticContext *world = nl_checked_context(fragment);
    size_t count = nl_checked_node_count(fragment);
    printf("{\"event\":\"world\",\"path\":\"%s\",\"world\":\"%p\","
           "\"nodes\":%zu}\n",
           path, (const void *)world, count);
    for (NLCheckedNodeId id = 1; id <= count; id++) {
        const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
        if (!node) {
            failures++;
            continue;
        }
        printf("{\"event\":\"node\",\"path\":\"%s\",\"id\":%zu,"
               "\"kind\":%u,\"start\":%zu,\"end\":%zu,\"symbol\":%zu,"
               "\"initializer\":%zu,\"has_scalar\":%u,\"known\":%u,"
               "\"scalar\":%zu}\n",
               path, id, (unsigned)node->kind, node->span.start_byte,
               node->span.end_byte, node->symbol, node->initializer,
               (unsigned)node->has_scalar_result,
               (unsigned)node->scalar_result.known, node->scalar_result.value);
        if (node->field.present) {
            const NLCheckedField *field = &node->field;
            field_count++;
            printf("{\"event\":\"field\",\"path\":\"%s\",\"id\":%zu,"
                   "\"kind\":%u,\"index\":%zu,\"root\":%zu,\"child\":%zu,"
                   "\"root_inc\":%zu,\"child_inc\":%zu,\"root_fact\":%zu,"
                   "\"child_fact\":%zu,\"root_post\":%zu,\"child_post\":%zu,"
                   "\"old\":%zu,\"new\":%zu,\"access\":%u}\n",
                   path, id, (unsigned)node->kind, field->index, field->parent,
                   field->child, field->parent_incarnation,
                   field->child_incarnation, field->parent_fact,
                   field->child_fact, field->parent_post_fact,
                   field->child_post_fact, field->old_value, field->new_value,
                   (unsigned)field->access);
            if (node->kind == NL_CHECKED_REPLACE && field->parent_post_fact &&
                field->child_post_fact) {
                refresh_count++;
                if (field->parent_fact == field->parent_post_fact ||
                    field->child_fact == field->child_post_fact)
                    failures++;
            }
            for (NLCheckedNodeId before = 1; before < id; before++) {
                const NLCheckedNodeView *other =
                    nl_checked_node_view(fragment, before);
                if (!other || !other->field.present)
                    continue;
                const NLCheckedField *prior = &other->field;
                if (prior->parent == field->parent &&
                    prior->parent_incarnation != field->parent_incarnation)
                    failures++;
                if (prior->child == field->child &&
                    prior->child_incarnation != field->child_incarnation)
                    failures++;
            }
        }
        char child_path[512];
        if (node->kind == NL_CHECKED_LOOP) {
            loop_count++;
            printf("{\"event\":\"loop\",\"path\":\"%s\",\"id\":%zu,"
                   "\"inductive\":%u,\"continue\":%zu,\"break\":%zu,"
                   "\"return\":%zu}\n",
                   path, id, (unsigned)node->header_inductive,
                   node->continue_edges, node->break_edges, node->return_edges);
            if (!node->header_inductive || !node->continue_edges ||
                !node->break_edges || node->return_edges)
                failures++;
            snprintf(child_path, sizeof(child_path), "%s/loop%zu", path, id);
            visit(nl_checked_loop_body(fragment, id), child_path, depth + 1);
        } else if (node->kind == NL_CHECKED_IF) {
            for (size_t arm = 0; arm < 2; arm++) {
                const NLCheckedFragment *owned =
                    nl_checked_if_arm(fragment, id, arm);
                if (!owned)
                    continue;
                snprintf(child_path, sizeof(child_path), "%s/if%zu/arm%zu",
                         path, id, arm);
                visit(owned, child_path, depth + 1);
            }
        } else if (node->kind == NL_CHECKED_REGISTERED_CALL &&
                   node->body_backed) {
            snprintf(child_path, sizeof(child_path), "%s/call%zu", path, id);
            visit(nl_checked_call_body(fragment, id), child_path, depth + 1);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 64;
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLParseDiagnostic parse = {0};
    if (nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK)
        return 65;
    NLParseStatus parsed = nl_parser_parse_function_unit(parser, &unit, &parse);
    if (parsed != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr, source, &parse);
        return 66;
    }
    NLSemanticContext *context = NULL;
    if (nl_semantic_create(&context) != NL_CHECK_OK)
        return 67;
    const NLSyntaxTree *inputs[] = {unit};
    NLFunctionUnitDiagnostic definition = {0};
    NLCheckStatus status =
        nl_semantic_register_function_unit(context, inputs, 1, &definition);
    if (status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, source, &definition.diagnostic);
        return 68;
    }
    NLSource *entry_source = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *entry = NULL;
    NLCheckedFragment *checked = NULL;
    NLCheckDiagnostic diagnostic = {0};
    if (nl_source_create("main()", strlen("main()"), "R1173 entry",
                         &entry_source) != NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_source_fragment(entry_parser, &entry, &parse) !=
            NL_PARSE_OK)
        return 69;
    status = nl_semantic_check_source_fragment(context, entry, &checked,
                                               &diagnostic);
    if (status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, entry_source, &diagnostic);
        return 70;
    }
    visit(checked, "entry", 0);
    if (!loop_count)
        failures++;
    printf("{\"event\":\"summary\",\"worlds\":%zu,\"loops\":%zu,"
           "\"fields\":%zu,\"public_refresh_records\":%zu,\"failures\":%zu}\n",
           seen_count, loop_count, field_count, refresh_count, failures);
    nl_checked_destroy(checked);
    nl_syntax_tree_destroy(entry);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    return failures ? 71 : 0;
}
