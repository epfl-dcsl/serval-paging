#ifndef __PAGING_H__
#define __PAGING_H__

#include <stddef.h>
#include "allocation.h"


/*
 * Tags of a page (mainly access rights)
 */
struct page_tags {
  int shareable;
};


/* 
 * Structures of the page table entries
 * L4s are the roots -> L1s are the leaves
 */
struct l1_entry {
  struct page_frame* frame;
  struct page_tags tags;
};

struct l2_entry {
  struct l1_entry* next;
  struct page_tags tags;
};

struct l3_entry {
  struct l2_entry* next;
  struct page_tags tags;
};

struct l4_entry {
  struct l3_entry* next;
  struct page_tags tags;
};


/* 
 * Structures that the user can interact with
 */
struct address_space {
  struct l4_entry* pte_root;
};

struct vm_area_tags {

};

struct vm_area {
  struct address_space root;
  size_t size;
  struct vm_area_tags tags;
};




/* Function to allocate a VMA */
struct vm_area* allocate_vm_area(struct address_space root, size_t size, struct vm_area_tags tags);

/* Function to free a VMA */
int destroy_vm_area(struct vm_area* vma);


#endif
