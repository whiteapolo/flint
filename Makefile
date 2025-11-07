CC := cc
CFLAGS := -Wall -Wextra -O3 -Wno-unused-result
LIBS := -lreadline -lm
SRC_DIR := src
OBJ_DIR := obj
SRC := $(shell find $(SRC_DIR) -name '*.c')
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
BIN := exe

all: $(BIN)

$(BIN): $(OBJ)
	@echo "Linking $@"
	@$(CC) $(OBJ) -o $@ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR) $(BIN)
	@echo "Cleaned."

.PHONY: all clean
