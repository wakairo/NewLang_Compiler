"""Frozen expectations + public source evidence; passive original C carrier reads.
No private checker state, generated-C-derived oracle, or guest state writes.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def run(args):
    return subprocess.run(args, capture_output=True, text=True)


def observer(e, roles):
    # All values frozen before execution; binding IDs from public checked nodes.
    constants = dict(roles, initial=e['initial_peer_tag'], post=e['post_tag'],
                     variant=e['runtime_variant'])
    header = r'''
#include <stdio.h>
#include <stdlib.h>
static const nl_node *anchor, *peer;
static const nl_node *const *saved_ptr;
static const nl_node_option *observed, *joined;
static size_t step;
static void require(int x) {
    if (!x) { fputs("P466 passive observation failed\n", stderr); abort(); }
}
static void option(const nl_node_option *o, unsigned tag) {
    require(o && o->tag == tag && o->ptr == (tag ? anchor : NULL));
}
static void bind(size_t frame, size_t symbol, size_t kind, const void *p) {
    require(p != NULL);
    if (step == 0) {
        require(frame == 0 && symbol == @anchor@ && kind == 1);
        anchor = p; require(anchor->f1 == 29); option(&anchor->f0, 0);
        printf("anchor=%p payload=%u\n", (const void *)anchor, (unsigned)anchor->f1);
    } else if (step == 1) {
        require(frame == 0 && symbol == @pointer@ && kind == 2);
        saved_ptr = p; require(*saved_ptr == anchor);
    } else if (step == 2) {
        require(frame == 0 && symbol == @peer@ && kind == 1);
        peer = p; require(peer != anchor && peer->f1 == 61);
        option(&peer->f0, @initial@);
        printf("peer=%p initial=%u/%p payload=%u distinct=1\n", (const void *)peer,
               peer->f0.tag, (const void *)peer->f0.ptr, (unsigned)peer->f1);
    } else if (step == 3) {
        require(frame == 0 && symbol == @observed@ && kind == 3);
        observed = p; require(observed != &peer->f0 && observed != &anchor->f0);
        option(observed, @initial@);
    } else if (step == 6) {
        require(frame != 0 && symbol == @old@ && kind == 3);
        const nl_node_option *old = p;
        require(old != &peer->f0 && old != observed); option(old, @initial@);
        /* Branch-local carrier is inspected now, never retained beyond scope. */
    } else if (step == 7) {
        require(frame == 0 && symbol == @joined@ && kind == 3);
        joined = p; require(joined != &peer->f0 && joined != observed);
        option(joined, @post@); option(observed, @initial@);
    } else require(0);
    ++step;
}
static void arm(size_t variant, const nl_node *pointer) {
    require(step++ == 4 && variant == @variant@ && pointer == (@initial@ ? anchor : NULL));
    printf("branch=%zu pointer=%p\n", variant, (const void *)pointer);
}
static void replaced(const nl_node *root, const nl_node_option *old) {
    require(step++ == 5 && root == peer && old != &peer->f0);
    option(old, @initial@); option(&peer->f0, @post@); option(observed, @initial@);
    require(anchor->f1 == 29 && peer->f1 == 61);
    printf("old=%u/%p new=%u/%p sibling=%u\n", old->tag, (const void *)old->ptr,
           peer->f0.tag, (const void *)peer->f0.ptr, (unsigned)peer->f1);
}
static void reloan(const nl_node *p) {
    require(step++ == 8 && p == anchor && p == *saved_ptr && p->f1 == 29);
    printf("reloan=%p same_original=1\n", (const void *)p);
}
static void finish(void) {
    require(step == 9 && peer != anchor && anchor->f1 == 29 && peer->f1 == 61);
    option(&anchor->f0, 0); option(&peer->f0, @post@);
    option(observed, @initial@); option(joined, @post@); require(*saved_ptr == anchor);
    puts("P466 passive observations complete");
}
#define NL_NODE_BIND(f,s,k,p) bind(f,s,k,p)
#define NL_NODE_ARM(v,p) arm(v,p)
#define NL_NODE_REPLACE(r,o) replaced(r,o)
#define NL_NODE_RELOAN(p) reloan(p)
#define NL_NODE_FINISH() finish()
'''
    for key, value in constants.items():
        header = header.replace('@' + key + '@', str(value))
    assert not re.search(r'@[a-z]+@', header)
    return header


def roles_from_public(text, expected):
    def fields(line):
        return dict((key, int(value)) for key, value in re.findall(r'(\w+)=(\d+)', line))
    roots = [fields(x) for x in text.splitlines() if x.startswith('ROOT ')]
    pointers = [fields(x) for x in text.splitlines() if x.startswith('POINTER ')]
    reads = [fields(x) for x in text.splitlines() if x.startswith('READ ')]
    old = [fields(x) for x in text.splitlines() if x.startswith('OLD ')]
    assert len(roots) == 2 and len(pointers) == 1 and len(reads) == 2 and len(old) == 2
    assert roots[1]['variant'] == expected['runtime_variant']
    assert 'PUBLIC VALID;' in text
    return dict(anchor=roots[0]['symbol'], peer=roots[1]['symbol'], pointer=pointers[0]['symbol'],
                observed=reads[0]['symbol'], joined=reads[1]['symbol'],
                old=next(x['symbol'] for x in old if x['variant'] == expected['runtime_variant']))


def main():
    compiler, probe, cc, folder, sanitizer = sys.argv[1:]
    folder = Path(folder)
    frozen = json.loads((folder / 'frozen.json').read_text())
    records = []
    with tempfile.TemporaryDirectory(prefix='newlang-p466-') as d:
        root = Path(d)
        for name, expected in frozen['cases'].items():
            source = folder / (name + '.nl')
            sha = hashlib.sha256(source.read_bytes()).hexdigest()
            assert sha == expected['sha256']
            checked = run([compiler, '--check', str(source)])
            assert (checked.returncode, checked.stdout, checked.stderr) == (0, '', ''), checked
            proof = run([probe, str(source)])
            assert proof.returncode == 0 and not proof.stderr, proof
            roles = roles_from_public(proof.stdout, expected)
            emitted = run([compiler, str(source)])
            assert emitted.returncode == 0 and not emitted.stderr and emitted.stdout, emitted
            c = root / (name + '.c')
            c.write_text(emitted.stdout)
            header = root / (name + '.h')
            header.write_text(observer(expected, roles))
            runs = []
            for opt in ('-O0', '-O2'):
                for observed in (False, True):
                    exe = root / f'{name}{opt}{observed}'
                    cmd = [cc, '-std=c17', '-Wall', '-Wextra', '-Wpedantic', '-Werror', opt]
                    if observed:
                        cmd += [f'-DNEWLANG_NODE_OBSERVER="{header}"']
                    if sanitizer != 'none':
                        cmd += [f'-fsanitize={sanitizer}', '-fno-sanitize-recover=all',
                                '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
                    built = run(cmd + [str(c), '-o', str(exe)])
                    assert built.returncode == 0, built
                    native = run([str(exe)])
                    assert native.returncode == 0 and not native.stderr, native
                    assert ('P466 passive observations complete' in native.stdout) if observed else not native.stdout
                    runs.append(dict(optimization=opt, observed=observed, sanitizer=sanitizer,
                                     compile_exit=built.returncode, native_exit=native.returncode,
                                     stdout=native.stdout, stderr=native.stderr))
            records.append(dict(source=name, source_sha256=sha, check_exit=checked.returncode,
                                public_exit=proof.returncode, public_owned=proof.stdout,
                                backend_exit=emitted.returncode, C_bytes=len(emitted.stdout.encode()),
                                C_sha256=hashlib.sha256(emitted.stdout.encode()).hexdigest(), runs=runs))
    print(json.dumps(dict(compiler=frozen['compiler'], host=cc, sanitizer=sanitizer, cases=records), indent=2))


if __name__ == '__main__':
    main()
