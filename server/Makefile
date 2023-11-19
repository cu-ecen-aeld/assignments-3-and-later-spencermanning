# target ... : prerequisites ...
#		recipe
#		...
BUILD_DIR = .
CFLAGS ?= -g -Wall -Werror
# $(CROSS_COMPILE) can be "aarch64-none-linux-gnu-"
CC ?= $(CROSS_COMPILE)gcc $(CFLAGS)
TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt
# TODO: Any INCLUDES are necessary?

# Build for both the aesdsocket.o and aesdsocket dependencies
all: aesdsocket.o $(TARGET)

aesdsocket.o : aesdsocket.c
	$(CC) -c -o aesdsocket.o aesdsocket.c

aesdsocket : aesdsocket.o
	$(CC) aesdsocket.o -o $(TARGET) $(LDFLAGS)
	@echo "------- Successfully built --------"

clean :
	@echo "The main directory is $(BUILD_DIR)"
	rm -rf $(BUILD_DIR)/aesdsocket.o aesdsocket
