#!/bin/sh
# sechsctl against a simulated module (sechsctl_sim, test/sim.h): every
# command a user types, with its output and exit status.

set -u
cd "$(dirname "$0")/.." || exit 1
SECHS_SIM_IMAGE=$(mktemp)
rm -f "$SECHS_SIM_IMAGE"
export SECHS_SIM_IMAGE
trap 'rm -f "$SECHS_SIM_IMAGE"' EXIT

fail=0
check() {   # check NAME EXPECTED_STATUS PATTERN command...
	name=$1; want=$2; pat=$3; shift 3
	out=$("$@" 2>&1); st=$?
	if [ "$st" -ne "$want" ] || ! printf '%s\n' "$out" | grep -q -- "$pat"; then
		echo "FAIL $name: status $st (want $want), output:"
		printf '%s\n' "$out" | sed 's/^/  /'
		fail=1
	fi
}

check scan       0 "0x0c"              ./sechsctl_sim scan
check info       0 "mod=SIM"           ./sechsctl_sim info 0x0c
check reg-write  0 ""                  ./sechsctl_sim reg 0x0c 3 100
check send-ok    0 "5555"              sh -c 'echo "PRINT 1234+4321" | ./sechsctl_sim send 0x0c'
check send-fail  1 "ERROR"             sh -c 'echo FAIL | ./sechsctl_sim send 0x0c'
check program    1 "no programming mode" ./sechsctl_sim program 0x0c
check addr       0 ""                  ./sechsctl_sim addr 0x0c 0x21
check moved      0 "0x21"              ./sechsctl_sim scan
check addr-bad   2 "0x08-0x77"         ./sechsctl_sim addr 0x21 0x78

if [ $fail -ne 0 ]; then
	echo "sechsctl_test: failed"
	exit 1
fi
echo "sechsctl_test: sechsctl against a simulated module: passed"
