#
# src/Makefile
# =============================================================================
# VIRTual BLocK IO SIMulating (virtblkiosim). Version 0.9.9
# =============================================================================
# Virtual Linux block device driver for simulating and performing I/O.
#
# (See outer Makefile to understand how this one is processed.)
# =============================================================================
# Copyright (C) 2016-2022 Radislav (Radicchio) Golubtsov
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
