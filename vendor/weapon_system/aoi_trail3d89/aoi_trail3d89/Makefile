CC ?= gcc
CFLAGS ?= -std=c89 -pedantic -Wall -Wextra -Werror -Iinclude
BUILD_DIR = build
LIB_OBJS = $(BUILD_DIR)/trail3d89.o $(BUILD_DIR)/trail3d89_profiles.o

all: check demo provider_demo

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/trail3d89.o: src/trail3d89.c include/trail3d89.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/trail3d89.c -o $(BUILD_DIR)/trail3d89.o

$(BUILD_DIR)/trail3d89_profiles.o: src/trail3d89_profiles.c include/trail3d89_profiles.h include/trail3d89.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/trail3d89_profiles.c -o $(BUILD_DIR)/trail3d89_profiles.o

$(BUILD_DIR)/test_trail3d89: tests/test_trail3d89.c $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_trail3d89.c $(LIB_OBJS) -o $(BUILD_DIR)/test_trail3d89

$(BUILD_DIR)/trail3d89_demo: samples/trail3d89_demo.c $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) samples/trail3d89_demo.c $(LIB_OBJS) -o $(BUILD_DIR)/trail3d89_demo

$(BUILD_DIR)/trail3d89_provider_demo: samples/trail3d89_provider_demo.c $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) samples/trail3d89_provider_demo.c $(LIB_OBJS) -o $(BUILD_DIR)/trail3d89_provider_demo

check: $(BUILD_DIR)/test_trail3d89
	./$(BUILD_DIR)/test_trail3d89

demo: $(BUILD_DIR)/trail3d89_demo
	./$(BUILD_DIR)/trail3d89_demo

provider_demo: $(BUILD_DIR)/trail3d89_provider_demo
	./$(BUILD_DIR)/trail3d89_provider_demo

clean:
	rm -rf $(BUILD_DIR)
