#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"

#include <stdio.h>
#include <stdlib.h>

static bool inspect(const NLCheckedFragment *fragment, const char *path,
                    unsigned depth, size_t *loops)
{
    if (fragment == NULL || depth > 12)
        return false;
    const NLSource *source = nl_checked_source(fragment);
    for (size_t i = 1; i <= nl_checked_node_count(fragment); ++i) {
        const NLCheckedNodeView *view = nl_checked_node_view(fragment, i);
        if (view == NULL)
            return false;
        if (view->symbol != 0 && view->name.end_byte > view->name.start_byte) {
            NLSourceView name = {0};
            if (!nl_source_view(source, view->name, &name))
                return false;
            printf("R1134 NAME %s %zu %zu %d ", path, i, (size_t)view->symbol,
                   (int)view->kind);
            if (fwrite(name.bytes, 1, name.length, stdout) != name.length)
                return false;
            putchar('\n');
        }
        if (view->field.present) {
            const NLCheckedField *field = &view->field;
            if (field->base == 0 || field->parent == 0 || field->child == 0 ||
                field->parent == field->child || field->index > 1 ||
                field->parent_incarnation == 0 || field->child_incarnation == 0)
                return false;
            printf("R1134 FIELD %s %zu %zu %zu %zu %zu %zu %zu %zu %zu\n", path,
                   i, (size_t)field->base, (size_t)field->parent,
                   (size_t)field->child, field->index,
                   (size_t)field->parent_incarnation,
                   (size_t)field->child_incarnation, (size_t)field->parent_fact,
                   (size_t)field->child_fact);
        }
        char child[256];
        if (view->kind == NL_CHECKED_LOOP) {
            printf("R1134 LOOP %s %zu %d %zu %zu %zu\n", path, i,
                   (int)view->header_inductive, view->continue_edges,
                   view->break_edges, view->return_edges);
            if (!view->header_inductive)
                return false;
            ++*loops;
            int length = snprintf(child, sizeof(child), "%s/loop%zu", path, i);
            if (length < 0 || (size_t)length >= sizeof(child) ||
                !inspect(nl_checked_loop_body(fragment, i), child, depth + 1,
                         loops))
                return false;
        } else if (view->kind == NL_CHECKED_IF) {
            for (size_t arm = 0; arm < 2; ++arm) {
                const NLCheckedFragment *owned_arm =
                    nl_checked_if_arm(fragment, i, arm);
                if (owned_arm == NULL)
                    continue;
                int length = snprintf(child, sizeof(child), "%s/if%zu/%zu",
                                      path, i, arm);
                if (length < 0 || (size_t)length >= sizeof(child) ||
                    !inspect(owned_arm, child, depth + 1, loops))
                    return false;
            }
        } else if (view->kind == NL_CHECKED_REGISTERED_CALL &&
                   view->body_backed) {
            int length = snprintf(child, sizeof(child), "%s/call%zu", path, i);
            if (length < 0 || (size_t)length >= sizeof(child) ||
                !inspect(nl_checked_call_body(fragment, i), child, depth + 1,
                         loops))
                return false;
        }
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    int result = 1;
    NLSource *source = NULL, *entry_source = NULL;
    NLParser *parser = NULL, *entry_parser = NULL;
    NLSyntaxTree *unit = NULL, *entry_tree = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *entry = NULL;
    if (nl_source_load(argv[1], &source) != NL_SOURCE_OK ||
        nl_parser_create(source, &parser) != NL_PARSE_OK ||
        nl_parser_parse_function_unit(parser, &unit, NULL) != NL_PARSE_OK ||
        nl_semantic_create(&context) != NL_CHECK_OK)
        goto cleanup;
    const NLSyntaxTree *inputs[] = {unit};
    if (nl_semantic_register_function_unit(context, inputs, 1, NULL) !=
            NL_CHECK_OK ||
        nl_source_create("main()", 6, "r1134-real-entry", &entry_source) !=
            NL_SOURCE_OK ||
        nl_parser_create(entry_source, &entry_parser) != NL_PARSE_OK ||
        nl_parser_parse_expression_fragment(entry_parser, &entry_tree, NULL) !=
            NL_PARSE_OK ||
        nl_semantic_check_expression(context, entry_tree, &entry, NULL) !=
            NL_CHECK_OK)
        goto cleanup;
    const NLCheckedFragment *body =
        nl_checked_call_body(entry, nl_checked_root(entry));
    size_t loops = 0;
    if (!inspect(body, "main", 0, &loops) || loops != 1)
        goto cleanup;
    puts("R1134 OWNED SOURCE_REGISTERED MAIN_CHECKED READ_ONLY PASS");
    result = 0;
cleanup:
    if (result != 0)
        fputs("R1134 OWNED PROBE FAILED\n", stderr);
    nl_checked_destroy(entry);
    nl_syntax_tree_destroy(entry_tree);
    nl_parser_destroy(entry_parser);
    nl_source_destroy(entry_source);
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    nl_semantic_destroy(context);
    return result;
}
