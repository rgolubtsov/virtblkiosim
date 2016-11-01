/*
 * src/virtblkiosim.h
 * ============================================================================
 * VIRTual BLocK IO SIMulating (virtblkiosim). Version 0.1
 * ============================================================================
 * Virtual Linux block device driver for simulating and performing I/O.
 * ============================================================================
 * Copyright (C) 2016 Radislav (Radicchio) Golubtsov
 */

#ifndef __LINUX__VIRTBLKIOSIM_H
#define __LINUX__VIRTBLKIOSIM_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/blkdev.h>

/** Helper constants. */
#define  EXIT_FAILURE        1 /*    Failing exit status. */
#define  EXIT_SUCCESS        0 /* Successful exit status. */
#define _EMPTY_STRING       ""
#define _ONE_SPACE_STRING  " "
#define _COLON_SPACE_SEP  ": "
#define _COMMA_SPACE_SEP  ", "
#define _NEW_LINE         "\n"

/** Module signature identifying constants. */
#define _MODULE_NAME        "virtblkiosim"
#define _MODULE_DESCRIPTION \
         "Virtual Linux block device driver for simulating and performing I/O"
#define _MODULE_VERSION_S__ "Version"
#define _MODULE_VERSION     "0.1"
#define _MODULE_COPYRIGHT__ "Copyright (C) 2016"
#define _MODULE_AUTHOR      "Radislav Golubtsov <ragolubtsov@my.com>"
#define _MODULE_LICENSE     "GPL"

/** Constant. Print this when registering the device failed. */
#define _REGISTER_DEVICE_FAILED_ERR "Failed to register device"

/** Constant. Print this when registering the device succeeded. */
#define _REGISTER_DEVICE_SUCCEED_MSG \
         "Device registered with major number of %d"

/** Constant. Print this when allocating the request queue failed. */
#define _ALLOCATE_REQ_QU_FAILED_ERR "Failed to allocate request queue"

/** Constant. Print this when allocating the device structure failed. */
#define _ALLOCATE_DEVICE_STRUCT_FAILED_ERR \
         "Failed to allocate device structure"

/** Constant. Print this when unable to copy some data to user space. */
#define _COPY_TO_USER_DEAD_BYTES_EXIST_ERR \
         "Cannot copy %lu byte(s) to user space"

/** Constant. Print this when unable to copy some data to kernel space. */
#define _COPY_FROM_USER_DEAD_BYTES_EXIST_ERR \
         "Cannot copy %lu byte(s) to kernel space"

/**
 * Constant. Print this when <bio> segment data does not match
 *           with request data.
 */
#define _BIO_DOES_NOT_MATCH_REQUEST_ERR \
         "<bio> segment data does not match with request data"

/**
 * Constant. Print this when the device capacity reached
 *           during a sequential read try.
 */
#define _READ_CAPACITY_REACHED_MSG "Device read capacity reached"

/**
 * Constant. Print this when the device capacity reached
 *           during a sequential write try.
 */
#define _WRITE_CAPACITY_REACHED_MSG "Device write capacity reached"

/** Constant. Print this when trying to remove the device driver module. */
#define _REMOVE_MODULE_MSG "Removing module..."

/** Constant. Print this when */
#define _UNREGISTER_AND_REMOVE_MODULE_DONE_MSG \
         "Device unregistered and removed from the system"

/** Constant. The device name as it appears in <code>/proc/devices</code>. */
#define DEVICE_NAME _MODULE_NAME

/**
 * Constant. The device major number trial to get a one
 *           of actual major numbers.
 */
#define DEVICE_MAJOR_NUM_TRIAL 0

/**
 * Constant. The max sectors limit for a request for the request queue
 *           in 512-byte units.
 */
#define DEVICE_REQ_QU_MAX_HW_SECTORS 1024

/** Constant. The device first minor number. */
#define DEVICE_MINOR_NUM_FIRST 0

/** Constant. The device minor numbers amount. */
#define DEVICE_MINOR_NUMS_MAX 16

/** Constant. The number of 512-byte blocks. */
#define DEVICE_NUMBER_OF_BLOCKS 8

/** Constant. The number of pages per 512-byte block. */
#define DEVICE_NUMBER_OF_PAGES_PER_BLOCK 1024

/** Constant. The number of pages. */
#define DEVICE_NUMBER_OF_PAGES (DEVICE_NUMBER_OF_BLOCKS * \
                                DEVICE_NUMBER_OF_PAGES_PER_BLOCK)

/** Constant. The device page size. */
#define DEVICE_PAGE_SIZE 4096

/** Constant. The device sector size. */
#define DEVICE_SECTOR_SIZE 512

/** Constant. The number of sectors per page. */
#define DEVICE_NUMBER_OF_SECTORS_PER_PAGE (DEVICE_PAGE_SIZE / \
                                           DEVICE_SECTOR_SIZE)

/** Constant. The size of the table containing device page map entries. */
#define DEVICE_PAGE_MAP_TABLE_SIZE (DEVICE_NUMBER_OF_PAGES * \
                                    DEVICE_NUMBER_OF_SECTORS_PER_PAGE)

/** Constant. The device total size. */
#define DEVICE_TOTAL_SIZE (DEVICE_NUMBER_OF_PAGES * \
                           DEVICE_PAGE_SIZE)

/** Constant. The device request size divisor. */
#define DEVICE_REQUEST_SIZE_DIV 8

/**
 * Constant. The ioctl() type letter used to create a corresponding number
 *           (see below).
 */
#define DEVICE_IOCTL_TYPE_LETTER 'q'

/** Constant. The ioctl() command to register a user space caller. */
#define DEVICE_IOCTL_REG_USER_CALLER \
        _IO(DEVICE_IOCTL_TYPE_LETTER,  0)

/** Constant. The ioctl() command to get the request size. */
#define DEVICE_IOCTL_GET_REQUEST_SIZE \
        _IOR(DEVICE_IOCTL_TYPE_LETTER, 1, unsigned long)

/** Constant. The ioctl() command to read data block. */
#define DEVICE_IOCTL_GET_BLOCK \
        _IOR(DEVICE_IOCTL_TYPE_LETTER, 2, unsigned long)

/** Constant. The ioctl() command to write data block. */
#define DEVICE_IOCTL_SET_BLOCK \
        _IOW(DEVICE_IOCTL_TYPE_LETTER, 3, unsigned long)

/**
 * The structure to hold the device page mapping data.
 * It is used to communicate with user space.
 */
struct viosim_page_map {
    /** The logical page number (LPN). Calculated by the kernel. */
    u64 lpn;

    /**
     * The physical page number (PPN). Provided by the user
     * and is based on <code>lpn</code>.
     */
    u64 ppn;

    /**
     * The new physical page number. Provided by the user
     * for write ops only.
     */
    u64 ppnx;

    /**
     * The data transfer direction: <code>0</code> &ndash;
     * read from the device; write to the device otherwise.
     */
    int transf_dir;
};

/**
 * The structure to hold the device read/write request mapping data.
 * It is used to process actual read/write ops.
 */
struct viosim_request_map {
    /** The structure containing the device page mapping data. */
    struct viosim_page_map page_map;

    /** The starting sector to read/write. */
    u64 start_sector;

    /** The number of sectors to read/write. */
    u64 num_of_sectors;

    /** The pointer to the read/write request buffer. */
    void *req_buffer;
};

#endif /* __LINUX__VIRTBLKIOSIM_H */

/* vim:set nu:et:ts=4:sw=4: */
