#include "allocation.h"


/*
 * This is the global list of free frames used by other functions
 */
struct page_frame* free_frames;


void init_frames()
{
}


struct page_frame* allocate_frame()
{
  return NULL;
}


void free_frame(struct page_frame* frame)
{
  (void) frame;
}
