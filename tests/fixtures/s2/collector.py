#!/usr/bin/env python3
"""Capture one uninterrupted Wokwi S2 run. MCU HEART is the time authority."""
import json, re, struct, sys, time, zlib
from datetime import datetime, timezone
from pathlib import Path
import serial

out=Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=False)
minimum_us=int(sys.argv[2]) if len(sys.argv)>2 else 3_600_000_000
bare='--bare' in sys.argv[3:]
state={'phase':'CONNECT','started_utc':datetime.now(timezone.utc).isoformat(),'minimum_us':minimum_us,'bare':bare,'last_heart_us':None,'heart_count':0,'error':None}
def save():
    (out/'state.json.tmp').write_text(json.dumps(state,indent=2)+'\n'); (out/'state.json.tmp').replace(out/'state.json')
def fail(msg):
    state.update(phase='FAILED',error=msg,finished_utc=datetime.now(timezone.utc).isoformat()); save(); raise SystemExit(msg)
save()
try:
    port=serial.serial_for_url('rfc2217://127.0.0.1:23347',baudrate=115200,timeout=1)
    state['phase']='WAIT_BOOT'; save(); boot=0; first=None; epoch=None; last=None; final_requested=False; saw_final_rtos=bare; saw_final_cadence=False; crc_records=0; pending=b''; last_data=time.monotonic(); sent=set()
    with (out/'uart.raw').open('wb',buffering=0) as raw:
        while True:
            data=port.read(1024)
            if data:
                raw.write(data); pending += data; last_data=time.monotonic()
            elif time.monotonic()-last_data > 60: fail('No UART data for 60 wall-clock seconds')
            while b'\n' in pending:
                b,pending=pending.split(b'\n',1)
                try: line=b.rstrip(b'\r').decode('ascii')
                except UnicodeDecodeError: fail('Non-ASCII UART byte')
                if line.startswith('BOOT '):
                    boot+=1
                    if boot>1: fail('Unexpected second BOOT/reset')
                    state.update(boot=line,phase='RUNNING'); save()
                elif line.startswith('HEART '):
                    if not boot: fail('HEART without captured BOOT')
                    match=re.search(r't_us=(\d+)',line)
                    if not match: fail('HEART timestamp missing')
                    now=int(match.group(1))
                    if last is not None and (now<=last or now-last>2_500_000): fail(f'HEART timestamp discontinuity {last} -> {now}')
                    if first is None:
                        first=now; port.write(b'zs'); sent.add('start'); state['first_heart_us']=first
                    last=now; elapsed=now-epoch if epoch is not None else 0; state.update(last_heart_us=now,elapsed_us=elapsed,heart_count=state['heart_count']+1); save()
                    if epoch is None and now-first>10_000_000: fail('No confirmation of metrics reset')
                    for key,threshold,cmd in [('access',90_000_000,b'a'),('guard',160_000_000,b'g')]:
                        if elapsed>=threshold and key not in sent:
                            port.write(cmd); sent.add(key)
                    if elapsed>=minimum_us and not final_requested:
                        port.write(b'sd'); final_requested=True; state['phase']='FINALIZING'; state['final_request_heart_us']=now; save()
                    if final_requested and saw_final_rtos and saw_final_cadence and crc_records and now-state['final_request_heart_us']>=10_000_000:
                        state.update(phase='COMPLETE',finished_utc=datetime.now(timezone.utc).isoformat(),crc_records=crc_records,boot_count=boot); save(); port.close(); raise SystemExit(0)
                    if final_requested and now-state['final_request_heart_us']>40_000_000: fail('Final diagnostics or CRC journal dump missing')
                elif line.startswith('CMD ') and 'char=z ' in line:
                    if epoch is not None: fail('Unexpected second metrics reset')
                    match=re.search(r't_us=(\d+)',line)
                    if not match: fail('Metrics reset timestamp missing')
                    epoch=int(match.group(1)); state['metrics_epoch_us']=epoch; save()
                elif line.startswith('RESOURCE '):
                    for key in ('rx_drop','ore','event_drop','flash_fail','clock_ambiguous'):
                        m=re.search(r'\b'+key+r'=(\d+)',line)
                        if not m or int(m.group(1)): fail('Resource error or missing field: '+line)
                    state['last_resource']=line
                elif line.startswith('CADENCE ') and final_requested:
                    saw_final_cadence=True; state['final_cadence']=line
                elif line.startswith('RTOS ') and final_requested:
                    saw_final_rtos=True; state['final_rtos']=line
                elif line.startswith('RECORD '):
                    m=re.search(r'RECORD slot=(\d+) seq=(\d+) t_ms=(\d+) bits=(\d+) observation=(\d+) crc=([0-9A-F]{8})',line)
                    if not m: fail('Malformed journal record: '+line)
                    _,seq,t,bits,obs,crc=m.groups(); calc=zlib.crc32(struct.pack('<6I',0x4d470301,int(seq),int(t),int(bits),int(obs),0))
                    if calc != int(crc,16): fail('Bad journal CRC: '+line)
                    crc_records+=1
except SystemExit: raise
except Exception as ex: fail(type(ex).__name__+': '+str(ex))
