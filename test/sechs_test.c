/*
 * The Sechs master (master/sechsm.c) against the module core
 * (module/sechs.c), over a simulated I2C bus (test/sim.h), without an
 * interpreter: every transfer goes through the module core's interrupt
 * entry points exactly as on a module, including the byte a CH32V003
 * loads ahead at the end of a read.
 *
 * Checks what every master relies on: discovery, identity, INFO, the
 * program registers, CONTROL, address changes (kept across a restart),
 * the I2C console (input paced by CIN, output drained, OK bit 4), and
 * that PROGRAM is refused without CAPS bit 7.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sechs.h"
#include "sechsm.h"
#include "sim.h"

static int failures;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL %d: ", __LINE__); \
    printf(__VA_ARGS__); printf("\n"); failures++; } } while (0)

static char out[4096];
static int outn;
static long clock_ms;

static int t_write(void *c, uint8_t addr, const uint8_t *d, int n)
{ (void)c; return bus_write(addr, d, n); }
static int t_read(void *c, uint8_t addr, uint8_t reg, uint8_t *d, int n)
{ (void)c; return bus_read(addr, reg, d, n); }
static void t_idle(void *c) { (void)c; clock_ms += 2; bus_idle(); }
static long t_ms(void *c) { (void)c; return clock_ms++; }
static void t_out(void *c, char ch)
{ (void)c; if (outn < (int)sizeof(out) - 1) out[outn++] = ch; out[outn] = 0; }

static const sm_bus B = { t_write, t_read, t_idle, t_ms, t_out, NULL };

int main(void)
{
    char img[] = "/tmp/sechs_test_XXXXXX";
    int fd = mkstemp(img);
    if (fd < 0) { perror("mkstemp"); return 1; }
    close(fd);
    unlink(img);                        /* a fresh module: default address */
    setenv("SECHS_SIM_IMAGE", img, 1);
    bus_open(0);

    /* discovery and identity */
    uint8_t found[8];
    int n = sm_scan(&B, found, 8);
    CHECK(n == 1 && found[0] == SECHS_DEFAULT_ADDR, "scan: %d modules", n);
    CHECK(sm_is_module(&B, SECHS_DEFAULT_ADDR) == 1, "S6 signature");
    CHECK(sm_is_module(&B, 0x21) != 1, "nothing at 0x21");

    sm_info_t inf;
    CHECK(sm_info(&B, SECHS_DEFAULT_ADDR, &inf) == 0, "info");
    CHECK(inf.version == SECHS_VERSION, "version 0x%02x", inf.version);
    CHECK(inf.caps & CAP_I2C_CON, "caps 0x%02x", inf.caps);
    CHECK(strstr(inf.info, "fw=Sechs simulator") && strstr(inf.info, "mod=SIM"),
          "INFO: %s", inf.info);
    CHECK(inf.status & ST_NETWORKED, "networked after being addressed");

    /* program registers */
    for (int r = 0; r < 16; r++) {
        uint8_t v = (uint8_t)(r * 17 + 3);
        CHECK(sm_reg_write(&B, SECHS_DEFAULT_ADDR, SR_REG + r, &v, 1) == 0, "REG %d", r);
    }
    for (int r = 0; r < 16; r++)
        CHECK(sm_reg(&B, SECHS_DEFAULT_ADDR, SR_REG + r) == (uint8_t)(r * 17 + 3), "REG %d read", r);

    /* CONTROL */
    CHECK(sm_control(&B, SECHS_DEFAULT_ADDR, CMD_RUN) == 0, "RUN");
    t_idle(NULL);
    CHECK(sm_reg(&B, SECHS_DEFAULT_ADDR, SR_STATUS) & ST_RUNNING, "running after RUN");
    CHECK(sm_control(&B, SECHS_DEFAULT_ADDR, CMD_HALT) == 0, "HALT");
    t_idle(NULL);
    CHECK(!(sm_reg(&B, SECHS_DEFAULT_ADDR, SR_STATUS) & ST_RUNNING), "stopped after HALT");
    CHECK(sm_control(&B, SECHS_DEFAULT_ADDR, CMD_PROGRAM) == -2,
          "PROGRAM refused without CAPS bit 7");

    /* the I2C console: a line in, its answer out, OK bit 4 */
    outn = 0;
    CHECK(sm_line(&B, SECHS_DEFAULT_ADDR, "PRINT 1234+4321", 15) == 0, "console line");
    CHECK(strstr(out, "5555") != NULL, "console answer: %s", out);
    CHECK(sm_last_ok(&B, SECHS_DEFAULT_ADDR) == 1, "last command ok");
    outn = 0;
    CHECK(sm_line(&B, SECHS_DEFAULT_ADDR, "FAIL", 4) == 0, "console line FAIL");
    CHECK(strstr(out, "?ERROR") != NULL, "error answer: %s", out);
    CHECK(sm_last_ok(&B, SECHS_DEFAULT_ADDR) == 0, "last command failed");

    /* a long input: paced by CIN, never more than the module takes */
    char longline[200];
    memset(longline, 'X', sizeof(longline) - 1);
    longline[sizeof(longline) - 1] = 0;
    outn = 0;
    CHECK(sm_type(&B, SECHS_DEFAULT_ADDR, longline, (int)strlen(longline)) == 0, "long input");
    sm_settle(&B, SECHS_DEFAULT_ADDR, 50);
    int echoed = 0;
    for (int i = 0; i < outn; i++) echoed += out[i] == 'X';
    CHECK(echoed == (int)strlen(longline), "every byte echoed: %d of %d",
          echoed, (int)strlen(longline));

    /* a new address, kept across a restart */
    CHECK(sm_set_address(&B, SECHS_DEFAULT_ADDR, 0x21) == 0, "ADDR 0x21");
    CHECK(sm_is_module(&B, 0x21) == 1, "answers at 0x21");
    CHECK(sm_is_module(&B, SECHS_DEFAULT_ADDR) != 1, "not at 0x0C any more");
    CHECK(sm_set_address(&B, 0x21, 0x78) == -1, "0x78 refused");
    bus_open(0);                        /* power cycle */
    CHECK(sm_is_module(&B, 0x21) == 1, "0x21 after a restart");
    CHECK(sm_control(&B, 0x21, CMD_RESET) == 0, "RESET");
    t_idle(NULL);
    CHECK(sm_reg(&B, 0x21, SR_REG + 1) == 0, "REG cleared by RESET");

    unlink(img);
    if (failures) {
        printf("sechs_test: %d failures\n", failures);
        return 1;
    }
    printf("sechs_test: master against the module core: passed\n");
    return 0;
}
