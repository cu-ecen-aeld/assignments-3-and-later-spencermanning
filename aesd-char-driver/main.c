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
#include <linux/string.h> // for strchr()
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Spencer Manning"); /** DONE: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");

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
    PDEBUG("release");
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
*/
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_buffer_entry *entry;
    struct aesd_dev *dev = filp->private_data;
    size_t offset_byte_rtn = 0;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
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
        return 1;
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

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    // DONE: handle write
    /*
    fpos - will either append to the command being written when no newline received or
        write to the command buffer when newline received.
    
    Return:
    TODO: Technically, I am able to write MORE than count into the buffer if I had previously received a non-null
            terminated buffer to write. Is returning more than count possible? Will this break the test script?
    If retval == count, success number of bytes written
    If less than count, only part written. May retry. (I'm not doing)
    If 0, nothing written, may retry.
    If neg, error occurred.
    */

    if ((!filp) || (!buf)) {
     return -EINVAL;
    }

    PDEBUG("Writing %zu bytes at offset of %lld", count, *f_pos);

    struct aesd_dev *dev = filp->private_data;

    // The data to write needs to be dynamically allocated to be available 
    // char *temp_write_data = kmalloc(count, GFP_KERNEL);
    char *temp_write_data = NULL; // FIXME: Can I get by with just this instead of allocating the write data here?

    // if (!temp_write_data) {
    //     PDBEBUG("Write data not kmalloc'd");
    //     retval = -ENOMEM; // signifies that memory was not malloc'd
    //     return retval;
    // }

    if (copy_from_user(temp_write_data, buf, count)) {
        // kfree(temp_write_data);
        retval = -EFAULT;
        return retval;
    }

    if (mutex_lock_interruptible(&dev->lock) != 0) {
        PDEBUG("Couldn't lock mutex");
        return -ERESTARTSYS;
    }

    // Write all dev->incomplete_write_buffer_size + count bytes to circular buffer if a \n was found.
    if (strchr(temp_write_data, '\n')) {
        struct aesd_buffer_entry new_entry;
        
        // If haven't yet started to fill in the write buffer
        if (dev->incomplete_write_buffer == NULL) {
            new_entry.buffptr = temp_write_data;
            new_entry.size = count;
        }
        else { // append 
            memcpy(dev->incomplete_write_buffer + dev->incomplete_write_buffer_size, temp_write_data, count);
            new_entry.buffptr = dev->incomplete_write_buffer;
            new_entry.size = dev->incomplete_write_buffer_size + count;
        }
        
        lost_entry = aesd_circular_buffer_add_entry(&dev->circ_buffer, &new_entry);
        
        if (lost_entry) {
            kfree(lost_entry); // FIXME: Will this work??
            lost_entry = NULL;
        }
        // clear out incomplete_write_buffer and incomplete_write_size
        dev->incomplete_write_buffer = NULL;
        dev->incomplete_write_buffer_size = 0;

        retval = count;
    }
    else { // append to saved write buffer because a \n was not yet found.
        // temp_write_data = krealloc(dev->incomplete_write_buffer, count, GFP_KERNEL);
        // if (!dev->incomplete_write_buffer) {
        //     PDEBUG("Realloc didn't work for %s", dev->incomplete_write_buffer);
        //     kfree(dev->incomplete_write_buffer);
        //     return -ENOMEM;
        // }

        memcpy(dev->incomplete_write_buffer + dev->incomplete_write_buffer_size, temp_write_data, count);
        // TODO: Confirm the index value is right
        // dev->incomplete_write_buffer[dev->incomplete_write_buffer_size + count] = temp_write_data; 
        dev->incomplete_write_buffer_size += count;
        retval = 0; // nothing will be currently written
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
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    // DONE: cleanup AESD specific portions here as necessary
    // Balance what I initialized in the aesd_init_module. ie Free memory, stop using locking primitiives.
    // Remove all members of buffer
    struct aesd_buffer_entry *entry;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circ_buffer, int i = 0) {
        kfree(entry->buffptr);
    }

    mutex_destroy(&aesd_device.lock);
    unregister_chrdev_region(devno, 1);
    PDEBUG("Cleanup the aesd module");
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
