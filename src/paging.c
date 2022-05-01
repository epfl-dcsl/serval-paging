#include "paging.h"

extern struct page_frame* all_frames;
extern struct frame_metadata* frames_metadata;


int map_page_table_l3_entry(pn_t l4e, uint16_t l4_offset, pn_t l3e)
{
  if (l4_offset >= 0x200)
    return 1;
  if (frames_metadata[l4e].type != PAGE_L4_ENTRY)
    return 1;
  if (frames_metadata[l3e].type != PAGE_L3_ENTRY)
    return 1;

  ((void**) (all_frames[l4e].content)) [l4_offset] = frames_metadata[l3e].address;

  return 0;
}

int map_page_table_l2_entry(pn_t l3e, uint16_t l3_offset, pn_t l2e)
{
  if (l3_offset >= 0x200)
    return 1;
  if (frames_metadata[l3e].type != PAGE_L3_ENTRY)
    return 1;
  if (frames_metadata[l2e].type != PAGE_L2_ENTRY)
    return 1;

  ((void**) (all_frames[l3e].content)) [l3_offset] = frames_metadata[l2e].address;

  return 0;
}


int map_page_table_l1_entry(pn_t l2e, uint16_t l2_offset, pn_t l1e)
{
  if (l2_offset >= 0x200)
    return 1;
  if (frames_metadata[l2e].type != PAGE_L2_ENTRY)
    return 1;
  if (frames_metadata[l1e].type != PAGE_L1_ENTRY)
    return 1;

  ((void**) (all_frames[l2e].content)) [l2_offset] = frames_metadata[l1e].address;

  return 0;
}

int map_page_table_frame(pn_t l1e, uint16_t l1_offset, pn_t frame)
{
  if (l1_offset >= 0x200)
    return 1;
  if (frames_metadata[l1e].type != PAGE_L1_ENTRY)
    return 1;
  if (frames_metadata[frame].type != PAGE_FRAME)
    return 1;

  ((void**) (all_frames[l1e].content)) [l1_offset] = frames_metadata[frame].address;

  return 0;
}

