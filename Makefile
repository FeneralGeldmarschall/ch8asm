CC := g++
CFLAGS := -Wall -Werror
CLINKS := -lstdc++
DEBUGFLAGS := -g

EXEC := ch8asm.x
DEBUG := debug.x

SRC_DIR := ./src
BUILD_DIR := ./build

HEADERS := $(wildcard $(SRC_DIR)/*.h)
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(subst $(SRC_DIR),$(BUILD_DIR),$(subst .cpp,.o, $(SOURCES)))

.PHONY: debug all
debug: CFLAGS += $(DEBUGFLAGS)
debug: EXEC=$(DEBUG)
debug: all

# all: $(EXEC)
all: $(SOURCES)
	gcc -o $(EXEC) $^ $(CFLAGS) ${CLINKS}

$(EXEC): $(OBJECTS)
	$(CC) -o $(EXEC) $(CFLAGS) $^

$(BUILD_DIR)/%.o:$(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD_DIR)/main.o:$(SRC_DIR)/main.c $(HEADERS)
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean
clean: $(EXEC)
	rm -rf $(BUILD_DIR)/*

