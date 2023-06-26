# target ... : prerequisites ...
#		recipe
#		...
BUILD_DIR = .
CFLAGS = -g -Wall -Werror
# $(CROSS_COMPILE) can be "aarch64-none-linux-gnu-"
CC = $(CROSS_COMPILE)gcc $(CFLAGS)

# Build for both the aesdsocket.o and aesdsocket dependencies
all: aesdsocket.o aesdsocket

aesdsocket.o : aesdsocket.c
	$(CC) -c -o aesdsocket.o aesdsocket.c

aesdsocket : aesdsocket.o
	$(CC) aesdsocket.o -o aesdsocket
	@echo "------- Successfully built --------"

clean :
	@echo "The main directory is $(BUILD_DIR)"
	rm -rf $(BUILD_DIR)/aesdsocket.o aesdsocket
