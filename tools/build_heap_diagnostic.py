#!/usr/bin/env python3
"""Link optional mallinfo/reset instrumentation without changing the baseline ELF."""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
source=root/'tools/diagnostics/heap-reset-probe.c';shutil.copy2(source,out/source.name)
mf=(root/'firmware/Makefile').read_text();cs=re.search(r'C_SOURCES\s*=\s*(.*?)# ASM sources',mf,re.S).group(1)
objects=['build/'+Path(x).with_suffix('.o').name for x in cs.replace('\\',' ').split()]+['build/startup_stm32c031xx.o']
if not all((root/'firmware'/x).is_file() for x in objects):raise SystemExit('Run make build before linking the diagnostic ELF')
docker=['docker','run','--rm','--name','museguard-heap-diagnostic','-v',str(root)+':/workspace','-v',str(out)+':/evidence','-w','/workspace/firmware','museguard-build:lab3']
cc=['arm-none-eabi-gcc','-mcpu=cortex-m0plus','-mthumb','-DSTM32C031xx','-DUSE_HAL_DRIVER','-Og','-g','-Wall','-Wextra','-Werror','-fstack-usage','-ffunction-sections','-fdata-sections','-ICore/Inc','-IDrivers/STM32C0xx_HAL_Driver/Inc','-IDrivers/CMSIS/Device/ST/STM32C0xx/Include','-IDrivers/CMSIS/Include','-c','/evidence/heap-reset-probe.c','-o','/evidence/heap-reset-probe.o']
link=['arm-none-eabi-gcc','-mcpu=cortex-m0plus','-mthumb',*objects,'/evidence/heap-reset-probe.o','-specs=nano.specs','-TSTM32C031C6Tx_FLASH.ld','-lc','-lm','-lnosys','-Wl,--wrap=lab3_heap_peak,--wrap=lab3_rx,--wrap=runner_start,--gc-sections,--print-memory-usage,-Map=/evidence/heap-reset-probe.map','-o','/evidence/heap-reset-probe.elf']
for cmd in [cc,link]:subprocess.run(docker+cmd,check=True)
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
(out/'manifest.json').write_text(json.dumps({'kind':'separate diagnostic, not baseline','baseline_elf_sha256':sha(root/'firmware/build/firmware.elf'),'extra_source_sha256':sha(source),'elf_sha256':sha(out/'heap-reset-probe.elf'),'compile':cc,'link':link},indent=2)+'\n')
