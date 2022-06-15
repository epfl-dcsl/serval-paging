#ifndef __ALL_H__
#define __ALL_H__

#include <stddef.h>
#include <stdint.h>

typedef uint64_t pn_t;         /* page number */
typedef uint16_t pid_t;        /* identity of the page requester */
typedef uint32_t page_type_t;  /* page type */

/* Page types */
#define PAGE_FREE       0
#define PAGE_FRAME      1
#define PAGE_L1_ENTRY   2
#define PAGE_L2_ENTRY   3
#define PAGE_L3_ENTRY   4
#define PAGE_L4_ENTRY   5

/* Pages permissions */
#define PAGE_ACCESS_READ   (1 << 0)
#define PAGE_ACCESS_WRITE  (1 << 1)
#define PAGE_ACCESS_EXEC   (1 << 2)
#define PAGE_ACCESS_SHARED (1 << 3)


#define PAGE_SIZE 0x1000
#define NUMBER_OF_FRAMES 0x10000
#define INVALID_PAGE_NUMBER ((pn_t) (-1))
#define NUMBER_OF_USERS 0x400


/*
 * A frame is just a contiguous portion of PAGE_SIZE bytes
 */
struct page_frame {
  //uint8_t content[PAGE_SIZE];
  uint64_t content[PAGE_SIZE / 8];
};

/*
 * Extra metadata to keep track of allocated frames
 */
struct frame_metadata {
  uint64_t address;
  pn_t next_free; /* No proof is done on this as it represents a linked list */
  page_type_t type;
  uint32_t refcount;
  uint32_t permissions;
  pid_t owner;
  uint16_t entry_count;
}; /* struct size: 64+64+32+32+32+16+16 = 256b = 32B */

/*
 * Function to initialize the list of free frames
 */
int init_frames(void);

/*
 * Returns a page number, supposedly a free one, and removes if from the free list
 * This function is not proven and should only be used as an advice by
 * applications
 */
pn_t pick_free_frame(void);

/*
 * Function to allocate a page
 * Returns the page asked if it is available, NULL otherwise
 */
int allocate_frame(pn_t frame_number, page_type_t type, uint32_t permissions);

/* 
 * Function to call by a user to add a freshly freed frame back on top of the
 * free list
 * Because this function involves the free list, it cannot be proved
 */
void add_free_frame_to_free_list(pn_t frame_number);

/*
 * Function to free a frame
 */
int free_frame(pn_t frame_number);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                                PAGING                                     //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


/*
 * Functions to map a frame to a given address
 */
int map_page_table_l3_entry(pn_t l4e, uint16_t l4_offset, pn_t l3e);
int map_page_table_l2_entry(pn_t l3e, uint16_t l3_offset, pn_t l2e);
int map_page_table_l1_entry(pn_t l2e, uint16_t l2_offset, pn_t l1e);
int map_page_table_frame(pn_t l1e, uint16_t l1_offset, pn_t frame);

/*
 * Function to remove a mapping from a page table entry
 */
int unmap_page_table_entry(pn_t pte, uint16_t offset, pn_t pte_entry);



#endif

