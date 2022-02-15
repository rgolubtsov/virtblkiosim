/*
 * tests/ioctl/virtblkioctl.h
 * ============================================================================
 * VIRTual BLocK IO SIMulating (virtblkiosim). Version 0.9.9
 * ============================================================================
 * Virtual Linux block device driver for simulating and performing I/O.
 *
 * This utility tests block device I/O through the ioctl() system call.
 * ============================================================================
 * Copyright (C) 2016-2022 Radislav (Radicchio) Golubtsov
 *
 * (See the LICENSE file at the top of the source tree.)
 */

#ifndef __VIRTBLKIOCTL_H
#define __VIRTBLKIOCTL_H

#define _GNU_SOURCE /* <== Needs to use O_DIRECT in open(). */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/* Helper constants. */
#define _EMPTY_STRING       ""
#define _ONE_SPACE_STRING  " "
#define _COLON_SPACE_SEP  ": "
#define _COMMA_SPACE_SEP  ", "
#define _NEW_LINE         "\n"
#define _PRINT_BANNER_OPT "-V"

/* App name, version, and copyright banners. */
#define _APP_NAME        "VIRTual BLocK IOCTLing (virtblkioctl)"
#define _APP_DESCRIPTION \
         "Tests block device I/O through the ioctl() system call"
#define _APP_VERSION_S__ "Version"
#define _APP_VERSION     "0.9.9"
#define _APP_COPYRIGHT__ "Copyright (C) 2016-2022"
#define _APP_AUTHOR      "Radislav Golubtsov <radicchio@vk.com>"

/** The device node to test. */
#define _DEVNODE_HUB "/dev/"
#define _MODULE_NAME "virtblkiosim"

/** Constant: Print this when there is insufficient number of args passed. */
#define _CLI_ARGS_MUST_BE_THREE_ERR "%s: There must be three or four args " \
                                    "passed: %d arg(s) found" _NEW_LINE

/**
 * Constant: Print this usage info just after
 *           the <code>_CLI_ARGS_MUST_BE_THREE_ERR</code> error message.
 */
#define _CLI_USAGE_MSG \
         "Usage: virtblkioctl <device_node> <ioctl_command> <request_size"                               \
                                                           "|num_of_io_ops> [-V]"              _NEW_LINE \
                                                                                               _NEW_LINE \
         "       <device_node>       Something like " _DEVNODE_HUB _MODULE_NAME                _NEW_LINE \
                                                                                               _NEW_LINE \
         "       <ioctl_command>     One of the following:"                                    _NEW_LINE \
         "           --reguser       Register a user space caller (itself)"                    _NEW_LINE \
         "           --getreqsize    Get the request size to use in read/write operations"     _NEW_LINE \
         "           --getblkdata    Read data block from disk"                                _NEW_LINE \
         "           --setblkdata    Write data block to disk"                                 _NEW_LINE \
                                                                                               _NEW_LINE \
         "           --io            Continuously perform I/O (read/write) operations"         _NEW_LINE \
         "                           using the commands above in a loop"                       _NEW_LINE \
         "                           The <request_size> param is not applicable here"          _NEW_LINE \
         "                           The <num_of_io_ops> param should be set"                  _NEW_LINE \
         "                           to the number of I/O operations preferred,"               _NEW_LINE \
         "                           or to 0 for infinite repetitions"                         _NEW_LINE \
                                                                                               _NEW_LINE \
         "       <request_size>      Unsigned integer or 'none' (without quotes) when unknown" _NEW_LINE \
                                                                                               _NEW_LINE \
         "       <num_of_io_ops>     The number of I/O ops (see '--io' command description)"   _NEW_LINE

/**
 * Constant: Print this when the device node is not specified
 *           or is empty string.
 */
#define _DEVNODE_IS_NULL_OR_EMPTY_ERR \
         "%s: Device node is not specified or is empty string"

/** Constant: Print this when opening the device node failed. */
#define _DEVNODE_OPEN_FAILED_ERR "%s: Cannot open device node: %s"

/**
 * Constant: Print this when an unhandled error occurred
 *           during the device node operating.
 */
#define _DEVNODE_RELATED_UNHANDLED_ERR "%s: Device node unhandled error"

/** Constant: Print this when closing the device node failed. */
#define _DEVNODE_CLOSE_FAILED_ERR "%s: Cannot close device node: %s"

/**
 * Constant: Print this when the <code>ioctl()</code> command is not specified
 *           or is empty string.
 */
#define _IOCTLCMD_IS_NULL_OR_EMPTY_ERR \
         "%s: ioctl() command is not specified or is empty string"

/**
 * Constant: Print this when the request size is not specified
 *           or is empty string.
 */
#define _REQSIZE_IS_NULL_OR_EMPTY_ERR \
         "%s: Request size is not specified or is empty string"

/**
 * Constant: The cli request size arg indicator meaning that the request size
 *           is unknown or not applicable for the particular
 *           <code>ioctl()</code> command (i.e.\ it has the value
 *           of <code>1</code> as a multiplier).
 */
#define _REQSIZE_IS_NONE "none"

/**
 * Constant: Print this when the given <code>ioctl()</code> command is unknown.
 */
#define _IOCTLCMD_UNKNOWN_COMMAND_ERR "%s: ioctl() command unknown"

/**
 * Constant: Print this just before doing the <code>ioctl()</code> system call
 *           against the device.
 */
#define _MAKE_IOCTL_CALL_PRINT_ARGS_DBG "%s: ===> %s ===> %s"

/**
 * Constant: Print this when an unhandled error occurred
 *           during making <code>ioctl()</code> calls.
 */
#define _MAKE_IOCTL_CALL_UNHANDLED_ERR "%s: ioctl() unhandled error: %s"

/** Constants: Print for each appropriate <code>ioctl()</code> call result. */
#define _IOCTL_CALL_COMMAND_NOT_IMPL_MSG        "%s: Command not implemented"
#define _IOCTL_CALL_REG_USER_CALLER_RESULT_MSG  "%s: User app registered"
#define _IOCTL_CALL_GET_REQUEST_SIZE_RESULT_MSG "%s: %lu"
#define _IOCTL_CALL_GET_BLOCK_RESULT_MSG                   \
         "%s: I/O direction: %lu | Start sector: %lu"      \
                               " | Number of sectors: %lu" \
                               " | Request buffer: %lu"
#define _IOCTL_CALL_SET_BLOCK_RESULT_MSG \
        _IOCTL_CALL_GET_BLOCK_RESULT_MSG
#define _IOCTL_CALL_UNKNOWN_COMMAND_MSG         "%s: Unknown command"

/** Constants: Print during <code>ioctl()</code> pseudo-command execution. */
#define _IOCTL_CALL_IO_GET_REQUEST_SIZE_RESULT_MSG "Request size: %lu"
#define _IOCTL_CALL_IO_GET_BLOCK_RESULT_MSG            \
         "I/O direction: %lu | PPN: %lu"               \
                           " | Start sector: %lu"      \
                           " | Number of sectors: %lu" \
                           " | Request buffer: %lu"
#define _IOCTL_CALL_IO_SET_BLOCK_RESULT_MSG \
        _IOCTL_CALL_IO_GET_BLOCK_RESULT_MSG

/**
 * Constant: The ioctl() type letter used to create a corresponding number
 *           (see below).
 */
#define DEVICE_IOCTL_TYPE_LETTER 'q'

/** Constant: The ioctl() command to register a user space caller. */
#define _DEVICE_IOCTL_REG_USER_CALLER "--reguser"
#define  DEVICE_IOCTL_REG_USER_CALLER \
        _IO (DEVICE_IOCTL_TYPE_LETTER, 0)

/** Constant: The ioctl() command to get the request size. */
#define _DEVICE_IOCTL_GET_REQUEST_SIZE "--getreqsize"
#define  DEVICE_IOCTL_GET_REQUEST_SIZE \
        _IOR(DEVICE_IOCTL_TYPE_LETTER, 1, unsigned long)

/** Constant: The ioctl() command to read data block. */
#define _DEVICE_IOCTL_GET_BLOCK "--getblkdata"
#define  DEVICE_IOCTL_GET_BLOCK \
        _IOR(DEVICE_IOCTL_TYPE_LETTER, 2, unsigned long)

/** Constant: The ioctl() command to write data block. */
#define _DEVICE_IOCTL_SET_BLOCK "--setblkdata"
#define  DEVICE_IOCTL_SET_BLOCK \
        _IOW(DEVICE_IOCTL_TYPE_LETTER, 3, unsigned long)

/**
 * Constant: The ioctl() pseudo-command to continuously perform
 *           I/O operations in a loop.
 */
#define _DEVICE_IOCTL_PERF_IO "--io"

/* Continuously performs I/O (read/write) operations in a loop. */
int viosim_perf_io(const int, const char *, const unsigned, const char *);

/* Helper function. Closes the device node. */
extern int _viosim_devnode_close(const int, const char *);

/**
 * The structure to hold the device page mapping data.
 * It is used to communicate with user space.
 */
struct viosim_page_map {
    /** The logical page number (LPN). Calculated by the kernel. */
    unsigned long lpn;

    /**
     * The physical page number (PPN). Provided by the user
     * and is based on <code>lpn</code>.
     */
    unsigned long ppn;

    /**
     * The new physical page number. Provided by the user
     * for write ops only.
     */
    unsigned long ppnx;

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
    unsigned long start_sector;

    /** The number of sectors to read/write. */
    unsigned long num_of_sectors;

    /** The pointer to the read/write request buffer. */
    void *req_buffer;
};

#endif /* __VIRTBLKIOCTL_H */

/* vim:set nu et ts=4 sw=4: */
