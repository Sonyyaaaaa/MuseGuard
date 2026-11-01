from pathlib import Path
import json

vcd = Path('startup-ready.vcd')
signals = {'!': 1, '"': 1, '&': 0}
t = 0
active = False
bits = []
transaction = None
transactions = []
sample_rises = []
sample_falls = []
for line in vcd.open():
    line = line.strip()
    if line.startswith('#'):
        t = int(line[1:])
        if t > 5_500_000_000:
            break
    elif len(line) == 2 and line[0] in '01' and line[1] in signals:
        v, pin = int(line[0]), line[1]
        old = signals[pin]
        if old == v:
            continue
        if pin == '&':
            (sample_rises if v else sample_falls).append(t)
        if pin == '"' and signals['!']:
            if old == 1 and v == 0:
                if transaction:
                    transactions.append(transaction)
                transaction = {'start_ns': t, 'bytes': []}
                active, bits = True, []
            elif old == 0 and v == 1 and active:
                if transaction:
                    transaction['stop_ns'] = t
                    transactions.append(transaction)
                transaction, active, bits = None, False, []
        if pin == '!' and old == 0 and v == 1 and active and transaction:
            bits.append(signals['"'])
            if len(bits) == 9:
                value = 0
                for bit in bits[:8]:
                    value = (value << 1) | bit
                transaction['bytes'].append({'hex':f'{value:02x}','ack':bits[8] == 0})
                bits = []
        signals[pin] = v

before = [x for x in transactions if x['bytes'] and x['bytes'][0]['hex'] in ('d0','d1') and x['start_ns'] < 5_000_000_000]
after = [x for x in transactions if x['bytes'] and x['bytes'][0]['hex'] in ('d0','d1') and x['start_ns'] >= 5_000_000_000]
frames = [x for x in after if x['bytes'][0]['hex']=='d1' and len(x['bytes'])>=15]
assert len(before)>=5 and all(not x['bytes'][0]['ack'] for x in before), before
assert after and after[0]['bytes'][0]['ack']
assert frames, 'No full 14-byte sensor frame decoded'
first = frames[0]
assert [b['hex'] for b in first['bytes'][1:]] == ['00','00','00','00','40','00','00','00','00','00','00','00','00','00']
assert all(b['ack'] for b in first['bytes'][:-1]) and not first['bytes'][-1]['ack']
assert any(x['bytes'][0]['hex']=='d1' and len(x['bytes'])>=2 and x['bytes'][1]['hex']=='68' for x in after)
inside = [n for n in sample_rises if n<=first['start_ns']]
end = [n for n in sample_falls if n>first.get('stop_ns',first['start_ns'])]
out={'nack_address_phases':before[:5], 'first_ack_address':after[0],
     'first_full_read':first, 'sample_start_ns':inside[-1] if inside else None,
     'sample_end_ns':end[0] if end else None,
     'limits':'I2C decoded from SCL rising edges; VCD reports wire activity, not MCU return codes; matching SAMPLE is enclosing pulse.'}
Path('startup-ready-i2c.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out,indent=2))
