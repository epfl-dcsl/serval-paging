#include "paging.h"

extern struct page_frame* all_frames;
extern struct frame_metadata* frames_metadata;
extern pid_t current;


/*
 * This is a mapping from a PID to its address space, aka CR3 aka page table
 * root
 */
pn_t address_space_mapping[NUMBER_OF_USERS];

void init_address_spaces(void)
{
  size_t i;

  for (i=0; i<NUMBER_OF_USERS; i++)
  {
    address_space_mapping[i] = INVALID_PAGE_NUMBER;
  }
}


int map_page_table_l3_entry(uint16_t l4_offset, pn_t l3e)
{
  pn_t current_root = address_space_mapping[current];

  /* You can't map a PTE entry to an offset that doesn't fit on 9 bits */
  if (l4_offset >= 0x200)
    return 1;
  /* You can't map a PTE to your L4 root if it is not an L3 entry */
  if (frames_metadata[l3e].type != PAGE_L3_ENTRY)
    return 1;
  /* You can't map a private PTE to your L4 root if your L4 root is shared */
  if ((frames_metadata[l3e].permissions & PAGE_ACCESS_SHARED) <
      (frames_metadata[current_root].permissions & PAGE_ACCESS_SHARED))
    return 1;
  /* You can't map to your address space a private PTE that you don't own */
  if (!(frames_metadata[l3e].permissions & PAGE_ACCESS_SHARED) &&
      frames_metadata[l3e].owner != current)
    return 1;


  ((void**) (all_frames[current_root].content)) [l4_offset] = &all_frames[l3e];
  frames_metadata[current_root].entry_count += 1;
  frames_metadata[l3e].refcount += 1;

  return 0;
}

int map_page_table_l2_entry(pn_t l3e, uint16_t l3_offset, pn_t l2e)
{
  /* You can't map a PTE entry to an offset that doesn't fit on 9 bits */
  if (l3_offset >= 0x200)
    return 1;
  /* You can't map a PTE to an L3 PTE if it is not an L2 entry */
  if (frames_metadata[l3e].type != PAGE_L3_ENTRY)
    return 1;
  if (frames_metadata[l2e].type != PAGE_L2_ENTRY)
    return 1;
  /* You can't map a shared PTE's offset to a private PTE */
  if ((frames_metadata[l2e].permissions & PAGE_ACCESS_SHARED) <
      (frames_metadata[l3e].permissions & PAGE_ACCESS_SHARED))
    return 1;
  /* You can't map to your address space a private PTE that you don't own */
  if (!(frames_metadata[l2e].permissions & PAGE_ACCESS_SHARED) &&
      frames_metadata[l2e].owner != current)
    return 1;

  ((void**) (all_frames[l3e].content)) [l3_offset] = &all_frames[l2e];
  frames_metadata[l3e].entry_count += 1;
  frames_metadata[l2e].refcount += 1;

  return 0;
}


int map_page_table_l1_entry(pn_t l2e, uint16_t l2_offset, pn_t l1e)
{
  /* You can't map a PTE entry to an offset that doesn't fit on 9 bits */
  if (l2_offset >= 0x200)
    return 1;
  /* You can't map a PTE to an L2 PTE if it is not an L1 entry */
  if (frames_metadata[l2e].type != PAGE_L2_ENTRY)
    return 1;
  if (frames_metadata[l1e].type != PAGE_L1_ENTRY)
    return 1;
  /* You can't map a shared PTE's offset to a private PTE */
  if ((frames_metadata[l1e].permissions & PAGE_ACCESS_SHARED) <
      (frames_metadata[l2e].permissions & PAGE_ACCESS_SHARED))
    return 1;
  /* You can't map to your address space a private PTE that you don't own */
  if (!(frames_metadata[l1e].permissions & PAGE_ACCESS_SHARED) &&
      frames_metadata[l1e].owner != current)
    return 1;

  ((void**) (all_frames[l2e].content)) [l2_offset] = &all_frames[l1e];
  frames_metadata[l2e].entry_count += 1;
  frames_metadata[l1e].refcount += 1;

  return 0;
}

int map_page_table_frame(pn_t l1e, uint16_t l1_offset, pn_t frame)
{
  /* You can't map a frame to an offset that doesn't fit on 9 bits */
  if (l1_offset >= 0x200)
    return 1;
  /* You can't map a PTE to an L1 PTE if it is not a frame */
  if (frames_metadata[l1e].type != PAGE_L1_ENTRY)
    return 1;
  if (frames_metadata[frame].type != PAGE_FRAME)
    return 1;
  /* You can't map a shared PTE's offset to a private frame */
  if ((frames_metadata[frame].permissions & PAGE_ACCESS_SHARED) <
      (frames_metadata[l1e].permissions & PAGE_ACCESS_SHARED))
    return 1;
  /* You can't map to your address space a private frame that you don't own */
  if (!(frames_metadata[frame].permissions & PAGE_ACCESS_SHARED) &&
      frames_metadata[frame].owner != current)
    return 1;

  ((void**) (all_frames[l1e].content)) [l1_offset] = &all_frames[frame];
  frames_metadata[l1e].entry_count += 1;
  frames_metadata[frame].refcount += 1;

  return 0;
}

int unmap_page_table_entry(pn_t pte, uint16_t offset, pn_t pte_entry)
{
  /* You can't unmap an offset that is not addressable with 9 bits */
  if (offset >= 0x200)
    return 1;
  /* You can't unmap an entry of something other than a PTE */
  if (frames_metadata[pte].type != PAGE_L4_ENTRY &&
      frames_metadata[pte].type != PAGE_L3_ENTRY &&
      frames_metadata[pte].type != PAGE_L2_ENTRY &&
      frames_metadata[pte].type != PAGE_L1_ENTRY)
    return 1;
  /* You can't unmap an empty entry */
  if (((void**) (&all_frames[pte])) [offset] == NULL)
    return 1;

  ((void**) (all_frames[pte].content)) [offset] = NULL;
  frames_metadata[pte].entry_count -= 1;
  frames_metadata[pte_entry].refcount -= 1;

  return 0;
}
