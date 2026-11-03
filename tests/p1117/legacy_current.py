"""Version the one obsolete precision expectation, preserving both originals.

Every other parser/value/type/loan/write/transaction/OOM assertion is compiled
verbatim. The current oracle requires real successful registration and an
actual typed Bool result; no checker return value or diagnostic is faked.
"""
from pathlib import Path
import hashlib
import sys

PINS = {
    'p504': '6fc73c22f544e4d66d1eaf8d6526868cf7bb15d76ea2ab93f1bae880987e4e79',
    'p603': 'dba648f261a0ae2415c0ccb0e0bc0070399b9ee1dcb52fad5c97fe1b7fa0e040',
}
root, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
for name, digest in PINS.items():
    old = root/'tests'/name/('equality_test.c' if name == 'p504' else 'comparison_test.c')
    raw = old.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == digest, old
    code = 'USIZE-EQUALITY-PRECISION' if name == 'p504' else 'USIZE-LESS-THAN-PRECISION'
    before = '''    CHECK(nl_semantic_register_function_unit(c, inputs, 1, &d) ==
          NL_CHECK_ANALYSIS_PRECISION_LIMIT);
    CHECK(strcmp(d.diagnostic.diagnostic.code, "'''+code+'''") ==
              0 &&
          test_unchanged(c, &before));'''
    assert raw.decode().count(before) == 1
    after = '''    CHECK(nl_semantic_register_function_unit(c, inputs, 1, &d) ==
          NL_CHECK_OK);
    CHECK(!test_unchanged(c, &before));
    NLSource *call_source = NULL;
    NLParser *call_parser = NULL;
    NLSyntaxTree *call_tree = NULL;
    NLCheckedFragment *call_owned = NULL;
    const char *call_text = "same(usize(2),usize(3))";
    CHECK(nl_source_create(call_text, strlen(call_text),
                           "P1117-current-legacy", &call_source) == NL_SOURCE_OK);
    CHECK(nl_parser_create(call_source, &call_parser) == NL_PARSE_OK);
    CHECK(nl_parser_parse_expression_fragment(call_parser, &call_tree, NULL) ==
          NL_PARSE_OK);
    CHECK(nl_semantic_check_expression(c, call_tree, &call_owned, NULL) ==
          NL_CHECK_OK);
    const NLCheckedNodeView *actual =
        nl_checked_node_view(call_owned, nl_checked_root(call_owned));
    NLSemanticValueView value;
    CHECK(actual && actual->result_count == 1 &&
          actual->type == nl_semantic_core_type(c, NL_TYPE_BOOL) &&
          nl_semantic_value_view(c, actual->results[0].value, &value) &&
          value.scalar_known && value.scalar_value == '''+('0' if name == 'p504' else '1')+''' &&
          value.dependencies == NL_DEPENDENCY_FREE);
    nl_checked_destroy(call_owned);
    nl_syntax_tree_destroy(call_tree);
    nl_parser_destroy(call_parser);
    nl_source_destroy(call_source);'''
    (output/(name+'_current.c')).write_text(raw.decode().replace(before,after))
    print('P1117 current oracle preserves original',name,digest)
