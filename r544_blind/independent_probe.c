#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "newlang/source.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/checked.h"
#include "newlang/array4.h"

#define REQ(test,msg) do { if (!(test)) { fprintf(stderr, "R544-PROBE STOP %s at %d\n", (msg), __LINE__); goto cleanup; } } while (0)
int main(int argc, char **argv) {
  int exitcode = 1;
  NLSource *src = NULL;
  NLParser *parser = NULL;
  NLSyntaxTree *tree = NULL;
  NLSemanticContext *sem = NULL;
  NLCheckedFragment *frag = NULL;
  NLArray4Construction a = {0}, b = {0};
  NLParseDiagnostic pd = {0};
  NLCheckDiagnostic cd = {0};
  REQ(argc == 2, "source argument");
  REQ(nl_source_load(argv[1], &src) == NL_SOURCE_OK, "load authentic body source");
  REQ(nl_parser_create(src, &parser) == NL_PARSE_OK, "create parser");
  NLParseStatus ps = nl_parser_parse_source_fragment(parser, &tree, &pd);
  if (ps != NL_PARSE_OK) fprintf(stderr,"R544-PROBE parse-status=%d\n", (int)ps);
  REQ(ps == NL_PARSE_OK, "public parse real body");
  REQ(nl_semantic_create(&sem) == NL_CHECK_OK, "create semantic context");
  NLCheckStatus cs = nl_semantic_check_source_fragment(sem, tree, &frag, &cd);
  if (cs != NL_CHECK_OK) fprintf(stderr,"R544-PROBE check-status=%d\n", (int)cs);
  REQ(cs == NL_CHECK_OK, "source-issued checked owned fragment");
  size_t n = nl_checked_array4_count(frag);
  fprintf(stderr, "R544-PROBE PUBLIC count=%zu\n", n);
  REQ(n == 2, "two source-written genuine constructors");
  REQ(nl_checked_array4_view_at(frag, 0, &a), "first public view");
  REQ(nl_checked_array4_view_at(frag, 1, &b), "second public view");
  REQ(nl_checked_array4_valid_at(frag, 0, &a), "first authentic proof validated");
  REQ(nl_checked_array4_valid_at(frag, 1, &b), "second authentic proof validated");
  REQ(a.origin == b.origin && a.origin == frag, "common genuine source artifact");
  REQ(a.root != b.root && a.root && b.root, "independent original roots");
  REQ(a.binding != b.binding && a.constructor != b.constructor, "independent source constructor nodes");
  REQ(a.incarnation && b.incarnation && a.incarnation != b.incarnation, "independent current incarnations");
  REQ(a.array_type == b.array_type && a.element_type == b.element_type, "shared structural types, different roots");
  for (size_t j = 0; j < 2; j++) {
    const NLArray4Construction *v = j ? &b : &a;
    const NLSemanticContext *w = nl_checked_array4_world_at(frag,j);
    REQ(w, "real immutable event-qualified world");
    NLSemanticPlaceView root = {0};
    REQ(nl_semantic_place_view(w,v->root,&root), "captured live root");
    REQ(root.live && root.independent_root && root.implicit_local, "original implicit live root");
    REQ(root.incarnation == v->incarnation, "root incarnation intact");
    REQ(root.fixed_field_count == 4, "exact four fixed child places");
    NLSemanticTypeView at = {0}, et = {0};
    REQ(nl_semantic_type_view(w,v->array_type,&at), "Array type");
    REQ(nl_semantic_type_view(w,v->element_type,&et), "Option type");
    REQ(at.kind == NL_TYPE_ARRAY && at.element_count == 4 &&
        at.target == v->element_type, "genuine structural Array<Option,4>");
    REQ(et.kind == NL_TYPE_SUM, "each full Option typed element");
    for (size_t i = 0; i < 4; i++) {
      NLArray4Element e = v->elements[i];
      REQ(e.index == i && e.place && e.incarnation && e.initializer, "ordered source-written item");
      REQ(root.fixed_fields[i] == e.place, "root points at exact dependent place");
      NLSemanticPlaceView child = {0};
      REQ(nl_semantic_place_view(w,e.place,&child), "public child");
      REQ(child.live && !child.independent_root && child.parent_aggregate == v->root &&
          child.parent_incarnation == v->incarnation && child.parent_field_index == i,
          "child depends on its real original root");
      REQ(child.type == v->element_type && child.incarnation == e.incarnation,
          "typed child current incarnation");
      for (size_t k = 0; k < i; k++)
        REQ(v->elements[k].place != e.place, "per-original unique projections");
      fprintf(stderr,"R544-PROBE root%zu child%zu parent=%zu place=%zu inc=%zu\n",
              j,i,(size_t)child.parent_aggregate,(size_t)e.place,(size_t)e.incarnation);
    }
  }
  for (size_t i = 0; i < 4; i++)
    for (size_t k = 0; k < 4; k++)
      REQ(a.elements[i].place != b.elements[k].place, "no cross-root child alias");
  NLArray4Construction modified = b;
  modified.elements[0].place = a.elements[0].place;
  REQ(!nl_checked_array4_valid_at(frag,1,&modified), "reject cross-root reparent");
  modified = b; modified.root = a.root;
  REQ(!nl_checked_array4_valid_at(frag,1,&modified), "reject cloned root");
  modified = b; modified.elements[1].initializer = a.elements[1].initializer;
  REQ(!nl_checked_array4_valid_at(frag,1,&modified), "reject wrong original source leaf");
  modified = b; modified.elements[1].incarnation++;
  REQ(!nl_checked_array4_valid_at(frag,1,&modified), "reject stale incarnation");
  modified = b; modified.constructor = a.constructor;
  REQ(!nl_checked_array4_valid_at(frag,1,&modified), "reject foreign constructor origin");
  modified = b; modified.elements[3].index = 2;
  REQ(!nl_checked_array4_valid_at(frag,1,&modified), "reject repeated projection index");
  REQ(!nl_checked_array4_view(frag,&modified), "legacy singleton API failclosed");
  REQ(!nl_checked_array4_valid(frag,&b), "legacy validator failclosed on twin");
  fprintf(stderr,"R544-PROBE PUBLIC twin issued+validated, six altered-copy rejections, no authority fabricated\n");
  exitcode = 0;
cleanup:
  nl_checked_destroy(frag);
  nl_semantic_destroy(sem);
  nl_syntax_tree_destroy(tree);
  nl_parser_destroy(parser);
  nl_source_destroy(src);
  return exitcode;
}
