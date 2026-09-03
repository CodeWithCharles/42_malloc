# ---------------------------------------------------------------------------- #
#                                  Unit tests                                  #
# ---------------------------------------------------------------------------- #

CC      = gcc
AR      = ar rcs
BUILD   = build
CFLAGS  = -Wall -Wextra -g3 -O0 -MMD -DFTM_DEBUG -pthread -I../include
CORE_A  = $(BUILD)/libftm_core.a

SRC_CORE = $(wildcard ../src/core/*.c)
SRC_PORT = ../src/port/ftm_lock_pthread.c
SRC_FAKE = fake_port.c
OBJ_A    = $(patsubst %.c,$(BUILD)/%.o,$(notdir $(SRC_CORE)) $(notdir $(SRC_PORT)) $(SRC_FAKE))

SRC_TEST = $(wildcard test_*.c)
BIN_TEST = $(patsubst %.c,$(BUILD)/%,$(SRC_TEST))
DEPS     = $(OBJ_A:.o=.d) $(addsuffix .d,$(BIN_TEST))

vpath %.c ../src/core ../src/port

all: $(BIN_TEST) run

$(CORE_A): $(OBJ_A)
	@$(AR) $@ $^

$(BUILD)/%.o: %.c
	@mkdir -p $(BUILD)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%: %.c $(CORE_A)
	@mkdir -p $(BUILD)
	@$(CC) $(CFLAGS) $< $(CORE_A) -o $@

run: $(BIN_TEST)
	@./run_tests.sh

clean:
	@rm -rf $(BUILD)

-include $(DEPS)

.PHONY: all run clean