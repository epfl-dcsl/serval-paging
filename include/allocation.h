#ifndef __ALLOCATION_H__
#define __ALLOCATION_H__

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 0x1000

/*
 * A frame is just a contiguous portion of PAGE_SIZE bytes
 */
struct page_frame {
  uint8_t content[PAGE_SIZE];
};


/*
 * This is the global list of free frames used by other functions
 */
//struct page_frame* free_frames;


/*
 * Function to initialize the list of free frames
 */
void init_frames();

/*
 * Function to allocate a frame
 * Returns newly allocated frame if one is available, NULL otherwise
 */
struct page_frame* allocate_frame();

/*
 * Function to free a frame
 */
void free_frame(struct page_frame* frame);

#endif
