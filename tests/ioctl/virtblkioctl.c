/*
 * tests/ioctl/virtblkioctl.c
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

#include "virtblkioctl.h"

static struct viosim_request_map viosim_req_map;

static uintptr_t viosim_req_size = 0U;

/**
 * Makes <code>ioctl()</code> calls against the device.
 *
 * @param viosim_devnode The device node to test.
 * @param viosim_ioctl   The <code>ioctl()</code> command to tickle the device.
 * @param viosim_rq_sz   The request size to use in read/write operations.
 * @param app_name       The name of the application executable
 *                       including prepending run path (if any) &ndash;
 *                       i.e. is the bin filename of this test program.
 *
 * @return The exit code indicating the <code>ioctl()</code> operation
 *         execution status.
 */
int viosim_ioctl_test(const char *viosim_devnode,
                      const char *viosim_ioctl,
                      const char *viosim_rq_sz,
                      const char *app_name) {

    int ret = EXIT_SUCCESS;

    unsigned num_of_io_ops;

    int fd;
    unsigned long request;
    void *argp;

    /* Checking for the first arg's presence. */
    if ((viosim_devnode == NULL)
        || (strcmp(viosim_devnode, _EMPTY_STRING) == 0)) {

        ret = EXIT_FAILURE;

        fprintf(stderr, _DEVNODE_IS_NULL_OR_EMPTY_ERR _NEW_LINE, app_name);

        return ret;
    /* Ensuring the right device node is to operate with. */
    } else if (strcmp(viosim_devnode, _DEVNODE_HUB _MODULE_NAME) == 0) {
        fd = open(viosim_devnode, O_RDWR | O_DIRECT);

        if (fd < 0) {
            ret = EXIT_FAILURE;

            fprintf(stderr, _DEVNODE_OPEN_FAILED_ERR _NEW_LINE,
                    app_name, strerror(errno));

            return ret;
        }
    } else {
        ret = EXIT_FAILURE;

        fprintf(stderr, _DEVNODE_RELATED_UNHANDLED_ERR _NEW_LINE, app_name);

        return ret;
    }

    /* Checking for the second arg's presence. */
    if ((viosim_rq_sz == NULL)
        || (strcmp(viosim_rq_sz, _EMPTY_STRING) == 0)) {

        fprintf(stderr, _REQSIZE_IS_NULL_OR_EMPTY_ERR _NEW_LINE, app_name);

        /* Don't forget to close the device node opened previously. */
        ret = _viosim_devnode_close(fd, app_name);

    } else if (strcmp(viosim_rq_sz, "none") == 0) {
        viosim_req_size = 1U;
    } else {
        viosim_req_size = strtoul(viosim_rq_sz, NULL, 0);
    }

    /* Checking for the third arg's presence. */
    if ((viosim_ioctl == NULL)
        || (strcmp(viosim_ioctl, _EMPTY_STRING) == 0)) {

        fprintf(stderr, _IOCTLCMD_IS_NULL_OR_EMPTY_ERR _NEW_LINE, app_name);

        /* Don't forget to close the device node opened previously. */
        ret = _viosim_devnode_close(fd, app_name);

        return ret;
    } else {
               if (strcmp(viosim_ioctl, _DEVICE_IOCTL_REG_USER_CALLER)  == 0) {
            request = DEVICE_IOCTL_REG_USER_CALLER;
            argp    = 0; /* <== There is no param in this ioctl() call. */
        } else if (strcmp(viosim_ioctl, _DEVICE_IOCTL_GET_REQUEST_SIZE) == 0) {
            request = DEVICE_IOCTL_GET_REQUEST_SIZE;

            /* Allocating memory for storing the request size. */
            argp    = malloc(sizeof(unsigned long));
        } else if (strcmp(viosim_ioctl, _DEVICE_IOCTL_GET_BLOCK)        == 0) {
            request = DEVICE_IOCTL_GET_BLOCK;

            /*
             * Allocating memory for storing a pointer to the structure
             * containing the device page mapping data.
             */
            argp    = malloc(sizeof(struct viosim_request_map)
                                         * viosim_req_size);
        } else if (strcmp(viosim_ioctl, _DEVICE_IOCTL_SET_BLOCK)        == 0) {
            request = DEVICE_IOCTL_SET_BLOCK;

            /*
             * Allocating memory for storing a pointer to the structure
             * containing the device page mapping data.
             */
            argp    = malloc(sizeof(struct viosim_request_map)
                                         * viosim_req_size);
        } else if (strcmp(viosim_ioctl, _DEVICE_IOCTL_PERF_IO)          == 0) {
            num_of_io_ops = viosim_req_size;

            /* Performing I/O operations continuously. */
            viosim_perf_io(fd, viosim_ioctl, num_of_io_ops, app_name);

            /* Normally closing the device node after ioctl'ing it. */
            ret = _viosim_devnode_close(fd, app_name);

            return ret;
        } else {
            fprintf(stderr, _IOCTLCMD_UNKNOWN_COMMAND_ERR _NEW_LINE, app_name);

            /* Don't forget to close the device node opened previously. */
            ret = _viosim_devnode_close(fd, app_name);

            return ret;
        }
    }

    printf(_MAKE_IOCTL_CALL_PRINT_ARGS_DBG _NEW_LINE,
           app_name, viosim_devnode, viosim_ioctl);

    /* Ioctl'ing the device. */
    ret = ioctl(fd, request, argp);

    if (ret < 0) {
        fprintf(stderr, _MAKE_IOCTL_CALL_UNHANDLED_ERR _NEW_LINE,
                app_name, strerror(errno));
    } else {
               if (strcmp(viosim_ioctl, _DEVICE_IOCTL_REG_USER_CALLER)  == 0) {
            printf(_IOCTL_CALL_REG_USER_CALLER_RESULT_MSG _NEW_LINE,
                   viosim_ioctl);
        } else if (strcmp(viosim_ioctl, _DEVICE_IOCTL_GET_REQUEST_SIZE) == 0) {
            viosim_req_size = (uintptr_t) argp;

            printf(_IOCTL_CALL_GET_REQUEST_SIZE_RESULT_MSG _NEW_LINE,
                   viosim_ioctl, viosim_req_size);

            free(argp);
        } else if (strcmp(viosim_ioctl, _DEVICE_IOCTL_GET_BLOCK)        == 0) {
            printf(_IOCTL_CALL_GET_BLOCK_RESULT_MSG _NEW_LINE,
                   viosim_ioctl,
                   ((struct viosim_request_map *) argp)->page_map.transf_dir,
                   ((struct viosim_request_map *) argp)->start_sector,
                   ((struct viosim_request_map *) argp)->num_of_sectors,
                   ((struct viosim_request_map *) argp)->req_buffer);

            free(argp);
        } else if (strcmp(viosim_ioctl, _DEVICE_IOCTL_SET_BLOCK)        == 0) {
            printf(_IOCTL_CALL_SET_BLOCK_RESULT_MSG _NEW_LINE,
                   viosim_ioctl,
                   ((struct viosim_request_map *) argp)->page_map.transf_dir,
                   ((struct viosim_request_map *) argp)->start_sector,
                   ((struct viosim_request_map *) argp)->num_of_sectors,
                   ((struct viosim_request_map *) argp)->req_buffer);

            free(argp);
        } else {
            printf(_IOCTL_CALL_UNKNOWN_COMMAND_MSG _NEW_LINE,
                   viosim_ioctl);
        }
    }

    /* Normally closing the device node after ioctl'ing it. */
    ret = _viosim_devnode_close(fd, app_name);

    return ret;
}

/**
 * Continuously performs I/O (read/write) operations in a loop.
 *
 * @param viosim_devnode The device node to test.
 * @param viosim_ioctl   The <code>ioctl()</code> command to tickle the device.
 * @param num_of_io_ops  The number of I/O operations (iterations).
 * @param app_name       The name of the application executable.
 *
 * @return The exit code indicating the current <code>ioctl()</code> operation
 *         execution status.
 */
int viosim_perf_io(const int       viosim_devnode,
                   const char     *viosim_ioctl,
                   const unsigned  num_of_io_ops,
                   const char     *app_name) {

    int ret = EXIT_SUCCESS;

    unsigned i;

    /* Registering... */
    ret = ioctl(viosim_devnode, DEVICE_IOCTL_REG_USER_CALLER, 0);

    if (ret < 0) {
        fprintf(stderr, _MAKE_IOCTL_CALL_UNHANDLED_ERR _NEW_LINE,
                app_name, strerror(errno));

        ret = _viosim_devnode_close(viosim_devnode, app_name);

        return ret;
    }

    printf(_IOCTL_CALL_REG_USER_CALLER_RESULT_MSG _NEW_LINE, viosim_ioctl);

    /* Defining the number of I/O operations (iterations). */
    if (num_of_io_ops != 0) {
        i = num_of_io_ops;
    } else {
        i = 1; /* <== Setting the loop for infinite repetitions. */
    }

    /* Stop on <Ctrl+C>. */
    while (i) {                /* while (true) { */
        /* Sizing... */
        ret = ioctl(viosim_devnode, DEVICE_IOCTL_GET_REQUEST_SIZE,
            &viosim_req_size);

        if (ret < 0) {
            fprintf(stderr, _MAKE_IOCTL_CALL_UNHANDLED_ERR _NEW_LINE,
                    app_name, strerror(errno));

            ret = _viosim_devnode_close(viosim_devnode, app_name);

            return ret;
        }

        printf(_IOCTL_CALL_IO_GET_REQUEST_SIZE_RESULT_MSG _NEW_LINE,
               &viosim_req_size);

        /* Reading... */
        ret = ioctl(viosim_devnode, DEVICE_IOCTL_GET_BLOCK,
            &viosim_req_map);

        if (ret < 0) {
            fprintf(stderr, _MAKE_IOCTL_CALL_UNHANDLED_ERR _NEW_LINE,
                    app_name, strerror(errno));

            ret = _viosim_devnode_close(viosim_devnode, app_name);

            return ret;
        }

        printf(_IOCTL_CALL_IO_GET_BLOCK_RESULT_MSG _NEW_LINE,
               viosim_req_map.page_map.transf_dir,
               viosim_req_map.start_sector,
               viosim_req_map.num_of_sectors,
               viosim_req_map.req_buffer);

        /* Converting LPN to PPN. */
        viosim_req_map.page_map.ppn = viosim_req_map.page_map.lpn;

        /* Writing... */
        ret = ioctl(viosim_devnode, DEVICE_IOCTL_SET_BLOCK,
            &viosim_req_map);

        if (ret < 0) {
            fprintf(stderr, _MAKE_IOCTL_CALL_UNHANDLED_ERR _NEW_LINE,
                    app_name, strerror(errno));

            ret = _viosim_devnode_close(viosim_devnode, app_name);

            return ret;
        }

        printf(_IOCTL_CALL_IO_SET_BLOCK_RESULT_MSG _NEW_LINE,
               viosim_req_map.page_map.transf_dir,
               viosim_req_map.page_map.ppn,
               viosim_req_map.start_sector,
               viosim_req_map.num_of_sectors,
               viosim_req_map.req_buffer);

        if (num_of_io_ops != 0) {
            i--;
        }
    }

    return ret;
}

/* Helper function. Closes the device node. */
int _viosim_devnode_close(const int viosim_devnode, const char *app_name) {
    int ret = close(viosim_devnode);

    if (ret < 0) {
        ret = EXIT_FAILURE;

        fprintf(stderr, _DEVNODE_CLOSE_FAILED_ERR _NEW_LINE,
                app_name, strerror(errno));
    }

    return ret;
}

/* Helper function. Draws a horizontal separator banner. */
void _separator_draw(const char *banner_text) {
    unsigned char i = strlen(banner_text);

    do { putchar('='); i--; } while (i); puts(_EMPTY_STRING);
}

/* The application entry point. */
int main(int argc, char *const *argv) {
    int ret = EXIT_SUCCESS;

    char *print_banner_opt = _EMPTY_STRING;

    int argv4_len, /*
        ^
        |
        +-------- Needs this for toupper'ing argv[4] only.
        |
        v
    */  i;

    if (argc > 4) {
        argv4_len = strlen(argv[4]);

        print_banner_opt = malloc(argv4_len);

        for (i = 0; i <= argv4_len; i++) {
            print_banner_opt[i] = toupper(argv[4][i]);
        }
    }

    if (strcmp(print_banner_opt, _PRINT_BANNER_OPT) == 0) {
        _separator_draw(_APP_COPYRIGHT__ _ONE_SPACE_STRING _APP_AUTHOR);

        printf(_APP_NAME        _COMMA_SPACE_SEP                         \
               _APP_VERSION_S__ _ONE_SPACE_STRING _APP_VERSION _NEW_LINE \
               _APP_DESCRIPTION                                _NEW_LINE \
               _APP_COPYRIGHT__ _ONE_SPACE_STRING _APP_AUTHOR  _NEW_LINE);

        _separator_draw(_APP_COPYRIGHT__ _ONE_SPACE_STRING _APP_AUTHOR);
    }

    if (argc > 4) {
        free(print_banner_opt);
    }

    /* Checking for args presence. */
    if (argc < 4) {
        ret = EXIT_FAILURE;

        fprintf(stderr, _CLI_ARGS_MUST_BE_THREE_ERR _NEW_LINE,
                argv[0], (argc - 1));

        fprintf(stderr, _CLI_USAGE_MSG _NEW_LINE);
    } else {
        /* Making ioctl() calls against the device. */
        ret = viosim_ioctl_test(argv[1], argv[2], argv[3], argv[0]);
    }

    return ret;
}

/* vim:set nu et ts=4 sw=4: */
