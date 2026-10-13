# Sechs: the master and module cores, sechsctl, and the tests
#
#   make            sechsctl (Linux: /dev/i2c-N, or Werkzeug's bridge)
#   make test       the master against the module core, and sechsctl
#                   against a simulated module

CC ?= cc
CFLAGS ?= -std=gnu99 -O2 -Wall -Wextra
CPPFLAGS += -Imodule -Imaster

all : sechsctl

sechsctl : tools/sechsctl.c master/sechsm.c master/sechsm.h module/sechs.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ tools/sechsctl.c master/sechsm.c

# sechsctl talking to a simulated module (test/sim.h)
sechsctl_sim : tools/sechsctl.c test/sim.h master/sechsm.c module/sechs.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -Itest -DSECHS_SIM -o $@ \
		tools/sechsctl.c master/sechsm.c module/sechs.c

test/sechs_test : test/sechs_test.c test/sim.h master/sechsm.c master/sechsm.h \
		module/sechs.c module/sechs.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ test/sechs_test.c master/sechsm.c module/sechs.c

# the cores must build as freestanding C99, as on the modules and in
# the programmers
cores :
	$(CC) -std=c99 -pedantic -Wall -Wextra -Werror -ffreestanding -Imodule -c \
		-o /dev/null module/sechs.c
	$(CC) -std=c99 -pedantic -Wall -Wextra -Werror -ffreestanding -Imodule -Imaster -c \
		-o /dev/null master/sechsm.c

test : cores test/sechs_test sechsctl sechsctl_sim
	./test/sechs_test
	./test/sechsctl_test.sh

clean :
	rm -f sechsctl sechsctl_sim test/sechs_test ZZSIM.IMG

.PHONY : all cores test clean
