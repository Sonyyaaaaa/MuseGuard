#!/usr/bin/env python3
"""Measure the external phase20 input from real Wokwi VCD, never from host waits."""
import argparse,bisect,hashlib,json,re,statistics
from pathlib import Path
from analyze_vcd import load,uart

def stats(values):
    return {'n':len(values),'min':min(values),'avg':statistics.mean(values),'max':max(values)} if values else {'n':0}

def analyze(path):
    events=load(path);_,lines=uart(events)
    pulses=[];opened=None
    for t,value in events['PIR']:
        if value:opened=t
        elif opened is not None:
            pulses.append({'rise_ns':opened,'fall_ns':t,'width_us':(t-opened)/1000});opened=None
    presence=[x for x in lines if (m:=re.match(r'EVENT .* bits=(\d+)',x['text'])) and int(m[1])&1]
    sample_rises=[t for t,v in events['SAMPLE'] if v]
    anchor=next((t for t in sample_rises if t>=200000000),sample_rises[0])
    groups=[]
    for p in pulses:
        if not groups or p['rise_ns']-groups[-1][-1]['rise_ns']>1000000000:groups.append([])
        groups[-1].append(p)
    results=[]
    all_rises=[p['rise_ns'] for p in pulses]
    for group in groups:
        bins=[0]*20;latencies=[];missed=[];duplicate=[];late=[];during=0
        for p in group:
            phase=(p['rise_ns']-anchor)%20000000
            bins[int(phase//1000000)]+=1
            pos=bisect.bisect_right(all_rises,p['rise_ns'])
            end=min(p['rise_ns']+200000000,all_rises[pos] if pos<len(all_rises) else p['rise_ns']+200000000)
            matches=[x for x in presence if p['rise_ns']<=x['start_ns']<end]
            if len(matches)==1:
                latency=(matches[0]['start_ns']-p['rise_ns'])/1000
                latencies.append(latency)
                if latency>100000:late.append({'rise_s':p['rise_ns']/1e9,'latency_us':latency})
            elif not matches:missed.append(p['rise_ns']/1e9)
            else:duplicate.append({'rise_s':p['rise_ns']/1e9,'event_count':len(matches)})
            times,values=zip(*events['COMMAND'])
            i=bisect.bisect_right(times,p['rise_ns'])-1
            during+=bool(i>=0 and values[i])
        results.append({'start_s':group[0]['rise_ns']/1e9,'end_s':group[-1]['fall_ns']/1e9,'pulse_n':len(group),'width_us':stats([p['width_us'] for p in group]),'phase_anchor_sample_s':anchor/1e9,'phase_bins_1ms':bins,'phase_bins_covered':sum(x>0 for x in bins),'command_high_at_rise':during,'pir_to_event_before_next_input_us':stats(latencies),'without_event_before_next_input':missed,'events_over_100ms':late,'duplicate_event_windows':duplicate})
    return {'source_sha256':hashlib.sha256(Path(path).read_bytes()).hexdigest(),'kind':'real captured input; phase bins use fixed20ms grid anchored at one actual SAMPLE rise, not an inferred physical oscillator phase','groups':results,'presence_lines_n':len(presence),'pending_input_rise_ns':opened,'limits':'Each event is matched within the unique interval before the next input, capped at200ms. The fixture must have20ms pulse width and200.2ms period. Missing EVENT in that interval may be loss or excessive latency; inspect later events before naming permanent loss. Exclude incomplete final200ms windows.'}

if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('vcd');ap.add_argument('--output',required=True);args=ap.parse_args()
    result=analyze(args.vcd);Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
