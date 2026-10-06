CC       = gcc
AR       = ar
CFLAGS   = -std=c11 -Wall -Wextra -Wpedantic -O2 -g
CPPFLAGS = -Iinclude
BUILD    = build

ifeq ($(OS),Windows_NT)
EXE = .exe
endif

SRCS    = $(wildcard src/*.c)
HDRS    = $(wildcard include/*.h)
OBJS    = $(patsubst src/%.c,$(BUILD)/obj/%.o,$(SRCS))
LIB     = $(BUILD)/libds.a
TESTS   = $(patsubst tests/%.c,$(BUILD)/%$(EXE),$(wildcard tests/test_*.c))
EXAMPLE = $(BUILD)/word_freq$(EXE)

.PHONY: all test run asan clean

all: $(LIB) $(TESTS) $(EXAMPLE)

$(BUILD)/obj:
	mkdir -p $@

$(BUILD)/obj/%.o: src/%.c $(HDRS) | $(BUILD)/obj
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(BUILD)/test_%$(EXE): tests/test_%.c tests/test.h $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LIB)

$(EXAMPLE): examples/word_freq.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LIB)

test: $(TESTS)
	@for t in $(TESTS); do ./$$t || exit 1; echo; done

run: $(EXAMPLE)
	./$(EXAMPLE) examples/sample.txt

# Linux/macOS only: rebuild everything with AddressSanitizer + UBSan.
asan: clean
	$(MAKE) test CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer"

clean:
	rm -rf $(BUILD)
