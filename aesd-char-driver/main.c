/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/fs.h> // file_operations. For MKDEV()
#include <linux/slab.h> // for kfree()
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Spencer Manning"); /** DONE: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("Opening aesdchar module");

    // DONE: handle open
    // "Use inode->i_cdev with container_of to locate within aesd_dev"
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    // "Set filp->private_data with our aesd_dev device struct"
    filp->private_data = dev;

    // Check for device errors or other hardware problems if necessary

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("Releasing aesdchar module");
    // DONE: handle release
    // Do nothing like scull does since filp->private_data was allocated in module_init().
    // Will be freed in module_exit().
    return 0;
}

/*
 b. Return the content (or partial content) related to the most recent 10 write commands,
 in the order they were received, on any read attempt.
    1. You should use the position specified in the read to determine the location and number of bytes to return.
    2. You should honor the count argument by sending only up to the first “count” bytes
    back of the available bytes remaining.
        *** I do this indirectly because even though I only send 1 byte back per read, the calling fuction will
        *** call aesd_read() multiple times to get the full count.
*/
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_buffer_entry *entry;
    struct aesd_dev *dev = filp->private_data;
    size_t offset_byte_rtn = 0;

    PDEBUG("Reading up to 0x%zx bytes at offset %lld", count, *f_pos);
    // DONE: handle read
    /*
    Private_data member from filp can be used to get aesd_dev
    buff is the buffer to fill
        Need to use copy_to_user to access this buffer directly
    count is the max number of bytes to buf. May want/need less than this.
        The number of bytes to read back into userspace using buff
    f_pos is the pointer to the read offset
        References a location in your virtual device
        A specific byte of the circular buffer (char_offset)
        Start the read at this offset
        Update the pointer to point to the next offset

    Return:
    If retval == count, the requested number of bytes were transferred
    If 0 < retval < count, only a portion has been returned (partial read). Encouraged.
        "Read one command at a time from your driver.""
    If 0, end of file
    If negative, error occurred
    */

    // Need to make sure that filp and buf are defined
    if ((!filp) || (!buf)) {
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&dev->lock)) {
        PDEBUG("Mutex not acquired");
        // restart because mutex interrupted and we shouldn't continue
        return -ERESTARTSYS;
    }

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circ_buffer, *f_pos, &offset_byte_rtn);

    if (entry) {
        if (copy_to_user(buf, entry->buffptr+offset_byte_rtn, 1)) {
            // something bad occurred during copy to user
            retval = -EFAULT;
        }

        // increment the byte offset pointer value to be used the next time aesd_read() is called
        (*f_pos)++;
        retval = 1;
    }
    // else no entry found and reached end of the circular buffer

    mutex_unlock(&dev->lock);
    return retval;
}


/*
  a. Allocate memory for each write command as it is received,
  supporting any length of write request (up to the length of memory which can be allocated through kmalloc),
  and saving the write command content within allocated memory.
    i. The write command will be terminated with a \n character as was done with the aesdsocket application.
        1. Write operations which do not include a \n character should be saved and appended by
        future write operations and should not be written to the circular buffer until terminated.
    ii. The content for the most recent 10 write commands should be saved.
    iii. Memory associated with write commands more than 10 writes ago should be freed.
    iv. Write position and write file offsets can be ignored on this assignment,
    each write will just write to the most recent entry in the history buffer or append to any unterminated command.
    v. For the purpose of this assignment you can use kmalloc for all allocations
    regardless of size, and assume writes will be small enough to work with kmalloc.
*/
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -EAGAIN;
    const char *lost_entry = NULL;
    struct aesd_dev *dev = filp->private_data;
    // The data to write needs to be dynamically allocated to work with copy_from_user()
    // FIXME: Using count+1 to account for the newline character. IS THIS NECESSARY??
    char *temp_write_data = kmalloc(count+1, GFP_KERNEL);
    // char *temp_write_data = kmalloc(count, GFP_KERNEL);
    struct aesd_buffer_entry new_entry;

    // Have to put this after kmalloc to satisfy the compiler.
    // This is to account for the null terminator.
    // The code works without this, but this just helps to make sure the null terminator is included in the input buffer.
    // count += 1;

    PDEBUG("----------------->");
    PDEBUG("Writing %zu bytes at offset of %lld", count, *f_pos);
    PDEBUG("buf: %s", buf);
    PDEBUG("dev->incomplete_write_buffer: %s", dev->incomplete_write_buffer);
    PDEBUG("dev->incomplete_write_buffer_size: %zu", dev->incomplete_write_buffer_size);
    PDEBUG("-----------------.");

    // DONE: handle write
    /*
    fpos - will either append to the command being written when no newline received or
        write to the command buffer when newline received.

    Return:
    If retval == count, success number of bytes written
    If less than count, only part written (may be written into incomplete_write_buffer only). May retry.
    If 0, nothing written, may retry.
    If neg, error occurred.
    */

    if ((!filp) || (!buf)) {
     return -EINVAL;
    }

    if (!temp_write_data) {
        PDEBUG("Write data not kmalloc'd");
        retval = -ENOMEM; // signifies that memory was not malloc'd
        return retval;
    }

    if (copy_from_user(temp_write_data, buf, count)) {
        kfree(temp_write_data);
        PDEBUG("Copy from user didn't work");
        retval = -EFAULT;
        return retval;
    }

    if (mutex_lock_interruptible(&dev->lock) != 0) {
        PDEBUG("Couldn't lock mutex");
        return -ERESTARTSYS;
    }

    // Write all dev->incomplete_write_buffer_size + count bytes to circular buffer if a \n was found.
    if (temp_write_data != NULL && memchr(temp_write_data, '\n', count)) {
        PDEBUG("Found a newline in the write data");

        // If haven't yet started to fill in the write buffer
        if (dev->incomplete_write_buffer == NULL) {
            PDEBUG("Started a new_entry");
            new_entry.buffptr = temp_write_data;
            new_entry.size = count;
        }
        else { // append the \n data to previous data
            PDEBUG("Appending \\n data to previous data");
            // if (temp_write_data != NULL) { // to satisfy the compiler warnings. Didn't work.
                dev->incomplete_write_buffer = krealloc(dev->incomplete_write_buffer, dev->incomplete_write_buffer_size + count, GFP_KERNEL);

                if (!dev->incomplete_write_buffer) {
                    PDEBUG("Realloc didn't work for %s", dev->incomplete_write_buffer);
                    kfree(dev->incomplete_write_buffer);
                    mutex_unlock(&dev->lock);
                    return -ENOMEM;
                }

                // copy temp_write_data to the end of the incomplete_write_buffer
                memcpy(dev->incomplete_write_buffer + dev->incomplete_write_buffer_size, temp_write_data, count);
                new_entry.buffptr = dev->incomplete_write_buffer;
                new_entry.size = dev->incomplete_write_buffer_size + count;
            // }
            // else {
            //     PDEBUG("temp_write_data is NULL");
            //     mutex_unlock(&dev->lock);
            //     return -ENOMEM;
            // }
        }

        PDEBUG("Adding entry to circular buffer");
        lost_entry = aesd_circular_buffer_add_entry(&dev->circ_buffer, &new_entry);

        if (lost_entry) {
            PDEBUG("Freeing one entry from circular buffer because it was overwritten.");
            kfree(lost_entry); // FIXME: Will this work??
            lost_entry = NULL;
        }
        // clear out incomplete_write_buffer and incomplete_write_size
        dev->incomplete_write_buffer = NULL;
        dev->incomplete_write_buffer_size = 0;

        retval = count;
    }
    else { // append to incomplete_write_buffer because a \n was not yet found.
        PDEBUG("Appending to incomplete_write_buffer, but not writing to circular buffer.");
        dev->incomplete_write_buffer = krealloc(dev->incomplete_write_buffer, dev->incomplete_write_buffer_size + count, GFP_KERNEL);

        if (!dev->incomplete_write_buffer) {
            PDEBUG("Realloc didn't work for %s", dev->incomplete_write_buffer);
            kfree(dev->incomplete_write_buffer);
            mutex_unlock(&dev->lock);
            return -ENOMEM;
        }

        // copy temp_write_data to the end of the incomplete_write_buffer
        memcpy(dev->incomplete_write_buffer + dev->incomplete_write_buffer_size, temp_write_data, count);
        dev->incomplete_write_buffer_size += count;

        retval = count; // NEEDS to return count even if not writing to circular buffer
    }

    mutex_unlock(&dev->lock);
    *f_pos += retval;
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar"); // calls register_chrdev_region()
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    // DONE: initialize the AESD specific portion of the device
    // Initializing the locking primitive here for example
    aesd_circular_buffer_init(&aesd_device.circ_buffer);
    mutex_init(&aesd_device.lock);
    aesd_device.incomplete_write_buffer = NULL;
    aesd_device.incomplete_write_buffer_size = 0;
    result = aesd_setup_cdev(&aesd_device);

    if (result) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    struct aesd_buffer_entry *entry;
    int i = 0;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    // DONE: cleanup AESD specific portions here as necessary
    // Balance what I initialized in the aesd_init_module. ie Free memory, stop using locking primitiives.
    // Remove all members of buffer
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circ_buffer, i) {
        kfree(entry->buffptr);
    }

    mutex_destroy(&aesd_device.lock);
    unregister_chrdev_region(devno, 1);
    PDEBUG("Clean up the aesd module");
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
