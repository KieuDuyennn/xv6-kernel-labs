#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc <= 1 || argc >= 3) {
    fprintf(2, "Usage: set_pgtbl <flag_value>\n");
    exit(1);
  }

  if (set_pgtbl(atoi(argv[1])) < 0) {
    printf("set_pgtbl failed\n");
    exit(0);
  }

  exit(0);
}