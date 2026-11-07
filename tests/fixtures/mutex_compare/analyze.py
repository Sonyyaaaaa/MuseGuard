#!/usr/bin/env python3
"""Reproduce the paired Wokwi capture analysis; never substitutes a run."""
import json
import re
import struct
import sys
import zlib
from bisect import bisect_left
from pathlib import Path
from statistics import mean, median

root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'tools'))
from analyze_vcd import load,pulses,uart
work=Path(sys.argv[1]).resolve()
result={}
input_edges={}
inputs={}
for variant in ('wide','short'):
    e=load(work/f'compare-{variant}.vcd')
    inp=load(work/f'compare-{variant}-input.vcd')['UART_RX']
    input_edges[variant]=[x for x in inp if 0<x[0]<44_000_000_000]
    # Initial duplicated simulator zero-time levels are not UART start bits.
    inputs[variant]=uart({'UART_TX':[(0,1)]+input_edges[variant]})[0]
    text,lines=uart(e)
    (work/f'compare-{variant}.uart.txt').write_text(text)
    pir=pulses(e,'PIR')['rises_ns']
    lock=pulses(e,'STATE_LOCK')
    sample=pulses(e,'SAMPLE')['rises_ns']
    periods=[(b-a)/1e6 for a,b in zip(sample,sample[1:]) if 2e9<=a<23e9]
    event=[q for q in lines if q['text'].startswith('EVENT ') and int(re.search(r'bits=(\d+)',q['text']).group(1))&1]
    assert len(pir)==len(event)==50
    latency=[(v['start_ns']-t)/1e6 for t,v in zip(pir,event)]
    assert 0<=min(latency)<=max(latency)<400
    lock_intervals=list(zip(lock['rises_ns'],lock['widths_ns']))
    def stats(values):
        a=sorted(values)
        return {'n':len(a),'mean_ms':mean(a),'median_ms':median(a),'p95_ms':a[int(.95*(len(a)-1))], 'p99_ms':a[int(.99*(len(a)-1))], 'max_ms':max(a),'over_20ms':sum(x>20 for x in a)}
    def window(lo,hi): return [w/1e6 for t,w in lock_intervals if lo*1e9<=t<hi*1e9]
    service=[]
    for t,w in lock_intervals:
        if 2e9<=t<23e9:
            j=bisect_left(sample,t)
            if j==len(sample) or sample[j]>=t+w: service.append(w/1e6)
    low=[]; begin=None
    for t,level in e['SDA']:
        if not level and begin is None: begin=t
        elif level and begin is not None:
            if t-begin>1e9:low.append([begin,t])
            begin=None
    assert [25_000_000_000,30_000_000_000] in low
    records=re.findall(r'RECORD slot=(\d+) seq=(\d+) t_ms=(\d+) bits=(\d+) observation=(\d+) crc=([0-9A-F]{8})',text)
    for slot,seq,t,bits,obs,crc in records:
        value=zlib.crc32(struct.pack('<6I',0x4d470301,int(seq),int(t),int(bits),int(obs),0))
        assert f'{value:08X}'==crc,(slot,seq,crc)
    assert records
    result[variant]={'pir_rises_ns':pir,'uart_input_bytes':len(inputs[variant]),'uart_n':inputs[variant].count('n'),
        'lock_traffic':stats(window(2,23)), 'lock_without_sample_traffic':stats(service),
        'lock_fault':stats(window(25,30)), 'lock_recovery':stats(window(30,43)),
        'sample_periods_traffic':{'n':len(periods),'min_ms':min(periods),'max_ms':max(periods)},
        'pir_uart_latency_ms':{'n':len(latency),'min':min(latency),'mean':mean(latency),'max':max(latency)},
        'journal_crc_rows':len(records),'journal_crc_unique_sequences':len({x[1] for x in records}),
        'snapshots':[v['text'] for v in lines if v['text'].startswith(('CADENCE ','RTOS ','RESOURCE '))],
        'fault_event_lines':[v for v in lines if v['text'].startswith('EVENT ') and ('bits=16 ' in v['text'] or 'bits=32 ' in v['text']) and 24e9<v['start_ns']<35e9],
        'read_id_lines':[v for v in lines if v['text'].startswith('READ ') and 'id=104' in v['text']],
        'sda_low_intervals_ns':low}
assert inputs['wide']==inputs['short']=='z'+'n'*544+'ds'+'cds'
assert result['wide']['pir_rises_ns']==result['short']['pir_rises_ns']
a,b=input_edges['wide'],input_edges['short']
assert len(a)==len(b) and all(x[1]==y[1] and abs(x[0]-y[0])<=25 for x,y in zip(a,b))
result['input_uart_max_edge_difference_ns']=max(abs(x[0]-y[0]) for x,y in zip(a,b))
(work/'compare-analysis.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:{x:y for x,y in v.items() if x!='pir_rises_ns'} if isinstance(v,dict) else v for k,v in result.items()},indent=2))
