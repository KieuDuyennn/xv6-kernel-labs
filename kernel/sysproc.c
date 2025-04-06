#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

extern int pagetable_exec_flag;  // Được định nghĩa trong exec.c

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgpte(void)
{
  uint64 va;
  struct proc *p;  

  p = myproc();
  argaddr(0, &va);
  pte_t *pte = pgpte(p->pagetable, va);
  if(pte != 0) {
      return (uint64) *pte;
  }
  return 0;
}
#endif

#ifdef LAB_PGTBL
int
sys_kpgtbl(void)
{
  struct proc *p;  

  p = myproc();
  vmprint(p->pagetable);
  return 0;
}
#endif


uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 
sys_pgaccess(void) {
  uint64 start_va;  // Địa chỉ ảo của trang đầu tiên
  int num_pages;  // Số lượng trang cần kiểm tra
  uint64 user_mask; // Địa chỉ của bitmask trong userspace

  // Lấy tham số từ userspace
  argaddr(0, &start_va);  // Lấy địa chỉ ảo của trang đầu tiên từ para đầu tiên
  argint(1, &num_pages);  // Lấy số lượng trang cần kiểm tra từ para thứ hai
  argaddr(2, &user_mask); // Lấy địa chỉ của bitmask từ para thứ ba

  // Kiểm tra xem địa chỉ ảo có hợp lệ không
  if (start_va >= MAXVA || start_va % PGSIZE != 0) {
    // Nếu địa chỉ vượt quá MAXVA
    // Nếu địa chỉ không căn chỉnh theo 4KB
    return -1; 
  }

  // Kiểm tra xem số lượng trang có hợp lệ không
  if (num_pages <= 0 || num_pages > MAXVA / PGSIZE) {
    return -1; 
  }

  struct proc *p = myproc();
  uint64 mask = 0;

  for (int i = 0; i < num_pages; i++) {
    pte_t * pte = walk(p->pagetable, start_va + i * PGSIZE, 0); // Tìm PTE

    if (pte && (*pte & PTE_V)) { // Nếu PTE hợp lệ
      if (*pte & PTE_A) { // Nếu trang đã được truy cập
        mask |= (1ULL << i); // Đánh dấu bit trong mask
        *pte &= ~PTE_A; // Giữ nguyên các bit khác trừ bit PTE_A bị đặt về 0
      }
    }
  }  

  // Sao chép bitmask về user space
  if (copyout(p->pagetable, user_mask, (char *)&mask, sizeof(mask)) < 0) {
    return -1; // Nếu không sao chép được
  }

  return 0; // Thành công
}

uint64
sys_set_pgtbl(void) {
  int flag_value;
  argint(0, &flag_value);
  
  if (flag_value != 0 && flag_value != 1) {
    return -1; // Nếu giá trị không hợp lệ
  }

  pagetable_exec_flag = flag_value;  // Cập nhật giá trị flag

  if (flag_value) {
    printf("The flag of pagetable_exec_flag is turned on\n");
  }
  else {
    printf("The flag of pagetable_exec_flag is turned off\n");
  }

  return 0; // Trả về 0 để báo thành công
}