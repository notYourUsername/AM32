/*
 * test_crc.c — Unit tests for EEPROM CRC-16/CCITT-FALSE (T1.4 stub)
 *
 * The production CRC-16 implementation does not yet exist (it's scheduled
 * as Tier-1 task T1.4).  These tests define the EXPECTED BEHAVIOUR that
 * the implementation must satisfy when it is written, acting as a
 * test-driven specification.
 *
 * Algorithm: CRC-16/CCITT-FALSE
 *   Poly  = 0x1021
 *   Init  = 0xFFFF
 *   RefIn = false, RefOut = false, XorOut = 0x0000
 *
 * Reference vectors from https://crccalc.com/ and the ECMA-182 standard.
 */

#include "unity/unity.h"
#include <stdint.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────────────────
 * CRC-16/CCITT-FALSE implementation
 * This is the reference implementation; T1.4 will add this (or equivalent)
 * to the production codebase and protect EEPROM settings with it.
 * ───────────────────────────────────────────────────────────────────────── */

static uint16_t crc16_ccitt(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* ─────────────────────────────────────────────────────────────────────────
 * Setup / teardown
 * ───────────────────────────────────────────────────────────────────────── */

void setUp(void) {}
void tearDown(void) {}

/* ─────────────────────────────────────────────────────────────────────────
 * Tests
 * ───────────────────────────────────────────────────────────────────────── */

void test_crc16_empty_buffer(void)
{
    /* CRC of empty data must return init value 0xFFFF */
    uint16_t result = crc16_ccitt(NULL, 0);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, result);
}

void test_crc16_single_zero_byte(void)
{
    /* CRC-16/CCITT-FALSE of 0x00 = 0xE1F0 (verified against crccalc.com) */
    uint8_t data[] = { 0x00 };
    uint16_t result = crc16_ccitt(data, 1);
    TEST_ASSERT_EQUAL_HEX16(0xE1F0u, result);
}

void test_crc16_single_0xFF_byte(void)
{
    /* CRC-16/CCITT-FALSE of 0xFF:
     * data=0xFF: crc=0xFFFF, crc^=(0xFF<<8)=0x00FF, then 8 shifts with MSB=0
     * → 0xFF00 = 65280
     */
    uint8_t data[] = { 0xFF };
    uint16_t result = crc16_ccitt(data, 1);
    TEST_ASSERT_EQUAL_HEX16(0xFF00u, result);
}

void test_crc16_known_ascii_string(void)
{
    /* CRC-16/CCITT-FALSE of "123456789" = 0x29B1 (ECMA-182 standard vector) */
    const uint8_t* data = (const uint8_t*)"123456789";
    uint16_t result = crc16_ccitt(data, 9);
    TEST_ASSERT_EQUAL_HEX16(0x29B1u, result);
}

void test_crc16_all_zeros_16bytes(void)
{
    /* CRC-16/CCITT-FALSE of 16 zero bytes.
     * Computed value is 0x6A0A — verified by the algorithm that correctly
     * produces the standard 0x29B1 check value for "123456789".
     */
    uint8_t data[16];
    memset(data, 0, sizeof(data));
    uint16_t result = crc16_ccitt(data, sizeof(data));
    TEST_ASSERT_EQUAL_HEX16(0x6A0Au, result);
}

void test_crc16_same_data_same_result(void)
{
    /* Determinism: same input must always give same CRC */
    uint8_t data[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    uint16_t r1 = crc16_ccitt(data, sizeof(data));
    uint16_t r2 = crc16_ccitt(data, sizeof(data));
    TEST_ASSERT_EQUAL_HEX16(r1, r2);
}

void test_crc16_different_data_different_result(void)
{
    /* Collision resistance: changing one byte must change the CRC */
    uint8_t d1[] = { 0x01, 0x02, 0x03, 0x04 };
    uint8_t d2[] = { 0x01, 0x02, 0x03, 0x05 };  /* last byte differs */
    uint16_t r1 = crc16_ccitt(d1, sizeof(d1));
    uint16_t r2 = crc16_ccitt(d2, sizeof(d2));
    TEST_ASSERT_TRUE(r1 != r2);
}

void test_crc16_length_sensitivity(void)
{
    /* Adding a byte must change the CRC (length matters) */
    uint8_t data[] = { 0xAB, 0xCD };
    uint16_t r1 = crc16_ccitt(data, 1);
    uint16_t r2 = crc16_ccitt(data, 2);
    TEST_ASSERT_TRUE(r1 != r2);
}

void test_crc16_eeprom_settings_roundtrip(void)
{
    /*
     * Simulate T1.4 EEPROM CRC workflow:
     *   1. Compute CRC over a settings buffer
     *   2. Append CRC to buffer
     *   3. Re-compute over buffer+CRC — result must be zero (or known residue)
     *
     * For CRC-16/CCITT-FALSE the residue after appending the CRC (big-endian)
     * is 0x0000.  This validates that the saved + verified pattern works
     * correctly for EEPROM integrity checking.
     */
    uint8_t settings[16];
    uint8_t storage[18];     /* settings + 2-byte CRC */

    /* Fill settings with a realistic pattern */
    for (size_t i = 0; i < sizeof(settings); i++)
        settings[i] = (uint8_t)(0x10 + i);

    uint16_t crc = crc16_ccitt(settings, sizeof(settings));

    memcpy(storage, settings, sizeof(settings));
    storage[16] = (uint8_t)(crc >> 8);    /* CRC high byte */
    storage[17] = (uint8_t)(crc & 0xFF);  /* CRC low  byte */

    /* Re-running over the full buffer+CRC must give zero residue */
    uint16_t residue = crc16_ccitt(storage, sizeof(storage));
    TEST_ASSERT_EQUAL_HEX16(0x0000u, residue);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Main
 * ───────────────────────────────────────────────────────────────────────── */

int main(void)
{
    UnityBegin("test_crc.c");

    RUN_TEST(test_crc16_empty_buffer);
    RUN_TEST(test_crc16_single_zero_byte);
    RUN_TEST(test_crc16_single_0xFF_byte);
    RUN_TEST(test_crc16_known_ascii_string);
    RUN_TEST(test_crc16_all_zeros_16bytes);
    RUN_TEST(test_crc16_same_data_same_result);
    RUN_TEST(test_crc16_different_data_different_result);
    RUN_TEST(test_crc16_length_sensitivity);
    RUN_TEST(test_crc16_eeprom_settings_roundtrip);

    return UnityEnd();
}
