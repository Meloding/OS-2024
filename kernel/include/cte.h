#ifndef __CTE_H__
#define __CTE_H__

#include <stdint.h>

// TODO: WEEK2: adjust the struct to the correct order
// TODO: WEEK2: add esp and ss
typedef struct Context {
  uint32_t ds, ebp, edi, esi, edx,
           ecx, ebx, eax, irq, errcode, 
           eip, cs, eflags, esp, ss;
} Context;

void init_cte();
void irq_iret(Context *ctx) __attribute__((noreturn));

void do_syscall(Context *ctx);
void exception_debug_handler(Context *ctx);

inline void print_context(const Context *ctx) {
  printf("ds: 0x%08X\tebp: 0x%08X\tedi: 0x%08X\tesi: 0x%08X\tedx: 0x%08X\n",
          ctx->ds, ctx->ebp, ctx->edi, ctx->esi, ctx->edx);
  printf("ecx: 0x%08X\tebx: 0x%08X\teax: 0x%08X\tirq: 0x%08X\terrcode: 0x%08X\n",
          ctx->ecx, ctx->ebx, ctx->eax, ctx->irq, ctx->errcode);
  printf("eip: 0x%08X\tcs: 0x%08X\teflags: 0x%08X\tesp: 0x%08X\tss: 0x%08X\n",
          ctx->eip, ctx->cs, ctx->eflags, ctx->esp, ctx->ss);
}

#endif
