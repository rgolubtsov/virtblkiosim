# VIRTual BLocK IO SIMulating (virtblkiosim) [![Build Status](https://travis-ci.org/rgolubtsov/virtblkiosim.svg?branch=master)](https://travis-ci.org/rgolubtsov/virtblkiosim)

**Virtual Linux block device driver for simulating and performing I/O**

Despite the fact of existence of various tutorial and referential sources on the Net on how to write a custom block device driver in the form of a loadable kernel module (LKM) for the Linux kernel, they are mostly quite outdated and referred back to somewhat old versions of the Linux kernel. So that the decision to create a simple block device driver LKM suitable (properly working) for recent Linux versions is the goal of this project.

It is created using the way of aggressively utilizing source code comments, even in those places of code where actually there are no needs to emphasize or describe what a particular construct or expression does. But on the other hand, such approach allows one to use these LKM sources as a some kind of tutorial to learn the sequence and calling techniques of the modern Linux LKM API and particularly block device driver API.

The current implementation of a block device driver actually does nothing except it has methods to perform I/O (read/write operations) with blocks of memory of strictly predefined sizes. The process of moving blocks of data to/from a virtual device is simulating the data exchange between a physical storage device and a userland program. This is possible (and is implemented) through the use of the `ioctl()` system call.

## Building

The building process is straightforward: simply `cd` to the `src` directory and `make` the module:

```
$ make
make -C/lib/modules/`uname -r`/build M=$PWD
make[1]: Entering directory '/usr/lib/modules/4.8.13-1-ARCH/build'
  LD      /home/<username>/virtblkiosim/src/built-in.o
  CC [M]  /home/<username>/virtblkiosim/src/virtblkiosim.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/<username>/virtblkiosim/src/virtblkiosim.mod.o
  LD [M]  /home/<username>/virtblkiosim/src/virtblkiosim.ko
make[1]: Leaving directory '/usr/lib/modules/4.8.13-1-ARCH/build'
```

The output above demonstrates building the module using the Linux kernel headers version 4.8.13 (on Arch Linux system). It may vary depending on build tools and/or Linux distribution and kernel version used.

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

To build the module one needs to have installed build tools and Linux kernel headers along with their respective dependencies. (As for the example above, the required package containing Linux kernel headers is `linux-headers 4.8.13-1`).

## Running

It is common and usual decision to play with the block device driver inside a virtual machine. It allows to keep the current development host system in a safe state when something will go wrong with the testing driver, and especially, if it will erroneously touch the running operating system, turning it into an unusable state, freeze or hang it up (worst case). So, first of all it needs to install, run, and set up a Linux distribution inside a VM. Then it needs to build the kernel module for the block device driver there. And finally, insert the module into the running kernel... of course inside a VM.

The following example practically demonstrates how to do all these steps. Chosen VM is QEMU (with KVM), chosen Linux distribution is Ubuntu Server 16.04 LTS x86-64 (Linux kernel 4.4.0).

### Setting up the VM

**Note:** All the commands (and related output) shown below were executed under the aforementioned Arch Linux system (x86-64) used as a host OS.

#### Creating QEMU HDD

First create an HDD image file of size, for example, 20GB. (The other three commands given below are not necessary.)

```
$ qemu-img create -f raw <hdd-image-file> 20G
Formatting '<hdd-image-file>', fmt=raw size=21474836480
$
$ file <hdd-image-file>
<hdd-image-file>: UNIF v0 format NES ROM image
$
$ chmod -v 600 <hdd-image-file>
mode of '<hdd-image-file>' changed from 0644 (rw-r--r--) to 0600 (rw-------)
$
$ ls -al <hdd-image-file>
-rw------- 1 <username> <usergroup> 21474836480 Jun 14 17:37 <hdd-image-file>
```

#### Installing Ubuntu Server

Run VM, install OS:

```
$ qemu-system-x86_64 -m 1G -enable-kvm -cdrom ubuntu-16.04.1-server-amd64.iso -boot order=d -drive file=<hdd-image-file>,format=raw
$
$ file <hdd-image-file>
<hdd-image-file>: DOS/MBR boot sector
```

#### Starting up Ubuntu Server

In order to get access to the VM via SSH, first it needs to properly set up host-guest networking:

```
$ sudo usermod -a -G kvm <username>
$ su <username> -
```

Or simply do relogin. Next:

```
$ groups
wheel kvm <usergroup>    <== Notice the kvm group membership.
$
$ sudo ip tuntap add tap0 mode tap user <username> group <usergroup>
$ sudo ip link set tap0 up
$ sudo ip addr add 10.0.2.1/24 dev tap0
$
$ sudo sysctl -w net.ipv4.ip_forward=1
net.ipv4.ip_forward = 1
$
$ sudo iptables -t nat -A POSTROUTING -s 10.0.2.0/24 -j MASQUERADE
$
$ sudo iptables -t nat -L
...
Chain POSTROUTING (policy ACCEPT)
target     prot opt source               destination
MASQUERADE  all  --  10.0.2.0/24          anywhere
```

~~If there are no any of DHCP clients running &ndash; for instance if the network is configured to utilize static IP addressing &ndash; it needs to run a DHCP client daemon against the `tap0` interface:~~

```
$ sudo dhcpcd -qb tap0  # <== Note: Though this is generally applicable, looks quite redundant and unneeded in this case (see below).
```

Instead, run the following command: `$ sudo ip link set tap0 up` (it is already presented above, just after adding the `tap0` interface). It will bring the interface up.

Next, run the VDE switch to acquire carrier:

```
$ sudo vde_switch -mod 660 -group <usergroup> -tap tap0 -daemon
```

Finally, start up Ubuntu Server:

```
$ qemu-system-x86_64 -m 512M -enable-kvm -cpu host -smp 2 -net nic,model=virtio -net vde -drive file=<hdd-image-file>,format=raw
```

When the guest OS (Ubuntu Server) is up and running, login into it and do the following:

```
$ sudo ip addr add 10.0.2.100/24 dev eth0
$ sudo ip route add default via 10.0.2.1
$ sudo su -
# echo 'nameserver 8.8.8.8' >> /etc/resolv.conf
```

Or, alternatively, put the `utils/vm-network-setup` shell script into the home dir (say, via SCP) of the current VM user and execute it as a superuser:

```
$ pwd
/home/<vmusername>
$
$ sudo ./vm-network-setup
# Dynamic resolv.conf(5) file for glibc resolver(3) generated by resolvconf(8)
#     DO NOT EDIT THIS FILE BY HAND -- YOUR CHANGES WILL BE OVERWRITTEN
nameserver 8.8.8.8
```

Log out from root, log out from the current user. The guest OS is now ready to accept connections from the host OS via SSH.

#### Logging into Ubuntu Server through SSH

```
$ ssh -C <vmusername>@10.0.2.100
Welcome to Ubuntu 16.04.1 LTS (GNU/Linux 4.4.0-57-generic x86_64)
...
<vmusername>@<vmhostname>:~$
```

Now it is time to install and set up additional packages, utilities, development tools, etc. in the guest OS. These steps are not necessary to describe, so that they will be omitted.

### Obtaining block device driver sources

The most obvious way to get block device driver sources inside a running VM is to `git clone` this repository. But if one doesn't plan to use Git for some reason, simply `scp` all the required files from host to guest system:

```
$ cd virtblkiosim
$
$ scp -C src/Makefile src/virtblkiosim.* <vmusername>@10.0.2.100:/home/<vmusername>/virtblkiosim/src
Makefile                      100% 1164    32.3KB/s   00:00
virtblkiosim.c                100%   27KB   2.3MB/s   00:00
virtblkiosim.h                100% 6631     3.7MB/s   00:00
```

**Note:** In this example it is supposed that the `virtblkiosim` directory and the `src` subdirectory inside a VM have already been created manually.

### Building the block device driver module

Nothing tricky &ndash; just as it is stated in the main **Building** section:

```
$ make
make -C/lib/modules/`uname -r`/build M=$PWD
make[1]: Entering directory '/usr/src/linux-headers-4.4.0-57-generic'
  LD      /home/<vmusername>/virtblkiosim/src/built-in.o
  CC [M]  /home/<vmusername>/virtblkiosim/src/virtblkiosim.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/<vmusername>/virtblkiosim/src/virtblkiosim.mod.o
  LD [M]  /home/<vmusername>/virtblkiosim/src/virtblkiosim.ko
make[1]: Leaving directory '/usr/src/linux-headers-4.4.0-57-generic'
```

Again, the block device driver module is the file `virtblkiosim.ko`.

### Inserting the module into the kernel

Before actually inserting the module into the running VM kernel, it is handy to see/know which system modules are currently loaded and how they are reported:

```
$ lsmod
Module                  Size  Used by
ppdev                  20480  0
crct10dif_pclmul       16384  0
crc32_pclmul           16384  0
cryptd                 20480  0
joydev                 20480  0
input_leds             16384  0
serio_raw              16384  0
i2c_piix4              24576  0
8250_fintek            16384  0
mac_hid                16384  0
parport_pc             32768  1
lp                     20480  0
parport                49152  3 lp,ppdev,parport_pc
autofs4                40960  2
psmouse               126976  0
floppy                 73728  0
pata_acpi              16384  0
$
$ lsblk
NAME   MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT
fd0      2:0    1    4K  0 disk
sda      8:0    0   20G  0 disk
└─sda1   8:1    0   20G  0 part /
sr0     11:0    1 1024M  0 rom
$
$ free
              total        used        free      shared  buff/cache   available
Mem:         499988       36276       56600        6856      407112      432988
Swap:             0           0           0
```

**Insert the module into the running kernel:**

**Variant (a):** Use the `insmod` command to insert the module from the working (current) directory where it is actually placed just after build:

```
$ sudo insmod virtblkiosim.ko
```

**Variant (b):** Use the `modprobe` command to insert the module registered in the kernel modules configuration database (first register it):

```
$ sudo cp -v virtblkiosim.ko /lib/modules/4.4.0-57-generic/kernel/drivers/block
'virtblkiosim.ko' -> '/lib/modules/4.4.0-57-generic/kernel/drivers/block/virtblkiosim.ko'
$
$ sudo depmod
$
$ sudo modprobe virtblkiosim
```

Once the module is registered (see `depmod` above) &ndash; not necessarily it is loaded &ndash; it might be checked for its metadata:

```
$ modinfo virtblkiosim
filename:       /lib/modules/4.4.0-57-generic/kernel/drivers/block/virtblkiosim.ko
license:        GPL
author:         Radislav (Radicchio) Golubtsov <radicchio@vk.com>
version:        0.1
description:    Virtual Linux block device driver for simulating and performing I/O
srcversion:     0E8954471AD2C23E84BFAB6
depends:
vermagic:       4.4.0-57-generic SMP mod_unload modversions
```

After that examine what has changed:

```
$ lsmod
Module                  Size  Used by
virtblkiosim        33574912  0                   <== The new kernel module
ppdev                  20480  0
crct10dif_pclmul       16384  0
crc32_pclmul           16384  0
...
$
$ lsblk
NAME         MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT
fd0            2:0    1    4K  0 disk
sda            8:0    0   20G  0 disk
└─sda1         8:1    0   20G  0 part /
sr0           11:0    1 1024M  0 rom
virtblkiosim 251:0    0   32M  0 disk             <== The new block device
$
$ free
              total        used        free      shared  buff/cache   available
Mem:         499988       69740       22160        6860      408088      399508
Swap:             0           0           0
```

Notice the new kernel module (`virtblkiosim`) has appeared in the list of loaded modules, and also there is the new block device with the same name has appeared in the list of active block devices. In addition, a significant memory usage change is clearly visible: `69740 - 36276 = 33464` kilobytes. This is almost the same size as what is shown in the list of loaded modules for this module. (An additional tiny amount of memory is consumed by other system activities.)

Once the module is loaded, it is registered and available as the special block device file:

```
$ ls -al /dev/virtblkiosim
brw-rw---- 1 root disk 251, 0 Nov  4 07:07 /dev/virtblkiosim
```

All further communications with the new block device and operations on controlling it will be performed by accessing this file.

The module is designed to log informational messages of what it is doing and debug/error messages to the kernel log. Right after the command to insert it into the kernel is issued, it starts writing to the log:

```
$ tailf /var/log/kern.log
...
Nov  4 07:07:53 <vmhostname> kernel: [687996.897544] virtblkiosim: Virtual Linux block device driver for simulating and performing I/O, Version 0.1
Nov  4 07:07:53 <vmhostname> kernel: [687996.897544] virtblkiosim: Copyright (C) 2016-2021 Radislav (Radicchio) Golubtsov <radicchio@vk.com>
Nov  4 07:07:53 <vmhostname> kernel: [687996.897550] virtblkiosim: Device registered with major number of 251
Nov  4 07:07:53 <vmhostname> kernel: [687996.899753] virtblkiosim: Device engage: private_data: virtblkiosim
Nov  4 07:07:53 <vmhostname> kernel: [687996.899873] virtblkiosim: Device release: private_data: virtblkiosim
Nov  4 07:07:53 <vmhostname> kernel: [687996.900657] virtblkiosim: Device engage: private_data: virtblkiosim
Nov  4 07:07:53 <vmhostname> kernel: [687996.901143] virtblkiosim: Device release: private_data: virtblkiosim
```

Each module message in the log is prepended with the module name (`virtblkiosim`) to easily `grep` on them.

### Removing the module from the kernel

To **remove the module from the running kernel**, execute one of the following two commands: `rmmod` or `modprobe -r`:

```
$ sudo rmmod virtblkiosim
```

or

```
$ sudo modprobe -r virtblkiosim
```

Check the kernel log after that:

```
$ tailf /var/log/kern.log
...
Nov  4 07:38:23 <vmhostname> kernel: [689826.504388] virtblkiosim: Removing module...
Nov  4 07:38:23 <vmhostname> kernel: [689826.530682] virtblkiosim: Device unregistered and removed from the system
```

Also notice that the special block device file has been deleted during this operation:

```
$ ls -al /dev/virtblkiosim
ls: cannot access '/dev/virtblkiosim': No such file or directory
```

After that the block device `virtblkiosim` becomes absent, hence it is no longer known to the kernel.
