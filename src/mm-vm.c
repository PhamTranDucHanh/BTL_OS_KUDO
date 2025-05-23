// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = pvma->vm_id;

  while (vmait < vmaid)
  {
    if (pvma == NULL)
      return NULL;

    pvma = pvma->vm_next;
    vmait = pvma->vm_id;
  }

  return pvma;
}

int __mm_swap_page(struct pcb_t *caller, int vicfpn , int swpfpn)
{
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg;
  /* TODO retrive current vma to obtain newrg, current comment out due to compiler redundant warning*/
  //struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  // newrg = malloc(sizeof(struct vm_rg_struct));

  /* TODO: update the newrg boundary
  // newrg->rg_start = ...
  // newrg->rg_end = ...
  */
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   newrg = malloc(sizeof(struct vm_rg_struct));
 
   newrg->rg_start = cur_vma->sbrk;
   newrg->rg_end = newrg->rg_start + size;
 
   cur_vma->sbrk += size;
  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  //struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm,vmaid);
  // struct vm_area_struct *node = caller->mm->mmap;


  // while (node != NULL) {
  // 	if (cur_vma->vm_end)
  // 	node = node->vm_next;
  // }

  /* TODO validate the planned memory area is not overlapped */
  struct vm_area_struct *vma = caller->mm->mmap;
  if(vma->vm_end - vma->vm_start > caller->mram->maxsz) return -1;
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int origin_size)
{
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  //int inc_amt = PAGING_PAGE_ALIGNSZ(origin_size); //  	512 = 256 * 2
  int inc_amt = PAGING_PAGE_ALIGNSZ(origin_size + cur_vma->sbrk) - PAGING_PAGE_ALIGNSZ(cur_vma->sbrk);
  int incnumpage = inc_amt / PAGING_PAGESZ;		  // 	2


  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, origin_size, inc_amt);

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* The obtained vm area (only)
   * now will be alloc real ram region */
  cur_vma->vm_end += inc_amt;
  // cur_vma->sbrk += inc_sz;
  // if (vm_map_ram(caller, area->rg_start, area->rg_end,
  // 			   old_end, incnumpage, newrg) < 0)
  // 	return -1; /* Map the memory to MEMRAM */
  if (vm_map_ram(caller, area->rg_start, area->rg_end,
           old_end, incnumpage, area) < 0)
    return -1; /* Map the memory to MEMRAM */

  free(area);
  return 0;
}
 /* find min element 
  * 
  */
 void find_min(int* arr, int size, int* ret_elmnt) {
  *ret_elmnt = 0;
  for (int i = 1; i < size; ++i) {
    if (arr[*ret_elmnt] < arr[i]) *ret_elmnt = i;
  }
}

// #endif
