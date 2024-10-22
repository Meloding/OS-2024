#include "klib.h"
#include "cte.h"
#include "proc.h"

#define PROC_NUM 64

static __attribute__((used)) int next_pid = 1;

proc_t pcb[PROC_NUM];
static proc_t* curr = &pcb[0];

void init_proc() {
  // WEEK1: init proc status
  curr->status = RUNNING;
  // WEEK2: add ctx and kstack for interruption
  curr->ctx = &(((kstack_t*)(KER_MEM - PGSIZE))->ctx);
  curr->kstack = (kstack_t*)(KER_MEM - PGSIZE);
  // WEEK3: add pgdir
  curr->pgdir = vm_curr();
  // WEEK4: fork
  curr->child_num = 0;
  curr->parent = NULL;
  // WEEK5: semaphore
  sem_init(&curr->zombie_sem, 0);
  memset(curr->usems, 0, sizeof(curr->usems));

  curr->tgid = 0;
  curr->thread_num = 0;
  curr->thread_group = NULL;
  curr->group_leader = NULL;

  // Lab2-1, set status and pgdir
  // Lab2-4, init zombie_sem
  // Lab3-2, set cwd
}

proc_t* proc_alloc() {
  // WEEK1: alloc a new proc, find a unused pcb from pcb[1..PROC_NUM-1], return NULL if no such one
  for (int id = 0; id < PROC_NUM; id++) {
    void* new_stack = kalloc();
    if (pcb[id].status == UNUSED) {
      proc_t* cur = &pcb[id];
      // cur->entry = 0; remove in week2
      cur->pid = next_pid++;
      cur->status = UNINIT;
      cur->ctx = &(((kstack_t*)new_stack)->ctx);
      cur->kstack = (kstack_t*)new_stack;
      cur->pgdir = vm_alloc();
      cur->child_num = 0;
      cur->parent = NULL;
      sem_init(&cur->zombie_sem, 0);
      memset(cur->usems, 0, sizeof(cur->usems));
      cur->tgid = cur->pid;
      cur->group_leader = cur;
      cur->thread_num = 0;
      cur->thread_group = NULL;
      return cur;
    }
  }
  return NULL;
}

void proc_free(proc_t* proc) {
  // WEEK3-virtual-memory: free proc's pgdir and kstack and mark it UNUSED
  // TODO();
  proc->status = UNUSED;
}

proc_t* proc_curr() {
  return curr;
}

void proc_run(proc_t* proc) {
  // WEEK1: start os
  proc->status = RUNNING;
  curr = proc;
  set_cr3(proc->pgdir);
  set_tss(KSEL(SEG_KDATA), (uint32_t)STACK_TOP(proc->kstack));
  irq_iret(proc->ctx);
}

void proc_addready(proc_t* proc) {
  // WEEK4-process-api: mark proc READY
  // TODO();
  proc->status = READY;
}

void proc_yield() {
  // WEEK4-process-api: mark curr proc READY, then int $0x81
  curr->status = READY;
  INT(0x81);
}

void proc_copycurr(proc_t* proc) {
  // WEEK4-process-api: copy curr proc
  vm_copycurr(proc->pgdir);
  proc->brk = proc_curr()->brk;
  proc->kstack->ctx = proc_curr()->kstack->ctx;
  proc->kstack->ctx.eax = 0;
  proc->parent = proc_curr();
  proc_curr()->child_num++;
  // WEEK5-semaphore: dup opened usems
  for (int i = 0; i < MAX_USEM; i++) {
    if (curr->usems[i] == NULL) continue;
    proc->usems[i] = usem_dup(curr->usems[i]);
  }
  // Lab3-1: dup opened files
  // Lab3-2: dup cwd
  // TODO();
}

void proc_makezombie(proc_t* proc, int exitcode) {
  // WEEK4-process-api: mark proc ZOMBIE and record exitcode, set children's parent to NULL
  proc->status = ZOMBIE;
  proc->exit_code = exitcode;
  for (int i = 0; i < PROC_NUM; ++i) {
    if (pcb[i].parent == proc) pcb[i].parent = NULL;
  }

  // WEEK5-semaphore: release parent's semaphore
  if (proc->parent)
    sem_v(&proc->parent->zombie_sem);
  // Lab3-1: close opened files
  // Lab3-2: close cwd
  // TODO();
}

proc_t* proc_findzombie(proc_t* proc) {
  // WEEK4-process-api: find a ZOMBIE whose parent is proc, return NULL if none
  // TODO();
  for (int i = 0; i < PROC_NUM; i++) {
    if (pcb[i].parent == proc && pcb[i].status == ZOMBIE) return pcb + i;
  }
  return NULL;
}

void proc_block() {
  // WEEK4-process-api: mark curr proc BLOCKED, then int $0x81
  curr->status = BLOCKED;
  INT(0x81);
}

int proc_allocusem(proc_t* proc) {
  // WEEK5: find a free slot in proc->usems, return its index, or -1 if none
  // TODO();
  for (int i = 0; i < MAX_USEM; i++) {
    if (proc->group_leader->usems[i] != NULL) continue;
    return i;
  }
  return -1;
}

usem_t* proc_getusem(proc_t* proc, int sem_id) {
  // WEEK5: return proc->usems[sem_id], or NULL if sem_id out of bound
  // TODO();
  return (sem_id >= MAX_USEM || sem_id < 0) ? NULL : proc->group_leader->usems[sem_id];
}

int proc_allocfile(proc_t* proc) {
  // Lab3-1: find a free slot in proc->files, return its index, or -1 if none
  TODO();
}

file_t* proc_getfile(proc_t* proc, int fd) {
  // Lab3-1: return proc->files[fd], or NULL if fd out of bound
  TODO();
}

void schedule(Context* ctx) {
  // WEEK4-process-api: save ctx to curr->ctx, then find a READY proc and run it
  // TODO();
  for (int pid = curr - pcb + 1; pid != curr - pcb; pid++) {
    pid %= PROC_NUM;
    if (pcb[pid].status == READY) {
      (proc_curr()->ctx) = ctx;
      proc_run(pcb + pid);
      break;
    }
  }
}

void thread_free(proc_t *thread) {
  thread->status = UNUSED;
  thread->group_leader->thread_num--;
}