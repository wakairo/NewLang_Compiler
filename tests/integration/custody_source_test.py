"""Complete §18.1c source admission and post-consume destructive controls.

Semantic evidence is independent from the separate native observer. The companion
C test destroys input source/AST before inspecting owned per-world evidence.
"""
from pathlib import Path
import hashlib
import subprocess
import sys
import tempfile

compiler, fixture, evidence = sys.argv[1:]
s=Path(fixture).read_text()
assert hashlib.sha256(s.encode()).hexdigest() == "a30bbc37ff817020165d44a1436cb98ac934568f557068b378bf3118227d5644"
start=s.index('                    let recovered =')
prefix=s[:start];suffix=s[start:]
startpolicy=s.index('                    match admit_flag {')
endpolicy=s.index('                    let recovered =')
policy=s[startpolicy:endpolicy]
final='''                    match custody {
                        None => { unit },
                    };'''
terminal='''                            receive_and_release_tail(
                                owned_ptr, owned_allocation, owned_domain);'''
cases={}
def change(name,old,new,tail=True):
    part=suffix if tail else s
    assert part.count(old)==1,(name,part.count(old))
    cases[name]=(prefix+part.replace(old,new,1)) if tail else part.replace(old,new,1)
change('no-final-none',final,'unit;')
change('wrong-final-none',final,'match Option<LiveTail>::None {None=>{unit}};')
change('none-before-extraction','                    let recovered =',final+'\n                    let recovered =')
change('no-recovery','''                    match recovered {
                        None => { unit },
                        Some(saved) => {
                            let LiveTail {
                                owned_ptr, owned_allocation, owned_domain
                            } = saved;
                            receive_and_release_tail(
                                owned_ptr, owned_allocation, owned_domain);
                            unit
                        },
                    };''','unit;')
change('no-some-arm','''                        Some(saved) => {
                            let LiveTail {
                                owned_ptr, owned_allocation, owned_domain
                            } = saved;
                            receive_and_release_tail(
                                owned_ptr, owned_allocation, owned_domain);
                            unit
                        },''','')
change('no-none-arm','match recovered {\n                        None => { unit },','match recovered {')
change('some-wildcard','Some(saved)','Some(_)')
change('no-terminal-release',terminal,'unit;')
change('wrong-terminal-root',terminal,'receive_and_release_tail(ptr_h, owned_allocation, owned_domain);')
change('wrong-terminal-backing',terminal,'receive_and_release_tail(owned_ptr, allocation_h, owned_domain);')
change('wrong-terminal-domain',terminal,'receive_and_release_tail(owned_ptr, owned_allocation, life_h);')
change('late-original-donor','                    let recovered =','                    packet;\n                    let recovered =')
change('double-release',terminal,terminal+'\n'+terminal)
change('double-recovery','''                        replace(sink, Option<LiveTail>::None)''','''                        let previous=replace(sink, Option<LiveTail>::None);
                        replace(sink, Option<LiveTail>::None)''')
change('recovery-read-mode','loan_write(custody)','loan_read(custody)')
change('recovery-alias','''                        replace(sink, Option<LiveTail>::None)''','''                        let alias=sink;
                        replace(sink, Option<LiveTail>::None)''')
change('recovery-extra-scope','''                        replace(sink, Option<LiveTail>::None)''','''                        loan_read(life_h){|extra|replace(sink, Option<LiveTail>::None)}''')
change('missing-head-release','''                    deallocate(allocation_h, full_h);''','unit;')
change('duplicate-packet-after-adopt','''                                recipient_adopt(sink, packet)''','''                                recipient_adopt(sink, packet); packet''',False)
# Distinct conditional frame shapes must not be forced to unchanged or a chosen arm.
newpolicy=policy.replace('''                            loan_write(custody) { |sink|
                                recipient_adopt(sink, packet)
                            };''','unit;')
cases['retained-versus-closed-policy']=s.replace(policy,newpolicy)
newpolicy=policy.replace(terminal,'unit;')
assert newpolicy!=policy
cases['adopted-versus-live-dropped-policy']=s.replace(policy,newpolicy)
# Known-exact None must follow expired extraction scope.
new='''                        let previous=replace(sink, Option<LiveTail>::None);
                        match custody {None=>{unit}};
                        previous'''
change('final-none-in-loan','''                        replace(sink, Option<LiveTail>::None)''',new)
change('stale-tail-reloan-after-terminal',terminal,terminal+'''\n                            loan_read(owned_domain){|stale|ref_from_ptr(read,owned_ptr,stale);unit};''')
change('recovery-result-not-unit',terminal+'\n                            unit',terminal+'\n                            u8(7)')
# Recipient admission cannot accept a local Some carrying the original owner.
old='let custody = Option<LiveTail>::None;'
change('preexisting-some-sink',old,'let custody = Option<LiveTail>::Some(packet);',False)

expected = {
    "adopted-versus-live-dropped-policy": "P5-SCOPE-OBLIGATION",
    "double-recovery": "CUSTODY-EXTRACTION-PROOF",
    "double-release": "P3-USE-AFTER-CONSUME",
    "duplicate-packet-after-adopt": "P3-USE-AFTER-CONSUME",
    "final-none-in-loan": "P3-REF-CONFLICT",
    "late-original-donor": "P3-USE-AFTER-CONSUME",
    "missing-head-release": "P5-SCOPE-OBLIGATION",
    "no-final-none": "P5-SCOPE-OBLIGATION",
    "no-none-arm": "P6-EXHAUSTIVENESS",
    "no-recovery": "P6-EXHAUSTIVENESS",
    "no-some-arm": "P6-EXHAUSTIVENESS",
    "no-terminal-release": "P5-SCOPE-OBLIGATION",
    "none-before-extraction": "P6-EXHAUSTIVENESS",
    "preexisting-some-sink": "CUSTODY-CONSTRUCTION-PRECISION",
    "recovery-alias": "CUSTODY-EXTRACTION-ALIAS",
    "recovery-extra-scope": "CUSTODY-EXTRACTION-ALIAS",
    "recovery-read-mode": "P3-TYPE-MISMATCH",
    "retained-versus-closed-policy": "CUSTODY-CONDITIONAL-CONTINUATION",
    "some-wildcard": "P6-PAYLOAD-DISCARD",
    "stale-tail-reloan-after-terminal": "P3-USE-AFTER-CONSUME",
    "wrong-final-none": "P6-EXHAUSTIVENESS",
    "wrong-terminal-backing": "P193-CALL-BACKING",
    "wrong-terminal-domain": "P193-CALL-DOMAIN",
    "wrong-terminal-root": "P193-CALL-DOMAIN",
    "recovery-result-not-unit": "CUSTODY-RECOVERY-BODY",
}
assert cases.keys() == expected.keys()
positives = {
    "primary": s,
    "refusal": s.replace("let admit_flag = Option<ptr<Node>>::None;",
                         "let admit_flag = Option<ptr<Node>>::Some(ptr_h);"),
    "recipient-renamed": s.replace("recipient_adopt", "retain_tail"),
    "all-local-renamed": s.replace("Node", "Cell").replace("packet", "owner")
        .replace("custody", "retained").replace("sink", "slot"),
    "refusal-renamed": s.replace("let admit_flag = Option<ptr<Node>>::None;",
                         "let admit_flag = Option<ptr<Node>>::Some(ptr_h);")
        .replace("packet", "owner").replace("custody", "retained"),
    "main-first": s[s.index("fn main()"):]+s[:s.index("fn main()")],
}
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    def invoke(name,text,code):
        path=root/(name+".nl")
        path.write_text(text)
        runs=[subprocess.run([compiler,str(path)],capture_output=True) for _ in range(2)]
        for run in runs:
            if code == "native-supported":
                assert run.returncode == 0 and run.stdout and not run.stderr, (name,run.stderr)
                continue
            assert run.returncode == (4 if code == "V1-BACKEND-UNSUPPORTED" else 3), (name,run.returncode,run.stderr)
            assert not run.stdout and code.encode() in run.stderr, (name,run.stderr)
        assert runs[0].stderr == runs[1].stderr, name
        assert all(p.suffix == ".nl" for p in root.iterdir()), "emitted artifact on failure/unsupported"
        return path
    for name,text in positives.items():
        path=invoke(name,text,"native-supported")
        subprocess.run([evidence,"evidence",str(path)],check=True)
    for name,text in cases.items():
        path=invoke(name,text,expected[name])
        subprocess.run([evidence,"reject",str(path)],check=True)
    # This fails after actual adoption in the later Some-world receiver, not
    # at the old pre-consume applicability guard. OOM must roll back all of it.
    path=root/"wrong-terminal-backing.nl"
    subprocess.run([evidence,"reject-oom",str(path)],check=True)
print(f"{len(positives)} complete source admissions, both alternatives owned; "
      f"{len(cases)} source negatives + late post-transfer/OOM rollback; "
      "source rejection remains before C emission; native evidence tested separately")
