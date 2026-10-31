#!/usr/bin/env python3
"""Hash the exact firmware input set without timestamps or generated outputs."""
from pathlib import Path
import hashlib, subprocess, json
root=Path(__file__).resolve().parents[1]
files=sorted(p for p in (root/'firmware').rglob('*') if p.is_file()
             and 'build' not in p.parts and p.name!='build_id.h')
digest=hashlib.sha256()
manifest={}
for p in files:
    name=str(p.relative_to(root)); data=p.read_bytes()
    manifest[name]=hashlib.sha256(data).hexdigest()
    digest.update(name.encode()+b'\0'+data+b'\0')
identity=digest.hexdigest()
head=subprocess.check_output(['git','-C',str(root),'rev-parse','--short=12','HEAD'],text=True).strip()
if subprocess.check_output(['git','-C',str(root),'status','--porcelain'],text=True):head+='-dirty'
header=f'#define MG_BUILD_ID "{identity}"\n#define MG_GIT_ID "{head}"\n'
p=root/'firmware/Platform/build_id.h'
if not p.exists() or p.read_text()!=header:p.write_text(header)
out=root/'docs/metrics';out.mkdir(parents=True,exist_ok=True)
(out/'source-manifest.json').write_text(json.dumps({'source_sha256':identity,'git':head,'files':manifest},indent=2)+'\n')
print(identity)
