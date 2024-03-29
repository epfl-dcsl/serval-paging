#include "allocation.h"
#include <strings.h>


/*
 * This is the global list of free frames of the machine
 */
struct page_frame all_frames[NUMBER_OF_FRAMES];

/*
 * This is the global array of metadata of all the frames of the system
 */
struct frame_metadata frames_metadata[NUMBER_OF_FRAMES];
struct frame_metadata fm;

/*
 * This is the number of the first free page
 * No proof is done on this value, it is only used as a hint by applications
 */
pn_t first_free = 0;

/*
 * This is the PID of the currently running process
 */
pid_t current;

/*
 * This is a mapping from a PID to its address space, aka CR3 aka page table
 * root
 */
pn_t address_space_mapping[NUMBER_OF_USERS];


int init_frames(void)
{
  size_t i;
  pn_t toplev;

  // initializing the frames metadata
  for (i=0; i<NUMBER_OF_FRAMES; i++)
  {
    frames_metadata[i].address = (uint64_t) &all_frames[i];
    frames_metadata[i].next_free = i+1;
    frames_metadata[i].type = PAGE_FREE;
    frames_metadata[i].refcount = 0;
    frames_metadata[i].permissions = 0;
    frames_metadata[i].owner = 0;
    frames_metadata[i].entry_count = 0;
  }
  // initializing users' address space mappings
  for (i=0; i<NUMBER_OF_USERS; i++)
  {
    current = i;
    toplev = pick_free_frame();
    if (toplev == INVALID_PAGE_NUMBER)
      return 1;
    if (allocate_frame(toplev, PAGE_L4_ENTRY, 0) != 0)
      return 1;
    address_space_mapping[i] = toplev;
  }
  /* fixing the next free frame for the last frame */
  frames_metadata[i-1].next_free = INVALID_PAGE_NUMBER;

  current = 0;

  return 0;
}


pn_t pick_free_frame(void)
{
  pn_t frame_number = first_free;

  if (frame_number >= NUMBER_OF_FRAMES)
    return INVALID_PAGE_NUMBER;

  pn_t previous_frame;

  if (first_free == frame_number)
  {
    first_free = frames_metadata[frame_number].next_free;
    return frame_number;
  }

  // TODO: panic when reaching INVALID_PAGE_NUMBER
  for (previous_frame = first_free;
       frames_metadata[previous_frame].next_free != frame_number;
       previous_frame = frames_metadata[previous_frame].next_free);

  frames_metadata[previous_frame].next_free =
    frames_metadata[frame_number].next_free;

  return frame_number;
}

void add_free_frame_to_free_list(pn_t frame_number)
{
  first_free = frame_number;
  frames_metadata[frame_number].next_free = first_free;
}

int allocate_frame(pn_t frame_number, page_type_t type, uint32_t permissions)
{
  /* You can't allocate a page that doesn't exist */
  if (frame_number >= NUMBER_OF_FRAMES)
    return 1;
  /* You can't allocate a page that is already allocated */
  if (frames_metadata[frame_number].type != PAGE_FREE)
    return 1;
  if (type == PAGE_FREE)
    return 1;

  frames_metadata[frame_number].refcount = 0;
  frames_metadata[frame_number].entry_count = 0;
  frames_metadata[frame_number].permissions = permissions;
  frames_metadata[frame_number].type = type;
  frames_metadata[frame_number].owner = current;
  return 0;
}

int free_frame(pn_t frame_number)
{
  /* You can't free a frame that does not exist */
  if (frame_number >= NUMBER_OF_FRAMES)
    return 1;
  /* You can't free a frame if it's not yours */
  if (frames_metadata[frame_number].owner != current)
    return 1;
  /* You can't free a frame that is still referenced by page table entries */
  if (frames_metadata[frame_number].refcount > 0)
    return 1;
  /* You can't free a page table entry has non-empty entries */
  if (frames_metadata[frame_number].type == PAGE_L4_ENTRY ||
      frames_metadata[frame_number].type == PAGE_L3_ENTRY ||
      frames_metadata[frame_number].type == PAGE_L2_ENTRY ||
      frames_metadata[frame_number].type == PAGE_L1_ENTRY)
    if (frames_metadata[frame_number].entry_count > 0)
      return 1;

  frames_metadata[frame_number].type = PAGE_FREE;
  frames_metadata[frame_number].refcount = 0;
  frames_metadata[frame_number].permissions = 0;
  frames_metadata[frame_number].owner = 0;
  frames_metadata[frame_number].entry_count = 0;

  return 0;
}
