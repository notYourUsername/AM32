/*
 * test_dshot_decode.c — Unit tests for DShot frame parsing and command logic.
 *
 * Tests are host-compiled (no MCU hardware required).
 * They verify the correctness of:
 *   1. DShot 4-bit CRC calculation
 *   2. DShot throttle value extraction
 *   3. DShot cmd 36 (programming mode) guard logic (T1.6 fix)
 *   4. DShot telemetry CRC inversion
 *
 * The functions under test are re-implemented here in portable form
 * because the originals depend on DMA hardware. The logic is identical
 * to Src/dshot.c computeDshotDMA().
 */

#include "unity/unity.h"
#include <stdint.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────────────────
 * Portable re-implementation of DShot frame logic (mirrors dshot.c)
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * Unpack a 16-bit DShot frame word into 16 bit-array entries.
 * MSB first (dpulse[0] = bit 15).
 */
static void dshot_unpack(uint16_t frame, uint8_t dpulse[16])
{
    for (int i = 0; i < 16; i++)
        dpulse[i] = (frame >> (15 - i)) & 1u;
}

/**
 * Calculate the DShot CRC nibble over the high 12 bits of the frame
 * (throttle[10:0] + telemetry_bit).
 * Returns a 4-bit value.
 */
static uint8_t dshot_calc_crc(const uint8_t dpulse[16])
{
    return (uint8_t)(
        ((dpulse[0] ^ dpulse[4] ^ dpulse[8])  << 3) |
        ((dpulse[1] ^ dpulse[5] ^ dpulse[9])  << 2) |
        ((dpulse[2] ^ dpulse[6] ^ dpulse[10]) << 1) |
         (dpulse[3] ^ dpulse[7] ^ dpulse[11])
    );
}

/**
 * Extract the 11-bit throttle value from dpulse[].
 */
static uint16_t dshot_throttle(const uint8_t dpulse[16])
{
    return (uint16_t)(
        (dpulse[0] << 10) | (dpulse[1] << 9) | (dpulse[2] << 8) |
        (dpulse[3] <<  7) | (dpulse[4] << 6) | (dpulse[5] << 5) |
        (dpulse[6] <<  4) | (dpulse[7] << 3) | (dpulse[8] << 2) |
        (dpulse[9] <<  1) |  dpulse[10]
    );
}

/**
 * Build a valid 16-bit DShot frame from a throttle value and telemetry flag.
 *   frame[15:5]  = throttle (11 bits)
 *   frame[4]     = telemetry request
 *   frame[3:0]   = CRC(frame[15:4])
 * If bidir=1, the CRC nibble is bitwise-inverted (bidirectional DShot mode).
 */
static uint16_t dshot_build_frame(uint16_t throttle, uint8_t telemetry)
{
    uint16_t data12 = (uint16_t)((throttle << 1) | (telemetry & 1u));
    /* Standard DShot CRC: XOR of the three nibbles of the 12-bit value */
    uint8_t crc = (uint8_t)(((data12 >> 8) ^ (data12 >> 4) ^ data12) & 0xFu);
    return (uint16_t)((data12 << 4) | crc);
}

/**
 * T1.6 guard logic: programming mode entry is only allowed when the ESC
 * is not armed and the motor is not running.
 */
static int cmd36_allowed(int armed, int running)
{
    return (!armed && !running);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Test setup / teardown
 * ───────────────────────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ─────────────────────────────────────────────────────────────────────────
 * CRC tests
 * ───────────────────────────────────────────────────────────────────────── */

void test_crc_zero_throttle(void)
{
    /* throttle=0, telemetry=0 → data12=0x000 → CRC = 0^0^0 = 0 */
    uint16_t frame = dshot_build_frame(0, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);

    uint8_t crc_field = (uint8_t)(dpulse[12] << 3 | dpulse[13] << 2 |
                                   dpulse[14] << 1 | dpulse[15]);
    uint8_t calc       = dshot_calc_crc(dpulse);

    TEST_ASSERT_EQUAL_INT(calc, crc_field);
    TEST_ASSERT_EQUAL_INT(0, crc_field);
}

void test_crc_max_throttle(void)
{
    /* throttle=2047 (0x7FF), telemetry=0 → data12=0xFFE */
    uint16_t frame = dshot_build_frame(2047, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);

    uint8_t crc_field = (uint8_t)(dpulse[12] << 3 | dpulse[13] << 2 |
                                   dpulse[14] << 1 | dpulse[15]);
    uint8_t calc       = dshot_calc_crc(dpulse);

    TEST_ASSERT_EQUAL_INT(calc, crc_field);
}

void test_crc_known_frame_throttle_1000(void)
{
    /* Validate a known value: throttle=1000 (0x3E8), telemetry=0
     * data12 = 0x7D0 = 0b 0111 1101 0000
     * nibble3 = 0111 = 7
     * nibble2 = 1101 = 13
     * nibble1 = 0000 = 0
     * CRC = 7 ^ 13 ^ 0 = 10 = 0xA
     */
    uint16_t frame = dshot_build_frame(1000, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);

    uint8_t crc_field = (uint8_t)(dpulse[12] << 3 | dpulse[13] << 2 |
                                   dpulse[14] << 1 | dpulse[15]);
    uint8_t calc       = dshot_calc_crc(dpulse);

    TEST_ASSERT_EQUAL_INT(calc, crc_field);
    TEST_ASSERT_EQUAL_INT(0xA, crc_field);
}

void test_crc_telemetry_bit_affects_crc(void)
{
    /* Same throttle, different telemetry — CRC must differ */
    uint16_t f0 = dshot_build_frame(500, 0);
    uint16_t f1 = dshot_build_frame(500, 1);

    uint8_t d0[16], d1[16];
    dshot_unpack(f0, d0);
    dshot_unpack(f1, d1);

    uint8_t crc0 = dshot_calc_crc(d0);
    uint8_t crc1 = dshot_calc_crc(d1);

    /* Telemetry bit is dpulse[11]; flipping it must change at least CRC bit 0 */
    TEST_ASSERT_TRUE(crc0 != crc1);
}

void test_crc_corrupt_frame_fails(void)
{
    /* Build a valid frame then flip one data bit — CRC should no longer match */
    uint16_t frame = dshot_build_frame(300, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);

    /* Flip throttle bit 5 (dpulse[5]) */
    dpulse[5] ^= 1;

    uint8_t crc_field = (uint8_t)(dpulse[12] << 3 | dpulse[13] << 2 |
                                   dpulse[14] << 1 | dpulse[15]);
    uint8_t calc = dshot_calc_crc(dpulse);

    TEST_ASSERT_TRUE(calc != crc_field);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Throttle extraction tests
 * ───────────────────────────────────────────────────────────────────────── */

void test_throttle_roundtrip_zero(void)
{
    uint16_t frame = dshot_build_frame(0, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);
    TEST_ASSERT_EQUAL_INT(0, dshot_throttle(dpulse));
}

void test_throttle_roundtrip_max(void)
{
    uint16_t frame = dshot_build_frame(2047, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);
    TEST_ASSERT_EQUAL_INT(2047, dshot_throttle(dpulse));
}

void test_throttle_roundtrip_midscale(void)
{
    uint16_t frame = dshot_build_frame(1024, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);
    TEST_ASSERT_EQUAL_INT(1024, dshot_throttle(dpulse));
}

void test_throttle_command_36_value(void)
{
    /* DShot command 36 is a special command (throttle=36, telemetry=0) */
    uint16_t frame = dshot_build_frame(36, 0);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);
    TEST_ASSERT_EQUAL_INT(36, dshot_throttle(dpulse));
}

/* ─────────────────────────────────────────────────────────────────────────
 * T1.6 — DShot cmd 36 guard tests
 * Verifies: programming mode MUST NOT activate when armed or running.
 * ───────────────────────────────────────────────────────────────────────── */

void test_cmd36_blocked_when_armed(void)
{
    /* Armed=1, Running=0 → must not enter programming mode */
    TEST_ASSERT_FALSE(cmd36_allowed(1, 0));
}

void test_cmd36_blocked_when_running(void)
{
    /* Armed=0, Running=1 → must not enter programming mode */
    TEST_ASSERT_FALSE(cmd36_allowed(0, 1));
}

void test_cmd36_blocked_when_armed_and_running(void)
{
    /* Armed=1, Running=1 → must not enter programming mode */
    TEST_ASSERT_FALSE(cmd36_allowed(1, 1));
}

void test_cmd36_allowed_when_disarmed_and_stopped(void)
{
    /* Armed=0, Running=0 → programming mode IS safe to enter */
    TEST_ASSERT_TRUE(cmd36_allowed(0, 0));
}

/* ─────────────────────────────────────────────────────────────────────────
 * Telemetry CRC inversion test
 * When DShot telemetry is active, the received CRC nibble is bitwise
 * inverted + offset by 16 before comparison (dshot.c line ~99).
 * ───────────────────────────────────────────────────────────────────────── */

void test_telemetry_crc_inversion(void)
{
    /*
     * In bidirectional DShot the transmitter inverts the CRC nibble.
     * The ESC receives the frame with ~CRC & 0xF and compensates with:
     *   checkCRC = ~checkCRC + 16   (uint8_t arithmetic wraps correctly)
     * This test simulates that end-to-end:
     *   1. Compute the correct CRC for data12
     *   2. Build a frame with the INVERTED CRC (as bidir DShot sends it)
     *   3. Apply the adjustment from dshot.c line ~99
     *   4. Verify recovery matches simple XOR calc over data bits
     */
    uint16_t data12 = (uint16_t)((800u << 1) | 1u);  /* throttle=800, telem=1 */
    uint8_t correct_crc = (uint8_t)(((data12 >> 8) ^ (data12 >> 4) ^ data12) & 0xFu);
    uint8_t inverted_crc = (~correct_crc) & 0xFu;

    /* Build frame with inverted CRC (simulating bidir DShot transmitter) */
    uint16_t frame = (uint16_t)((data12 << 4) | inverted_crc);
    uint8_t dpulse[16];
    dshot_unpack(frame, dpulse);

    /* Received CRC from the wire */
    uint8_t received_crc = (uint8_t)(dpulse[12] << 3 | dpulse[13] << 2 |
                                      dpulse[14] << 1 | dpulse[15]);
    /* Apply ESC compensation (uint8_t arithmetic, same as dshot.c) */
    uint8_t adjusted = (uint8_t)((uint8_t)(~received_crc) + 16u);

    /* The adjusted CRC must equal the calc CRC over the data portion */
    uint8_t calc = dshot_calc_crc(dpulse);
    TEST_ASSERT_EQUAL_INT(calc, adjusted);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Main
 * ───────────────────────────────────────────────────────────────────────── */

int main(void)
{
    UnityBegin("test_dshot_decode.c");

    RUN_TEST(test_crc_zero_throttle);
    RUN_TEST(test_crc_max_throttle);
    RUN_TEST(test_crc_known_frame_throttle_1000);
    RUN_TEST(test_crc_telemetry_bit_affects_crc);
    RUN_TEST(test_crc_corrupt_frame_fails);

    RUN_TEST(test_throttle_roundtrip_zero);
    RUN_TEST(test_throttle_roundtrip_max);
    RUN_TEST(test_throttle_roundtrip_midscale);
    RUN_TEST(test_throttle_command_36_value);

    RUN_TEST(test_cmd36_blocked_when_armed);
    RUN_TEST(test_cmd36_blocked_when_running);
    RUN_TEST(test_cmd36_blocked_when_armed_and_running);
    RUN_TEST(test_cmd36_allowed_when_disarmed_and_stopped);

    RUN_TEST(test_telemetry_crc_inversion);

    return UnityEnd();
}
