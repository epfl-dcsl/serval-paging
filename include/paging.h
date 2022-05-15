#ifndef __PAGING_H__
#define __PAGING_H__

#include <stddef.h>
#include "allocation.h"



/* 
 * Functions to map a frame to a given address
 * Each user is only given a single L4 entry, so `map_page_table_l3_entry`
 * does not need to take the L4 entry as an argument
 */
int map_page_table_l3_entry(uint16_t l4_offset, pn_t l3e);
int map_page_table_l2_entry(pn_t l3e, uint16_t l3_offset, pn_t l2e);
int map_page_table_l1_entry(pn_t l2e, uint16_t l2_offset, pn_t l1e);
int map_page_table_frame(pn_t l1e, uint16_t l1_offset, pn_t frame);

/*
 * Function to remove a mapping from a page table entry
 */
int unmap_page_table_entry(pn_t pte, uint16_t l4_offset, pn_t pte_entry);



#endif
