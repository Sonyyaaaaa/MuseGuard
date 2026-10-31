#!/usr/bin/env python3
"""Plot real scalar probe transitions, preserving their captured timing."""
import argparse,bisect,hashlib
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from analyze_vcd import load,pulses,uart
import re

def trace(ax,events,name,start,end,offset):
    times=[t for t,v in events[name]]
    index=bisect.bisect_right(times,start)-1
    state=events[name][index][1] if index>=0 else 0
    segment=[(start,state)]+[(t,v) for t,v in events[name] if start<t<end]
    segment.append((end,segment[-1][1]))
    ax.step([(t-start)/1e6 for t,v in segment],[offset+v*.6 for t,v in segment],where='post',lw=1,color='#172b4d')

def main():
    p=argparse.ArgumentParser();p.add_argument('vcd');p.add_argument('output')
    p.add_argument('--from-s',type=float,default=0);p.add_argument('--to-s',type=float)
    p.add_argument('--latency',action='store_true');a=p.parse_args()
    e=load(a.vcd);rises=[t for t,v in e['SAMPLE'] if v and t>=a.from_s*1e9 and (a.to_s is None or t<=a.to_s*1e9)]
    if a.latency:
        physical=[t for t,v in e['PIR'] if v and t>=a.from_s*1e9]
        text,lines=uart(e)
        ends=[line['start_ns'] for line in lines if (m:=re.search(r'^EVENT .* bits=(\d+)',line['text'])) and int(m.group(1))&1]
        pairs=[]
        for t in physical:
            i=bisect.bisect_left(ends,t)
            if i<len(ends):pairs.append((t,ends[i]))
        left,right=max(pairs,key=lambda pair:pair[1]-pair[0]);start=left-2e6;end=right+8e6
        data=[(b-a)/1e6 for a,b in pairs]
        labels=['PIR','SAMPLE','FLASH','COMMAND','UART_TX'];metric='PIR → початок UART EVENT, мс'
        title=f'Найбільша затримка PIR → UART EVENT: {(right-left)/1e6:.3f} мс'
    else:
        left,right=max(zip(rises,rises[1:]),key=lambda pair:pair[1]-pair[0]);start=left-3e6;end=right+8e6
        data=[(b-a)/1e6 for a,b in zip(rises,rises[1:])]
        labels=['SAMPLE','UART_TX'];metric='Період SAMPLE, мс'
        title=f'Найбільший період SAMPLE: {(right-left)/1e6:.3f} мс'
    plt.rcParams.update({'font.family':'DejaVu Sans','font.size':10})
    fig,(ax,hist)=plt.subplots(2,1,figsize=(8,5.4),gridspec_kw={'height_ratios':[1.3,1]},layout='constrained')
    for i,name in enumerate(reversed(labels)):trace(ax,e,name,start,end,i)
    ax.set_yticks([i+.3 for i in range(len(labels))],list(reversed(labels)))
    ax.set_xlabel(f'Час від початку фрагмента, мс; t₀={start/1e9:.6f} с')
    ax.set_ylim(-.2,len(labels)+.6);ax.set_title(title,loc='left')
    ax.axvline((left-start)/1e6,color='#a22',ls='--',lw=.8)
    ax.axvline((right-start)/1e6,color='#a22',ls='--',lw=.8)
    ax.annotate(f'{(right-left)/1e6:.3f} мс',xy=((left-start)/1e6,len(labels)+.12),xytext=((right-start)/1e6,len(labels)+.12),arrowprops={'arrowstyle':'<->','color':'#a22'},ha='center',va='bottom',color='#a22')
    hist.hist(data,bins=35,color='#44617d',edgecolor='white')
    hist.set_xlabel(metric);hist.set_ylabel('Кількість')
    hist.set_title(f'n={len(data)}, min={min(data):.3f}, avg={sum(data)/len(data):.3f}, max={max(data):.3f} мс',loc='left',fontsize=10)
    raw=hashlib.sha256(Path(a.vcd).read_bytes()).hexdigest()
    fig.suptitle(f'Wokwi, власний VCD; SHA-256 {raw[:16]}…',fontsize=10)
    fig.savefig(a.output,dpi=190);plt.close(fig)
if __name__=='__main__':main()
