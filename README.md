# xv6 Kernel Labs

Two kernel projects from my Operating Systems course (CSC10007, University of Science, VNU-HCM), done in a team of three in spring 2025. We extended **xv6**, the small Unix-like teaching kernel from MIT, running on RISC-V under QEMU.

Each lab lives on its own branch, on top of the starter code from [MIT 6.1810 xv6-labs-2024](https://pdos.csail.mit.edu/6.1810/2024/). The last commit on each branch is our work, committed on the date we submitted it.

| Branch | Lab | What we added |
|---|---|---|
| [`syscall`](../../tree/syscall) | System calls | `trace`, `sysinfo`, `list_process`, `list_files` |
| [`pgtbl`](../../tree/pgtbl) | Page tables | `vmprint`, `set_pgtbl`, `pgaccess` |

To see only our changes, open the last commit on each branch.

## Lab 1: System calls (`syscall`)

- **`trace(mask)`** turns on syscall tracing for the calling process. The mask is stored in `struct proc` and copied to the child in `fork()`, so tracing follows every descendant. `syscall()` prints the PID, syscall name and return value for each traced call. `user/trace.c` runs any program with tracing on.
- **`sysinfo(struct sysinfo *)`** fills in free memory (by walking the allocator's free list), the number of processes that are not `UNUSED`, and the number of open files, then copies the struct back to user space with `copyout()`.
- **Extensions:** `list_process` prints PID, state and name for every live process; `list_files` prints file descriptor, type, reference count and name for every open file.

```
$ sysinfotest
Free memory: 133246976 byte(s)
Running process: 3 process(es)
Opened files: 1 file(s)
$ list_process
PID  State          Name
1    sleeping       init
2    sleeping       sh
4    running        list_process
$ trace 2 usertests forkforkfork
9: syscall fork -> 36
10: syscall fork -> 37
...
```

## Lab 2: Page tables (`pgtbl`)

- **`vmprint(pagetable)`** walks the three-level RISC-V page table depth first and prints every valid PTE with its index, flags and physical address.
- **`set_pgtbl(flag)`** turns page-table printing on or off for later `exec` calls, so `exec` still behaves normally unless the user asks for the printout.
- **`pgaccess(va, n, mask)`** checks the hardware access bit (`PTE_A`) of up to n user pages, returns a bitmask through `copyout()`, and clears each bit after reading it so the next call only reports new accesses.

```
$ pgtbltest
pgaccess_test: OK
$ set_pgtbl 1
$ echo hi
page table 0x0000000087f51000
..0: pte 0x0000000021fc6c01 pa 0x0000000087f1b000
.. ..0: pte 0x0000000021fc6801 pa 0x0000000087f1a000
.. .. ..0: pte 0x0000000021fd401b pa 0x0000000087f50000
...
```

## Running it

You need a RISC-V GCC toolchain and QEMU. On Ubuntu:

```bash
sudo apt install gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu qemu-system-misc
git checkout syscall   # or pgtbl
make qemu
```

Checked again on Ubuntu 24.04 in November 2026: all six `trace` tests from MIT's grader pass, `pgaccess_test` passes, and `usertests` passes on the `pgtbl` branch. MIT's 2024 grader also checks tasks our course did not assign (`attack`, `ugetpid`, `superpg`), so those are expected to fail.

xv6 is © MIT, under the license in each branch.
