// https://wiki.osdev.org/I8042_PS2_Controller#Initialising_the_PS2_Controller

#include "ps2.h"

#include <stddef.h>
#include <stdint.h>

#include "io.h"
#include "logging.h"

#define PS2_DATA   0x60
#define PS2_STATUS 0x64

#define STATUS_OUTPUT_FULL 0x01
#define STATUS_INPUT_FULL  0x02

#define CMD_READ_CONFIG   0x20
#define CMD_WRITE_CONFIG  0x60
#define CMD_DISABLE_PORT2 0xA7
#define CMD_ENABLE_PORT2  0xA8
#define CMD_TEST_PORT2    0xA9
#define CMD_SELF_TEST     0xAA
#define CMD_TEST_PORT1    0xAB
#define CMD_DISABLE_PORT1 0xAD
#define CMD_ENABLE_PORT1  0xAE
#define CMD_WRITE_PORT2   0xD4

#define CONFIG_PORT1_IRQ   0x01
#define CONFIG_PORT2_IRQ   0x02
#define CONFIG_PORT1_CLOCK 0x10
#define CONFIG_PORT2_CLOCK 0x20
#define CONFIG_TRANSLATE   0x40

#define SELF_TEST_PASSED     0x55
#define PORT_TEST_PASSED     0x00
#define DEV_RESET            0xFF
#define DEV_ACK              0xFA
#define DEV_RESET_PASSED     0xAA
#define DEV_SELF_TEST_FAILED 0xFC

#define TIMEOUT       100000
#define RESET_TIMEOUT 1000000

static bool wait_output(long timeout) {
    for (long i = 0; i < timeout; i++) {
        if (inb(PS2_STATUS) & STATUS_OUTPUT_FULL)
            return true;
    }
    return false;
}

static bool wait_input(void) {
    for (long i = 0; i < TIMEOUT; i++) {
        if (!(inb(PS2_STATUS) & STATUS_INPUT_FULL))
            return true;
    }
    return false;
}

// Read a byte, or return -1 on timeout
static int read_data(long timeout) {
    if (!wait_output(timeout))
        return -1;
    return inb(PS2_DATA);
}

static bool write_command(uint8_t cmd) {
    if (!wait_input())
        return false;
    outb(PS2_STATUS, cmd);
    return true;
}

static bool write_data(uint8_t data) {
    if (!wait_input())
        return false;
    outb(PS2_DATA, data);
    return true;
}

// Send a byte to the device on [port] (1 or 2)
static bool write_device(int port, uint8_t data) {
    if (port == 2 && !write_command(CMD_WRITE_PORT2))
        return false;
    return write_data(data);
}

static int read_config(void) {
    if (!write_command(CMD_READ_CONFIG))
        return -1;
    return read_data(TIMEOUT);
}

static bool write_config(uint8_t config) {
    return write_command(CMD_WRITE_CONFIG) && write_data(config);
}

static void flush_output(void) {
    while (inb(PS2_STATUS) & STATUS_OUTPUT_FULL) inb(PS2_DATA);
}

// Run an interface test command and check that the port passed
static bool test_port(int port, uint8_t cmd) {
    int r = -1;
    if (!write_command(cmd) || (r = read_data(TIMEOUT)) != PORT_TEST_PASSED) {
        warn("PS2: port %d interface test failed (0x%x)", port, r);
        return false;
    }
    return true;
}

static bool reset_device(int port) {
    if (!write_device(port, DEV_RESET)) {
        warn("PS2: could not send reset to port %d", port);
        return false;
    }

    bool acked = false, passed = false;
    int r = -1;
    while (!(acked && passed)) {
        r = read_data(RESET_TIMEOUT);
        if (r == DEV_ACK) {
            acked = true;
        } else if (r == DEV_RESET_PASSED) {
            passed = true;
        } else if (r == DEV_SELF_TEST_FAILED) {
            warn("PS2: port %d device failed its self-test", port);
            return false;
        } else if (r < 0) {
            if (acked || passed)
                warn("PS2: port %d device reset incomplete (ack %d, passed %d)", port, acked, passed);
            else
                info("PS2: port %d is empty", port);
            return false;
        }
    }

    // Read the ID bytes so they don't reach the IRQ handler as scancodes
    int id[2]  = { 0, 0 };
    int id_len = 0;
    while (id_len < 2 && (r = read_data(TIMEOUT)) >= 0) id[id_len++] = r;
    if (id_len == 0)
        info("PS2: port %d device ready (no ID)", port);
    else if (id_len == 1)
        info("PS2: port %d device ready (ID 0x%02x)", port, id[0]);
    else
        info("PS2: port %d device ready (ID 0x%02x 0x%02x)", port, id[0], id[1]);
    return true;
}

bool ps2_init(void) {
    // TODO: Initialise USB Controllers
    // TODO: Determine if the PS2 Controller Exists

    // Try to init 100 times
    int config;
    int r;
    size_t i;
    for (i = 0; i < 100; i++) {
        // Step 3: Disable Devices
        if (!write_command(CMD_DISABLE_PORT1) || !write_command(CMD_DISABLE_PORT2)) {
            warn("PS2: controller not responding");
            continue;
        }

        // Step 4: Flush The Output Buffer
        flush_output();

        // Step 5: Set the Controller Configuration Byte
        config = read_config();
        if (config < 0) {
            warn("PS2: could not read the configuration byte");
            continue;
        }
        config &= ~(CONFIG_PORT1_IRQ | CONFIG_PORT1_CLOCK | CONFIG_TRANSLATE);
        if (!write_config(config)) {
            warn("PS2: could not write the configuration byte");
            continue;
        }

        // Step 6: Perform Controller Self Test
        r = -1;
        if (!write_command(CMD_SELF_TEST) || (r = read_data(TIMEOUT)) != SELF_TEST_PASSED) {
            warn("PS2: controller self-test failed (0x%x)", r);
            continue;
        }
        if (!write_config(config)) {
            warn("PS2: could not restore the configuration byte");
            continue;
        }

        break;
    }
    if (i == 100) {
        warn("PS2: failed to init after 100 attempts");
        return false;
    }

    // Step 7: Determine If There Are 2 Channels
    bool dual = false;
    if (write_command(CMD_ENABLE_PORT2) && (config = read_config()) >= 0) {
        dual = !(config & CONFIG_PORT2_CLOCK);
    }
    if (config < 0) {
        warn("PS2: could not read the configuration byte");
        return false;
    }
    if (dual) {
        write_command(CMD_DISABLE_PORT2);
        config &= ~(CONFIG_PORT2_IRQ | CONFIG_PORT2_CLOCK);
        if (!write_config(config)) {
            warn("PS2: could not write the configuration byte");
            return false;
        }
    }

    // Step 8: Perform Interface Tests
    bool port1 = test_port(1, CMD_TEST_PORT1);
    bool port2 = dual && test_port(2, CMD_TEST_PORT2);
    if (!port1 && !port2) {
        warn("PS2: no working ports");
        return false;
    }

    // Step 9: Enable Devices
    if (port1 && !write_command(CMD_ENABLE_PORT1)) {
        warn("PS2: could not enable port 1");
        port1 = false;
    }
    if (port2 && !write_command(CMD_ENABLE_PORT2)) {
        warn("PS2: could not enable port 2");
        port2 = false;
    }

    // Step 10: Reset Devices
    port1 = port1 && reset_device(1);
    port2 = port2 && reset_device(2);

    if ((config = read_config()) < 0) {
        warn("PS2: could not read the configuration byte");
        return false;
    }
    if (port1)
        config |= CONFIG_PORT1_IRQ | CONFIG_TRANSLATE;
    if (port2)
        config |= CONFIG_PORT2_IRQ;
    if (!write_config(config)) {
        warn("PS2: could not enable IRQs");
        return false;
    }

    return port1;
}
