# target ... : prerequisites ...
#		recipe
#		...
BUILD_DIR = .
CFLAGS ?= -g -Wall -Werror
# $(CROSS_COMPILE) can be "aarch64-none-linux-gnu-" when cross-compiling
# Only for testing the cross compilation:
# CROSS_COMPILE = aarch64-none-linux-gnu-

# FIXME: Will "=" break when this is tested automatically?
# Must be "=" and not "?=" if I want to test this manually AND HAVE DEBUG INFO COMPILED
CC ?= $(CROSS_COMPILE)gcc $(CFLAGS)

TARGET ?= aesdsocket
LDFLAGS ?= -lpthread -lrt
# TODO: Any INCLUDES are necessary?

# Build for both the aesdsocket.o and aesdsocket dependencies
all: aesdsocket.o $(TARGET)

aesdsocket.o : aesdsocket.c
	@echo "Cross compile is $(CROSS_COMPILE)"...
	$(CC) -c -o aesdsocket.o aesdsocket.c

aesdsocket : aesdsocket.o
	$(CC) aesdsocket.o -o $(TARGET) $(LDFLAGS)
	@echo "------- Successfully built --------"

clean :
	@echo "The main directory is $(BUILD_DIR)"
	rm -rf $(BUILD_DIR)/aesdsocket.o aesdsocket
