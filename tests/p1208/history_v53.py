"""C263 local prospective history; external original adopted-v52 anchor."""
from pathlib import Path
import sys

V52 = 'tests/p990/current-freeze-v52.json'
SHA256 = 'cce48b632f4054e3c417d77bdaffd269385c9014c4f94f405eb9f538c7e6ff9b'
OID = 'efb611ed19bc802fec72dbd652d77a3aad29d5f9'

def verify(root,current):
    root=Path(root)
    sys.path.insert(0,str(root/'tests/p1193'))
    sys.path.insert(0,str(root/'tests/support'))
    from history_v52 import verify as previous_verify
    from versioned_freeze import digest,git_oid,json_data,local,manifest_data,require
    count=previous_verify(root,current)
    raw=local(root,V52).read_bytes()
    require(digest(raw)==SHA256,'C263 fixed original-v52 SHA256')
    require(git_oid('blob',raw)==OID,'C263 fixed original-v52 Git blob')
    old=json_data(raw)
    require(old['generation']==52 and len(old['files'])==3828,'C263 fixed original-v52 coverage')
    value,_=manifest_data(root,current)
    require(value['generation']>=53 and old['files'].keys()<=value['files'].keys(),'C263 inherited original-v52 coverage')
    for name,expected in old['files'].items():
        if name.startswith('tests/') and ('/evidence/' in name or '/lineage/' in name or name.endswith(('.json','.nl'))):
            require(digest(local(root,name).read_bytes())==expected,'C263 preserved predecessor evidence: '+name)
    return count
