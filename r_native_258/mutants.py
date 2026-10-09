#!/usr/bin/env python3
"""Mutate only THIS RUN's freshly generated C, never P's output."""
from pathlib import Path
import re
from hashlib import sha256
src=Path("r_native_258/out/generated.c").read_text()
out=Path("r_native_258/out/mutants")
out.mkdir(exist_ok=True)
cases={}
def exact(old,new,name):
    if src.count(old)!=1: raise ValueError(f"expected exactly one fresh emitted C site {name}: {src.count(old)}")
    cases[name]=src.replace(old,new,1)
exact("nl_option * nl_v_180=&nl_v_179->f1;",
      "nl_option * nl_v_180=&nl_v_179->f0;","wrong_field_projection")
exact("free(nl_v_263.handle);","(void)0;","omit_final_physical_free")
exact("free(nl_v_263.handle);","free(nl_v_263.handle); free(nl_v_263.handle);","double_real_free_safely_trapped")
exact("if(nl_heap_1 == NULL){","if(nl_heap_1 != NULL){","wrong_guest_allocation_arm")
exact("nl_option nl_v_173={1,nl_v_172};","nl_option nl_v_173={0,nl_v_172};","wrong_option_some_tag")
for name,code in cases.items():
    path=out/(name+".c")
    path.write_text(code)
    print(name,sha256(code.encode()).hexdigest())
