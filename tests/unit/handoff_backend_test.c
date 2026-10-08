#include "../../src/semantic_internal.h"
#include "../support/node_checked.h"
#include "newlang/checked_c_node.h"
#include <stdlib.h>

void *__real_malloc(size_t);
static bool inject;
static size_t at, calls;
void *__wrap_malloc(size_t n)
{
    return inject && calls++ == at ? NULL : __real_malloc(n);
}
static NLCheckedFragment *owner(NLCheckedFragment *f, NLCheckedNodeId *id)
{
    for (NLCheckedNodeId n = 1; n <= nl_checked_node_count(f); ++n) {
        const NLCheckedNodeView *v = nl_checked_node_view(f, n);
        if (v->kind == NL_CHECKED_REGISTERED_CALL &&
            v->owner_call.entry_proved) {
            *id = n;
            return f;
        }
        NLCheckedFragment *b = (NLCheckedFragment *)nl_checked_call_body(f, n);
        NLCheckedFragment *found = b == NULL ? NULL : owner(b, id);
        if (found != NULL)
            return found;
        if (v->kind == NL_CHECKED_MATCH)
            for (size_t a = 0; a < v->item_count; ++a) {
                found = owner(
                    (NLCheckedFragment *)nl_checked_match_arm(f, n, a), id);
                if (found != NULL)
                    return found;
            }
    }
    return NULL;
}
static bool expect(TestNode *n, NLNodeCStatus wanted, const char *baseline)
{
    char *out = NULL;
    size_t length = 37;
    CHECK(nl_checked_c_node(n->entry, &out, &length) == wanted);
    if (wanted == NL_NODE_C_OK) {
        CHECK(out != NULL && length == strlen(out) &&
              strcmp(out, baseline) == 0);
        free(out);
    } else
        CHECK(out == NULL && length == 37);
    return true;
}
static bool controls(const char *path)
{
    TestNode n = {0};
    CHECK(node_checked_load(path, &n));
    char *baseline = NULL;
    size_t length = 0;
    CHECK(nl_checked_c_node(n.entry, &baseline, &length) == NL_NODE_C_OK);
    NLCheckedNodeId id = 0;
    NLCheckedFragment *f = owner(n.entry, &id);
    CHECK(f != NULL);
    NLCheckedNodeView saved = f->nodes[id - 1];
    NLCheckedFragment *body = (NLCheckedFragment *)nl_checked_call_body(f, id);
    NLSemanticContext *entry = f->owner_entry,
                      *post = (NLSemanticContext *)f->context;
    CHECK(entry != NULL && body != NULL && body->context == post);
    for (size_t mutation = 0; mutation < 17; ++mutation) {
        NLCheckedNodeView *v = &f->nodes[id - 1];
        switch (mutation) {
        case 0:
            v->owner_call.entry_proved = false;
            break;
        case 1:
            v->owner_call.post_proved = false;
            break;
        case 2:
            v->owner_call.definition.definition_checked = false;
            break;
        case 3:
            v->owner_call.definition.requirements ^=
                NL_OWNER_SCOPE_COMPATIBILITY;
            break;
        case 4:
            v->owner_call.definition.steps[2] = NL_OWNER_END_REGION;
            break;
        case 5:
            v->owner_call.definition.target = 1;
            break;
        case 6:
            v->owner_call.range.start = 1;
            break;
        case 7:
            v->owner_call.range.length = 23;
            break;
        case 8:
            v->owner_call.root = 1;
            break;
        case 9:
            ++v->owner_call.incarnation;
            break;
        case 10:
            v->owner_call.domain = 1;
            break;
        case 11:
            v->owner_call.parameters[1] = v->owner_call.donor[1];
            break;
        case 12:
            v->owner_call.parameters[2] = v->owner_call.parameters[0];
            break;
        case 13:
            v->owner_call.inputs[1] = v->owner_call.inputs[2];
            break;
        case 14:
            v->function = 0;
            break;
        case 15:
            v->argument_count = 2;
            break;
        case 16:
            v->owner_call.donor[0] = v->owner_call.parameters[0];
            break;
        }
        CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
        *v = saved;
        CHECK(expect(&n, NL_NODE_C_OK, baseline));
    }
    const NLSemanticValueView pointer =
        entry->values[saved.owner_call.inputs[0] - 1];
    const NLSemanticPlaceView root = entry->places[saved.owner_call.root - 1];
    const NLRawRegionEntry region =
        entry->regions[saved.owner_call.range.region - 1];
    const NLSemanticDomainView domain =
        entry->domains[saved.owner_call.domain - 1];
    for (size_t mutation = 0; mutation < 10; ++mutation) {
        switch (mutation) {
        case 0:
            entry->values[saved.owner_call.inputs[0] - 1].reference.provenance =
                NL_PROVENANCE_UNKNOWN;
            break;
        case 1:
            entry->values[saved.owner_call.inputs[0] - 1].reference_count = 2;
            break;
        case 2:
            entry->values[saved.owner_call.inputs[0] - 1].dependencies =
                NL_DEPENDENCIES_UNKNOWN;
            break;
        case 3:
            entry->places[saved.owner_call.root - 1].live = false;
            break;
        case 4:
            ++entry->places[saved.owner_call.root - 1].incarnation;
            break;
        case 5:
            entry->regions[saved.owner_call.range.region - 1].view.size = 25;
            break;
        case 6:
            entry->regions[saved.owner_call.range.region - 1]
                .view.ordinary_read = false;
            break;
        case 7:
            entry->regions[saved.owner_call.range.region - 1].view.alignment =
                1;
            break;
        case 8:
            entry->domains[saved.owner_call.domain - 1].value = 1;
            break;
        case 9:
            entry->scopes[0].active = true;
            break;
        }
        CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
        entry->values[saved.owner_call.inputs[0] - 1] = pointer;
        entry->places[saved.owner_call.root - 1] = root;
        entry->regions[saved.owner_call.range.region - 1] = region;
        entry->domains[saved.owner_call.domain - 1] = domain;
        entry->scopes[0].active = false;
        CHECK(expect(&n, NL_NODE_C_OK, baseline));
    }
    body->context = entry; /* numerically equal prefix != callee call world */
    CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
    body->context = post;
    NLSemanticContext *clone = NULL;
    CHECK(nl_sem_clone(post, &clone) == NL_CHECK_OK);
    body->context =
        clone; /* identical numbers/facts do not authorize another world */
    CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
    body->context = post;
    nl_semantic_destroy(clone);
    f->owner_entry_call = 0;
    CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
    f->owner_entry_call = id;
    post->places[saved.owner_call.root - 1].live = true;
    CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
    post->places[saved.owner_call.root - 1].live = false;
    bool ended = false;
    for (size_t i = 0; i < body->count; ++i)
        if (body->nodes[i].kind == NL_CHECKED_DESTROY) {
            NLCheckedNodeView op = body->nodes[i];
            body->nodes[i].lifetime_place = 1;
            CHECK(expect(&n, NL_NODE_C_UNSUPPORTED, baseline));
            body->nodes[i] = op;
            ended = true;
        }
    CHECK(ended && expect(&n, NL_NODE_C_OK, baseline));
    for (at = 0; at < 3; ++at) {
        calls = 0;
        inject = true;
        CHECK(expect(&n, at < 2 ? NL_NODE_C_OUT_OF_MEMORY : NL_NODE_C_OK,
                     baseline));
        inject = false;
        CHECK(expect(&n, NL_NODE_C_OK, baseline));
    }
    free(baseline);
    node_checked_destroy(&n);
    puts("32 broken certificates/world/operands rejected empty; 2 OOM sites "
         "retry identically");
    return true;
}
int main(int argc, char **argv)
{
    return argc == 2 && controls(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
