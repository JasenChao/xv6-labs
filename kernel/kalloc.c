// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  struct spinlock reflock; // 页面引用计数锁
  // 直接使用数组保存引用计数，简单
  char ref_count[PHYSTOP/PGSIZE];
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kmem.reflock, "kmemref");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  int i;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(i=0; i<(uint64)p/PGSIZE; i++) {
    kmem.ref_count[i] = 0;
  }
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    kmem.ref_count[(uint64)p/PGSIZE] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  char refcnt = ksubref((void*)pa);
  if (refcnt > 0)
    return;
  
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&kmem.reflock);
    // 初始引用次数记为1
    kmem.ref_count[(uint64)r / PGSIZE] = 1;
    release(&kmem.reflock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

inline void
acquire_refcnt(){
    acquire(&kmem.reflock);
}

inline void
release_refcnt(){
    release(&kmem.reflock);
}

char
kgetref(void *pa){
  acquire_refcnt();
  char refcnt = kmem.ref_count[(uint64)pa/PGSIZE];
  release_refcnt();
  return refcnt;
}

void
kaddref(void *pa){
  acquire_refcnt();
  kmem.ref_count[(uint64)pa/PGSIZE]++;
  release_refcnt();
}

char
ksubref(void *pa){
  acquire_refcnt();
  char refcnt = --kmem.ref_count[(uint64)pa/PGSIZE];
  release_refcnt();
  return refcnt;
}
