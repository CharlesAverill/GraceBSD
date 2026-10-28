# GraceBSD

A Unix v6-like kernel for a BSD-like operating system built with CakeML.
Uses the Limine bootloader.

## Build

```bash
make
```

## Emulate in QEMU

```bash
make run

# Hello, World from GraceBSD!
# CakeML: setting up memory
# CakeML: calling cml_main
# CakeML: cml_exit called, return value = 42
```
