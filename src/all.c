#include "all.h"
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



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//                            PAGING                                        //
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////



void init_address_spaces(void)
{
  size_t i;

  for (i=0; i<NUMBER_OF_USERS; i++)
  {
    address_space_mapping[i] = INVALID_PAGE_NUMBER;
  }
}


int map_page_table_l3_entry(pn_t l4e, uint16_t l4_offset, pn_t l3e)
{
  //pn_t current_root = address_space_mapping[current];

  /* You can't map a PTE entry to an offset that doesn't fit on 9 bits */
  if (l4_offset >= 0x200)
    return 1;
  /* You can't map an entry that does not exist */
  if (l4e >= NUMBER_OF_FRAMES || l3e >= NUMBER_OF_FRAMES)
    return 1;
  /* You can't map a PTE to your L4 root if it is not an L3 entry */
  if (frames_metadata[l3e].type != PAGE_L3_ENTRY)
    return 1;
  /* You can't map a private PTE to your L4 root if your L4 root is shared */
  if ((frames_metadata[l3e].permissions & PAGE_ACCESS_SHARED) <
      (frames_metadata[l4e].permissions & PAGE_ACCESS_SHARED))
    return 1;
  /* You can't map to your address space a private PTE that you don't own */
  if (!(frames_metadata[l3e].permissions & PAGE_ACCESS_SHARED) &&
      frames_metadata[l3e].owner != current)
    return 1;
  /* You can't add mappings to an address space that is not yours */
  if (frames_metadata[l4e].owner != current)
    return 1;

  all_frames[l4e].content[l4_offset] = (uint64_t) &all_frames[l3e];
  frames_metadata[l4e].entry_count += 1;
  frames_metadata[l3e].refcount += 1;

  return 0;
}

int map_page_table_l2_entry(pn_t l3e, uint16_t l3_offset, pn_t l2e)
{
  /* You can't map a PTE entry to an offset that doesn't fit on 9 bits */
  if (l3_offset >= 0x200)
    return 1;
  /* You can't map an entry that does not exist */
  if (l3e >= NUMBER_OF_FRAMES || l2e >= NUMBER_OF_FRAMES)
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
  /* You can't add mappings to an address space that is not yours */
  if (frames_metadata[l3e].owner != current)
    return 1;

  all_frames[l3e].content[l3_offset] = (uint64_t) &all_frames[l2e];
  frames_metadata[l3e].entry_count += 1;
  frames_metadata[l2e].refcount += 1;

  return 0;
}


int map_page_table_l1_entry(pn_t l2e, uint16_t l2_offset, pn_t l1e)
{
  /* You can't map a PTE entry to an offset that doesn't fit on 9 bits */
  if (l2_offset >= 0x200)
    return 1;
  /* You can't map an entry that does not exist */
  if (l2e >= NUMBER_OF_FRAMES || l1e >= NUMBER_OF_FRAMES)
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
  /* You can't add mappings to an address space that is not yours */
  if (frames_metadata[l2e].owner != current)
    return 1;

  all_frames[l2e].content[l2_offset] = (uint64_t) &all_frames[l1e];
  frames_metadata[l2e].entry_count += 1;
  frames_metadata[l1e].refcount += 1;

  return 0;
}

int map_page_table_frame(pn_t l1e, uint16_t l1_offset, pn_t frame)
{
  /* You can't map a frame to an offset that doesn't fit on 9 bits */
  if (l1_offset >= 0x200)
    return 1;
  /* You can't map a frame or an entry that does not exist */
  if (l1e >= NUMBER_OF_FRAMES || frame >= NUMBER_OF_FRAMES)
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
  /* You can't add mappings to an address space that is not yours */
  if (frames_metadata[l1e].owner != current)
    return 1;

  all_frames[l1e].content[l1_offset] = (uint64_t) &all_frames[frame];
  frames_metadata[l1e].entry_count += 1;
  frames_metadata[frame].refcount += 1;

  return 0;
}

int unmap_page_table_entry(pn_t pte, uint16_t offset, pn_t pte_entry)
{
  /* You can't unmap an offset that is not addressable with 9 bits */
  if (offset >= 0x200)
    return 1;
  /* You can't unmap from a page table that is not a page */
  if (pte >= NUMBER_OF_FRAMES)
    return 1;
  /* You can't a page that is not a page */
  if (pte_entry >= NUMBER_OF_FRAMES)
    return 1;
  /* You can't unmap an entry of something other than a PTE */
  if (frames_metadata[pte].type != PAGE_L4_ENTRY &&
      frames_metadata[pte].type != PAGE_L3_ENTRY &&
      frames_metadata[pte].type != PAGE_L2_ENTRY &&
      frames_metadata[pte].type != PAGE_L1_ENTRY)
    return 1;
  /* You can't unmap an empty entry */
  if (((uint64_t*) (&all_frames[pte])) [offset] == 0)
    return 1;
  /* You can't unmap a page that does not belong to you */
  if (frames_metadata[pte].owner != current)
    return 1;
  /* You must specify the right number of the page you unmap */
  if (((uint64_t*) (&all_frames[pte])) [offset] != (uint64_t) &all_frames[pte_entry])
    return 1;

  ((uint64_t*) (all_frames[pte].content)) [offset] = 0;
  frames_metadata[pte].entry_count -= 1;
  frames_metadata[pte_entry].refcount -= 1;

  return 0;
}
