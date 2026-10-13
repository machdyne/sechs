# Sechs

Sechs ("six" in German) is a six-pin interface for small modules: two pins
for power and four for signals.

| Pin | Name | Use |
|---|---|---|
| 1 | A | global I2C bus (SCL) |
| 2 | B | global I2C bus (SDA) |
| 3 | C | the module's own I/O (UART, I2C, analog, digital) |
| 4 | D | the module's own I/O |
| 5 | GND | ground |
| 6 | 3V3 | 3.3V |

A controller on pins 1 and 2 finds every module, identifies it, controls
it and programs it, through a small set of registers and a text console;
pins 3 and 4 are the module's own.

Sechs grew out of the [Zwölf](https://github.com/machdyne/zwolf) project.
It drops the Zwölf VM requirements and focuses only on the interface.
Zwölf modules such as the [LS10A](https://machdyne.com/product/zwolf-ls10)
running [Machdyne BASIC](https://github.com/machdyne/basic) are
Sechs-compatible modules.

## What is here

| Path | |
|---|---|
| [docs/sechs.md](docs/sechs.md) | the specification |
| [module/](module/) | the module core: the I2C target protocol of a Sechs module, hardware-independent (as used by Machdyne BASIC on the LS10/LS11) |
| [master/](master/) | the master core (`sechsm`): discovery, identity, CONTROL, addresses, registers, the I2C console; independent of how the bus is reached |
| [tools/sechsctl.c](tools/sechsctl.c) | `sechsctl`, a small Sechs controller for Linux (`/dev/i2c-N`, or Werkzeug's USB bridge) |
| [test/sim.h](test/sim.h) | a simulated module (the module core with a small console), for the tests |
| [test/](test/) | the master against the module core, and `sechsctl` against the simulated module |
| [docs/using.md](docs/using.md) | using the cores from another project (machprog, BASIC, Zeitlos) |

Both cores are plain C99: no allocation, no stdio, no operating system.

```
$ make                 # sechsctl
$ make test            # the cores (freestanding), and the tests
$ ./sechsctl scan                      # modules on /dev/i2c-1
$ ./sechsctl -b 3 info 0x0c            # another bus
$ ./sechsctl console 0x0c              # the I2C console (Ctrl-D ends it)
$ echo "PRINT 1+1" | ./sechsctl send 0x0c
```

`sechsctl` is the small, single-purpose tool. For a full programmer
(MMOD memory modules, Sechs files and backups, module firmware) see
[machprog](https://github.com/machdyne/machprog), which uses the master
core from here.

## Where the rest lives

- Machdyne BASIC, the only Sechs-compliant module firmware so far, and
  its Werkzeug target with the USB bridge that `sechsctl -d` talks to:
  [machdyne/basic](https://github.com/machdyne/basic).
- The programmers (Wolfskralle, Schnecke) and `machprog`:
  [machdyne/machprog](https://github.com/machdyne/machprog).

## License

The contents of this repo are released under the [Lone Dynamics Open
License](LICENSE.md).
