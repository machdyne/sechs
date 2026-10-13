# Sechs master core

`sechsm.h`, `sechsm.c`: the master side of Sechs ([the
specification](../docs/sechs.md)), independent of how the bus is reached.
Plain C99: no allocation, no stdio, no operating system, so the same code
runs in `sechsctl` on Linux, in machprog (through its programmers), in
Machdyne BASIC's Werkzeug target and in Zeitlos.

A host provides an `sm_bus`: an I2C write, a register read (write the
register number, repeated start, read), a short idle, a millisecond clock
and a character sink for console output. On that the core does discovery
(`sm_scan`, `sm_is_module`), identity and INFO (`sm_info`), CONTROL
(`sm_control`), address changes (`sm_set_address`), registers
(`sm_reg`, `sm_reg_write`) and the I2C console (`sm_type`, `sm_drain`,
`sm_settle`, `sm_line`, `sm_last_ok`).

`test/sechs_test.c` runs it against the module core
([module/](../module/)) over a simulated bus.
