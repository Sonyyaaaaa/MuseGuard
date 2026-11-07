#!/usr/bin/env python3
"""Identify the full RTOS image inputs, including the fixed kernel."""
from pathlib import Path
import hashlib, json, subprocess
root=Path(__file__).resolve().parents[1]
files=[]
for part in ('firmware','lab4','pilot/freertos'):
    files += [p for p in (root/part).rglob('*') if p.is_file() and 'build' not in p.relative_to(root).parts and p.name != 'build_id.h' and p.suffix not in ('.pyc',)]
files.append(root/'Makefile')
files.sort()
h=hashlib.sha256(); hashes={}
for p in files:
    name=p.relative_to(root).as_posix(); b=p.read_bytes(); hashes[name]=hashlib.sha256(b).hexdigest()
    h.update(name.encode()+b'\0'+b+b'\0')
identity=h.hexdigest()
revision=subprocess.check_output(['git','-C',str(root),'rev-parse','--short=12','HEAD'],text=True).strip()
if subprocess.check_output(['git','-C',str(root),'status','--porcelain'],text=True): revision+='-dirty'
out=root/'firmware/build/lab4'; out.mkdir(parents=True,exist_ok=True)
header=f'#define MG_BUILD_ID "{identity}"\n#define MG_GIT_ID "{revision}"\n'
if not (out/'lab4_build_id.h').exists() or (out/'lab4_build_id.h').read_text()!=header:
    (out/'lab4_build_id.h').write_text(header)
(out/'source-manifest.json').write_text(json.dumps({'source_sha256':identity,'git':revision,'files':hashes},indent=2)+'\n')
print(identity)
