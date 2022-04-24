#include <stdio.h>
#include "paging.h"
#include "allocation.h"


int main(int argc, char* argv[])
{
  (void) argc;
  (void) argv;

  init_frames();
  free_frame(allocate_frame());

  printf("coucou\n");
}
