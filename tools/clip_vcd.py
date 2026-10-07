#!/usr/bin/env python3
"""Extract a time window from a real scalar VCD, preserving signal transitions."""
import argparse
import hashlib
import json
from pathlib import Path

def clip(source, destination, begin, end):
    raw=Path(source).read_bytes()
    lines=raw.decode().splitlines()
    end_header=next(i for i,line in enumerate(lines) if "$enddefinitions" in line)+1
    ids=[line.split()[3] for line in lines[:end_header] if line.startswith("$var")]
    before={signal:"x" for signal in ids}
    groups=[]
    time=0
    values=[]
    for line in lines[end_header:]:
        if line.startswith("#"):
            next_time=int(line[1:])
            if next_time!=time:
                if values: groups.append((time,values))
                time=next_time
                values=[]
        elif line[:1] in ("0","1") and line[1:] in before:
            values.append(line)
    if values: groups.append((time,values))
    for time,values in groups:
        if time>=begin: break
        for value in values: before[value[1:]]=value[0]
    metadata={"source_sha256":hashlib.sha256(raw).hexdigest(),
              "source_begin_ns":begin,"source_end_ns":end,
              "operation":"crop and subtract source_begin_ns; original transitions unchanged"}
    output=lines[:end_header-1]+["$comment "+json.dumps(metadata)+" $end",
                               lines[end_header-1],"#0"]
    output += [value+signal for signal,value in before.items()]
    for time,values in groups:
        if begin<=time<=end:
            output.append("#"+str(time-begin))
            output+=values
    Path(destination).write_text("\n".join(output)+"\n")
    return metadata

if __name__=="__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("source");parser.add_argument("destination")
    parser.add_argument("begin",type=float);parser.add_argument("end",type=float)
    args=parser.parse_args()
    assert 0<=args.begin<args.end
    metadata=clip(args.source,args.destination,int(args.begin*1e9),int(args.end*1e9))
    print(json.dumps(metadata))
