/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    // int entryptr = NULL;
    uint8_t index = 0;
    // uint8_t curr_pos = 0;
    struct aesd_buffer_entry *entry;
    int char_offset_signed = char_offset; // to allow it to go negative

    /* 4 possible scenarios to search through
    A. |out/in             | starting state. out = in. Buffer is empty.
    B. |  out------->in    | in has advanced but isn't yet behind out.
    C. |-->in       out----| in has rolled over and is now behind out.
    D. |------>out/in------| out has caught up to in. out = in again. Buffer is full.
    */
    // Scenario A
    if (buffer->out_offs == buffer->in_offs && !buffer->full) {
        // do nothing since the buffer has nothing in it.
    }

    // Scenario B
    if (buffer->in_offs > buffer->out_offs) {
        AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index) {
            // skip empty indices
            if (index < buffer->out_offs || index > buffer->in_offs) {
                continue;
            }

            // check current index
            char_offset_signed -= buffer->entry[index].size;
            if (char_offset_signed < 0) {
                // Add back in the subtracted size and return
                *entry_offset_byte_rtn = char_offset_signed + buffer->entry[index].size;
                return entry;
            }
        }
    }
    
    // Scenario C or Scenario D (same situation as far as looping goes)
    if ((buffer->out_offs >= buffer->in_offs) || // Scenario C
        (buffer->out_offs == buffer->in_offs && buffer->full)) { // Scenario D
        // from out_offs to the end
        AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index) {
            if (index < buffer->out_offs) {
                continue;
            }

            // check current index
            char_offset_signed -= buffer->entry[index].size;
            if (char_offset_signed < 0) {
                // Add back in the subtracted size and return
                *entry_offset_byte_rtn = char_offset_signed + buffer->entry[index].size;
                return entry;
            }
        }

        // now from the beginning to in_offs
        index = 0;
        AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index) {
            // check current index
            char_offset_signed -= buffer->entry[index].size;
            if (char_offset_signed < 0) {
                // Add back in the subtracted size and return
                *entry_offset_byte_rtn = char_offset_signed + buffer->entry[index].size;
                return entry;
            }

            if (index > buffer->in_offs) {
                // error occurred! should not get here
                return entry;
            }
        }
    }

    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    // Add the input value to the buffer
    memcpy(&buffer->entry[buffer->in_offs++], add_entry, sizeof(*add_entry));

    // Advance input offset
    buffer->in_offs++;
    buffer->in_offs %= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;

    if (buffer->full) {
        // Advance output offset as well. Previous data is lost.
        buffer->out_offs++;
        buffer->out_offs %= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    else {
        if (buffer->in_offs == buffer->out_offs) {
            buffer->full = true;
        }
    }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
