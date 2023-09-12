// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

// 页式存储管理
// 这个文件中的用到的地址都是物理地址

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

// 空闲链表并不保存在已分配的内存中，而是保存在空闲页面中，在每一个空闲页面的头部。
struct run {
  struct run *next;
};


// 用于保存空闲页面的链表。使用自旋锁保证线程安全
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// 初始化物理内存分配器，系统启动时只运行一次
void
kinit()
{
  initlock(&kmem.lock, "kmem"); // 初始化锁
  freerange(end, (void*)PHYSTOP);
}

// 将 pa_start 到 pa_end 范围从低到的内存页面依次加入到空闲链表中
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start); // 对齐页面
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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

  // Fill with junk to catch dangling refs.
  // 将要回收的内存页面中的内容全设为 1. why?
  // 在分配内存页面时（kalloc 函数），
  memset(pa, 1, PGSIZE);   

  r = (struct run*)pa;   // 将 pa 对应的地址强制转化为 stuct run

  acquire(&kmem.lock);
  // 使用头插法，将最近回收的内存页面放在链表头部。要分配时，也从头部开始分配
  // 这样的话，内核的页面不应该在物理内存的高位吗？确实如此
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
  if(r)   // 如果为真（r 不为空），则说明正确分配了内存页面
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
