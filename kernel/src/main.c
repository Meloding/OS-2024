#include "klib.h"
#include "serial.h"
#include "vme.h"
#include "cte.h"
#include "loader.h"
#include "fs.h"
#include "proc.h"
#include "timer.h"
#include "dev.h"

void init_user_and_go();

int main() {
  init_gdt();
  init_serial();
  init_fs();
  init_page(); // uncomment me at WEEK3-virtual-memory
  // printf("init_page");
  init_cte(); // uncomment me at WEEK2-interrupt
  init_timer(); // uncomment me at WEEK2-interrupt
  init_proc(); // uncomment me at WEEK1-os-start
  //init_dev(); // uncomment me at Lab3-1
  printf("Hello from OS!\n");
  init_user_and_go();
  panic("should never come back");
}

void init_user_and_go() {
  // WEEK1: ((void(*)())eip)();
  // uint32_t eip = load_elf(NULL, "iotest");
  // proc->entry = eip;
  // assert(eip != -1);

  // WEEK2: interrupt
  // PD *pgdir = NULL;
  proc_t* proc = proc_alloc();
  assert(proc);

  // char *argv[] = {"echo", "hello", "world", NULL};
  char* argv[] = { "sh1", NULL };
  // char* argv[] = {"ping3", "114514", "1919810", NULL};
  // printf("before load_user\n");
  assert(load_user(proc->pgdir, proc->ctx, "sh", argv) == 0);
  // printf("before proc_run\n");
  proc_addready(proc);

  
  // proc_t* proc1 = proc_alloc();
  // proc_t* proc2 = proc_alloc();
  // assert(load_user(proc1->pgdir, proc1->ctx, "ping2", argv) == 0);
  // // printf("before proc_run\n");
  // proc_addready(proc1);
  // assert(load_user(proc2->pgdir, proc2->ctx, "ping2", argv) == 0);
  // // printf("before proc_run\n");
  // proc_addready(proc2);
  // // while(1) proc_yield();
  sti();
  while(1);
  proc_run(proc);

}
