# Use this makefile to just build the library (.a) file for aesd-circular-buffer
# to confirm that it builds successfully. 
# NOTE: I don't think this will be used for the actual assignment

CFLAGS ?= -g -Wall -Werror
CC ?= $(CROSS_COMPILE)gcc $(CFLAGS)
SOURCES = aesd-circular-buffer.c
OBJECTS = $(SOURCES:.c=.o)
LIBRARY = libmylibrary.a

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $(LIBRARY) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(LIBRARY)