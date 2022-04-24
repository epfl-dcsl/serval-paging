#include "allocation.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*
 * This is the global list of free frames used by other functions
 */
struct page_frame* free_frames = NULL;


void init_frames()
{
  size_t i;

  free_frames = calloc(NUMBER_OF_FRAMES, PAGE_SIZE);
  assert(free_frames != NULL);

  for (i=0; i<NUMBER_OF_FRAMES-1; i++)
    *((void**) (&free_frames[i])) = (void*) &free_frames[i+1];
  *((void**) (&free_frames[i])) = NULL;
}


struct page_frame* allocate_frame()
{
  struct page_frame* result;

  if (free_frames != NULL)
  {
    result = &free_frames[0];
    free_frames = *((void**) &free_frames[0]);
  }
  else
  {
    result = NULL;
  }

  return result;
}


void free_frame(struct page_frame* frame)
{
  size_t i;
  struct page_frame* travel = free_frames;

  for (i=0; i<PAGE_SIZE; i++)
    frame->content[i] = 0;

  if (travel == NULL)
  {
    free_frames = frame;
  }
  else
  {
    while (*((void**) travel) != NULL)
      travel = *((void**) travel);
    *((void**) travel) = frame;
  }
}
