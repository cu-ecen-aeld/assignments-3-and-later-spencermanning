# target ... : prerequisites ...
#		recipe
#		...
BUILD_DIR = .
CFLAGS = -g -Wall
# $(CROSS_COMPILE) can be "aarch64-none-linux-gnu-"
CC = $(CROSS_COMPILE)gcc $(CFLAGS)

# Build for both the writer.o and writer dependencies
all: writer.o writer

writer.o : writer.c
	$(CC) -c -o writer.o writer.c

writer:
	$(CC) writer.o -o writer

clean :
	$(info The main directory is $(BUILD_DIR))
	rm -rf $(BUILD_DIR)/writer.o writer
