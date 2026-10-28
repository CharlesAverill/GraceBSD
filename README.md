# GraceBSD

A Unix v6-like kernel for a BSD-like operating system built with CakeML.
Uses the Limine bootloader.

## Build

```bash
make
```

## Emulate in QEMU

Boots, shows a splash screen, passes a value from C to CakeML, and then receives
the factorial of that value back from CakeML.

```bash
make run

# Hello, World from GraceBSD!
# CakeML: setting up memory
# CakeML: calling cml_main
# CakeML: cml_exit called, return value = 120
```
