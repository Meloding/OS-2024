#include "klib.h"
#include "cte.h"
#include "sysnum.h"
#include "vme.h"
#include "serial.h"
#include "loader.h"
#include "proc.h"
#include "timer.h"
#include "file.h"

typedef int (*syshandle_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

extern void* syscall_handle[NR_SYS];

void do_syscall(Context* ctx) {
  // TODO: WEEK2-interrupt call specific syscall handle and set ctx register
  int sysnum = ctx->eax;
  uint32_t arg1 = ctx->ebx;
  uint32_t arg2 = ctx->ecx;
  uint32_t arg3 = ctx->edx;
  uint32_t arg4 = ctx->esi;
  uint32_t arg5 = ctx->edi;
  int res;
  if (sysnum < 0 || sysnum >= NR_SYS) {
    res = -1;
  }
  else {
    res = ((syshandle_t)(syscall_handle[sysnum]))(arg1, arg2, arg3, arg4, arg5);
  }
  ctx->eax = res;
}

int sys_write(int fd, const void* buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  return serial_write(buf, count);
}

int sys_read(int fd, void* buf, size_t count) {
  // TODO: rewrite me at Lab3-1
  return serial_read(buf, count);
}

int sys_brk(void* addr) {
  // TODO: WEEK3-virtual-memory
  proc_t* main_thread = proc_curr()->group_leader;
  size_t brk = main_thread->brk; // rewrite me
  size_t new_brk = PAGE_UP(addr); // rewrite me
  if (brk == 0) {
    main_thread->brk = new_brk; // uncomment me in WEEK3-virtual-memory
  }
  else if (new_brk > brk) {
    PD* pd = vm_curr();
    vm_map(pd, brk, new_brk - brk, PTE_U | PTE_W | PTE_P);
    main_thread->brk = brk;
  }
  else if (new_brk < brk) {
    // can just do nothing
    // recover memory, Lab 1 extend
  }
  return 0;
}

void sys_sleep(int ticks) {
  // TODO(); // WEEK2-interrupt
  uint32_t beg_tick = get_tick();
  while (get_tick() - beg_tick <= ticks) {
    // sti(); hlt(); cli(); // chage to me in WEEK2-interrupt
    proc_yield(); // change to me in WEEK4-process-api
    // thread_yield();
  }
  return;
}

int sys_exec(const char* path, char* const argv[]) {
  // TODO(); // WEEK2-interrupt, WEEK3-virtual-memory
  proc_t* proc = proc_curr();
  // PD *pgdir = NULL;
  PD* pgdir = vm_alloc();
  proc->ctx = &(proc->kstack->ctx);
  int ret = load_user(pgdir, proc->ctx, path, argv);
  if (ret) return -1;
  proc->pgdir = pgdir;

  proc_t* main_thread = proc->group_leader;
  if (main_thread == proc) {
    // printf("1== %p  %p  %p\n", proc, proc->pgdir, pgdir);
    for (proc_t* cur = main_thread->thread_group; cur; cur = cur->thread_group) {
      thread_free(cur);
    }
    main_thread->thread_group = NULL;
    main_thread->thread_num = 1;
  }
  else {
    // printf("2== %p  %p  %p\n", proc, proc->pgdir, pgdir);
    // proc_t tmp;
    // memcpy(&tmp, main_thread, sizeof(proc_t));
    // memcpy(main_thread, proc, sizeof(proc_t));
    // memcpy(proc, &tmp, sizeof(proc_t));
    for (proc_t* cur = main_thread; cur; cur = cur->thread_group) {
      if (cur != proc) thread_free(cur);
    }
    main_thread->pgdir = proc->pgdir;
    // main_thread->kstack->ctx = *proc->ctx;
    // memcpy(&main_thread->kstack->ctx, proc->ctx, sizeof(Context));
    main_thread->kstack = proc->kstack;
    main_thread->ctx = proc->ctx;
    memcpy(proc, main_thread, sizeof(proc_t));
    proc->thread_group = NULL;
    proc->thread_num = 1;
    proc->group_leader = proc;
    // proc->pid = main_thread->pid;
    // proc->tgid = main_thread->pid;
  }
  // thread_free(proc);
  // printf("== %p  %p  %p\n", proc, proc->pgdir, pgdir);
  proc_run(proc);
  // return;
  // set_cr3(pgdir);
  // set_tss(KSEL(SEG_KDATA), (uint32_t)proc->kstack + PGSIZE);
  // irq_iret(proc->ctx);
  // DEFAULT
  printf("sys_exec is not implemented yet.");
  while (1);
}

int sys_getpid() {
  // TODO(); // WEEK3-virtual-memory
  return proc_curr()->tgid;
}

int sys_gettid() {
  return proc_curr()->pid; // Lab2-1
}

void sys_yield() {
  proc_yield();
}

int sys_fork() {
  // TODO(); // WEEK4-process-api
  proc_t* pcb = proc_alloc();
  if (pcb == NULL)
    return -1;
  proc_copycurr(pcb);
  proc_addready(pcb);
  return pcb->pid;
}

void sys_exit_group(int status) {
  // printf("----------------\n");
  // TODO();
  // WEEK4 process api
  proc_t* main_thread = proc_curr()->group_leader;
  for (proc_t* cur = main_thread->thread_group; cur; cur = cur->thread_group) {
    thread_free(cur);
  }
  proc_makezombie(main_thread, status);
  INT(0x81);
  assert(0);
}

void sys_exit(int status) {
  // printf("==========\n");
  proc_t* main_thread = proc_curr()->group_leader;
  proc_t* cur_thread = proc_curr();
  if (main_thread == cur_thread) {
    // printf("num %d\n", main_thread->thread_num);
    while (main_thread->thread_num > 1) { // cv-like operation
      proc_yield();
    }
    for (proc_t* cur = main_thread->thread_group; cur; cur = cur->thread_group) {
      thread_free(cur);
    }
    proc_makezombie(cur_thread, status);
    INT(0x81);

    // printf("num2 %d\n", main_thread->thread_num);
    assert(main_thread->thread_num > 1);
    return;
  }
  if (cur_thread->detached == 1) {
    proc_t *tmp = main_thread;
    while(tmp && tmp->thread_group != cur_thread) tmp = tmp->thread_group;
    tmp->thread_group = cur_thread->thread_group;
    cur_thread->thread_group = NULL;
    proc_set_kernel_parent(cur_thread);
  }
  main_thread->thread_num--;
  proc_makezombie(proc_curr(), status);
  INT(0x81);
  assert(0);
}

int sys_wait(int* status) {
  // TODO(); // WEEK4 process api
  proc_t* curr = proc_curr()->group_leader;
  if (curr->child_num == 0) {
    return -1;
  }
  proc_t* ch_proc;
  // while ((ch_proc = proc_findzombie(curr)) == NULL) {
    // proc_yield();
  // }
  sem_p(&curr->zombie_sem);
  ch_proc = proc_findzombie(curr);
  assert(ch_proc);
  if (status != NULL) {
    *status = ch_proc->exit_code;
  }
  int pid = ch_proc->pid;
  proc_free(ch_proc);
  curr->child_num--;
  return pid;
}

int sys_sem_open(int value) {
  // TODO(); // WEEK5-semaphore
  int id = proc_allocusem(proc_curr()->group_leader);
  if (id == -1) return -1;
  usem_t* usem_u = usem_alloc(value);
  if (usem_u == NULL) return -1;
  proc_curr()->group_leader->usems[id] = usem_u;
  if (proc_getusem(proc_curr()->group_leader, id) == NULL) return -1;
  return id;
}

int sys_sem_p(int sem_id) {
  // TODO(); // WEEK5-semaphore
  usem_t* usem_u = proc_getusem(proc_curr()->group_leader, sem_id);
  if (usem_u == NULL) return -1;
  sem_p(&usem_u->sem);
  return 0;
}

int sys_sem_v(int sem_id) {
  // TODO(); // WEEK5-semaphore
  usem_t* usem_u = proc_getusem(proc_curr()->group_leader, sem_id);
  if (usem_u == NULL) return -1;
  sem_v(&usem_u->sem);
  return 0;
}

int sys_sem_close(int sem_id) {
  // TODO(); // WEEK5-semaphore
  usem_t* usem_u = proc_getusem(proc_curr()->group_leader, sem_id);
  if (usem_u == NULL) return -1;
  usem_close(usem_u);
  proc_curr()->group_leader->usems[sem_id] = NULL;
  return 0;
}

int sys_open(const char* path, int mode) {
  TODO(); // Lab3-1
}

int sys_close(int fd) {
  TODO(); // Lab3-1
}

int sys_dup(int fd) {
  TODO(); // Lab3-1
}

uint32_t sys_lseek(int fd, uint32_t off, int whence) {
  TODO(); // Lab3-1
}

int sys_fstat(int fd, struct stat* st) {
  TODO(); // Lab3-1
}

int sys_chdir(const char* path) {
  TODO(); // Lab3-2
}

int sys_unlink(const char* path) {
  return iremove(path);
}

// optional syscall

void* sys_mmap() {
  // TODO();
  for (uint32_t addr = USR_MEM; addr < VIR_MEM; addr += PGSIZE) {
    PTE* pte = vm_walkpte(vm_curr(), addr, 0);
    if (pte == NULL || pte->present == 0) {
      // printf("find %x\n", addr);
      vm_map(vm_curr(), addr, PGSIZE, 7);
      return (void*)addr;
    }
  }
  return NULL;
}

void sys_munmap(void* addr) {
  // TODO();
}

int sys_clone(int (*entry)(void*), void* stack, void* arg, void (*ret_entry)(void)) {
  proc_t* proc = proc_alloc();
  if (proc == NULL) return -1;
  proc_t* curr = proc_curr();
  proc_t* main_thread = curr->group_leader;

  proc->pgdir = curr->pgdir;
  proc->tgid = curr->tgid;
  proc->group_leader = main_thread;

  void* stack_top = stack;
  stack_top -= sizeof(void*);
  *(size_t*)stack_top = (size_t)arg;
  stack_top -= sizeof(size_t);
  *(size_t*)stack_top = (size_t)ret_entry;

  proc->thread_group = main_thread->thread_group;
  main_thread->thread_group = proc;
  main_thread->thread_num++;

  proc->ctx->eip = (uint32_t)entry;
  proc->ctx->cs = USEL(SEG_UCODE);
  proc->ctx->ds = USEL(SEG_UDATA);
  proc->ctx->ss = USEL(SEG_UDATA);
  proc->ctx->esp = (uint32_t)stack_top;
  proc->ctx->eflags = 0x202;

  proc_addready(proc);

  return proc->pid;
}

int sys_join(int tid, void** retval) {
  proc_t *proc = pid2proc(tid);
  if (proc == NULL || proc == proc_curr() || proc->joinable != 1) {
    return 3;
  }
  proc->joinable = 0;
  sem_p(&proc->join_sem);
  if (retval != NULL) *retval = (void *)proc->exit_code;
  return 0;
}

int sys_detach(int tid) {
  return thread_detach(tid);
}

int sys_kill(int pid) {
  proc_t *proc = pid2proc(pid);
  if (proc == NULL) return -1;
  for (proc_t *cur = proc->thread_group; cur; cur = cur->thread_group) {
    thread_free(cur);
  }
  proc_makezombie(proc, 9);
  if (proc == proc_curr()) INT(0x81);
  return 0;
}

int sys_cv_open() {
  return sys_sem_open(0);
}

int sys_cv_wait(int cv_id, int sem_id) {
  sys_sem_v(sem_id);
  return sys_sem_p(cv_id);
}

int sys_cv_sig(int cv_id) {
  return sys_sem_v(cv_id);
}

int sys_cv_sigall(int cv_id) {
  sem_t* sem = &proc_getusem(proc_curr(), cv_id)->sem;
  int pcnt = -proc_getusem(proc_curr(), cv_id)->sem.value;
  if (pcnt <= 0) return 0;
  while (pcnt--)
    sem_v(sem);
  return 0;
}

int sys_cv_close(int cv_id) {
  return sys_sem_close(cv_id);
}

int sys_pipe(int fd[2]) {
  TODO();
}

int sys_mkfifo(const char* path, int mode) {
  TODO();
}

int sys_link(const char* oldpath, const char* newpath) {
  TODO();
}

int sys_symlink(const char* oldpath, const char* newpath) {
  TODO();
}

void* syscall_handle[NR_SYS] = {
  [SYS_write] = sys_write,
  [SYS_read] = sys_read,
  [SYS_brk] = sys_brk,
  [SYS_sleep] = sys_sleep,
  [SYS_exec] = sys_exec,
  [SYS_getpid] = sys_getpid,
  [SYS_gettid] = sys_gettid,
  [SYS_yield] = sys_yield,
  [SYS_fork] = sys_fork,
  [SYS_exit] = sys_exit,
  [SYS_exit_group] = sys_exit_group,
  [SYS_wait] = sys_wait,
  [SYS_sem_open] = sys_sem_open,
  [SYS_sem_p] = sys_sem_p,
  [SYS_sem_v] = sys_sem_v,
  [SYS_sem_close] = sys_sem_close,
  [SYS_open] = sys_open,
  [SYS_close] = sys_close,
  [SYS_dup] = sys_dup,
  [SYS_lseek] = sys_lseek,
  [SYS_fstat] = sys_fstat,
  [SYS_chdir] = sys_chdir,
  [SYS_unlink] = sys_unlink,
  [SYS_mmap] = sys_mmap,
  [SYS_munmap] = sys_munmap,
  [SYS_clone] = sys_clone,
  [SYS_join] = sys_join,
  [SYS_detach] = sys_detach,
  [SYS_kill] = sys_kill,
  [SYS_cv_open] = sys_cv_open,
  [SYS_cv_wait] = sys_cv_wait,
  [SYS_cv_sig] = sys_cv_sig,
  [SYS_cv_sigall] = sys_cv_sigall,
  [SYS_cv_close] = sys_cv_close,
  [SYS_pipe] = sys_pipe,
  [SYS_mkfifo] = sys_mkfifo,
  [SYS_link] = sys_link,
  [SYS_symlink] = sys_symlink,
  // [SYS_spinlock_open] = sys_spinlock_open,
  // [SYS_spinlock_acquire] = sys_spinlock_acquire,
  // [SYS_spinlock_release] = sys_spinlock_release,
  // [SYS_spinlock_close] = sys_spinlock_close,
};
