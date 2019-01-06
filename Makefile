#
# Makefile
# =============================================================================
# VIRTual BLocK IO SIMulating (virtblkiosim). Version 0.1
# =============================================================================
# Virtual Linux block device driver for simulating and performing I/O.
#
# (See inner Makefiles to find out actual build rules.)
# =============================================================================
# Copyright (C) 2016-2019 Radislav (Radicchio) Golubtsov
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# (See the LICENSE file at the top of the source tree.)
#

KMOD_BIN   = virtblkiosim.ko
KMOD_DIR   = src
TEST_BIN   = virtblkioctl
TEST_DIR   = tests/ioctl
DOCS_DIR   = docs/doxygen
ALL_TARGET = all
CLN_TARGET = clean

# Specify flags and other vars here.
DOXYGEN    = doxygen

# Using here this user-defined var because "The options ‘-C’, ‘-f’, ‘-o’,
# and ‘-W’ are not put into MAKEFLAGS; these options are not passed down."
# -- from the GNU Make Manual. I.e. use of MAKEFLAGS cannot be applicable
# in this case.
MAKE_FLAGS = -C

RMFLAGS    = -vR

# Making the first target (the driver itself).
$(KMOD_BIN):
	$(MAKE) $(MAKE_FLAGS)$(KMOD_DIR) $(ALL_TARGET)

# Making the second target (the ioctl() testing utility).
$(TEST_BIN):
	$(MAKE) $(MAKE_FLAGS)$(TEST_DIR) $(ALL_TARGET)

# Making the third target (Doxygen-generated docs).
$(DOCS_DIR):
	$(DOXYGEN)

.PHONY: all clean

all: $(KMOD_BIN) $(TEST_BIN) $(DOCS_DIR)

clean:
	$(MAKE) $(MAKE_FLAGS)$(KMOD_DIR) $(CLN_TARGET)
	$(MAKE) $(MAKE_FLAGS)$(TEST_DIR) $(CLN_TARGET)
	$(RM)   $(RMFLAGS)   $(DOCS_DIR)

# vim:set nu ts=4 sw=4:
