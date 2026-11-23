#define main newlang_driver_main
#include "../../src/main.c"
#undef main
#include "../support/node_checked.h"

#include <dirent.h>

static bool injecting;
static size_t calls, fail_at, hits;

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
char *__real_strdup(const char *);
char *__real_realpath(const char *, char *);
FILE *__real_fdopen(int, const char *);

static bool fail_here(void)
{
    if (!injecting || calls++ != fail_at)
        return false;
    ++hits;
    errno = ENOMEM;
    return true;
}
void *__wrap_malloc(size_t size)
{
    return fail_here() ? NULL : __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    return fail_here() ? NULL : __real_calloc(count, size);
}
void *__wrap_realloc(void *ptr, size_t size)
{
    return fail_here() ? NULL : __real_realloc(ptr, size);
}
char *__wrap_strdup(const char *s)
{
    return fail_here() ? NULL : __real_strdup(s);
}
char *__wrap_realpath(const char *path, char *resolved)
{
    return fail_here() ? NULL : __real_realpath(path, resolved);
}
FILE *__wrap_fdopen(int fd, const char *mode)
{
    return fail_here() ? NULL : __real_fdopen(fd, mode);
}

static void select_failure(size_t index)
{
    calls = hits = 0;
    fail_at = index;
    injecting = true;
}

bool p1673_receipt_mutation_rejected(NLCheckedFragment *, NLCheckedNodeId);

static bool receipts(const char *path)
{
    TestNode original = {0}, independent = {0};
    CHECK(node_checked_load(path, &original));
    CHECK(node_checked_load(path, &independent));
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(
        original.entry, nl_checked_root(original.entry));
    const NLCheckedFragment *other = nl_checked_call_body(
        independent.entry, nl_checked_root(independent.entry));
    CHECK(body && other && body->context != other->context);
    const size_t products[] = {391, 493, 253, 247, 0, 23, SIZE_MAX, 0};
    const size_t left_values[] = {23, 29, 23, 13, 0, 23, SIZE_MAX / 3, 0};
    const size_t right_values[] = {17, 17, 11, 19, 17, 1, 3, SIZE_MAX};
    size_t count = 0;
    NLCheckedNodeId first = 0, third = 0;
    for (NLCheckedNodeId id = 1; id <= nl_checked_node_count(body); ++id) {
        const NLCheckedNodeView *op = nl_checked_node_view(body, id);
        if (op->kind != NL_CHECKED_USIZE_MULTIPLICATION)
            continue;
        CHECK(count < sizeof(products) / sizeof(*products));
        CHECK(nl_usize_multiplication_valid(body, id));
        CHECK(nl_usize_multiplication_valid(other, id));
        const NLCheckedNodeView *left =
            nl_checked_node_view(body, op->first_argument);
        CHECK(left && op->first_argument < left->next_argument &&
              left->next_argument < id);
        const NLCheckedNodeView *right =
            nl_checked_node_view(body, left->next_argument);
        NLSemanticValueView a, b, result;
        CHECK(
            right &&
            nl_semantic_value_view(body->context, left->results[0].value, &a) &&
            nl_semantic_value_view(body->context, right->results[0].value,
                                   &b) &&
            nl_semantic_value_view(body->context, op->results[0].value,
                                   &result));
        CHECK(a.scalar_known && a.scalar_value == left_values[count] &&
              b.scalar_known && b.scalar_value == right_values[count] &&
              result.scalar_known && result.scalar_value == products[count] &&
              op->results[0].value != left->results[0].value &&
              op->results[0].value != right->results[0].value);
        printf("receipt %zu left=%zu right=%zu product=%zu\n", id,
               a.scalar_value, b.scalar_value, result.scalar_value);
        if (count == 0)
            first = id;
        if (count == 2)
            third = id;
        ++count;
    }
    CHECK(count == sizeof(products) / sizeof(*products) && first && third);

    /* Copy metadata alone cannot replace a receipt or a distinct use event.
     * Restore each test-local perturbation before checking its result. */
    struct NLUsizeAdditionProof *saved = body->usize_additions;
    body->usize_additions = other->usize_additions;
    const bool foreign_rejected = !nl_usize_multiplication_valid(body, first);
    body->usize_additions = saved;
    CHECK(foreign_rejected && nl_usize_multiplication_valid(body, first));

    NLCheckedNodeView *op = &body->nodes[third - 1];
    const NLCheckedNodeId original_left = op->first_argument;
    op->first_argument = nl_checked_node_view(body, first)->first_argument;
    const bool stale_rejected = !nl_usize_multiplication_valid(body, third);
    op->first_argument = original_left;
    CHECK(stale_rejected && nl_usize_multiplication_valid(body, third));

    const size_t original_product = op->scalar_result.value;
    op->scalar_result.value = 1;
    const bool mutation_rejected = !nl_usize_multiplication_valid(body, third);
    op->scalar_result.value = original_product;
    CHECK(mutation_rejected && nl_usize_multiplication_valid(body, third));

    CHECK(p1673_receipt_mutation_rejected(body, third));

    node_checked_destroy(&independent);
    node_checked_destroy(&original);
    puts("own receipts=8 foreign/stale/mutated node and receipt rejected");
    return true;
}

static bool dependencies(const char *path)
{
    NLSource *source = NULL;
    NLParser *parser = NULL;
    NLSyntaxTree *tree = NULL;
    NLSemanticContext *context = NULL, *dependent = NULL;
    CHECK(nl_source_load(path, &source) == NL_SOURCE_OK &&
          nl_parser_create(source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_source_fragment(parser, &tree, NULL) == NL_PARSE_OK &&
          nl_semantic_create(&context) == NL_CHECK_OK);
    NLSymbolId anchor = 0;
    CHECK(nl_semantic_seed_value(context, "anchor",
                                 nl_semantic_core_type(context, NL_TYPE_USIZE),
                                 NL_DEPENDENCY_FREE, &anchor) == NL_CHECK_OK);
    NLSemanticBindingView anchor_binding;
    NLSemanticPlaceView anchor_place;
    CHECK(
        nl_semantic_binding_view(context, anchor, &anchor_binding) &&
        nl_semantic_place_view(context, anchor_binding.place, &anchor_place) &&
        anchor_place.live && anchor_place.current_fact);
    TestChecked seed = {0};
    CHECK(
        test_run(context, "usize(37)", TEST_SOURCE, NL_CHECK_OK, NULL, &seed));
    NLSymbolId symbol = 0;
    const NLValueId id = test_root(&seed)->results[0].value;
    CHECK(nl_semantic_bind_result(context, "extent", id, &symbol) ==
          NL_CHECK_OK);
    test_checked_destroy(&seed);
    CHECK(nl_sem_clone(context, &dependent) == NL_CHECK_OK);
    NLSemanticValueView *fixture = &dependent->values[id - 1];
    fixture->dependencies = NL_EXACT_VALUE_DEPENDENCIES;
    fixture->value_dependency_count = 1;
    fixture->value_dependencies[0] =
        (NLValueDependency){anchor_binding.place, anchor_place.current_fact};
    CHECK(nl_sem_validate(dependent) == NL_CHECK_OK);

    TestState *before = malloc(sizeof(*before));
    CHECK(before && test_state(dependent, before));
    NLCheckedFragment *out = NULL;
    NLCheckDiagnostic diag = {0};
    CHECK(nl_semantic_check_source_fragment(dependent, tree, &out, &diag) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(!out && diag.diagnostic.code &&
          !strcmp(diag.diagnostic.code, "USIZE-MULTIPLICATION-PRECISION") &&
          test_unchanged(dependent, before));

    CHECK(nl_semantic_check_source_fragment(context, tree, &out, NULL) ==
          NL_CHECK_OK);
    const NLCheckedNodeView *node =
        nl_checked_node_view(out, nl_checked_root(out));
    CHECK(node && node->kind == NL_CHECKED_USIZE_MULTIPLICATION &&
          node->scalar_result.known && node->scalar_result.value == 111);
    nl_checked_destroy(out);
    free(before);
    nl_semantic_destroy(dependent);
    nl_semantic_destroy(context);
    nl_syntax_tree_destroy(tree);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    puts("known source-fragment product=111; dependent Copy rejected, exact "
         "rollback");
    return true;
}

static bool check_oom(const char *path, bool helper_product)
{
    TestNode original = {0};
    CHECK(node_checked_load(path, &original));
    /* Use the registered declarations from the input. Each allocation trial
     * gets its own context; a failed check is retried on that same context. */
    NLParser *parser = NULL;
    NLSyntaxTree *call = NULL;
    CHECK(nl_parser_create(original.entry_source, &parser) == NL_PARSE_OK &&
          nl_parser_parse_expression_fragment(parser, &call, NULL) ==
              NL_PARSE_OK);
    TestState *before = malloc(sizeof(*before));
    CHECK(before);
    size_t count = 0;
    for (; count < 2000; ++count) {
        NLSemanticContext *c = NULL;
        NLCheckedFragment *out = NULL;
        CHECK(nl_sem_clone(original.context, &c) == NL_CHECK_OK &&
              test_state(c, before));
        select_failure(count);
        const NLCheckStatus status =
            nl_semantic_check_expression(c, call, &out, NULL);
        injecting = false;
        if (!hits) {
            CHECK(status == NL_CHECK_OK && out && calls == count);
            nl_checked_destroy(out);
            nl_semantic_destroy(c);
            break;
        }
        CHECK(hits == 1 && status == NL_CHECK_OUT_OF_MEMORY && !out &&
              test_unchanged(c, before));
        CHECK(nl_semantic_check_expression(c, call, &out, NULL) ==
                  NL_CHECK_OK &&
              out);
        const NLCheckedFragment *body =
            nl_checked_call_body(out, nl_checked_root(out));
        CHECK(body);
        if (helper_product) {
            CHECK(body->body_count == 1);
            const NLCheckedNodeId id = body->body_calls[0];
            NLDivisionUsizePlan helper;
            CHECK(nl_quantity_usize_call_valid(body, id, &helper) &&
                  !helper.formal &&
                  nl_usize_multiplication_valid(helper.body, helper.result));
        } else {
            CHECK(body->usize_additions);
        }
        nl_checked_destroy(out);
        nl_semantic_destroy(c);
    }
    free(before);
    nl_syntax_tree_destroy(call);
    nl_parser_destroy(parser);
    node_checked_destroy(&original);
    CHECK(count && count < 2000);
    printf("CHECK allocation failures=%zu exact state, no artifact, "
           "same-context retry\n",
           count);
    return true;
}

static bool file_has(const char *path, const char *bytes, size_t length)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    char block[8192];
    const size_t read = fread(block, 1, sizeof(block), f);
    const bool equal = !ferror(f) && read == length && length < sizeof(block) &&
                       memcmp(bytes, block, length) == 0;
    return fclose(f) == 0 && equal;
}

static bool no_staging_file(const char *path)
{
    DIR *d = opendir(path);
    if (!d)
        return false;
    bool clean = true;
    const struct dirent *entry;
    while ((entry = readdir(d)))
        if (strstr(entry->d_name, ".tmp."))
            clean = false;
    return closedir(d) == 0 && clean;
}

static bool check_named_failures(const char *input, const char *dir)
{
    char target[128], absent[128], expected[8192];
    CHECK(snprintf(target, sizeof(target), "%s/output.c", dir) > 0 &&
          snprintf(absent, sizeof(absent), "%s/absent.c", dir) > 0);
    CHECK(emit_c17_to(target, input) == 0);
    FILE *f = fopen(target, "rb");
    CHECK(f);
    const size_t expected_length = fread(expected, 1, sizeof(expected), f);
    CHECK(!ferror(f) && fclose(f) == 0 && expected_length &&
          expected_length < sizeof(expected));

    size_t count = 0;
    for (; count < 2000; ++count) {
        struct stat before, after;
        CHECK(stat(target, &before) == 0);
        select_failure(count);
        const int result = emit_c17_to(target, input);
        injecting = false;
        if (!hits) {
            CHECK(result == 0 && calls == count);
            break;
        }
        CHECK(hits == 1 && result != 0 && stat(target, &after) == 0 &&
              before.st_ino == after.st_ino &&
              before.st_mtim.tv_sec == after.st_mtim.tv_sec &&
              before.st_mtim.tv_nsec == after.st_mtim.tv_nsec &&
              file_has(target, expected, expected_length));

        select_failure(count);
        const int absent_result = emit_c17_to(absent, input);
        injecting = false;
        CHECK(hits == 1 && absent_result != 0 && access(absent, F_OK) != 0 &&
              errno == ENOENT && no_staging_file(dir));
        CHECK(emit_c17_to(target, input) == 0 &&
              file_has(target, expected, expected_length));
    }
    CHECK(unlink(target) == 0 && no_staging_file(dir) && count && count < 2000);
    printf("named C17 allocation failures=%zu each for existing/absent; "
           "no-clobber and same-process retry\n",
           count);
    return true;
}

static bool named_oom(const char *input)
{
    char dir[] = "p1673-stage.XXXXXX";
    FILE *diagnostics = tmpfile();
    const int saved = dup(STDERR_FILENO);
    CHECK(diagnostics && saved >= 0 && mkdtemp(dir));
    CHECK(fflush(stderr) == 0 && dup2(fileno(diagnostics), STDERR_FILENO) >= 0);
    const bool ok = check_named_failures(input, dir);
    CHECK(fflush(stderr) == 0 && dup2(saved, STDERR_FILENO) >= 0 &&
          close(saved) == 0);
    if (!ok) {
        CHECK(fseek(diagnostics, 0, SEEK_END) == 0);
        const long size = ftell(diagnostics);
        CHECK(size >= 0);
        CHECK(fseek(diagnostics, size > 2000 ? size - 2000 : 0, SEEK_SET) == 0);
        char line[512];
        while (fgets(line, sizeof(line), diagnostics))
            fputs(line, stderr);
    }
    CHECK(fclose(diagnostics) == 0 && ok && rmdir(dir) == 0);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 4)
        return 2;
    if (!strcmp(argv[1], "receipt"))
        return receipts(argv[2]) && dependencies(argv[3]) ? 0 : 1;
    const bool helper_product = !strcmp(argv[1], "helper-oom");
    if (helper_product || !strcmp(argv[1], "oom"))
        return check_oom(argv[2], helper_product) && named_oom(argv[2]) ? 0 : 1;
    return 2;
}
