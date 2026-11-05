"""Offline integrity verification only. No compiler, build or guest execution."""
import base64
import gzip
import hashlib
import io
import json
from pathlib import Path
import re
import zipfile

ROOT=Path(__file__).resolve().parent


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    manifest=json.loads((ROOT/'manifest.json').read_text())
    for entry in manifest['files']:
        p=Path(entry['path']);assert not p.is_absolute() and '..' not in p.parts
        data=(ROOT/p).read_bytes()
        assert len(data)==entry['bytes'] and sha(data)==entry['sha256'], entry['path']
    jobs=json.loads((ROOT/'jobs.json').read_text())
    assert len(jobs)==15
    cap_count=0;raw_total=0;cap_total=0
    for job in jobs:
        info=job['decoded_log'];raw=gzip.decompress((ROOT/info['path']).read_bytes())
        assert len(raw)==info['uncompressed_bytes'] and sha(raw)==info['uncompressed_sha256']
        raw_total+=len(raw);chunks={};headers={};ends=set()
        for line in raw.decode('utf-8-sig').splitlines():
            marker='R1163 ARCHIVE_BEGIN '
            if marker in line:
                h=json.loads(line.split(marker,1)[1]);ident=h['id']
                assert ident not in headers or headers[ident]==h;headers[ident]=h
            m=re.search(r'R1163 ARCHIVE_CHUNK (\S+) (\d+) ([A-Za-z0-9+/=]+)',line)
            if m:
                ident,num,data=m.groups();num=int(num);slot=chunks.setdefault(ident,{})
                assert num not in slot or slot[num]==data;slot[num]=data
            m=re.search(r'R1163 ARCHIVE_END (\S+)',line)
            if m:ends.add(m.group(1))
        assert len(headers)==len(job['capsules'])
        for cap in job['capsules']:
            header=cap['header'];ident=header['id'];assert headers[ident]==header and ident in ends
            assert set(chunks[ident])==set(range(header['chunks']))
            decoded=base64.b64decode(''.join(chunks[ident][i] for i in range(header['chunks'])),validate=True)
            data=(ROOT/cap['path']).read_bytes()
            assert data==decoded and len(data)==header['bytes'] and sha(data)==header['sha256']
            cap_count+=1;cap_total+=len(data)
            member_list=json.loads((ROOT/Path(cap['path']).with_name('capsule-members.json')).read_text())
            with zipfile.ZipFile(io.BytesIO(data)) as z:
                assert z.testzip() is None and len(z.namelist())==len(set(z.namelist()))
                assert set(z.namelist())=={m['path'] for m in member_list}
                for m in member_list:
                    member=z.read(m['path']);assert len(member)==m['bytes'] and sha(member)==m['sha256']
                oracle=json.loads(z.read('oracle.json'))
                assert sha(z.read('oracle.json'))==manifest['V3_oracle_sha256']
                for case in oracle['positive']+oracle['negative']:
                    assert sha(z.read(case['id']+'.nl'))==case['sha256']
                for command in json.loads(z.read('commands.json')):
                    for stream in ('stdout','stderr'):
                        assert sha(z.read(command['name']+'.'+stream))==command[stream+'_sha256']
    assert cap_count==12 and raw_total==14800076 and cap_total==1617971
    print(json.dumps({'artifact_files_verified':len(manifest['files']),
                      'complete_decoded_logs':15,'decoded_bytes':raw_total,
                      'exact_capsules':cap_count,'capsule_bytes':cap_total,
                      'compiler_native_build_test_execution':0},indent=2))


if __name__=='__main__':
    main()
