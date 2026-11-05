/* Test-only access to the ordinary CLI's actual admission and emitter. */
#define main p1184_c246a_cli_main
#include "main.c"
#undef main

bool p1184_c246a_emit(FILE *stream, const NLCheckedFragment *root)
{
    const NLSemanticContext *context = nl_checked_context(root);
    V0Program program = {.unit = nl_semantic_unit_type(context),
                         .u8 = nl_semantic_core_type(context, NL_TYPE_U8)};
    return v0_validate_call(root, nl_checked_root(root), &program) &&
           v0_emit_c(stream, &program);
}
