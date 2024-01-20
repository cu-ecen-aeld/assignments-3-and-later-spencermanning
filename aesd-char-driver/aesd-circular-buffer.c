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
#include <stdio.h> // for printf()
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

    printf("* char_offset: %zu\n", char_offset);
    // printf("* char_offset_signed: %i\n", char_offset_signed);
    printf("* buffer is full?: %i\n", buffer->full);
    printf("* buffer->in_offs: %i\n", buffer->in_offs);
    printf("* buffer->out_offs: %i\n", buffer->out_offs);

    /* 4 possible scenarios to search through
    A. |out/in             | starting state. out = in. Buffer is empty.
    B. |  out------->in    | in has advanced but isn't yet behind out.
    C. |-->in       out----| in has rolled over and is now behind out.
    D. |------>out/in------| out has caught up to in. out = in again. Buffer is full.
    */
    // Scenario A
    if (buffer->out_offs == buffer->in_offs && !buffer->full) {
        printf("----Scenario A\n");
        // do nothing since the buffer has nothing in it.
    }

    // Scenario B
    if (buffer->in_offs > buffer->out_offs) {
        printf("----Scenario B\n");
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
        printf("----Scenario C or D\n");

        uint8_t entries_checked = 0; // will count from 1 to 10
        // from out_offs to the end
        AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index) {
            // printf("Checking index %i\n", index);
            // printf("Buffer entry: %s with size: %zu\n", (char*)entry->buffptr, entry->size);
            if (index < buffer->out_offs) {
                // printf("Skipped index %i\n", index);
                continue;
            }

            entries_checked++;
            printf("entries_checked: %i\n", entries_checked);
            // check current index
            // printf("Current entry size: %zu\n", buffer->entry[index].size);
            char_offset_signed -= buffer->entry[index].size;
            // printf("char_offset_signed: %i\n", char_offset_signed);
            if (char_offset_signed < 0) {
                printf("Entry %i is the one\n", index);
                // Add back in the subtracted size and return
                *entry_offset_byte_rtn = char_offset_signed + buffer->entry[index].size;
                // printf("entry_offset_byte_rtn: %zu\n", *entry_offset_byte_rtn);
                return entry;
            }
        }
        printf("Not found from out_offs to end. Wrapping to beginning\n");
        printf("char_offset_signed: %i\n", char_offset_signed);

        // now from the beginning to in_offs if haven't already found the answer
        index = 0;
        AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, index) {
            if (entries_checked++ == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) {
                printf("Reached end of buffer. Entry not found. entries_checked: %i\n", entries_checked);
                return NULL;
            }
            printf("entries_checked: %i\n", entries_checked);
            printf("Checking index %i\n", index);
            
            // check current index
            char_offset_signed -= buffer->entry[index].size;
            if (char_offset_signed < 0) {
                // Add back in the subtracted size and return
                printf("Entry %i is the one\n", index);
                *entry_offset_byte_rtn = char_offset_signed + buffer->entry[index].size;
                return entry;
            }

            if (index > buffer->in_offs) {
                // error occurred! should not get here
                printf("Arrived an an err'd location. index: %i in_offs: %i out_offs: %i\n", index, buffer->in_offs, buffer->out_offs);
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
    memcpy(&buffer->entry[buffer->in_offs], add_entry, sizeof(*add_entry));
    // memcpy(&buffer->entry[buffer->in_offs], add_entry, add_entry->size);
    printf("add_entry: %s with size: %zu at in_offs: %i and out_offs: %i\n", (char*)add_entry->buffptr, add_entry->size, buffer->in_offs, buffer->out_offs);
    // printf("size: %zu\n", add_entry->size);
    // for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
    //     printf("buff[%i]: %s\n", i, (char*)buffer->entry[i].buffptr);
    // }
    // printf("end\n");
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
            printf("Buffer full: %i\n", buffer->full);
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
