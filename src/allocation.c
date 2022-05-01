#include "allocation.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*
 * This is the global list of free frames of the machine
 */
struct page_frame* all_frames = NULL;

/*
 * This is the global array of metadata of all the frames of the system
 */
struct frame_metadata* frames_metadata = NULL;

/*
 * This is the number of the first free page
 * No proof is done on this value, it is only used as a hint by applications
 */
pn_t first_free = 0;


void init_frames(void)
{
  size_t i;

  all_frames = calloc(NUMBER_OF_FRAMES, PAGE_SIZE);
  assert(all_frames != NULL);
  frames_metadata = calloc(NUMBER_OF_FRAMES, sizeof(struct frame_metadata));
  assert(frames_metadata != NULL);

  for (i=0; i<NUMBER_OF_FRAMES; i++)
  {
    frames_metadata[i].address = (void*) &all_frames[i];
    frames_metadata[i].next_free = i+1;
    frames_metadata[i].refcount = 0;
  }
  /* fixing the next free frame for the last frame */
  frames_metadata[i-1].next_free = INVALID_PAGE_NUMBER;
}


uint64_t pick_free_frame(void)
{
  return first_free;
}

void remove_allocated_page_from_free_list(pn_t frame_number)
{
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

struct page_frame* allocate_frame(pn_t frame_number)
{
  // TODO: check refcount to panic if something's wrong
  /* Zeroing the freshly allocated frame */
  size_t i;
  for (i=0; i<PAGE_SIZE; i++)
    all_frames[frame_number].content[i] = 0;

  frames_metadata[frame_number].refcount = 1;
  return &all_frames[frame_number];
}

void free_frame(pn_t frame_number)
{

  // TODO: check refcount to panic if something's wrong
  frames_metadata[frame_number].next_free = first_free;
  first_free = frame_number;
}