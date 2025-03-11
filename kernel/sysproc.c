#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h" // Thêm file syscall.h để sử dụng MAX_SYSTEMCALL
#include "sysinfo.h" // Thêm file sysinfo.h để sử dụng struct sysinfo
#include "file.h" // Thêm file file.h để sử dụng struct file
#include "fs.h"   // Thêm file fs.h để sử dụng struct ftable

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

int
sys_trace(void) 
{
  int mask;

  // argint(0, &mask) gọi argraw(0), lấy giá trị từ thanh ghi a0.
  // gán giá trị đó vào mask, tức là lấy đúng giá trị mà user đã nhập vào.
  argint(0, &mask);

  // Kiểm tra lỗi xem mask có nằm trong phạm vi các system call không.
  // Nếu không thì return lại lỗi.
  if (mask < 0 || mask > MAX_MASK) {
    return -1;
  }

  struct proc *current_proc = myproc(); // Lấy process hiện tại
  current_proc->trace_mask = mask; // Gán mask vào trace_mask của process hiện tại
  // Nghĩa là sau này thì khi gọi myproc()->trace_mask sẽ trả về mask mà user đã nhập vào. 

  return 0;
}

extern struct proc proc[]; // Mảng tiến trình toàn cục
extern struct ftable ftable; // Bảng quản lý file mở

// Hàm dùng để đếm số bộ nhớ trống từ freelist trong file kalloc.c
uint64 freemem(void);


// Hàm đếm số tiến trình đang chạy
uint64 nproc(void) {
  uint64 count_nproc = 0;

  struct proc* p;
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state != UNUSED) {
      count_nproc++;
    }
  }

  return count_nproc;
}


// Hàm đếm số file đang mở
uint64 nopenfiles(void) {
  uint64 count_nopenfiles = 0;

  struct file* f;
  acquire(&ftable.lock);
  for (f = ftable.file; f < ftable.file + NFILE; f++) {
    if (f->ref != 0) {
      count_nopenfiles++;
    }
  }
  release(&ftable.lock);

  return count_nopenfiles;
}

uint64 sys_sysinfo(void) {
  uint64 addr;
  struct sysinfo info;

  // argaddr(0, &addr) gọi argraw(0), lấy giá trị từ thanh ghi a0,
  // lấy tham số đầu tiên dưới dạng địa chỉ uint64 (địa chỉ con trỏ struct sysinfo)
  argaddr(0, &addr);


  // Lấy thông tin từ hệ thống và gán vào struct sysinfo info
  info.freemem = freemem();
  info.nproc = nproc();
  info.nopenfiles = nopenfiles();


  // Sao chép struct sysinfo info từ kernel vào user-space
  if (copyout(myproc()->pagetable, addr, (char*)&info, sizeof(info)) < 0) {
    return -1;
  }

  return 0;
}

void print_col(const char *str, int width) {
  printf("%s", str);
  int len = strlen(str);
  for (int i = len; i < width; i++) {
      printf(" ");
  }
}

uint64 sys_list_process(void) {
  char* states_name[] = {
    [UNUSED]    "unused",
    [USED]    "used",
    [SLEEPING]    "sleeping",
    [RUNNABLE]    "runnable",
    [RUNNING]    "running",
    [ZOMBIE]    "zombie",
  };

  struct proc* p;
  printf("List of not unused processes:\n");

  printf("PID  ");
  print_col("State", 15);
  print_col("Name", 10);
  printf("\n----------------------------\n"); // Đường phân cách

  for (p = proc; p < &proc[NPROC]; p++) {
      if (p->state != UNUSED) {
          printf("%d    ", p->pid);
          print_col(states_name[p->state], 15);
          print_col(p->name, 10);
          printf("\n");

      }
  }
  
  return 0;
}

extern struct inode* namei(char *path); // Hàm tìm inode từ tên file

// Duyệt thư mục để tìm tên file từ inode
void get_filename(struct inode *ip, char *name) {
  struct inode *dp;
  struct dirent de;

  // Duyệt qua thư mục root "/"
  dp = namei("/");
  if (dp == 0) return;

  ilock(dp);
  for (int off = 0; off < dp->size; off += sizeof(de)) {
    if (readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de)) 
      continue;
    if (de.inum == ip->inum) {  // Nếu inode number khớp
      safestrcpy(name, de.name, DIRSIZ);
      iunlock(dp);
      return;
    }
  }
  iunlock(dp);
  safestrcpy(name, "unknown", DIRSIZ);
}

uint64 sys_list_files(void) {
  char* types_name[] = {
    [FD_NONE]    "closed file",
    [FD_PIPE]    "pipe",
    [FD_INODE]    "file",
    [FD_DEVICE]    "device",
  };

  struct file* f;
  char name[DIRSIZ + 1];  // Tên file (tối đa 14 ký tự)

  acquire(&ftable.lock);
  printf("List of opened files:\n");

  printf("FD  ");
  print_col("Type", 15);
  print_col("Ref", 10);
  print_col("Name", 10);
  printf("\n------------------------------------\n"); // Đường phân cách

  for (f = ftable.file; f < ftable.file + NFILE; f++) {
      if (f->ref != 0) {
          printf("%ld    ", f - ftable.file);
          print_col(types_name[f->type], 15);
          printf("%d        ", f->ref);
          //print_col(ref_str, 10);

          if (f->ip) {
              get_filename(f->ip, name);
              print_col(name, 10);
          } else {
              print_col("unknown", 10);
          }

          printf("\n");
      }
  }
  release(&ftable.lock);

  return 0;
}