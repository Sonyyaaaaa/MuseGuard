# Phase20 test input

Create Custom Chip `phase20` in web Wokwi, paste the `.chip.c` and `.chip.json`, then the supplied diagram. Upload the baseline ELF. The fixture only drives PA10; the PIR OUT is disconnected. It does not implement the detector or MCU flash.

Groups start at 1, 26, 51 and 76 seconds. Each has 100 pulses, high 20000 us, low 180200 us. The 200200 us period advances by 200 us modulo the 20 ms sampling grid. Capture the real VCD and verify all widths and phase bins. Keep groups 1/2/4 calm; during group 3 send packets of 64 `n` commands about once a second and one `d` dump. Match the actual COMMAND-high edges from VCD instead of assuming host delays equal MCU time.

Baseline result: 300/300 calm and 92/100 stress, 37 stress inputs during COMMAND high. Each group covered all 20 bins of 1 ms. Use `tools/analyze_phase20.py` for counts and notifications. Inspect missing inputs and later EVENTs before calling them permanent losses. The fixture is an instrument model; these are not physical PIR or shock measurements.
