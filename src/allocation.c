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


void init_frames(void)
{
  size_t i;

  for (i=0; i<NUMBER_OF_FRAMES; i++)
  {
    frames_metadata[i].next_free = i+1;
    frames_metadata[i].type = PAGE_FREE;
    frames_metadata[i].refcount = 0;
    frames_metadata[i].permissions = 0;
    frames_metadata[i].owner = 0;
    frames_metadata[i].entry_count = 0;
  }
  /* fixing the next free frame for the last frame */
  frames_metadata[i-1].next_free = INVALID_PAGE_NUMBER;

  current = 0;
}


uint64_t pick_free_frame(void)
{
  return first_free;
}

void remove_allocated_frame_from_free_list(pn_t frame_number)
{
  if (frame_number >= NUMBER_OF_FRAMES)
    return;

  pn_t previous_frame;

  if (first_free == frame_number)
  {
    first_free = frames_metadata[frame_number].next_free;
    return;
  }

  // TODO: panic when reaching INVALID_PAGE_NUMBER
  for (previous_frame = first_free; 
       frames_metadata[previous_frame].next_free != frame_number;
       previous_frame = frames_metadata[previous_frame].next_free);

  frames_metadata[previous_frame].next_free = 
    frames_metadata[frame_number].next_free;
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

  // TODO: check refcount to panic if something's wrong
  /* Zeroing the freshly allocated frame */
  //size_t i;
  //for (i=0; i<PAGE_SIZE; i++)
  //  all_frames[frame_number].content[i] = 0;

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
  frames_metadata[frame_number].refcount -= 1;
  frames_metadata[frame_number].permissions = 0;
  frames_metadata[frame_number].owner = 0;
  frames_metadata[frame_number].type = PAGE_FREE;

  return 0;
}
