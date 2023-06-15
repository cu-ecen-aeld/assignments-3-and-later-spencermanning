# target ... : prerequisites ...
#		recipe
#		...
BUILD_DIR = .
CFLAGS = -g -Wall
# $(CROSS_COMPILE) can be "aarch64-none-linux-gnu-"
CC = $(CROSS_COMPILE)gcc $(CFLAGS)

# Build for both the aesdsocket.o and aesdsocket dependencies
all: aesdsocket.o aesdsocket

aesdsocket.o : aesdsocket.c
	$(CC) -c -o aesdsocket.o aesdsocket.c

aesdsocket:
	$(CC) aesdsocket.o -o aesdsocket

clean :
	$(info The main directory is $(BUILD_DIR))
	rm -rf $(BUILD_DIR)/aesdsocket.o aesdsocket
