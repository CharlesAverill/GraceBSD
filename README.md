# GraceBSD

A Unix v6-like kernel for a BSD-like operating system built with CakeML.
Uses the Limine bootloader.

## Build

```bash
make
```

## Emulate in QEMU

Boots, shows a splash screen, passes off to [main.cml](kernel/cml/main.cml), then
returns to C.

```bash
make run

Hello, World from GraceBSD!
Filling framebuffer
Drawing splash
CakeML: setting up memory
CakeML: calling cml_main
What's your name? Hello, Charles
15! = 1307674368000
CakeML: cml_main returned
done running cakeml
```
