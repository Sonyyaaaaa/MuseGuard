#!/usr/bin/env python3
"""Reject hardware dependencies in portable application/device code."""
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
portable = [root / "firmware" / name for name in ("App", "Devices")]
headers = {p.name: p for folder in portable for p in folder.rglob("*.h")}
errors = []
for folder in portable:
    for path in folder.rglob("*"):
        if path.suffix not in (".c", ".h"):
            continue
        source = path.read_text()
        for include in re.findall(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', source, re.M):
            if any(token in include.lower() for token in ("stm32", "hal", "cmsis", "platform", "main.h", "runner")):
                errors.append(f"{path.relative_to(root)}: forbidden include {include}")
            if '"' + include + '"' in source and Path(include).name not in headers:
                errors.append(f"{path.relative_to(root)}: unknown portable header {include}")
        if re.search(r'\b(?:HAL_|LL_|GPIO[ABCDF]|hi2c|hadc|huart)', source):
            errors.append(f"{path.relative_to(root)}: hardware symbol in portable code")
if errors:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)
print("PASS: App and Devices depend only on portable headers")
