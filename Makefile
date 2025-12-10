CC = gcc
CFLAGS = -std=c99 -Isrc -Ivendor -g -Wno-unused-function
LDFLAGS = -lX11 -lm

# Source files
SRC = src/main.c \
      src/core/config.c \
      src/core/plugin_registry.c \
      src/core/time_utils.c \
      src/platform/platform.c \
      src/plugins/stopwatch.c \
      src/ui/clock_ui.c \
      vendor/ini.c

# Object files
OBJ = $(SRC:.c=.o)

# Binary name
TARGET = digitalclock

UNAME ?= $(shell uname)

LDFLAGS = -lm
ifeq ($(UNAME), Linux)
	LDFLAGS += -lX11
endif

# If cross-compiling or on Windows (often OS is set)
ifdef OS
	ifneq ($(findstring Windows,$(OS)),)
		LDFLAGS = -lgdi32 -luser32 -lkernel32 -lm
	endif
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC:.c=.o) $(TARGET)