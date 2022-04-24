#include "paging.h"


struct vm_area* allocate_vm_area(struct address_space root, size_t size, struct vm_area_tags tags)
{
  (void) root;
  (void) size;
  (void) tags;

  return NULL;
}


int destroy_vm_area(struct vm_area* vma)
{
  (void) vma;

  return 0;
}
