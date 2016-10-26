# VIRTual BLocK IO SIMulating (virtblkiosim)
**Virtual Linux block device driver for simulating and performing I/O**

Despite the fact of existence of various tutorial and referential sources on the Net on how to write a custom block device driver in the form of a loadable kernel module (LKM) for the Linux kernel, they are mostly quite outdated and referred back to somewhat old versions of the Linux kernel. So that the decision to create a simple block device driver LKM suitable (properly working) for recent Linux versions is the goal of this project.

It is created using the way of aggressively utilizing source code comments, even in those places of code where actually there are no needs to emphasize or describe what a particular construct or expression does. But on the other hand, such approach allows one to use these LKM sources as a some kind of tutorial to learn the sequence and calling techniques of the modern Linux LKM API and particularly block device driver API.

The current implementation of a block device driver actually does nothing except it has methods to perform I/O (read/write operations) with blocks of memory of strictly predefined sizes. The process of moving blocks of data to/from a virtual device is simulating the data exchange between a physical storage device and a userland program. This is possible (and is implemented) through the use of the `ioctl()` system call.

## Building

The building process is straightforward: simply `cd` to the `src` directory and `make` the module:

```
$ make
make -C/lib/modules/`uname -r`/build M=$PWD
make[1]: Entering directory '/usr/lib/modules/4.7.6-1-ARCH/build'
  LD      /home/<username>/virtblkiosim/src/built-in.o
  CC [M]  /home/<username>/virtblkiosim/src/virtblkiosim.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/<username>/virtblkiosim/src/virtblkiosim.mod.o
  LD [M]  /home/<username>/virtblkiosim/src/virtblkiosim.ko
make[1]: Leaving directory '/usr/lib/modules/4.7.6-1-ARCH/build'
```

The output above demonstrates building the module using the Linux kernel headers version 4.7.6 (on Arch Linux system). It may vary depending on build tools and/or Linux distribution and kernel version used.

The finally built module, amongst other files produced is `virtblkiosim.ko`. To see what is it, check it:

```
$ file virtblkiosim.ko
virtblkiosim.ko: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), BuildID[sha1]=88b3fb4e28410614db58548edbe2e7b48bbcf51c, not stripped
```

This module then should be inserted (loaded) into the running kernel through usual `insmod` or `modprobe` commands (see the **Running** section).

To cleanup the working directory (`src`), run `make` with the `clean` target:

```
$ make clean
rm -f -vR virtblkiosim.ko virtblkiosim.o virtblkiosim.mod.* .virtblkiosim.*.cmd built-in.o .built-in.* modules.order Module.symvers .tmp_versions
removed 'virtblkiosim.ko'
removed 'virtblkiosim.o'
removed 'virtblkiosim.mod.c'
removed 'virtblkiosim.mod.o'
removed '.virtblkiosim.ko.cmd'
removed '.virtblkiosim.mod.o.cmd'
removed '.virtblkiosim.o.cmd'
removed 'built-in.o'
removed '.built-in.o.cmd'
removed 'modules.order'
removed 'Module.symvers'
removed '.tmp_versions/virtblkiosim.mod'
removed directory '.tmp_versions'
```

Make changes and build again :-))).

## Dependencies

To build the module one needs to have installed build tools and Linux kernel headers along with their respective dependencies. (As for the example above, the required package containing Linux kernel headers is `linux-headers 4.7.6-1`).
