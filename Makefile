# Copyright 2017 Julian Ingram
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

DEFINES :=

CC := clang
AR := ar
CFLAGS += -g3 -O0 -Werror -Wall -Wextra $(DEFINES:%=-D%)
# expanded below
DEPFLAGS = -MMD -MP -MF $(@:$(BUILD_DIR)/%.o=$(DEP_DIR)/%.d)
LDFLAGS := -g3 -O0
SRCS := fetx.c fetx_io.c fetx_vector.c fetx_netlist.c
TEST_DIR := tests
TEST_SRCS := $(SRCS) $(TEST_DIR)/fetx_test.c
EXAMPLE_DIR := examples
EXAMPLE_SRCS := $(SRCS) $(EXAMPLE_DIR)/fetx_example.c
# sort removes duplicates
ALL_SRCS := $(sort $(SRCS) $(TEST_SRCS) $(EXAMPLE_SRCS))
BIN_DIR ?= bin
TARGET ?= $(BIN_DIR)/libfetx.a
TEST ?= $(BIN_DIR)/fetx_test
EXAMPLE ?= $(BIN_DIR)/fetx_example
RM := rm -rf
MKDIR := mkdir -p
CP := cp -r
# BUILD_DIR and DEP_DIR should both have non-empty values
BUILD_DIR ?= build
DEP_DIR ?= $(BUILD_DIR)/deps
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_OBJS := $(TEST_SRCS:%.c=$(BUILD_DIR)/%.o)
EXAMPLE_OBJS := $(EXAMPLE_SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(ALL_SRCS:%.c=$(DEP_DIR)/%.d)

.PHONY: all
all: $(TARGET)

# link lib
$(TARGET): $(OBJS)
	$(if $(BIN_DIR),$(MKDIR) $(BIN_DIR),)
	$(AR) -rcsD $@ $^

.PHONY: test
test: $(TEST)
	./$(BIN_DIR)/fetx_test netlists/inverter.nl vectors/inverter_test.vct 10
	./$(BIN_DIR)/fetx_test netlists/nand.nl vectors/nand_test.vct 100
	./$(BIN_DIR)/fetx_test netlists/xor_tg.nl vectors/xor_tg_test.vct 100
	./$(BIN_DIR)/fetx_test netlists/srlatch.nl vectors/srlatch_test.vct 100
	./$(BIN_DIR)/fetx_test netlists/flipflop.nl vectors/flipflop_test.vct 100 2
	./$(BIN_DIR)/fetx_test netlists/alu.nl vectors/alu_test.vct 1000
	./$(BIN_DIR)/fetx_test netlists/loop.nl vectors/loop_test.vct 10 1
	./$(BIN_DIR)/fetx_test netlists/dffl.nl vectors/dffl_test.vct 100

# link test
$(TEST): $(TEST_OBJS)
	$(if $(BIN_DIR),$(MKDIR) $(BIN_DIR),)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: example
example: $(EXAMPLE)

# link examples
$(EXAMPLE): $(EXAMPLE_OBJS)
	$(if $(BIN_DIR),$(MKDIR) $(BIN_DIR),)
	$(CC) -o $@ $^ $(LDFLAGS)

# compile and/or generate dep files
$(BUILD_DIR)/%.o: %.c
	$(MKDIR) $(BUILD_DIR)/$(dir $<)
	$(MKDIR) $(DEP_DIR)/$(dir $<)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) $(TARGET) $(TEST) $(EXAMPLE) $(BIN_DIR) $(DEP_DIR) $(BUILD_DIR)

-include $(DEPS)
