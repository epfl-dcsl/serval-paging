#include <stdio.h>
#include "paging.h"
#include "allocation.h"


int main(int argc, char* argv[])
{
  (void) argc;
  (void) argv;

  init_frames();

  pn_t frame_number = pick_free_frame();
  allocate_frame(frame_number);
  free_frame(frame_number);
}
