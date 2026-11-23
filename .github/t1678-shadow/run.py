"""Run or verify only the finite T1678 shadow cohort."""
from graphlib import TopologicalSorter
from hashlib import file_digest
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import time
from xml.etree import ElementTree

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
HEAVY = frozenset(("p1291_named_oom.integration", "p1299_named_oom.integration"))
LANES = {"Heavy-1": 1, "Heavy-2": 2, "Light-3": 3, "Light-4": 4}
EXPECTED_EDGES = {
    ("p1227_c283_alias_oom.unit", "p1227_oom.unit"),
    ("p1471_cli_native.integration", "p1471_terminal_owned_negative.unit"),
}
HOOK = "cmake/independent-r-tests-hook.cmake"
TRUSTED_FILES = {
    HOOK: "ecd14067f30cac138607b606916e84464b2391b61703ebe2e530940e27ec64af",
    "scripts/bootstrap.py": "1d7e70cba29fe39129433681c859d7127a7c6246694e2a15ff5c90733ae9fe4f",
    "scripts/toolchain-lock.json": "7c9921be7919a10d02877605761032b683c7258a0c09ea6c2d7d26a02dca8009",
}
PATCH_PATHS = {
    ".github/workflows/t1678-asan-shadow.yml", ".github/t1678-shadow/expected-names.txt",
    ".github/t1678-shadow/overlay.cmake", ".github/t1678-shadow/run.py",
}
SANITIZER = re.compile(rb"ERROR: (AddressSanitizer|LeakSanitizer)|AddressSanitizer:DEADLYSIGNAL")
START = re.compile(r"^\s*Start\s+\d+:\s+(\S+)\s*$", re.MULTILINE)
FINISH = re.compile(r"^\s*\d+/\d+\s+Test\s+#\d+:\s+(\S+)\s+\.+\s+Passed\b", re.MULTILINE)

def require(condition, reason):
    if not condition:
        raise SystemExit(f"T1678 STOP: {reason}")

def digest(path):
    with path.open("rb") as stream:
        return file_digest(stream, "sha256").hexdigest()

def record(path, data):
    path.write_text(json.dumps(data, sort_keys=True, indent=2) + "\n")

def check_output(*args):
    return subprocess.check_output(args, cwd=ROOT, text=True).strip()

def git(*args):
    return check_output("git", *args)

def verify_checkout(evidence):
    head = os.environ["GITHUB_SHA"]
    base = os.environ["T1678_BASE"]
    pr_head = os.environ["T1678_PR_HEAD"]
    pr_number = os.environ["T1678_PR_NUMBER"]
    attempt = os.environ["GITHUB_RUN_ATTEMPT"]
    require(pr_number.isdecimal() and os.environ["GITHUB_REF"]==f"refs/pull/{pr_number}/merge", "checkout ref")
    require(attempt=="1" and os.environ["GITHUB_EVENT_NAME"]=="pull_request", "event/attempt")
    require(head==git("rev-parse","HEAD") and git("show","-s","--format=%P",head).split()==[base,pr_head], "immutable merge parents")
    parents = [line.split()[1] for line in git("cat-file","-p",pr_head).split("\n\n",1)[0].splitlines() if line.startswith("parent ")]
    require(parents==[base], "pilot must have one commit on the released base")
    require(git("rev-parse",f"{base}^{{tree}}")==os.environ["T1678_BASE_TREE"], "released base tree")
    tree=git("rev-parse","HEAD^{tree}")
    require(tree==git("rev-parse",f"{pr_head}^{{tree}}"), "merge tree differs from pilot tree")
    changes=set(git("diff","--name-status",base,head).splitlines())
    require(changes=={f"A\t{path}" for path in PATCH_PATHS}, "unexpected patch paths")
    require(not git("status","--porcelain"), "checkout is dirty")
    for path in PATCH_PATHS:
        require(git("ls-tree",head,"--",path).startswith("100644 blob "), f"unexpected patch object mode {path}")
    for path,expected in TRUSTED_FILES.items():
        require(digest(ROOT/path)==expected, f"trusted input differs: {path}")
    record(evidence/"checkout.json",{
        "head": head, "tree":tree, "base":base, "base_tree":os.environ["T1678_BASE_TREE"],
        "pr_head":pr_head, "pr_number":int(pr_number), "run_id":os.environ["GITHUB_RUN_ID"],
        "attempt":int(attempt), "patch":{path:digest(ROOT/path) for path in sorted(PATCH_PATHS)},
        "trusted_inputs":TRUSTED_FILES,
    })

def inventory(path):
    data=json.loads(path.read_text())
    require(data["kind"]=="ctestInfo" and data["version"]["major"]==1, "CTest JSON format")
    result={}
    for test in data["tests"]:
        name=test["name"]
        props=test["properties"]
        require(name not in result and bool(test.get("command")), f"duplicate or missing command: {name}")
        prop_map={row["name"]:row["value"] for row in props}
        require(len(prop_map)==len(props), f"duplicate properties: {name}")
        result[name]={"command":test["command"],"properties":prop_map}
    return result

def test_partition(baseline, overlay, wanted):
    require(set(baseline)==set(overlay)==wanted and "llvm.c_api" in wanted, "full inventory drift")
    light=wanted-HEAVY
    require(HEAVY <= wanted and not (HEAVY & light) and HEAVY | light==wanted, "candidate partition")
    graph={}
    for name, before in baseline.items():
        original=before["properties"]
        graph[name]=set(original.get("DEPENDS",[]))
        require(graph[name] <= wanted, f"dependency missing for {name}")
        for prop in ("FIXTURES_SETUP","FIXTURES_REQUIRED","FIXTURES_CLEANUP","RESOURCE_LOCK","RESOURCE_GROUPS"):
            require(prop not in original, f"new fixture or resource property: {name} {prop}")
        require(not original.get("DISABLED"), f"disabled test {name}")
        expected=original.copy()
        if name in HEAVY:
            require(expected.pop("RUN_SERIAL",None) is True, f"original RUN_SERIAL missing: {name}")
        require(overlay[name]["command"]==before["command"] and overlay[name]["properties"]==expected,
                f"unexpected command/property delta: {name}")
    edges={(consumer,producer) for consumer,producers in graph.items() for producer in producers}
    require(edges==EXPECTED_EDGES and all(set(edge) <= light for edge in edges), "dependency closure drift")
    TopologicalSorter(graph).prepare()
    return light

def check_outcomes(junit, stdout, selected):
    suite=ElementTree.parse(junit).getroot()
    cases=suite.findall("./testcase")
    require(suite.get("tests")==str(len(selected)) and len(cases)==len(selected), "JUnit count")
    require(all(suite.get(key)=="0" for key in ("failures","disabled","skipped")), "JUnit failure/skip")
    require({case.get("name") for case in cases}==selected, "JUnit name set")
    require(all(case.get("status")=="run" and not any(case.find(key) is not None for key in ("skipped","error","failure")) for case in cases),
            "JUnit test status")
    text=stdout.read_text(errors="replace")
    starts=START.findall(text)
    passes=FINISH.findall(text)
    require(len(starts)==len(passes)==len(selected) and set(starts)==set(passes)==selected, "CTest log name/status bijection")
    active=set()
    peak=0
    for line in text.splitlines():
        if match:=START.match(line):
            name=match[1]
            require(name not in active, "duplicate active test")
            active.add(name)
            peak=max(peak,len(active))
        if match:=FINISH.match(line):
            name=match[1]
            require(name in active, "completion without start")
            active.remove(name)
    require(not active, "unmatched starts")
    return peak

def stable_build(build):
    sdk=Path(os.environ["NEWLANG_SDK_ROOT"])
    paths=(build/"newlangc", build/"p1280_cli_oom", build/"compile_commands.json",
           build/"CMakeFiles/newlangc.dir/flags.make", build/"CMakeFiles/newlangc.dir/link.txt",
           build/"CMakeFiles/p1280_cli_oom.dir/flags.make", build/"CMakeFiles/p1280_cli_oom.dir/link.txt",
           sdk/"bin/clang-23", sdk/"bin/llvm-config-23")
    return {str(path.relative_to(ROOT)):digest(path) for path in paths}

def host_observation(build):
    cgroup={}
    for name in ("cpu.max","cpu.stat","memory.max","memory.current","memory.peak","memory.events","io.stat"):
        try:
            cgroup[name]=(Path("/sys/fs/cgroup")/name).read_text().strip()
        except OSError:
            continue
    fs=os.statvfs(build)
    return {"runner_cgroup_scope":cgroup, "runner_filesystem_free_bytes":fs.f_bavail*fs.f_frsize,
            "runner_filesystem_free_inodes":fs.f_favail}

def oom_events(observation):
    text=observation["runner_cgroup_scope"].get("memory.events","")
    return {key:int(value) for key,value in (line.split() for line in text.splitlines()) if key in ("oom","oom_kill","oom_group_kill")}

def preserve_ctest_outputs(evidence, build):
    for name in ("LastTest.log", "LastTestsFailed.log"):
        path=build/"Testing/Temporary"/name
        if path.is_file():
            shutil.copyfile(path,evidence/name)
    for test in HEAVY:
        raw=build/f"{test.split('_',1)[0]}-named-oom"
        if raw.is_dir() and not raw.is_symlink():
            for path in raw.rglob("*"):
                if path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(raw):
                    copied=evidence/raw.name/path.relative_to(raw)
                    copied.parent.mkdir(parents=True,exist_ok=True)
                    shutil.copyfile(path,copied)

def run_cases(evidence, build, choice, workers, deadline):
    command=["ctest","--test-dir",str(build),"--parallel",str(workers),*choice,
             "--no-tests=error","--stop-on-failure","--output-on-failure","--output-junit",str(evidence/"test-junit.xml")]
    record(evidence/"ctest-argv.json",command)
    before=host_observation(build)
    started=time.time()
    started_clock=time.monotonic()
    timed_out=False
    with (evidence/"ctest.stdout").open("wb") as out, (evidence/"ctest.stderr").open("wb") as err:
        proc=subprocess.Popen(command,cwd=ROOT,stdout=out,stderr=err,start_new_session=True)
        try:
            while True:
                pid,status,usage=os.wait4(proc.pid,os.WNOHANG)
                if pid:
                    proc.returncode=os.waitstatus_to_exitcode(status)
                    break
                if time.monotonic() >= deadline:
                    timed_out=True
                    os.killpg(proc.pid,signal.SIGKILL)
                    _,status,usage=os.wait4(proc.pid,0)
                    proc.returncode=os.waitstatus_to_exitcode(status)
                    break
                time.sleep(0.1)
        finally:
            if proc.returncode is None:
                try:
                    os.killpg(proc.pid,signal.SIGKILL)
                except ProcessLookupError:
                    pass
                _,status,_=os.wait4(proc.pid,0)
                proc.returncode=os.waitstatus_to_exitcode(status)
    after=host_observation(build)
    result={"started_epoch":started,"wall_seconds":time.monotonic()-started_clock,"ctest_exit":proc.returncode,
            "evidence_watchdog":timed_out,"largest_waited_process_max_rss_kib":usage.ru_maxrss,
            "input_block_accounting":usage.ru_inblock,"output_block_accounting":usage.ru_oublock,
            "runner_before":before,"runner_after":after}
    record(evidence/"ctest-resources.json",result)
    preserve_ctest_outputs(evidence,build)
    require(not timed_out and proc.returncode==0,"CTest failed or exceeded the evidence deadline")
    require(oom_events(before)==oom_events(after),"runner OOM event")
    for path in (evidence/"ctest.stdout",evidence/"ctest.stderr",evidence/"LastTest.log"):
        require(not SANITIZER.search(path.read_bytes()),f"sanitizer finding in {path.name}")
    return result

def run(evidence):
    lane=os.environ["T1678_LANE"]
    workers=int(os.environ["T1678_WORKERS"])
    require(LANES.get(lane)==workers,"unsupported matrix lane")
    nproc=int(check_output("nproc"))
    require(nproc>=workers,"runner has fewer CPUs than the requested CTest workers")
    sdk=Path(os.environ["NEWLANG_SDK_ROOT"])
    require(sdk.is_relative_to(ROOT/".deps/toolchains"),"unexpected SDK root")
    require(Path(shutil.which("clang-23") or "")==sdk/"bin/clang-23","unexpected Clang path")
    require(check_output("llvm-config-23","--version")=="23.1.3" and
            "clang version 23.1.3" in check_output("clang-23","--version"),"unexpected SDK version")
    require(os.environ.get("CC")=="clang-23" and os.environ.get("CFLAGS")=="-Wno-error=gcc-install-dir-libstdcxx",
            "original ASan environment")
    require(not ({"ASAN_OPTIONS","UBSAN_OPTIONS"} & os.environ.keys()),"unexpected ambient sanitizer options")
    build=ROOT/"build"
    require(not build.exists(),"shadow build directory is already present")
    job_start=int((evidence/"job-start.epoch").read_text())
    # Leave three minutes inside the 45-minute job cap for sealing/upload.
    setup_seconds=time.time()-job_start
    require(0<=setup_seconds<42*60,"job setup consumed the evidence budget")
    job_clock=time.monotonic()-setup_seconds
    deadline=job_clock+42*60
    record(evidence/"run-setup.json",{"lane":lane,"ctest_workers":workers,"build_workers":nproc,
           "setup_seconds_to_configure":setup_seconds,"cmake":check_output("cmake","--version"),
           "ctest":check_output("ctest","--version"),"sdk_cache_hit":os.environ.get("T1678_SDK_CACHE_HIT","")})

    def phase(name, command):
        started=time.time()
        start=time.monotonic()
        require(start<deadline,f"no time left for {name}")
        with (evidence/f"{name}.stdout").open("wb") as out, (evidence/f"{name}.stderr").open("wb") as err:
            result=subprocess.run(command,cwd=ROOT,stdout=out,stderr=err,timeout=deadline-start)
        elapsed=time.monotonic()-start
        record(evidence/f"{name}-timing.json",{"started_epoch":started,"wall_seconds":elapsed,"exit":result.returncode})
        require(result.returncode==0,f"{name} exited {result.returncode}")
        print(f"{name} completed in {elapsed:.2f}s",flush=True)

    config=["cmake","-S",".","-B",str(build),"-DCMAKE_BUILD_TYPE=Debug","-DNEWLANG_SANITIZER=address"]
    phase("configure",[*config,f"-DCMAKE_PROJECT_INCLUDE={ROOT/HOOK}"])
    phase("build",["cmake","--build",str(build),"--parallel",str(nproc)])
    base_build=stable_build(build)
    ctest_json=["ctest","--test-dir",str(build),"--show-only=json-v1"]
    phase("baseline-ctest",ctest_json)
    phase("overlay",[*config,f"-DCMAKE_PROJECT_INCLUDE={HERE/'overlay.cmake'}"])
    phase("overlay-ctest",ctest_json)
    require(base_build==stable_build(build),"binary/tool/flags changed in configure-only overlay")
    marker="-- T1678: cleared original RUN_SERIAL for the approved pair"
    original_hook=[line for line in (evidence/"configure.stdout").read_text().splitlines() if line.startswith(("-- T991:","-- T993:"))]
    overlay_hook=[line for line in (evidence/"overlay.stdout").read_text().splitlines() if line.startswith(("-- T991:","-- T993:","-- T1678:"))]
    require(len(original_hook)==2 and overlay_hook==[*original_hook,marker],"original hook output/order drift")
    names=(HERE/"expected-names.txt").read_text().splitlines()
    wanted=set(names)
    require(names==sorted(wanted),"invalid expected-name manifest")
    baseline=inventory(evidence/"baseline-ctest.stdout")
    overlay=inventory(evidence/"overlay-ctest.stdout")
    light=test_partition(baseline,overlay,wanted)
    regex="^("+"|".join(re.escape(name) for name in sorted(HEAVY))+")$"
    filters={"Heavy":["-R",regex],"Light":["-E",regex]}
    for label,subset in (("Heavy",HEAVY),("Light",light)):
        phase(f"selection-{label.lower()}",[*ctest_json,"--no-tests=error",*filters[label]])
        found=inventory(evidence/f"selection-{label.lower()}.stdout")
        require(found=={name:overlay[name] for name in subset},f"{label} actual selection differs")
        (evidence/f"names-{label.lower()}.txt").write_text("".join(name+"\n" for name in sorted(subset)))
    record(evidence/"inventory-audit.json",{
        "test_count":len(wanted),"heavy_count":len(HEAVY),"light_count":len(light),
        "serial_changes":sorted(HEAVY),"commands_and_remaining_properties_match":True,
        "dependency_edges":sorted(EXPECTED_EDGES),"sealed_build":base_build,
    })
    group=lane.split("-")[0]
    require(time.monotonic()<deadline,"job setup consumed the CTest evidence budget")
    print(f"Running {lane} with {workers} CTest workers",flush=True)
    result=run_cases(evidence,build,filters[group],workers,deadline)
    selected=HEAVY if group=="Heavy" else light
    max_running=check_outcomes(evidence/"test-junit.xml",evidence/"ctest.stdout",selected)
    require(max_running==workers,"CTest did not observe the requested concurrency")
    if group=="Heavy":
        for name in sorted(HEAVY):
            family=name.split("_",1)[0]
            raw=build/f"{family}-named-oom"
            output,=raw.glob("named-*")
            require(output.is_dir() and not list(output.glob("*.tmp*")),f"{family} output collision")
            for path in output.glob("*.stderr"):
                require(not SANITIZER.search(path.read_bytes()),f"{family} sanitizer finding")
    require(base_build==stable_build(build),"binary/tool/flags changed during CTest")
    record(evidence/"result.json",{"lane":lane,"passed":len(selected),"skipped":0,
           "configured_workers":workers,"observed_ctest_concurrency":max_running,
           "ctest_seconds":result["wall_seconds"],"recorded_job_seconds":time.monotonic()-job_clock})
    print(f"{lane}: {len(selected)} PASS in {result['wall_seconds']:.3f}s, observed concurrency {max_running}")

if __name__=="__main__":
    evidence=Path(os.environ["RUNNER_TEMP"])/"t1678-evidence"
    require(len(sys.argv)==2 and sys.argv[1] in ("checkout","run"),"expected checkout or run")
    (verify_checkout if sys.argv[1]=="checkout" else run)(evidence)
