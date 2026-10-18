#!/usr/bin/env python3
"""R600 pre-registered first-pass; never substitutes P fixtures or generated C."""
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import tempfile

root=pathlib.Path(sys.argv[1]).resolve()
compiler=pathlib.Path(sys.argv[2]).resolve()
manifest_path=root/"independent-frozen-oracles.json"
manifest=json.loads(manifest_path.read_text())
assert manifest["authority"]["candidate"]=="80f2ae3786a63235138408b35a2129a3b12afa33"
frozen={"sub_71_9":"346f72be4d1ae6929e2fc4c843b7f25232da598d437c813e16085e7bbd403c74",
"sub_53_27":"55ee03b8ab1e27c808c83bc12d6401e778b7ac3c0d5bb38afd2f431d2ab0e761",
"sub_named_copy":"06c9a51e8725b266dc3596a6b655052b043954076bdafa6175731c1fa0c435e0",
"sub_equal_zero":"04f1c36110361856250e3ec8cc867da0ee9787ce55c9cb0a440a6e15c2a9ab2a",
"sub_max_boundary":"56f9e1005b0c15d767b8d9b05586db6e1a126d80e867295be0fbd5c5aed6bcc4",
"sub_separate_fns":"83c86a59104a0d5a15e71fa8a173cbf4fddcb59d12b27b9a5a52b8d5b1ed527f",
"sub_sequence":"f3d3d5b3d068e4521635a916defca11453762a6bb6a4c85ddd98875c7a0ddb35",
"plus_existing":"07b4e091573a7dc0ac94a4f2e3c31c4c77d7f885fb506281c1fe8b5632d9fe08",
"plus_near_max":"4acd44705d75efcb4855accb138af6beba7aa6b936d4f694083b9c6d8e565306",
"equality_existing":"40ff4bdffec5241058cc3279e95b1ed279aee02c6c027b26e459260eb9253a4c",
"underflow":"12793976c7287e1e5806ff46cf196df13603179c724fdea35b537f0cfd71408e",
"plus_overflow":"7357f33aaf23adcf125963bb7efcbd8fa0869fa44d86d094e6e23999bf2e1153",
"wrong_u8_type":"66a58719df6397aaaaa24908ca1f2201c9638b0d73e41f08c381025e8c2e9e4c",
"unknown_operand":"aa7dda7ff411dde32c6f516c4392ae5fcd4354bb2e8b381111f95c8af7fdfae6",
"sub_chaining":"cc286bcf5c642302c94aa3cc6c0b22425c0f5852f113d99af2868fc3599fa602",
"mixed_op_chain":"8877fd4e7f5afb4853a355568f61fb111d5b2074826d93dc14bb08cdbcc2bb29",
"parenthesized":"52a45a405d891587a3d9b94e5c3c2ef6007051c908e491f37a34c2ea596e6580",
"double_minus":"f7b9821258cc70e0abd51e8c624878449c3d597b346bab63d8360a81194034a6",
"minus_equals":"5133b35de5261e43a139261a69f5c0d5aba3b39d240ebe79a8a96ee4f3702cad",
"bare_operand":"334f8051a13d48d5731dbb1218a71435afa9de0830affb43b9a5a9d9ccfaf985",
"nested_call":"ace63a47b322eaacd4ceea061d2847cadced1724bcdb5426721e2d30be4a315f",
"unselected_uintptr":"36c23f0bd70e107d5a2cdb78a865fc00d501196c4cd338d0e5725711c07c4a61",
"return_arrow":"1caef0b018f1c50afcf0428c4360d8f83c1bb5425c0c719b291952884ae4be77"}
assert len(manifest["cases"])==len(frozen)==23
summaries=[]
failures=[]
with tempfile.TemporaryDirectory(prefix="r600-own-") as dirname:
    outdir=pathlib.Path(dirname)
    for entry in manifest["cases"]:
        name=entry["id"]; src=entry["source"]; sha=hashlib.sha256(src.encode()).hexdigest()
        assert sha==entry["source_sha256"]==frozen[name],name
        path=outdir/(name+".nl");path.write_bytes(src.encode())
        expected=entry["expected"];want=expected["source"]=="accept" and expected["check"]=="accept"
        run=subprocess.run([str(compiler),"--check",str(path)],capture_output=True,text=True,timeout=15)
        check_ok=run.returncode==0
        obs={"id":name,"sha":sha,"check_exit":run.returncode,
             "source_expected":expected["source"],"check_expected":expected["check"],
             "diagnostic":(run.stderr+" "+run.stdout).strip()[:450]}
        if check_ok!=want:failures.append(name+": unexpected CHECK acceptance/rejection")
        if check_ok:
            code=subprocess.run([str(compiler),str(path)],capture_output=True,text=True,timeout=15)
            obs["c17_exit"]=code.returncode
            if code.returncode!=0 or not code.stdout:
                failures.append(name+": no C17")
                obs["c17_err"]=code.stderr[:450]
            else:
                raw=code.stdout.encode()
                obs["c17_sha256"]=hashlib.sha256(raw).hexdigest()
                obs["c17_size"]=len(raw)
                path_c=outdir/(name+".c");path_c.write_bytes(raw)
                if "sub" in name or name=="return_arrow":
                    obs["c17_size_t_lines"]=[x.strip()[:150] for x in code.stdout.splitlines() if "size_t" in x and ("+" in x or "-" in x)][:10]
                for cc in ["gcc","clang"]:
                    for opt in ["-O0","-O2"]:
                        executable=outdir/(name+"-"+cc+opt.replace("-",""))
                        command=[cc,"-std=c17","-Wall","-Wextra","-Wpedantic","-Werror",opt,str(path_c),"-o",str(executable)]
                        build=subprocess.run(command,capture_output=True,text=True,timeout=20)
                        key=cc+opt
                        if build.returncode!=0:
                            obs[key]="BUILD_ERROR "+build.stderr[:300]
                            failures.append(name+":"+key+" build failed")
                        else:
                            native=subprocess.run([str(executable)],capture_output=True,text=True,timeout=5)
                            obs[key]="OK" if native.returncode==0 else "RUN_ERROR "+str(native.returncode)
                            if native.returncode!=0:failures.append(name+":"+key+" native failed")
        summaries.append(obs)
        print("R600_CASE "+json.dumps(obs,sort_keys=True),flush=True)
    print("R600_SUMMARY "+json.dumps({"cases":len(summaries),"mismatch_count":len(failures),"mismatches":failures},sort_keys=True),flush=True)
sys.exit(0 if not failures else 1)
