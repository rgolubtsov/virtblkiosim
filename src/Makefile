#
# src/Makefile
# =============================================================================
# VIRTual BLocK IO SIMulating (virtblkiosim). Version 0.1
# =============================================================================
# Virtual Linux block device driver for simulating and performing I/O.
#
# (See outer Makefile to understand how this one is processed.)
# =============================================================================
# Copyright (C) 2016-2021 Radislav (Radicchio) Golubtsov
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

# Suppress module signature verification: Get rid of kernel messages like
# >> module verification failed: signature and/or required key missing -
# >> tainting kernel
CONFIG_MODULE_SIG=n

KMOD = virtblkiosim
TMPS =  $(KMOD).mod.*  \
       .$(KMOD).*.cmd  \
        built-in.o     \
       .built-in.*     \
        modules.order  \
        Module.symvers \
       .tmp_versions

ifneq ($(KERNELRELEASE),)
    obj-m = $(KMOD).o
else
    MAKE_FLAGS = -C
    KDIR      ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) $(MAKE_FLAGS)$(KDIR) M=$$PWD
endif

# Specify flags and other vars here.
RMFLAGS = -vR

.PHONY: all clean

all: default

clean:
	$(RM) $(RMFLAGS) $(KMOD).ko $(KMOD).o $(TMPS)

# vim:set nu ts=4 sw=4:
