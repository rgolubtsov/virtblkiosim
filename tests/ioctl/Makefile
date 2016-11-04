#
# tests/ioctl/Makefile
# =============================================================================
# VIRTual BLocK IO SIMulating (virtblkiosim). Version 0.1
# =============================================================================
# Virtual Linux block device driver for simulating and performing I/O.
#
# This utility tests block device I/O through the ioctl() system call.
# =============================================================================
# Copyright (C) 2016 Radislav (Radicchio) Golubtsov
#

EXEC = virtblkioctl
DEPS = $(EXEC).o

# Specify flags and other vars here.
# Note: To use the system default C compiler (likely gcc, the GNU C Compiler)
#       instead of tcc (Tiny C Compiler), simply comment out the next line,
#       just like this: "#CC = tcc" - and it will be used automatically.
#CC = tcc

# The following standards-compliance options work well with gcc 6.2.1.
#C_STD = c90 # Same as c89, same as -ansi .
C_STD = c99
#C_STD = c11

# Note: The -std=XXX option is not allowed when using tcc.
CFLAGS = -Wall -pedantic -O3 -std=$(C_STD)

RMFLAGS = -v

# Making the target.
$(DEPS): %.o: %.c
$(EXEC): $(DEPS)

.PHONY: all clean

all: $(EXEC)

clean:
	$(RM) $(RMFLAGS) $(EXEC) $(DEPS)

# vim:set nu:et:ts=4:sw=4:
