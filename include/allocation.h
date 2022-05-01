#ifndef __ALLOCATION_H__
#define __ALLOCATION_H__

#include <stddef.h>
#include <stdint.h>

typedef uint64_t pn_t;    /* page number */
enum page_type {           /* page type */
  PAGE_FREE,
  PAGE_FRAME,
  PAGE_L1_ENTRY,
  PAGE_L2_ENTRY,
  PAGE_L3_ENTRY,
  PAGE_L4_ENTRY,
};

#define PAGE_SIZE 0x1000
#define NUMBER_OF_FRAMES 0x10000
#define INVALID_PAGE_NUMBER ((pn_t) (-1))


/*
 * A frame is just a contiguous portion of PAGE_SIZE bytes
 */
struct page_frame {
  uint8_t content[PAGE_SIZE];
};

/*
 * Extra metadata to keep track of allocated frames
 */
struct frame_metadata {
  void* address;
  pn_t next_free; /* No proof is done on this as it represents a linked list */
  enum page_type type;
  uint32_t refcount;
};

/*
 * Function to initialize the list of free frames
 */
void init_frames(void);

/*
 * Returns a page number, supposedly a free one
 * This function is not proven and should only be used as an advice by
 * applications
 */
pn_t pick_free_frame(void);

/*
 * Function to allocate a page
 * Returns the page asked if it is available, NULL otherwise
 */
void* allocate_frame(pn_t frame_number, enum page_type type);

/* 
 * Function to call by a user to remove a freshly allocated frame from the free
 * list
 * Because this function involves the free list, it cannot be proved
 */
void remove_allocated_frame_from_free_list(pn_t frame_number);

/*
 * Function to free a frame
 */
void free_frame(pn_t frame_number);

#endif
