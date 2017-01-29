/* Trampoline test */

/*
 * Copyright 1995-1999, 2001-2002, 2004-2006, 2017 Bruno Haible, <bruno@clisp.org>
 *
 * This is free software distributed under the GNU General Public Licence
 * described in the file COPYING. Contact the author if you don't have this
 * or can't live with it. There is ABSOLUTELY NO WARRANTY, explicit or implied,
 * on this software.
 */

#include "config.h"  /* Define __${host_cpu}__ */

#include <stdio.h>
#include <stdlib.h>

#include "trampoline_r.h"

#if BINFMT_ELF

/* We cannot test trampoline_r without vacall_r. */
int main ()
{
  printf("Skipping test: This is an ELF platform.\n"); exit(0);
}

#else

/* Set when we can check that the env register is being passed correctly. */
#if defined __GNUC__ && !defined __clang__
#define CHECK_ENV_REGISTER
#endif

#define MAGIC1  0x9db9af42
#define MAGIC2  0x614a13c9
#define MAGIC3  0x7aff3cb4
#define MAGIC4  0xa2f9d045

#ifdef __cplusplus
typedef int (*function)(...);
#else
typedef int (*function)();
#endif

int f (int x)
{
#ifdef CHECK_ENV_REGISTER
#ifdef __i386__
register void* env __asm__("%ecx");
#endif
#ifdef __m68k__
#ifdef __NetBSD__
register void* env __asm__("a1");
#else
register void* env __asm__("a0");
#endif
#endif
#if defined(__mips__) || defined(__mipsn32__) || defined(__mips64__)
register void* env __asm__("$2");
#endif
#if defined(__sparc__) && !defined(__sparc64__)
register void* env __asm__("%g2");
#endif
#ifdef __sparc64__
register void* env __asm__("%g5");
#endif
#ifdef __alpha__
register void* env __asm__("$1");
#endif
#ifdef __hppa__
register void* env __asm__("%r29");
#endif
#ifdef __arm__
register void* env __asm__("r12");
#endif
#ifdef __powerpc__
#ifdef __NetBSD__
register void* env __asm__("r13");
#else
register void* env __asm__("r11");
#endif
#endif
#ifdef __ia64__
register void* env __asm__("r15");
#endif
#ifdef __x86_64__
register void* env __asm__("r10");
#endif
#ifdef __s390__
register void* env __asm__("r0");
#endif

  return x + (int)((long*)env)[0] + (int)((long*)env)[1] + MAGIC3;
#else
  return x + MAGIC3;
#endif
}

int main ()
{
  function cf = alloc_trampoline_r((function)&f, (void*)MAGIC1, (void*)MAGIC2);
#ifdef CHECK_ENV_REGISTER
  if ((*cf)(MAGIC4) == MAGIC1+MAGIC2+MAGIC3+MAGIC4)
#else
  if ((*cf)(MAGIC4) == MAGIC3+MAGIC4)
#endif
    { free_trampoline_r(cf); printf("Works, test1 passed.\n"); exit(0); }
  else
    { printf("Doesn't work!\n"); exit(1); }
}

#endif
