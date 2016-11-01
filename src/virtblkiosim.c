/*
 * src/virtblkiosim.c
 * ============================================================================
 * VIRTual BLocK IO SIMulating (virtblkiosim). Version 0.1
 * ============================================================================
 * Virtual Linux block device driver for simulating and performing I/O.
 * ============================================================================
 * Copyright (C) 2016 Radislav (Radicchio) Golubtsov
 */

#include "virtblkiosim.h"

/** The device major number to register in <code>/dev</code>. */
int major_num;

static        spinlock_t          viosim_lock;
static struct request_queue      *viosim_req_qu;
static struct gendisk            *viosim_disk;
static        wait_queue_head_t   viosim_r_reqsz_wait_qu;
static        wait_queue_head_t   viosim_r_block_wait_qu;
static        wait_queue_head_t   viosim_w_block_wait_qu;
static struct work_struct         viosim_req_task;
static struct task_struct        *viosim_usr_app;
static struct viosim_request_map *viosim_req_map;

/**
 * The wait flags for three corresponding wait queues:
 * <ul>
 * <li><code>viosim_r_reqsz_wait_flag</code> is for getting
 * the request size.</li>
 * <li><code>viosim_r_block_wait_flag</code> is for reading data block.</li>
 * <li><code>viosim_w_block_wait_flag</code> is for writing data block.</li>
 */
static bool viosim_r_reqsz_wait_flag = true;
static bool viosim_r_block_wait_flag = true;
static bool viosim_w_block_wait_flag = true;

/**
 * The main working buffers:
 * <ul>
 * <li><code>page_buffer</code> is the container to store
 * page-sized device data portion.</li>
 * <li><code>data_buffer</code> is the container to store
 * all device data.</li>
 * </ul>
 */
static u8 page_buffer[DEVICE_PAGE_SIZE];
static u8 data_buffer[DEVICE_TOTAL_SIZE];

/** The device request size. */
static unsigned viosim_req_size;

/** Process requests that have been placed on the queue. */
static void viosim_req_proc(void) {
    /* Putting the task in the kernel-global workqueue. */
    schedule_work(&viosim_req_task);
}

/**
 * Inner helper function.
 * <br />
 * <br />Gets called from inside <code>viosim_dev_read(...)</code>
 * and <code>viosim_dev_write(...)</code> helpers.
 *
 * @param ppn The physical page number (PPN).
 *
 * @return The exit code indicating the status of reading a page of data
 *         from the device.
 */
static int viosim_dev_read_page(const u64 ppn) {
    int ret = EXIT_SUCCESS;

    if (ppn >= DEVICE_NUMBER_OF_PAGES) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                _READ_CAPACITY_REACHED_MSG _NEW_LINE);

        /* Returning "success" anyway, because it's not an error. */
        return ret;
    }

    /*
     * Copying one-page data portion from the consolidated data buffer
     * into the device page buffer, i.e. reading the data page.
     */
    memcpy(page_buffer, data_buffer + (ppn * DEVICE_PAGE_SIZE),
        DEVICE_PAGE_SIZE);

    return ret;
}

/**
 * Helper (wrapper) function.
 * <br />
 * <br />Reads data from the device.
 *
 * @param req_map The <code>viosim_request_map</code> structure which holds
 *                the device read/write request mapping data.
 *
 * @return The exit code indicating the status of reading data from the device.
 */
static int viosim_dev_read(struct viosim_request_map *req_map) {
    int ret = EXIT_SUCCESS;

    struct viosim_page_map *page_map = &req_map->page_map;

    /* Getting the PPN from page map. */
    u64 ppn = page_map->ppn;

    /* Calculating the offset from the starting sector to read data. */
    u16 sector_offset = \
        req_map->start_sector % DEVICE_NUMBER_OF_SECTORS_PER_PAGE;

    /* Getting the number of sectors from request map. */
    u64 num_of_sectors = req_map->num_of_sectors;

    /*
     * Getting the pointer to the read/write request buffer. --
     * I.e. where read data is stored.
     */
    void *req_buffer = req_map->req_buffer;

    /* Don't do anything if the page map table's end reached. */
    if (ppn == DEVICE_PAGE_MAP_TABLE_SIZE) {
        return ret;
    }

    /*
     * Reading a page of data from the device.
     * Here the page_buffer array is populated.
     */
    ret = viosim_dev_read_page(ppn);

    /*
     * Copying the amount of request sectors-occupied data portion
     * from the device page buffer into the read/write request buffer.
     */
    memcpy(req_buffer, page_buffer + (sector_offset * DEVICE_SECTOR_SIZE),
        (num_of_sectors * DEVICE_SECTOR_SIZE));

    viosim_r_block_wait_flag = true;

    return ret;
}

/**
 * Inner helper function.
 * <br />
 * <br />Gets called from inside <code>viosim_dev_write(...)</code> helper.
 *
 * @param ppnx The new physical page number (PPN).
 *
 * @return The exit code indicating the status of writing a page of data
 *         to the device.
 */
static int viosim_dev_write_page(const u64 ppnx) {
    int ret = EXIT_SUCCESS;

    if (ppnx >= DEVICE_NUMBER_OF_PAGES) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                _WRITE_CAPACITY_REACHED_MSG _NEW_LINE);

        /* Returning "success" anyway, because it's not an error. */
        return ret;
    }

    /*
     * Copying one-page data portion from the device page buffer
     * into the consolidated data buffer, i.e. writing the data page.
     */
    memcpy(data_buffer + (ppnx * DEVICE_PAGE_SIZE), page_buffer,
        DEVICE_PAGE_SIZE);

    return ret;
}

/**
 * Helper (wrapper) function.
 * <br />
 * <br />Writes data to the device.
 *
 * @param req_map The <code>viosim_request_map</code> structure which holds
 *                the device read/write request mapping data.
 *
 * @return The exit code indicating the status of writing data to the device.
 */
static int viosim_dev_write(struct viosim_request_map *req_map) {
    int ret = EXIT_SUCCESS;

    struct viosim_page_map *page_map = &req_map->page_map;

    /* Getting the PPN from page map. */
    u64 ppn = page_map->ppn;

    /* Getting the new PPN from page map (for write). */
    u64 ppnx = page_map->ppnx;

    /* Calculating the offset from the starting sector to read data. */
    u16 sector_offset = \
        req_map->start_sector % DEVICE_NUMBER_OF_SECTORS_PER_PAGE;

    /* Getting the number of sectors from request map. */
    u64 num_of_sectors = req_map->num_of_sectors;

    /*
     * Getting the pointer to the read/write request buffer. --
     * I.e. where read data is stored.
     */
    void *req_buffer = req_map->req_buffer;

    /* --- Performing the "Read-Modify-Write" atomic operation - Begin ----- */
    /* (1) Read                                 */
    /* Reading a page of data from the device.  */
    /* Here the page_buffer array is populated. */
    ret = viosim_dev_read_page(ppn);

    if (ret != EXIT_SUCCESS) {
        return ret;
    }

    /* (2) Modify                                                      */
    /* Copying the amount of request sectors-occupied data portion     */
    /* from the read/write request buffer into the device page buffer. */
    memcpy(page_buffer + (sector_offset * DEVICE_SECTOR_SIZE), req_buffer,
        (num_of_sectors * DEVICE_SECTOR_SIZE));

    /* (3) Write                                */
    /* Writing data page to the device.         */
    /* Here the data_buffer array is populated. */
    ret = viosim_dev_write_page(ppnx);
    /* --- Performing the "Read-Modify-Write" atomic operation - End ------- */

    viosim_w_block_wait_flag = false;

    return ret;
}

/**
 * Processes the request fetched from the request queue, i.e. data transfer.
 *
 * @param req The <code>request</code> structure containing the request
 *            to process.
 *
 * @return The exit code indicating the overall status of processing
 *         the request.
 */
static int viosim_req_transfer(const struct request *req) {
    int ret = EXIT_SUCCESS;

    char *transf_dir_s = NULL;

    struct bio_vec      bv;
    struct req_iterator iter;

    unsigned short i = 0;

    /* Getting the data transfer direction (read/write from/to the device). */
    int transf_dir = rq_data_dir(req);

    /* Getting the starting sector (current sector). */
    sector_t start_sector  = blk_rq_pos(req);
    sector_t sector_offset = 0;

    /* Getting the amount of sectors left in the entire request. */
    unsigned num_of_sectors = blk_rq_sectors(req);

    viosim_req_size = num_of_sectors / DEVICE_REQUEST_SIZE_DIV;

    if (viosim_req_size == 0) {
        viosim_req_size = 1;
    }

    /* --- DEBUG: Printing the data transfer direction - Begin ------------- */
#define TRANSF_DIR_READ  "read from device"
#define TRANSF_DIR_WRITE "write to device"
#define TRANSF_DIR_MSG   "===> Data transfer dir: %d, i.e. %s"

    if (transf_dir == 0) {
        transf_dir_s = kzalloc(sizeof(TRANSF_DIR_READ)  + 1, GFP_KERNEL);
        transf_dir_s = strcpy(transf_dir_s, TRANSF_DIR_READ);
    } else {
        transf_dir_s = kzalloc(sizeof(TRANSF_DIR_WRITE) + 1, GFP_KERNEL);
        transf_dir_s = strcpy(transf_dir_s, TRANSF_DIR_WRITE);
    }

    pr_info(_MODULE_NAME _COLON_SPACE_SEP \
            TRANSF_DIR_MSG _NEW_LINE, transf_dir, transf_dir_s);

    kfree(transf_dir_s);
    /* --- DEBUG: Printing the data transfer direction - End --------------- */

    viosim_r_reqsz_wait_flag = true;

    /* Waking up the process before getting the request size. */
    wake_up_interruptible(&viosim_r_reqsz_wait_qu);

    viosim_req_map = kzalloc(
        sizeof(struct viosim_request_map) * viosim_req_size, GFP_KERNEL);

    if (viosim_req_map == NULL) {
        ret = -ENOMEM;

        return ret;
    }

    /*
     * Traversing <bio> segments in the request list.
     * - bv   -- bio_vec structure, a vector representation of <bio>s:
     * struct bio_vec {
     *     struct   page *bv_page;
     *     unsigned int   bv_len;
     *     unsigned int   bv_offset;
     * };
     * - req  -- Request queue (runqueue) structure.
     * - iter -- Request queue iterator (structure).
     * See https://www.kernel.org/doc/Documentation/block/biodoc.txt ,
     * section 3.2.1 for details.
     */
    rq_for_each_segment(bv, req, iter) {
        struct viosim_request_map *viosim_rq_map = &viosim_req_map[i++];
        struct viosim_page_map    *viosim_pg_map = &viosim_rq_map->page_map;

        /* The logical block address (LBA). */
        u64 lba = start_sector + sector_offset;

        /* The logical page number (LPN). */
        viosim_pg_map->lpn = lba / DEVICE_NUMBER_OF_SECTORS_PER_PAGE;

        viosim_pg_map->transf_dir = transf_dir; /* Read or write. */

        viosim_rq_map->start_sector   = lba;
        viosim_rq_map->num_of_sectors = bv.bv_len / DEVICE_SECTOR_SIZE;
        viosim_rq_map->req_buffer     = page_address(bv.bv_page) + bv.bv_offset;

        if (viosim_rq_map->num_of_sectors > DEVICE_REQUEST_SIZE_DIV) {
            ret = -EIO;

            return ret;
        }

        sector_offset += viosim_rq_map->num_of_sectors;
    }

    viosim_r_block_wait_flag = true;

    /* Waking up the process before reading. */
    wake_up_interruptible(&viosim_r_block_wait_qu);

    /* Putting the process to sleep before writing. */
    if (wait_event_interruptible(
        viosim_w_block_wait_qu, viosim_w_block_wait_flag)) {

        /* When interrupted by a signal -- return with error. */
        ret = -ERESTARTSYS;

        return ret;
    }

    viosim_w_block_wait_flag = false;

    /* Actually performing read/write ops using the appropriate helpers. */
    for (i = 0; i < viosim_req_size; i++) {
        struct viosim_request_map *viosim_rq_map = &viosim_req_map[i];
        struct viosim_page_map    *viosim_pg_map = &viosim_rq_map->page_map;

        viosim_pg_map->ppnx = viosim_pg_map->lpn;
        viosim_pg_map->ppn  = viosim_pg_map->ppnx;

        if (transf_dir == 0) {
            ret = viosim_dev_read(viosim_rq_map);
        } else {
            ret = viosim_dev_write(viosim_rq_map);
        }
    }

    kfree(viosim_req_map);

    if (sector_offset != num_of_sectors) {
        ret = -EXIT_FAILURE;

        pr_alert(_MODULE_NAME _COLON_SPACE_SEP \
                 _BIO_DOES_NOT_MATCH_REQUEST_ERR _NEW_LINE);
    }

    return ret;
}

/**
 * Executes a task prepared and waited to be run out of a workqueue.
 *
 * @param task The <code>work_struct</code> device structure
 *             which contains the working task.
 */
static void viosim_req_exec(const struct work_struct *task) {
    struct request *req;

    int ret = EXIT_SUCCESS;

    /* Fetching the request from the request queue and handling it. */
    while ((req = blk_fetch_request(viosim_req_qu)) != NULL) {
        if (viosim_usr_app != NULL) {
            /* Handling the request: its main processing is going there. */
            ret = viosim_req_transfer(req);
        }

        /* Completely finishing the request. */
        __blk_end_request_all(req, ret);
    }
}

/**
 * Implements engaging the device operation.
 *
 * @param blkdev The <code>block_device</code> structure describing the device.
 * @param mode   N/A. (TODO: Provide description.)
 *
 * @return The exit code indicating engaging the device operation
 *         execution status.
 */
static int viosim_open_proc(struct block_device *blkdev, const fmode_t mode) {
    int ret = EXIT_SUCCESS;

    /* --- DEBUG: Printing the device private data - Begin ----------------- */
#define OPEN_PROC_DBG_00 "===> 00: blkdev != NULL"
#define OPEN_PROC_DBG_01 "===> 01: blkdev->bd_disk != NULL"
#define OPEN_PROC_DBG_02 "===> 02: viosim_private_data == NULL"
#define OPEN_PROC_DBG_03 "===> 03: viosim_private_data != NULL"
#define OPEN_PROC_DBG_99 "Device engage: private_data: %s"

    char *viosim_private_data = NULL;

    if (blkdev != NULL) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP OPEN_PROC_DBG_00 _NEW_LINE);

        if (blkdev->bd_disk != NULL) {
            pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                    OPEN_PROC_DBG_01 _NEW_LINE);

            viosim_private_data = kzalloc(
                sizeof(blkdev->bd_disk->private_data) + 1, GFP_KERNEL);

            viosim_private_data = strcpy(viosim_private_data,
                                         blkdev->bd_disk->private_data);
        }
    }

    if (viosim_private_data == NULL) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP OPEN_PROC_DBG_02 _NEW_LINE);

        ret = EXIT_FAILURE;

        viosim_private_data = kzalloc(3 + 1, GFP_KERNEL);

        viosim_private_data = strcpy(viosim_private_data, "N/A");
    }

    if (viosim_private_data != NULL) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP OPEN_PROC_DBG_03 _NEW_LINE);

        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                OPEN_PROC_DBG_99 _NEW_LINE, viosim_private_data);

        kfree(viosim_private_data);
    }
    /* --- DEBUG: Printing the device private data - End ------------------- */

    return ret;
}

/**
 * Implements releasing the device operation.
 *
 * @param viosim_disc The <code>gendisk</code> structure describing the device.
 * @param mode        N/A. (TODO: Provide description.)
 */
static void viosim_release_proc(      struct gendisk *viosim_disc,
                                const        fmode_t  mode) {

    /* --- DEBUG: Printing the device private data - Begin ----------------- */
#define RLZZ_PROC_DBG_04 "===> 04: viosim_disc != NULL"
#define RLZZ_PROC_DBG_05 "===> 05: viosim_private_data == NULL"
#define RLZZ_PROC_DBG_06 "===> 06: viosim_private_data != NULL"
#define RLZZ_PROC_DBG_99 "Device release: private_data: %s"

    char *viosim_private_data = NULL;

    if (viosim_disc != NULL) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP RLZZ_PROC_DBG_04 _NEW_LINE);

        viosim_private_data = kzalloc(
            sizeof(viosim_disc->private_data) + 1, GFP_KERNEL);

        viosim_private_data = strcpy(viosim_private_data,
                                     viosim_disc->private_data);
    }

    if (viosim_private_data == NULL) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP RLZZ_PROC_DBG_05 _NEW_LINE);

        viosim_private_data = kzalloc(3 + 1, GFP_KERNEL);

        viosim_private_data = strcpy(viosim_private_data, "N/A");
    }

    if (viosim_private_data != NULL) {
        pr_info(_MODULE_NAME _COLON_SPACE_SEP RLZZ_PROC_DBG_06 _NEW_LINE);

        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                RLZZ_PROC_DBG_99 _NEW_LINE, viosim_private_data);

        kfree(viosim_private_data);
    }
    /* --- DEBUG: Printing the device private data - End ------------------- */

    if (viosim_usr_app != current) {
        return;
    }

    /*
     * Nullifying the user space calling process task pointer ->
     * releasing the device.
     */
    viosim_usr_app = NULL;
}

/**
 * Implements the ioctl() system call to control device I/O ops.
 *
 * @param blkdev The <code>block_device</code> structure describing the device.
 * @param mode   N/A. (TODO: Provide description.)
 * @param cmd    The ioctl() command ID to execute.
 * @param arg    The ioctl() argument to pass to a command set. (Optional.)
 *
 * @return The exit code indicating the ioctl() operation execution status.
 */
static int viosim_ioctl_proc(      struct block_device  *blkdev,
                             const        fmode_t        mode,
                             const        unsigned       cmd,
                             const        unsigned long  arg) {

    int ret = EXIT_SUCCESS;

    unsigned long dead_bytes = 0UL;

    /* --- DEBUG: Printing the ioctl() call ID - Begin --------------------- */
#define IOCTL_PROC_CMD_AND_ARG_DBG \
        "===> ioctl() call ID: %#010x " \
        "(dir: %#x | size: %#05x | chr: %#04x '%c' | func: %#04x) " \
        "===> arg: %lu"

    pr_info(_MODULE_NAME _COLON_SPACE_SEP IOCTL_PROC_CMD_AND_ARG_DBG _NEW_LINE,
                      cmd,
             _IOC_DIR(cmd),
            _IOC_SIZE(cmd),
            _IOC_TYPE(cmd), _IOC_TYPE(cmd),
              _IOC_NR(cmd),
                      arg);

#define IOCTL_PROC_CMD_SYM_0_DBG "===> REG_USER_CALLER"
#define IOCTL_PROC_CMD_SYM_1_DBG "===> GET_REQUEST_SIZE"
#define IOCTL_PROC_CMD_SYM_2_DBG "===> GET_BLOCK"
#define IOCTL_PROC_CMD_SYM_3_DBG "===> SET_BLOCK"
    /* --- DEBUG: Printing the ioctl() call ID - End ----------------------- */

    switch(cmd) {
    case DEVICE_IOCTL_REG_USER_CALLER:
        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                IOCTL_PROC_CMD_SYM_0_DBG _NEW_LINE);

        /*
         * <current> points to the task structure of the calling process
         *           in user space.
         */
        viosim_usr_app = current;

        break;

    case DEVICE_IOCTL_GET_REQUEST_SIZE:
        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                IOCTL_PROC_CMD_SYM_1_DBG _NEW_LINE);

        /* Putting the process to sleep before reading. */
        if (wait_event_interruptible(
            viosim_r_reqsz_wait_qu, viosim_r_reqsz_wait_flag)) {

            /* When interrupted by a signal -- return with error. */
            ret = -ERESTARTSYS;

            return ret;
        }

        /* Returning the request size to user space. */
        ret = put_user(viosim_req_size, (unsigned long __user *) arg);

        if (ret != 0) {
            return ret; /* <== -EFAULT */
        }

        /*
         * Setting the get-(read)-request-size-wait-flag back to FALSE
         * to allow the next process to read from the device.
         */
        viosim_r_reqsz_wait_flag = false;

        break;

    case DEVICE_IOCTL_GET_BLOCK:
        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                IOCTL_PROC_CMD_SYM_2_DBG _NEW_LINE);

        /* Putting the process to sleep before reading. */
        if (wait_event_interruptible(
            viosim_r_block_wait_qu, viosim_r_block_wait_flag)) {

            /* When interrupted by a signal -- return with error. */
            ret = -ERESTARTSYS;

            return ret;
        }

        /* Returning block of data to user space. */
        dead_bytes = copy_to_user(
          (struct viosim_request_map __user *) arg, /* <== Dest address.     */
          viosim_req_map,                           /* <== Source address.   */
          (sizeof(*viosim_req_map) *                /* <== How much to copy? */
                   viosim_req_size));

        if (dead_bytes > 0UL) {
            pr_alert(_MODULE_NAME _COLON_SPACE_SEP \
                     _COPY_TO_USER_DEAD_BYTES_EXIST_ERR _NEW_LINE,
                     dead_bytes);
        }

        /*
         * Setting the read wait flag back to FALSE to allow the next process
         * to read from the device.
         */
        viosim_r_block_wait_flag = false;

        break;

    case DEVICE_IOCTL_SET_BLOCK:
        pr_info(_MODULE_NAME _COLON_SPACE_SEP \
                IOCTL_PROC_CMD_SYM_3_DBG _NEW_LINE);

        /* Passing block of data to kernel space. */
        dead_bytes = copy_from_user(
          viosim_req_map,                           /* <== Dest address.     */
          (struct viosim_request_map __user *) arg, /* <== Source address.   */
          (sizeof(*viosim_req_map) *                /* <== How much to copy? */
                   viosim_req_size));

        if (dead_bytes > 0UL) {
            pr_alert(_MODULE_NAME _COLON_SPACE_SEP \
                     _COPY_FROM_USER_DEAD_BYTES_EXIST_ERR _NEW_LINE,
                     dead_bytes);
        }

        /*
         * Setting the write wait flag back to TRUE to put the next write
         * process to sleep.
         */
        viosim_w_block_wait_flag = true;

        /* Waking up the process before writing. */
        wake_up_interruptible(&viosim_w_block_wait_qu);

        break;

    default:
        ret = -ENOTTY;
    }

    return ret;
}

/** The structure to hold and register device operations data and callbacks. */
static struct block_device_operations viosim_ops = {
    .open    = viosim_open_proc,    /* <== Doing something when           */
                                    /*     engaging the device.           */
    .release = viosim_release_proc, /* <== Doing something when           */
                                    /*     releasing the device.          */
    .ioctl   = viosim_ioctl_proc,   /* <== Making the ioctl() system call */
                                    /*     to control device I/O ops.     */
    .owner   = THIS_MODULE,
};

/** Initialize a block device driver module. */
static int __init virtblkiosim_init(void) {
    int ret = EXIT_SUCCESS;

    pr_info(_MODULE_NAME        _COLON_SPACE_SEP                            \
            _MODULE_DESCRIPTION _COMMA_SPACE_SEP                            \
            _MODULE_VERSION_S__ _ONE_SPACE_STRING _MODULE_VERSION _NEW_LINE \
            _MODULE_NAME        _COLON_SPACE_SEP                            \
            _MODULE_COPYRIGHT__ _ONE_SPACE_STRING _MODULE_AUTHOR  _NEW_LINE);

    /* (1)                                                  */
    /* Registering the block device.                        */
    /* If major=0, try to allocate any unused major number. */
    /*          |                                           */
    /*          +---------------+                           */
    /*                          |                           */
    /*                          v                           */
    major_num = register_blkdev(DEVICE_MAJOR_NUM_TRIAL, DEVICE_NAME);

    if (major_num < DEVICE_MAJOR_NUM_TRIAL) {
        ret = major_num;

        pr_alert(_MODULE_NAME _COLON_SPACE_SEP \
                 _REGISTER_DEVICE_FAILED_ERR _NEW_LINE);

        return ret;
    }

    pr_info(_MODULE_NAME _COLON_SPACE_SEP \
            _REGISTER_DEVICE_SUCCEED_MSG _NEW_LINE, major_num);

    /* (2)                                       */
    /* Initializing the request queue spin lock. */
    spin_lock_init(&viosim_lock);

    /* (3)                                    */
    /* Initializing the request queue itself. */
    viosim_req_qu = blk_init_queue((request_fn_proc *) viosim_req_proc,
                                                      &viosim_lock);

    if (viosim_req_qu == NULL) {
        ret = -EXIT_FAILURE;

        pr_alert(_MODULE_NAME _COLON_SPACE_SEP \
                 _ALLOCATE_REQ_QU_FAILED_ERR _NEW_LINE);

        return ret;
    }

    /* (4)                                                               */
    /* Setting the max sectors limit for a request for the request queue */
    /* in 512-byte units.                                                */
    blk_queue_max_hw_sectors(viosim_req_qu, DEVICE_REQ_QU_MAX_HW_SECTORS);

    /* (5)                                        */
    /* Allocating the "gendisk" device structure. */
    viosim_disk = alloc_disk(DEVICE_MINOR_NUMS_MAX);

    if (viosim_disk == NULL) {
        ret = -EXIT_FAILURE;

        pr_alert(_MODULE_NAME _COLON_SPACE_SEP \
                 _ALLOCATE_DEVICE_STRUCT_FAILED_ERR _NEW_LINE);

        /* Destroying the request queue. */
        blk_cleanup_queue(viosim_req_qu);

        /* Deregistering the block device. */
        unregister_blkdev(major_num, DEVICE_NAME);

        return ret;
    }

    /* --- Filling the "gendisk" device structure - Begin ------------------ */
    viosim_disk->major        = major_num;
    viosim_disk->first_minor  = DEVICE_MINOR_NUM_FIRST;
    viosim_disk->minors       = DEVICE_MINOR_NUMS_MAX;

    sprintf(viosim_disk->disk_name, DEVICE_NAME);

    viosim_disk->fops         = &viosim_ops;
    viosim_disk->queue        = viosim_req_qu;
    viosim_disk->private_data = DEVICE_NAME; /* <== Does it need for debug */
                                             /*     purposes only?         */

    set_capacity(viosim_disk, DEVICE_NUMBER_OF_PAGES * \
                              DEVICE_NUMBER_OF_SECTORS_PER_PAGE);
    /* --- Filling the "gendisk" device structure - End -------------------- */

    /* (6)(7)(8)                                       */
    /* Creating the device read and write wait queues. */
    init_waitqueue_head(&viosim_r_reqsz_wait_qu);
    init_waitqueue_head(&viosim_r_block_wait_qu);
    init_waitqueue_head(&viosim_w_block_wait_qu);

    /* (9)                                                                  */
    /* Setting up the "work_struct" device structure which represents tasks */
    /* to be run out of a workqueue.                                        */
    INIT_WORK(&viosim_req_task, (void *) viosim_req_exec);

    /* (10)                                                        */
    /* Adding the device into the system, i.e. allowing the kernel */
    /* to deal with the device.                                    */
    add_disk(viosim_disk);

    return ret;
}

/** Clean up and remove a block device driver module. */
static void __exit virtblkiosim_exit(void) {
    pr_info(_MODULE_NAME _COLON_SPACE_SEP _REMOVE_MODULE_MSG _NEW_LINE);

    /* (1)(2)                                                 */
    /* Removing references to the "gendisk" device structure. */
    del_gendisk(viosim_disk);
    put_disk(viosim_disk);

    /* (3)                           */
    /* Destroying the request queue. */
    blk_cleanup_queue(viosim_req_qu);

    /* (4)                             */
    /* Deregistering the block device. */
    unregister_blkdev(major_num, DEVICE_NAME);

    pr_info(_MODULE_NAME _COLON_SPACE_SEP \
            _UNREGISTER_AND_REMOVE_MODULE_DONE_MSG _NEW_LINE);
}

/* Initializing the block device driver module. */
module_init(virtblkiosim_init);

/* Cleaning up and removing the block device driver module. */
module_exit(virtblkiosim_exit);

MODULE_DESCRIPTION ( _MODULE_DESCRIPTION );
MODULE_VERSION     ( _MODULE_VERSION     );
MODULE_AUTHOR      ( _MODULE_AUTHOR      );
MODULE_LICENSE     ( _MODULE_LICENSE     );

/* vim:set nu:et:ts=4:sw=4: */
