#!/usr/bin/env python3
"""Summarize this project's raw Wokwi probes and MCU-stamped UART, separately."""
import argparse,bisect,hashlib,json,re,statistics
from pathlib import Path
from analyze_vcd import load,pulses,uart

def summary(values):
    values=sorted(values)
    return {'n':len(values),'min':min(values),'avg':statistics.mean(values),
            'max':max(values),'p99':values[min(len(values)-1,int(.99*len(values)))]} if values else {'n':0}

def inspect(path,begin_s=0,end_s=None):
    events=load(path); text,lines=uart(events)
    result={'vcd_sha256':hashlib.sha256(Path(path).read_bytes()).hexdigest(),
            'timescale':'1ns','origin':'actual Wokwi logic analyzer, not synthesized',
            'last_transition_s':max(t for channel in events.values() for t,_ in channel)/1e9}
    bound=end_s if end_s is not None else result['last_transition_s']
    intervals=[]
    for name in ('SAMPLE','FLASH','COMMAND'):
        opened=None
        for t,value in events[name]:
            if value:opened=t
            elif opened is not None:intervals.append((opened,t));opened=None
    # UART is synchronous in this firmware. This is a probe/serial activity
    # lower bound, not an independently measured core idle/sleep percentage.
    intervals += [(line['start_ns'],line['end_ns']+86875) for line in lines]
    clipped=sorted((max(a,begin_s*1e9),min(b,bound*1e9)) for a,b in intervals
                   if b>begin_s*1e9 and a<bound*1e9)
    total=0;left=right=None
    for a,b in clipped:
        if right is not None and a<=right:right=max(right,b)
        else:
            if right is not None:total+=right-left
            left,right=a,b
    if right is not None:total+=right-left
    result['probe_serial_activity_permille']=total/((bound-begin_s)*1e9)*1000
    result['analysis_window_s']=[begin_s,end_s]
    if begin_s or end_s is not None:
        events={name:[(t,v) for t,v in values if t>=begin_s*1e9 and (end_s is None or t<=end_s*1e9)] for name,values in events.items()}
    for name in ('SAMPLE','FLASH','COMMAND','ADC_MARK'):
        p=pulses(events,name)
        result[name]={'width_us':summary([v/1000 for v in p['widths_ns']]),
                      'period_us':summary([(b-a)/1000 for a,b in zip(p['rises_ns'],p['rises_ns'][1:])])}
    falling=[t for t,value in events['SAMPLE'] if not value]
    rising=[t for t,value in events['SAMPLE'] if value]
    latencies=[];notifications=[]
    presence_lines=[line for line in lines if (m:=re.search(r'^EVENT .* bits=(\d+)',line['text'])) and int(m.group(1))&1]
    notification_times=[line['start_ns'] for line in presence_lines]
    for t,value in events['PIR']:
        if value:
            pos=bisect.bisect_right(rising,t)
            if pos<len(rising):
                stop=bisect.bisect_right(falling,rising[pos])
                if stop<len(falling):latencies.append((falling[stop]-t)/1000)
            pos=bisect.bisect_left(notification_times,t)
            if pos<len(notification_times):notifications.append((notification_times[pos]-t)/1000)
    result['pir_to_next_sample_end_bound_us']=summary(latencies)
    result['pir_to_uart_event_us']=summary(notifications)
    # Burst membership uses RX timestamps to group the 64 n commands, then
    # physical UART line times to delimit their actual foreground drain.
    groups=[]
    for line in lines:
        m=re.search(r'^CMD .* rx_us=(\d+) char=n ',line['text'])
        if not m:continue
        rx=int(m.group(1))
        if not groups or rx-groups[-1]['last_rx_us']>50000:
            groups.append({'start_ns':line['start_ns'],'end_ns':line['end_ns'],'last_rx_us':rx,'commands':0})
        groups[-1].update(end_ns=line['end_ns'],last_rx_us=rx,commands=groups[-1]['commands']+1)
    context=[]
    for t,value in events['PIR']:
        if not value:continue
        row={'pir_s':t/1e9,'during_n_burst_drain':any(g['start_ns']<=t<=g['end_ns'] and g['commands']==64 for g in groups)}
        for name in ('COMMAND','FLASH','SAMPLE'):
            transitions=events[name];idx=bisect.bisect_right([x[0] for x in transitions],t)-1
            row[name.lower()+'_high']=bool(idx>=0 and transitions[idx][1])
        row['uart_line_active']=any(x['start_ns']<=t<=x['end_ns']+86875 for x in lines)
        context.append(row)
    result['pir_context']=context
    result['n_bursts']=[{'start_s':g['start_ns']/1e9,'end_s':g['end_ns']/1e9,'commands':g['commands']} for g in groups]
    widths=pulses(events,'FLASH')['widths_ns']
    reported=[int(m.group(1)) for line in lines if line['start_ns']>=begin_s*1e9 and (end_s is None or line['start_ns']<=end_s*1e9) and (m:=re.search(r'^EVENT .* flash_us=(\d+)',line['text']))]
    result['flash_comparison']=[{'gpio_us':w/1000,'firmware_us':v,'difference_us':v-w/1000}
                                 for w,v in zip(widths,reported)]
    result['firmware_stats']=[line['text'] for line in lines if line['text'].startswith(('BOOT','STAT','RESOURCE','CADENCE'))]
    return result,text

def main():
    p=argparse.ArgumentParser();p.add_argument('vcd');p.add_argument('--output',required=True)
    p.add_argument('--from-s',type=float,default=0);p.add_argument('--to-s',type=float);a=p.parse_args()
    result,text=inspect(a.vcd,a.from_s,a.to_s);out=Path(a.output);out.parent.mkdir(parents=True,exist_ok=True)
    out.with_suffix('.json').write_text(json.dumps(result,indent=2)+'\n')
    out.with_suffix('.uart.txt').write_text(text)
    print(json.dumps(result,indent=2))
if __name__=='__main__':main()
