/*
 * A simulated Sechs module, without an interpreter: the real module core
 * (module/sechs.c) behind a simulated I2C bus, and a small console in
 * place of a language. sechsctl built with -DSECHS_SIM uses it, and so do
 * the tests (test/).
 *
 * The console answers each line it is given:
 *   PRINT A+B     prints the sum (A and B integers), as a language would
 *   FAIL          fails: OK bit 4 ("last console command succeeded") clears
 *   anything else prints "OK"
 * CONTROL RUN starts a "program" (STATUS running) until HALT or RESET.
 *
 * The module's address persists, as on a real module, in the file named
 * by SECHS_SIM_IMAGE (default ZZSIM.IMG): three bytes, A5 addr ~addr.
 *
 * It lives in test/, not beside tools/sechsctl.c: a compiler looks for
 * "sim.h" in the including file's directory first, and machdyne/basic
 * builds sechsctl with its own sim.h (Machdyne BASIC's real interpreter)
 * from its include path.
 */

#include <fcntl.h>
#include <unistd.h>

static const char *sim_image_path(void)
{
    const char *p = getenv("SECHS_SIM_IMAGE");
    return p ? p : "ZZSIM.IMG";
}

static uint8_t sim_regs[16];
static int sim_running, sim_cmd_err;
static const char sim_info_text[] = "fw=Sechs simulator\nmod=SIM\nlang=none\n";

void sechs_wait(void) { }
uint8_t *sechs_regs(void) { return sim_regs; }
uint8_t sechs_info(uint8_t i)
{
    return i < sizeof(sim_info_text) - 1 ? (uint8_t)sim_info_text[i] : 0;
}

void sechs_set_addr(uint8_t a)
{
    uint8_t c[3] = { 0xA5, a, (uint8_t)~a };
    int f = open(sim_image_path(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (f >= 0) {
        if (write(f, c, 3) != 3) { /* the address is then not kept */ }
        close(f);
    }
}

static uint8_t sim_saved_addr(void)
{
    uint8_t c[3] = { 0, 0, 0 };
    int f = open(sim_image_path(), O_RDONLY);
    if (f >= 0) {
        if (read(f, c, 3) != 3) c[0] = 0;
        close(f);
    }
    if (c[0] == 0xA5 && c[2] == (uint8_t)~c[1] && c[1] >= 0x08 && c[1] <= 0x77)
        return c[1];
    return SECHS_DEFAULT_ADDR;
}

/* power-on: the stored address, nothing running, REG all 0 */
static void sim_start(void)
{
    sechs_init(sim_saved_addr(), CAP_I2C_CON);
    memset(sim_regs, 0, sizeof(sim_regs));
    sim_running = sim_cmd_err = 0;
    sechs.con_active = 1;
}

static int bus_open(int bus)
{
    (void)bus;
    sim_start();
    return 0;
}

static void sim_puts(const char *s)
{
    while (*s) sechs_putc(*s++);
}

static void sim_line(const char *line)
{
    long a, b;
    char out[32];
    sim_cmd_err = 0;
    if (sscanf(line, "PRINT %ld+%ld", &a, &b) == 2) {
        snprintf(out, sizeof(out), "%ld\n", a + b);
        sim_puts(out);
    } else if (!strcmp(line, "FAIL")) {
        sim_cmd_err = 1;
        sim_puts("?ERROR\n");
    } else {
        sim_puts("OK\n");
    }
}

/* STATUS, FAULT and OK, kept as a module's main loop keeps them */
static void sim_status(void)
{
    sechs.r[SR_FAULT] = 0;
    sechs.r[SR_STATUS] = (sechs.networked ? ST_NETWORKED : 0) |
        (sim_running ? ST_RUNNING : 0) | ST_CONSOLE;
    uint8_t s = sechs.r[SR_STATUS];
    sechs.r[SR_OK] = ~(((s >> 4) & 0x06) | ((s << 2) & 0x08) |
        (sim_cmd_err ? 0x10 : 0)) & 0x1F;
}

/* the module's main loop: console lines and CONTROL commands */
static void bus_idle(void)
{
    static char line[128];
    static int n;
    int c;
    while ((c = sechs_getc()) >= 0) {
        sechs_putc((char)c);
        if (c == '\r' || c == '\n') {
            sechs_putc('\n');
            line[n] = 0;
            n = 0;
            sim_line(line);
        } else if (n < (int)sizeof(line) - 1) {
            line[n++] = (char)c;
        }
    }
    if (sechs.cmd == CMD_HALT) {
        sechs.cmd = 0;
        sim_running = 0;
    } else if (sechs.cmd == CMD_RUN) {
        sechs.cmd = 0;
        sim_running = 1;
    } else if (sechs.cmd == CMD_RESET) {
        sechs.cmd = 0;
        sim_start();
    }
    sim_status();
}

static int bus_write(uint8_t a, const uint8_t *d, int n)
{
    if (a != sechs.addr) return -1;
    sechs_start(0);
    for (int i = 0; i < n; i++) sechs_rx(d[i]);
    sechs_stop(0);
    bus_idle();
    return 0;
}

static int bus_read(uint8_t a, uint8_t reg, uint8_t *d, int n)
{
    if (a != sechs.addr) return -1;
    bus_idle();
    sechs_start(0);
    sechs_rx(reg);
    sechs_start(0);
    /* like the hardware: one byte is always loaded ahead, and the last one
     * loaded is never sent */
    for (int i = 0; i < n; i++) d[i] = sechs_tx();
    sechs_tx();
    sechs_stop(1);
    return 0;
}
