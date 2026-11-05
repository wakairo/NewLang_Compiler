#include "newlang/checked.h"
#include "newlang/parser.h"

#include <stdio.h>
#include <string.h>

#define R1185_MAX_NODES 32768
#define R1185_MAX_ARTIFACTS 2048

typedef struct {
    const NLCheckedFragment *artifact;
    NLCheckedNodeId node;
} SeenNode;

static SeenNode seen_nodes[R1185_MAX_NODES];
static size_t seen_count;
static const NLCheckedFragment *artifacts[R1185_MAX_ARTIFACTS];
static const NLSemanticContext *worlds[R1185_MAX_ARTIFACTS];
static size_t artifact_count;
static size_t world_count;

static void json_string(const char *value)
{
    fputc('"', stdout);
    if (value != NULL) {
        for (const unsigned char *p = (const unsigned char *)value; *p; ++p) {
            if (*p == '"' || *p == '\\') {
                fputc('\\', stdout);
                fputc(*p, stdout);
            } else if (*p < 32) {
                printf("\\u%04x", (unsigned)*p);
            } else {
                fputc(*p, stdout);
            }
        }
    }
    fputc('"', stdout);
}

static void span_hex(const NLSource *source, NLSourceSpan span)
{
    NLSourceView view = {0};
    fputc('"', stdout);
    if (nl_source_view(source, span, &view)) {
        for (size_t i = 0; i < view.length; ++i) {
            printf("%02x", (unsigned)view.bytes[i]);
        }
    }
    fputc('"', stdout);
}

static void stage(const char *name, int status, const char *code)
{
    fputs("{\"record\":\"stage\",\"stage\":", stdout);
    json_string(name);
    printf(",\"status\":%d,\"code\":", status);
    json_string(code);
    fputs("}\n", stdout);
}

static size_t world_id(const NLSemanticContext *context)
{
    for (size_t i = 0; i < world_count; ++i) {
        if (worlds[i] == context) {
            return i + 1;
        }
    }
    if (world_count == R1185_MAX_ARTIFACTS) {
        return 0;
    }
    worlds[world_count++] = context;
    return world_count;
}

static bool walk_fragment(const NLCheckedFragment *fragment, size_t parent,
                          NLCheckedNodeId from, const char *edge, size_t arm,
                          size_t depth);

static bool walk_node(const NLCheckedFragment *fragment, size_t artifact,
                      size_t world, NLCheckedNodeId id, size_t depth)
{
    if (id == 0) {
        return true;
    }
    if (depth > 512 || seen_count == R1185_MAX_NODES) {
        return false;
    }
    for (size_t i = 0; i < seen_count; ++i) {
        if (seen_nodes[i].artifact == fragment && seen_nodes[i].node == id) {
            return true;
        }
    }
    seen_nodes[seen_count++] = (SeenNode){fragment, id};
    const NLCheckedNodeView *node = nl_checked_node_view(fragment, id);
    if (node == NULL) {
        return false;
    }
    const NLSource *source = nl_checked_source(fragment);
    const NLSemanticContext *context = nl_checked_context(fragment);
    printf("{\"record\":\"node\",\"artifact\":%zu,\"world\":%zu,"
           "\"id\":%zu,\"kind\":%d,\"span\":[%zu,%zu],\"name_hex\":",
           artifact, world, id, (int)node->kind, node->span.start_byte,
           node->span.end_byte);
    span_hex(source, node->name);
    printf(",\"type\":%zu,\"symbol\":%zu,\"function\":%zu,"
           "\"body_backed\":%d,\"terminates\":%d,\"value_use\":%d,"
           "\"parameter_type\":%zu,\"result_count\":%zu,"
           "\"initializer\":%zu,\"first_argument\":%zu,"
           "\"next_argument\":%zu,\"argument_count\":%zu,"
           "\"first_item\":%zu,\"next_item\":%zu,\"tail\":%zu,"
           "\"item_count\":%zu,\"field_index\":%zu,"
           "\"has_scalar_result\":%d,\"scalar_known\":%d,"
           "\"scalar_value\":%zu,\"header_inductive\":%d,"
           "\"continue_edges\":%zu,\"break_edges\":%zu,"
           "\"return_edges\":%zu}\n",
           node->type, node->symbol, node->function, node->body_backed,
           node->terminates, (int)node->value_use, node->parameter_type,
           node->result_count, node->initializer, node->first_argument,
           node->next_argument, node->argument_count, node->first_item,
           node->next_item, node->tail, node->item_count, node->field_index,
           node->has_scalar_result,
           node->has_scalar_result && node->scalar_result.known,
           node->has_scalar_result && node->scalar_result.known
               ? node->scalar_result.value
               : 0,
           node->kind == NL_CHECKED_LOOP && node->header_inductive,
           node->kind == NL_CHECKED_LOOP ? node->continue_edges : 0,
           node->kind == NL_CHECKED_LOOP ? node->break_edges : 0,
           node->kind == NL_CHECKED_LOOP ? node->return_edges : 0);
    if (node->kind == NL_CHECKED_LOAN_HEADER) {
        printf("{\"record\":\"loan\",\"artifact\":%zu,\"world\":%zu,"
               "\"node\":%zu,\"place\":%zu,\"incarnation\":%zu,"
               "\"scope\":%zu,\"dependency_scope\":%zu,"
               "\"ref_symbol\":%zu,\"body_nonescape_proved\":%d,"
               "\"normal_result_forwarded\":%d}\n",
               artifact, world, id, node->loan.place, node->loan.incarnation,
               node->loan.scope, node->loan.dependency_scope,
               node->loan.ref_symbol, node->loan.body_nonescape_proved,
               node->loan.normal_result_forwarded);
    }
    if (node->field.present) {
        const NLCheckedField *field = &node->field;
        printf("{\"record\":\"field\",\"artifact\":%zu,\"world\":%zu,"
               "\"node\":%zu,\"base\":%zu,\"nominal\":%zu,"
               "\"index\":%zu,\"parent\":%zu,\"child\":%zu,"
               "\"parent_incarnation\":%zu,\"child_incarnation\":%zu,"
               "\"parent_fact\":%zu,\"child_fact\":%zu,"
               "\"parent_post_fact\":%zu,\"child_post_fact\":%zu,"
               "\"old_value\":%zu,\"new_value\":%zu,"
               "\"dependency_compatible\":%d}\n",
               artifact, world, id, field->base, field->nominal,
               field->index, field->parent, field->child,
               field->parent_incarnation, field->child_incarnation,
               field->parent_fact, field->child_fact, field->parent_post_fact,
               field->child_post_fact, field->old_value, field->new_value,
               field->dependency_compatible);
    }
    for (size_t i = 0; i < node->result_count && i < 2; ++i) {
        NLSemanticValueView value = {0};
        bool valid = nl_semantic_value_view(context, node->results[i].value,
                                            &value);
        NLSemanticTypeView type = {0};
        bool type_valid = nl_semantic_type_view(context, node->results[i].type,
                                                &type);
        printf("{\"record\":\"result\",\"artifact\":%zu,\"world\":%zu,"
               "\"node\":%zu,\"index\":%zu,\"type\":%zu,"
               "\"value\":%zu,\"view_valid\":%d,\"scalar_known\":%d,"
               "\"scalar_value\":%zu,\"carrier\":%d,\"owner_place\":%zu,"
               "\"dependencies\":%d,\"value_dependency_count\":%zu,"
               "\"domain\":%zu,\"allocation_region\":%zu,"
               "\"aggregate_owner\":%zu,\"reference_count\":%zu,"
               "\"type_view_valid\":%d,\"type_kind\":%d,\"is_copy\":%d}\n",
               artifact, world, id, i, node->results[i].type,
               node->results[i].value, valid, value.scalar_known,
               value.scalar_value, (int)value.carrier, value.owner_place,
               (int)value.dependencies, value.value_dependency_count,
               value.domain, value.allocation_region, value.aggregate_owner,
               value.reference_count, type_valid, (int)type.kind, type.is_copy);
    }
    NLCheckedNodeId links[] = {
        node->initializer, node->first_argument, node->next_argument,
        node->first_item, node->next_item, node->tail};
    for (size_t i = 0; i < sizeof(links) / sizeof(links[0]); ++i) {
        if (!walk_node(fragment, artifact, world, links[i], depth + 1)) {
            return false;
        }
    }
    if (node->kind == NL_CHECKED_REGISTERED_CALL) {
        const NLCheckedFragment *body = nl_checked_call_body(fragment, id);
        if (body != NULL &&
            !walk_fragment(body, artifact, id, "call_body", 0, depth + 1)) {
            return false;
        }
    }
    if (node->kind == NL_CHECKED_LOOP) {
        const NLCheckedFragment *body = nl_checked_loop_body(fragment, id);
        if (body != NULL &&
            !walk_fragment(body, artifact, id, "loop_body", 0, depth + 1)) {
            return false;
        }
    }
    if (node->kind == NL_CHECKED_IF) {
        for (size_t i = 0; i < 2; ++i) {
            const NLCheckedFragment *body = nl_checked_if_arm(fragment, id, i);
            if (body != NULL &&
                !walk_fragment(body, artifact, id, "if_arm", i, depth + 1)) {
                return false;
            }
        }
    }
    return !ferror(stdout);
}

static bool walk_fragment(const NLCheckedFragment *fragment, size_t parent,
                          NLCheckedNodeId from, const char *edge, size_t arm,
                          size_t depth)
{
    if (depth > 512 || artifact_count == R1185_MAX_ARTIFACTS) {
        return false;
    }
    size_t artifact = 0;
    for (size_t i = 0; i < artifact_count; ++i) {
        if (artifacts[i] == fragment) {
            artifact = i + 1;
        }
    }
    bool fresh = artifact == 0;
    if (fresh) {
        artifacts[artifact_count++] = fragment;
        artifact = artifact_count;
    }
    size_t world = world_id(nl_checked_context(fragment));
    if (world == 0) {
        return false;
    }
    printf("{\"record\":\"artifact\",\"id\":%zu,\"world\":%zu,"
           "\"parent\":%zu,\"from_node\":%zu,\"edge\":",
           artifact, world, parent, from);
    json_string(edge);
    printf(",\"arm\":%zu,\"root\":%zu,\"nodes\":%zu,\"fresh\":%d}\n",
           arm, nl_checked_root(fragment), nl_checked_node_count(fragment),
           fresh);
    return !fresh || walk_node(fragment, artifact, world,
                               nl_checked_root(fragment), depth + 1);
}

int main(int argc, char **argv)
{
    NLSource *source = NULL;
    NLSource *entry_source = NULL;
    NLParser *parser = NULL;
    NLParser *entry_parser = NULL;
    NLSyntaxTree *unit = NULL;
    NLSyntaxTree *entry = NULL;
    NLSemanticContext *context = NULL;
    NLCheckedFragment *owned = NULL;
    NLParseDiagnostic parse_diag = {0};
    NLFunctionUnitDiagnostic unit_diag = {0};
    NLCheckDiagnostic check_diag = {0};
    int result = 70;
    if (argc != 2) {
        fputs("usage: r1185_public SOURCE_PATH\n", stderr);
        return 64;
    }
    NLSourceStatus source_status = nl_source_load(argv[1], &source);
    stage("source_load", (int)source_status, NULL);
    if (source_status != NL_SOURCE_OK) {
        goto cleanup;
    }
    NLParseStatus parse_status = nl_parser_create(source, &parser);
    if (parse_status != NL_PARSE_OK) {
        stage("parser_create", (int)parse_status, NULL);
        goto cleanup;
    }
    parse_status = nl_parser_parse_function_unit(parser, &unit, &parse_diag);
    stage("parse_unit", (int)parse_status, parse_diag.diagnostic.code);
    if (parse_status != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr, source, &parse_diag);
        result = 20;
        goto cleanup;
    }
    NLCheckStatus check_status = nl_semantic_create(&context);
    if (check_status != NL_CHECK_OK) {
        stage("semantic_create", (int)check_status, NULL);
        goto cleanup;
    }
    const NLSyntaxTree *inputs[] = {unit};
    check_status = nl_semantic_register_function_unit(context, inputs, 1,
                                                      &unit_diag);
    stage("register_unit", (int)check_status,
          unit_diag.diagnostic.diagnostic.code);
    if (check_status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, source, &unit_diag.diagnostic);
        result = 21;
        goto cleanup;
    }
    const char entry_text[] = "main()";
    source_status = nl_source_create(entry_text, sizeof(entry_text) - 1,
                                     "R1185 ordinary main call", &entry_source);
    if (source_status != NL_SOURCE_OK) {
        stage("entry_source", (int)source_status, NULL);
        goto cleanup;
    }
    parse_status = nl_parser_create(entry_source, &entry_parser);
    if (parse_status != NL_PARSE_OK) {
        stage("entry_parser", (int)parse_status, NULL);
        goto cleanup;
    }
    memset(&parse_diag, 0, sizeof(parse_diag));
    parse_status = nl_parser_parse_source_fragment(entry_parser, &entry,
                                                   &parse_diag);
    stage("parse_main_call", (int)parse_status, parse_diag.diagnostic.code);
    if (parse_status != NL_PARSE_OK) {
        nl_parse_diagnostic_render(stderr, entry_source, &parse_diag);
        goto cleanup;
    }
    check_status = nl_semantic_check_source_fragment(context, entry, &owned,
                                                      &check_diag);
    stage("check_main_call", (int)check_status, check_diag.diagnostic.code);
    if (check_status != NL_CHECK_OK) {
        nl_check_diagnostic_render(stderr, entry_source, &check_diag);
        result = 22;
        goto cleanup;
    }
    fputs("{\"record\":\"limitation\",\"whole_loop_validator\":"
          "\"NOT_RUN/NO_APPLICABLE_PUBLIC_API\",\"ids\":"
          "\"qualified by artifact and world; never native addresses\"}\n",
          stdout);
    result = walk_fragment(owned, 0, 0, "ordinary_main_call", 0, 0) ? 0 : 70;
cleanup:
    if (owned != NULL) {
        nl_checked_destroy(owned);
    }
    if (entry != NULL) {
        nl_syntax_tree_destroy(entry);
    }
    if (entry_parser != NULL) {
        nl_parser_destroy(entry_parser);
    }
    if (entry_source != NULL) {
        nl_source_destroy(entry_source);
    }
    if (context != NULL) {
        nl_semantic_destroy(context);
    }
    if (unit != NULL) {
        nl_syntax_tree_destroy(unit);
    }
    if (parser != NULL) {
        nl_parser_destroy(parser);
    }
    if (source != NULL) {
        nl_source_destroy(source);
    }
    if (ferror(stdout) || ferror(stderr)) {
        result = 74;
    }
    return result;
}
