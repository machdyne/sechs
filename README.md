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

Sechs grew out of the [Zwölf](https://github.com/machdyne/zwolf) project. It drops the Zwölf VM requirements and focuses only on the interface.

Zwölf modules such as the [LS10A](https://machdyne.com/product/zwolf-ls10) are Sechs-compatible modules.

## Where it lives

The specification and everything that implements it are in the
[Machdyne BASIC](https://github.com/machdyne/basic) repository, which is currently the only Sechs-compliant firmware available:

- [docs/sechs.md](https://github.com/machdyne/basic/blob/main/docs/sechs.md):
  the specification
- [sechs/](https://github.com/machdyne/basic/tree/main/sechs): the module
  side, as used by the LS10 firmware
- [tools/sechs/](https://github.com/machdyne/basic/tree/main/tools/sechs):
  `sechsctl`, a controller for Linux, and the Werkzeug bridge
