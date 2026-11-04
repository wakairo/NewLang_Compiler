from pathlib import Path
import json,hashlib,subprocess,sys,os
root=Path(__file__).resolve().parent
manifest=json.loads((root/"freeze.json").read_text())
for name,digest in manifest["files"].items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
compiler=root/"newlangc-base"
assert hashlib.sha256(compiler.read_bytes()).hexdigest()==manifest["compiler_sha256"]
raw=root/"first-cli";raw.mkdir(exist_ok=False);rows=[]
for source in sorted((root/"sources").glob("*.nl")):
    for mode in ("check","emit","named"):
        label=source.stem+"-"+mode
        target=raw/(source.stem+"-named.c")
        sentinel=b"C226 initial named output must survive rejection\n"
        if mode=="check":command=[compiler,"--check",source]
        elif mode=="emit":command=[compiler,source]
        else:
            target.write_bytes(sentinel);before=target.stat()
            command=[compiler,"--emit-c17-to",target,source]
        r=subprocess.run(list(map(str,command)),capture_output=True,timeout=120)
        (raw/(label+".out")).write_bytes(r.stdout)
        (raw/(label+".err")).write_bytes(r.stderr)
        row={"case":source.stem,"mode":mode,"command":list(map(str,command)),
             "status":r.returncode,"stdout_bytes":len(r.stdout),"stderr_bytes":len(r.stderr)}
        if mode=="named":
            after=target.stat()
            row["unchanged"]=(target.read_bytes()==sentinel and before.st_ino==after.st_ino and before.st_mtime_ns==after.st_mtime_ns)
            row["temporary_files"]=[p.name for p in raw.iterdir() if p.name.startswith(target.name+".")]
        rows.append(row)
        (raw/"observations.json").write_text(json.dumps(rows,indent=2)+"\n")
        print(source.stem,mode,r.returncode,"stdout",len(r.stdout),r.stderr.decode().strip(),flush=True)
print("C226 fixed-main FIRST observations preserved",len(rows))
