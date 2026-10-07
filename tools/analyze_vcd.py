#!/usr/bin/env python3
"""Decode Wokwi VCD timing without assuming a dense sample buffer."""
import argparse
import bisect
import json
from pathlib import Path

def load(path):
    names, events, time = {}, {}, 0
    for line in Path(path).read_text().splitlines():
        if line.startswith("$var"):
            words = line.split()
            names[words[3]] = words[4]
            events[words[4]] = []
        elif line.startswith("#"):
            time = int(line[1:])
        elif line[:1] in ("0", "1") and line[1:] in names:
            events[names[line[1:]]].append((time, int(line[0])))
    return events

def i2c(events):
    changes = {}
    for name in ("SCL", "SDA"):
        for time, value in events[name]:
            changes.setdefault(time, {})[name] = value
    scl = sda = 1
    active, bits, words, rising, starts, stops = False, [], [], [], [], []
    for time, values in sorted(changes.items()):
        next_scl, next_sda = values.get("SCL", scl), values.get("SDA", sda)
        if scl == next_scl == 1 and sda != next_sda:
            active = next_sda == 0
            bits = []
            (starts if active else stops).append(time)
        if active and scl == 0 and next_scl == 1:
            rising.append(time)
            bits.append(next_sda)
            if len(bits) == 9:
                byte = sum(bit << (7 - n) for n, bit in enumerate(bits[:8]))
                words.append({"ns": time, "byte": byte, "ack": bits[8] == 0})
                bits = []
        scl, sda = next_scl, next_sda
    periods = [b-a for a,b in zip(rising,rising[1:]) if 1000 < b-a < 20000]
    return {"bytes": words, "starts_ns": starts, "stops_ns": stops,
            "scl_period_median_ns": sorted(periods)[len(periods)//2] if periods else None}

def uart(events):
    times, levels = zip(*events["UART_TX"])
    bit = 1e9 / 115200
    end, previous, output, byte_times = -1, 1, [], []
    for time, value in zip(times, levels):
        if previous == 1 and value == 0 and time >= end:
            positions = [bisect.bisect_right(times, time+(1.5+n)*bit)-1 for n in range(8)]
            stop = bisect.bisect_right(times,time+9.5*bit)-1
            if min(positions) >= 0 and stop >= 0 and levels[stop] == 1:
                output.append(sum(levels[position] << n for n,position in enumerate(positions)))
                byte_times.append(time)
            end = time + 9.8*bit
        previous = value
    lines, begin = [], 0
    for index,value in enumerate(output):
        if value == 10:
            lines.append({"start_ns":byte_times[begin],"end_ns":byte_times[index],
                          "text":bytes(output[begin:index+1]).decode("ascii",errors="replace").strip()})
            begin=index+1
    return bytes(output).decode("ascii", errors="replace"), lines

def pulses(events, name):
    opened, widths, rising = None, [], []
    for time,value in events[name]:
        if value:
            opened=time
            rising.append(time)
        elif opened is not None:
            widths.append(time-opened)
            opened=None
    return {"rises_ns":rising,"widths_ns":widths}

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("capture")
    parser.add_argument("--output",required=True)
    args=parser.parse_args()
    events=load(args.capture)
    prefix=Path(args.output)
    prefix.parent.mkdir(parents=True,exist_ok=True)
    result=i2c(events)
    result["adc"]=pulses(events,"ADC_MARK")
    result["pir"]=pulses(events,"PIR")
    uart_text, result["uart_lines"] = uart(events)
    prefix.with_suffix(".json").write_text(json.dumps(result,indent=2)+"\n")
    prefix.with_suffix(".uart.txt").write_text(uart_text)
    print(f"Decoded {len(result['bytes'])} I2C bytes; wrote {prefix}.json and UART")
if __name__=="__main__":
    main()
