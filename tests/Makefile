# AM32 Host Unit Test Build System
# Builds and runs portable unit tests against the host GCC (no ARM toolchain required).
# Tests cover DShot frame parsing, CRC-16, and command guard logic.

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Werror -g -I.
UNITY   = unity/unity.c

TESTS   = test_dshot_decode test_crc

.PHONY: all run clean

all: $(TESTS)

test_dshot_decode: test_dshot_decode.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

test_crc: test_crc.c $(UNITY)
	$(CC) $(CFLAGS) -o $@ $^

# Run all tests; collect exit codes.
# A non-zero exit from any test binary causes the CI job to fail.
run: all
	@PASS=0; FAIL=0; \
	for t in $(TESTS); do \
	    echo ""; \
	    echo "=== Running $$t ==="; \
	    ./$$t; \
	    if [ $$? -eq 0 ]; then PASS=$$((PASS+1)); else FAIL=$$((FAIL+1)); fi; \
	done; \
	echo ""; \
	echo "=== Test Suite Summary: $$PASS passed, $$FAIL failed ==="; \
	[ $$FAIL -eq 0 ]

clean:
	rm -f $(TESTS)
