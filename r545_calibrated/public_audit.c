/* Independent R545 calibrated audit. Two PUBLIC routes, no internal struct or test-support helper. */
#include "newlang/array4.h"
#include "newlang/checked.h"
#include "newlang/parser.h"
#include "newlang/semantic.h"
#include "newlang/source.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"R545 STOP %s:%d (%s)\n",__FILE__,__LINE__,#x); goto done; } } while (0)

static bool fragment_boundary(const char *path) {
    bool ok = false;
    NLSource *s = NULL;
    NLParser *p = NULL;
    NLSyntaxTree *ast = NULL;
    NLSemanticContext *ctx = NULL;
    NLCheckedFragment *f = NULL;
    NLArray4Construction views[2] = {{0}};
    REQUIRE(nl_source_load(path,&s)==NL_SOURCE_OK);
    REQUIRE(nl_parser_create(s,&p)==NL_PARSE_OK);
    REQUIRE(nl_parser_parse_source_fragment(p,&ast,NULL)==NL_PARSE_OK);
    REQUIRE(nl_semantic_create(&ctx)==NL_CHECK_OK);
    REQUIRE(nl_semantic_check_source_fragment(ctx,ast,&f,NULL)==NL_CHECK_OK);
    REQUIRE(nl_checked_array4_count(f)==2);
    for (size_t i=0;i<2;++i) {
        REQUIRE(nl_checked_array4_view_at(f,i,&views[i]));
        bool accepted = nl_checked_array4_valid_at(f,i,&views[i]);
        fprintf(stderr,"R545 A fragment event=%zu view=true validator=%s\n",
                i, accepted?"true":"false");
        REQUIRE(!accepted);
    }
    ok = true;
done:
    nl_checked_destroy(f);
    nl_semantic_destroy(ctx);
    nl_syntax_tree_destroy(ast);
    nl_parser_destroy(p);
    nl_source_destroy(s);
    return ok;
}

/* The real source function must be registered while parsed input is still live.
 * After registration all original caller-owned source/AST/parser are destroyed. */
static bool registered_context(const char *file, NLSemanticContext **out) {
    bool ok=false;
    NLSource *source=NULL;
    NLParser *parser=NULL;
    NLSyntaxTree *unit=NULL;
    NLSemanticContext *ctx=NULL;
    REQUIRE(nl_source_load(file,&source)==NL_SOURCE_OK);
    REQUIRE(nl_parser_create(source,&parser)==NL_PARSE_OK);
    REQUIRE(nl_parser_parse_function_unit(parser,&unit,NULL)==NL_PARSE_OK);
    REQUIRE(nl_semantic_create(&ctx)==NL_CHECK_OK);
    const NLSyntaxTree *const input[1]={unit};
    REQUIRE(nl_semantic_register_function_unit(ctx,input,1,NULL)==NL_CHECK_OK);
    ok=true;
done:
    nl_syntax_tree_destroy(unit);
    nl_parser_destroy(parser);
    nl_source_destroy(source);
    if(ok) *out=ctx;
    else nl_semantic_destroy(ctx);
    return ok;
}
static bool checked_main(NLSemanticContext *ctx, NLSource **out_src,
                         NLCheckedFragment **out_entry) {
    bool ok=false;
    NLSource *source=NULL;
    NLParser *parser=NULL;
    NLSyntaxTree *expression=NULL;
    NLCheckedFragment *entry=NULL;
    REQUIRE(nl_source_create("main()",6,"r545-real-main-call",&source)==NL_SOURCE_OK);
    REQUIRE(nl_parser_create(source,&parser)==NL_PARSE_OK);
    REQUIRE(nl_parser_parse_expression_fragment(parser,&expression,NULL)==NL_PARSE_OK);
    REQUIRE(nl_semantic_check_expression(ctx,expression,&entry,NULL)==NL_CHECK_OK);
    REQUIRE(nl_checked_call_body(entry,nl_checked_root(entry)) != NULL);
    ok=true;
done:
    nl_syntax_tree_destroy(expression);
    nl_parser_destroy(parser);
    if(ok) { *out_src=source; *out_entry=entry; }
    else { nl_checked_destroy(entry); nl_source_destroy(source); }
    return ok;
}
static bool registered_audit(const char *path) {
    bool ok=false;
    NLSemanticContext *ctx=NULL, *otherctx=NULL;
    NLSource *callsrc=NULL,*othercallsrc=NULL;
    NLCheckedFragment *entry=NULL,*otherentry=NULL;
    NLArray4Construction v[2]={{0}}, bad={0};
    size_t mutations=0;
    REQUIRE(registered_context(path,&ctx));
    REQUIRE(checked_main(ctx,&callsrc,&entry));
    const NLCheckedFragment *body=nl_checked_call_body(entry,nl_checked_root(entry));
    REQUIRE(body!=NULL);
    REQUIRE(nl_checked_array4_count(body)==2);
    REQUIRE(nl_checked_array4_world(body)==NULL); /* singleton APIs cannot select twins */
    REQUIRE(!nl_checked_array4_view(body,&bad));
    for(size_t i=0;i<2;++i) {
        REQUIRE(nl_checked_array4_view_at(body,i,&v[i]));
        REQUIRE(nl_checked_array4_valid_at(body,i,&v[i]));
        REQUIRE(v[i].origin==body);
        REQUIRE(v[i].root && v[i].incarnation && v[i].fact && v[i].value);
        const NLSemanticContext *world=nl_checked_array4_world_at(body,i);
        REQUIRE(world!=NULL);
        NLSemanticSnapshot snap={0};
        REQUIRE(nl_semantic_snapshot(world,&snap));
        REQUIRE(snap.places==5*(i+1) && snap.values==5*(i+1) &&
                snap.bindings==i+1 && snap.backing_regions==0 &&
                snap.domains==0 && snap.occurrences==0);
        NLSemanticTypeView array={0}, option={0};
        REQUIRE(nl_semantic_type_view(world,v[i].array_type,&array));
        REQUIRE(nl_semantic_type_view(world,v[i].element_type,&option));
        REQUIRE(array.kind==NL_TYPE_ARRAY && array.element_count==4 &&
                array.target==v[i].element_type && array.is_copy &&
                array.is_discardable);
        REQUIRE(option.kind==NL_TYPE_SUM && option.variant_count==2);
        NLSemanticPlaceView parent={0}, final={0};
        NLSemanticBindingView b={0};
        NLSemanticValueView av={0};
        REQUIRE(nl_semantic_place_view(world,v[i].root,&parent));
        REQUIRE(parent.live && parent.independent_root && parent.implicit_local &&
                !parent.parent_aggregate && !parent.parent_sum &&
                !parent.governing_domain && !parent.placement.region &&
                parent.incarnation==v[i].incarnation &&
                parent.current_fact==v[i].fact &&
                parent.current_value==v[i].value &&
                parent.fixed_field_count==4);
        REQUIRE(nl_semantic_binding_view(world,v[i].symbol,&b));
        REQUIRE(b.availability==NL_AVAILABLE && b.place==v[i].root);
        REQUIRE(nl_semantic_value_view(world,v[i].value,&av));
        REQUIRE(av.field_count==4 && av.carrier==NL_CARRIER_PLACE &&
                av.owner_place==v[i].root && !av.domain &&
                !av.allocation_region && av.dependencies==NL_DEPENDENCY_FREE);
        REQUIRE(nl_semantic_place_view(ctx,v[i].root,&final) && !final.live);
        REQUIRE(nl_semantic_binding_view(ctx,v[i].symbol,&b) &&
                b.availability==NL_CONSUMED);
        for(size_t j=0;j<4;++j) {
            NLArray4Element e=v[i].elements[j];
            NLSemanticPlaceView child={0};
            NLSemanticValueView ev={0};
            REQUIRE(e.index==j && e.place && e.initializer && e.incarnation &&
                    e.fact && e.value && parent.fixed_fields[j]==e.place);
            REQUIRE(nl_semantic_place_view(world,e.place,&child));
            REQUIRE(nl_semantic_value_view(world,e.value,&ev));
            REQUIRE(child.live && !child.independent_root &&
                    child.parent_aggregate==v[i].root &&
                    child.parent_incarnation==v[i].incarnation &&
                    child.parent_field_index==j &&
                    child.current_value==e.value &&
                    child.current_fact==e.fact &&
                    child.incarnation==e.incarnation &&
                    child.type==v[i].element_type);
            REQUIRE(ev.carrier==NL_CARRIER_AGGREGATE &&
                    ev.aggregate_owner==v[i].value &&
                    ev.type==v[i].element_type && ev.variant==1 &&
                    !ev.sum_payload && !ev.domain && !ev.allocation_region);
            REQUIRE(av.fields[j]==e.value);
            for(size_t k=0;k<j;++k)
                REQUIRE(v[i].elements[k].place!=e.place);
            fprintf(stderr,"R545 B issued[%zu].child[%zu] root=%zu place=%zu incarnation=%zu fact=%zu value=%zu\n",
                    i,j,(size_t)v[i].root,(size_t)e.place,
                    (size_t)e.incarnation,(size_t)e.fact,(size_t)e.value);
        }
        fprintf(stderr,"R545 B certified[%zu] root=%zu identity=%zu fact=%zu\n",
                i,(size_t)v[i].root,(size_t)v[i].incarnation,(size_t)v[i].fact);
    }
    REQUIRE(v[0].root!=v[1].root && v[0].incarnation!=v[1].incarnation &&
            v[0].fact!=v[1].fact && v[0].value!=v[1].value &&
            v[0].binding!=v[1].binding && v[0].constructor!=v[1].constructor);
    for(size_t j=0;j<4;++j)
        for(size_t k=0;k<4;++k)
            REQUIRE(v[0].elements[j].place != v[1].elements[k].place);
    REQUIRE(registered_context(path,&otherctx));
    REQUIRE(checked_main(otherctx,&othercallsrc,&otherentry));
    const NLCheckedFragment *foreign =
        nl_checked_call_body(otherentry,nl_checked_root(otherentry));
    REQUIRE(foreign && foreign!=body && nl_checked_array4_count(foreign)==2);
#define MUTATE(i,stmt) do { \
    bad=v[(i)]; stmt; \
    if(nl_checked_array4_valid_at(body,(i),&bad)) { \
        fprintf(stderr,"R545 CALIBRATED FINDING accepted mutant at %d\n",__LINE__); goto done; \
    } \
    REQUIRE(nl_checked_array4_valid_at(body,(i),&v[(i)])); \
    ++mutations; \
} while(0)
    for(size_t i=0;i<2;++i) {
        size_t peer=1-i;
        MUTATE(i,bad.root=v[peer].root);
        MUTATE(i,bad.root=SIZE_MAX);
        MUTATE(i,bad.binding=v[peer].binding);
        MUTATE(i,bad.receiver=v[peer].receiver);
        MUTATE(i,bad.constructor=v[peer].constructor);
        MUTATE(i,bad.symbol=v[peer].symbol);
        MUTATE(i,bad.origin=foreign);
        MUTATE(i,bad.incarnation+=1);
        MUTATE(i,bad.fact+=1);
        MUTATE(i,bad.value=v[peer].value);
        MUTATE(i,bad.array_type=v[i].element_type);
        MUTATE(i,bad=v[peer]); /* duplicate other root's event */
        for(size_t j=0;j<4;++j) {
            MUTATE(i,bad.elements[j].index=(j+1)%4);
            MUTATE(i,bad.elements[j].place=v[peer].elements[j].place);
            MUTATE(i,bad.elements[j].place=v[i].root);
            MUTATE(i,bad.elements[j].place=v[i].elements[(j+1)%4].place);
            MUTATE(i,bad.elements[j].initializer=v[peer].elements[j].initializer);
            MUTATE(i,bad.elements[j].incarnation+=1);
            MUTATE(i,bad.elements[j].fact+=1);
            MUTATE(i,bad.elements[j].value=v[peer].elements[j].value);
            MUTATE(i,bad.elements[j]=v[peer].elements[j]);
        }
        REQUIRE(!nl_checked_array4_valid_at(foreign,i,&v[i]));
        REQUIRE(!nl_checked_array4_valid_at(body,1-i,&v[i]));
        REQUIRE(nl_checked_array4_valid_at(body,i,&v[i]));
    }
#undef MUTATE
    fprintf(stderr,"R545 B PASS: registered genuine source, 2 originals, 8 child places, %zu rejected copy mutations, final lexicals ended; NOT live rights\n",mutations);
    ok=true;
done:
    nl_checked_destroy(otherentry);
    nl_source_destroy(othercallsrc);
    nl_semantic_destroy(otherctx);
    nl_checked_destroy(entry);
    nl_source_destroy(callsrc);
    nl_semantic_destroy(ctx);
    return ok;
}
int main(int argc, char **argv) {
    if(argc!=3) return 2;
    if(!fragment_boundary(argv[2])) return 11;
    if(!registered_audit(argv[1])) return 12;
    return 0;
}
