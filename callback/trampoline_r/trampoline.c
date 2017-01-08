/* Trampoline construction */

/*
 * Copyright 1995-1999, 2001-2006, 2016 Bruno Haible, <bruno@clisp.org>
 *
 * This is free software distributed under the GNU General Public Licence
 * described in the file COPYING. Contact the author if you don't have this
 * or can't live with it. There is ABSOLUTELY NO WARRANTY, explicit or implied,
 * on this software.
 */


#include "config.h"
#include "trampoline_r.h"

#if defined(__hppa__)
#if 0
#define __hppaold__  /* Old trampoline, real machine code. */
#else
#define __hppanew__  /* New trampoline, just a closure. */
#endif
#endif
#if defined(__powerpc__) && !defined(__powerpc64__)
#if !defined(_AIX)
#define __powerpcsysv4__  /* SysV.4 ABI, real machine code. */
#else
#define __powerpcaix__  /* AIX ABI, just a closure. */
#endif
#endif
#if defined(__powerpc64__) && !defined(__powerpc64le__)
/* The ABI on powerpc64 other than powerpc64le is the AIX ABI. */
#define __powerpc64aix__  /* AIX ABI, just a closure. */
#endif
#if defined(__hppanew__)
/*
 * A function pointer is a biased pointer to a data area whose first word
 * contains the actual address of the function.
 */
extern void tramp_r (); /* trampoline prototype */
/* We don't need to take any special measures to make the code executable
 * since the actual instructions are in the text segment.
 */
#ifndef CODE_EXECUTABLE
#define CODE_EXECUTABLE
#endif
#endif
#if defined(__powerpcaix__) || defined(__powerpc64aix__) || defined(__ia64__)
/*
 * A function pointer is a pointer to a data area whose first word contains
 * the actual address of the function.
 */
extern void (*tramp_r) (); /* trampoline prototype */
/* We don't need to take any special measures to make the code executable
 * since the actual instructions are in the text segment.
 */
#ifndef CODE_EXECUTABLE
#define CODE_EXECUTABLE
#endif
#endif
#if defined(__m68k__)
#if defined(AMIGA) /* Amiga running AmigaOS, not Linux */
#ifndef CODE_EXECUTABLE /* configure guesses wrong?? */
#define CODE_EXECUTABLE
#endif
#endif
#endif

#ifndef CODE_EXECUTABLE
/* How do we make the trampoline's code executable? */
#if defined(HAVE_MACH_VM) || defined(HAVE_WORKING_MPROTECT)
/* mprotect() [or equivalent] the malloc'ed area. */
#define EXECUTABLE_VIA_MPROTECT
#else
#ifdef HAVE_MMAP
/* Use an mmap'ed page. */
#define EXECUTABLE_VIA_MMAP
#ifdef HAVE_MMAP_ANONYMOUS
/* Use mmap with the MAP_ANONYMOUS or MAP_ANON flag. */
#define EXECUTABLE_VIA_MMAP_ANONYMOUS
#else
/* Use mmap on /dev/zero. */
#define EXECUTABLE_VIA_MMAP_DEVZERO
#endif
#else
#ifdef HAVE_SHM
/* Use an shmat'ed page. */
#define EXECUTABLE_VIA_SHM
#else
??
#endif
#endif
#endif
#endif

#include <stdio.h> /* declares fprintf() */

#include <sys/types.h>
#include <stdlib.h> /* declares abort(), malloc(), free() */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Declare getpagesize(). */
#ifdef HAVE_GETPAGESIZE
#ifdef __cplusplus
extern "C" RETGETPAGESIZETYPE getpagesize (void);
#else
extern RETGETPAGESIZETYPE getpagesize (void);
#endif
#else
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#else
/* Not Unix, e.g. mingw32 */
#define PAGESIZE 4096
#endif
#define getpagesize() PAGESIZE
#endif

/* Declare mprotect() or equivalent. */
#ifdef EXECUTABLE_VIA_MPROTECT
#ifdef HAVE_MACH_VM
#include <sys/resource.h>
#include <mach/mach_interface.h>
#ifdef NeXT
#include <mach/mach_init.h>
#endif
#ifdef __osf__
#include <mach_init.h>
#endif
#include <mach/machine/vm_param.h>
#else
#include <sys/types.h>
#include <sys/mman.h>
#endif
#endif

/* Declare mmap(). */
#ifdef EXECUTABLE_VIA_MMAP
#include <sys/types.h>
#include <sys/mman.h>
#if !defined(PROT_EXEC) && defined(PROT_EXECUTE) /* Irix 4.0.5 needs this */
#define PROT_EXEC PROT_EXECUTE
#endif
#endif

/* Declare open(). */
#ifdef EXECUTABLE_VIA_MMAP_DEVZERO
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif

/* Declare shmget(), shmat(), shmctl(). */
#ifdef EXECUTABLE_VIA_SHM
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif
#endif

/* Support for instruction cache flush. */
#ifdef __i386__
#if defined(_WIN32) /* WindowsNT or Windows95 */
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#endif
#endif
#ifdef __m68k__
#if defined(AMIGA) /* Amiga running AmigaOS, not Linux */
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#endif
#ifdef hpux
#include <sys/cache.h>
#endif
#endif
#if defined(__mips__) || defined(__mipsn32__) || defined(__mips64__)
#ifdef ultrix
#include <mips/cachectl.h>
#else
#ifdef linux
#include <asm/cachectl.h>
#else
#ifdef HAVE_SYS_CACHECTL_H
#include <sys/cachectl.h>
#endif
#endif
#endif
#endif
/* Inline assembly function for instruction cache flush. */
#if defined(__sparc__) || defined(__sparc64__) || defined(__alpha__) || defined(__hppaold__) || defined(__powerpcsysv4__)
#if defined(__sparc__) || defined(__sparc64__)
extern void __TR_clear_cache_2();
#else
extern void __TR_clear_cache();
#endif
#endif

/* Length and alignment of trampoline */
#ifdef __i386__
#if BINFMT_ELF
#define TRAMP_LENGTH 16
#else
#define TRAMP_LENGTH 12
#endif
#define TRAMP_ALIGN 16  /* 4 for a i386, 16 for a i486 */
#endif
#ifdef __m68k__
#define TRAMP_LENGTH 14
#define TRAMP_ALIGN 16
#endif
#if defined(__mips__) || defined(__mipsn32__) && !defined(__mips64__)
#if BINFMT_ELF
#define TRAMP_LENGTH 32
#else
#define TRAMP_LENGTH 24
#endif
#define TRAMP_ALIGN 4
#endif
#ifdef __mips64old__
#define TRAMP_LENGTH 56
#define TRAMP_ALIGN 4
#endif
#ifdef __mips64__
#if BINFMT_ELF
#define TRAMP_LENGTH 40
#else
#define TRAMP_LENGTH 32
#endif
#define TRAMP_ALIGN 8
#endif
#if defined(__sparc__) && !defined(__sparc64__)
#define TRAMP_LENGTH 16
#define TRAMP_ALIGN 16
#endif
#ifdef __sparc64__
#define TRAMP_LENGTH 32
#define TRAMP_ALIGN 16
#endif
#ifdef __alpha__
#if BINFMT_ELF
#define TRAMP_LENGTH 40
#else
#define TRAMP_LENGTH 32
#endif
#define TRAMP_ALIGN 8
#endif
#ifdef __hppaold__
#define TRAMP_LENGTH 48
#define TRAMP_ALIGN 16
#endif
#ifdef __hppanew__
#define TRAMP_LENGTH 16
#define TRAMP_ALIGN 16
#define TRAMP_BIAS 2
#endif
#ifdef __arm__
#if BINFMT_ELF
#define TRAMP_LENGTH 24
#else
#define TRAMP_LENGTH 16
#endif
#define TRAMP_ALIGN 4
#endif
#ifdef __powerpcsysv4__
#if BINFMT_ELF
#define TRAMP_LENGTH 32
#else
#define TRAMP_LENGTH 24
#endif
#define TRAMP_ALIGN 4
#endif
#ifdef __powerpcaix__
#define TRAMP_LENGTH 20
#define TRAMP_ALIGN 4
#endif
#ifdef __powerpc64aix__
#define TRAMP_LENGTH 40
#define TRAMP_ALIGN 8
#endif
#ifdef __powerpc64le__
#if BINFMT_ELF
#define TRAMP_LENGTH 40
#else
#define TRAMP_LENGTH 32
#endif
#define TRAMP_ALIGN 8
#endif
#ifdef __ia64__
#if BINFMT_ELF
#else
#define TRAMP_LENGTH 32
#endif
#define TRAMP_ALIGN 16
#endif
#ifdef __x86_64__
#if BINFMT_ELF
#define TRAMP_LENGTH 30
#else
#define TRAMP_LENGTH 22
#endif
#define TRAMP_ALIGN 16
#endif
#ifdef __s390__
#if BINFMT_ELF
#define TRAMP_LENGTH 30
#else
#define TRAMP_LENGTH 22
#endif
#define TRAMP_ALIGN 2
#endif

#ifndef TRAMP_BIAS
#define TRAMP_BIAS 0
#endif

#define TRAMP_TOTAL_LENGTH (TRAMP_LENGTH + 2*sizeof(void*))

#if !defined(CODE_EXECUTABLE) && !defined(EXECUTABLE_VIA_MPROTECT)
/* AIX doesn't support mprotect() in malloc'ed memory. Must get pages of
 * memory with execute permission via mmap(). Then keep a free list of
 * free trampolines.
 */
static char* freelist = NULL;
#endif

__TR_function alloc_trampoline_r (__TR_function address, void* data0, void* data1)
{
  char* function;
  char* data;

#if !defined(CODE_EXECUTABLE)
  static long pagesize = 0;
#if defined(EXECUTABLE_VIA_MMAP_DEVZERO)
  static int zero_fd;
#endif
  /* First, get the page size once and for all. */
  if (!pagesize)
    {
#if defined(HAVE_MACH_VM)
      pagesize = vm_page_size;
#else
      pagesize = getpagesize();
#endif
#if defined(EXECUTABLE_VIA_MMAP_DEVZERO)
      zero_fd = open("/dev/zero",O_RDONLY,0644);
      if (zero_fd < 0)
        { fprintf(stderr,"trampoline: Cannot open /dev/zero!\n"); abort(); }
#endif
    }
#endif

  /* 1. Allocate room */

#if !defined(CODE_EXECUTABLE) && !defined(EXECUTABLE_VIA_MPROTECT)
  /* Note: This memory allocation is not multithread-safe. We might need
   * to add special (platform dependent) code for locking.
   * Fortunately, most modern systems where multithread-safety matters
   * have EXECUTABLE_VIA_MPROTECT, and those which don't (AIX on powerpc and
   * HP-UX on hppa) have CODE_EXECUTABLE. Thus no locking code is needed
   * for the moment.
   */
  if (freelist == NULL)
    { /* Get a new page. */
      char* page;
#ifdef EXECUTABLE_VIA_MMAP_ANONYMOUS
      page = mmap(0, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_VARIABLE, -1, 0);
#endif
#ifdef EXECUTABLE_VIA_MMAP_DEVZERO
      page = mmap(0, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, zero_fd, 0);
#endif
#ifdef EXECUTABLE_VIA_SHM
      int shmid = shmget(IPC_PRIVATE, pagesize, 0700|IPC_CREAT);
      if (shmid<0)
        { page = (char*)(-1); }
      else
        { page = shmat(shmid, 0, 0); shmctl(shmid, IPC_RMID, 0); }
#endif
      if (page == (char*)(-1))
        { fprintf(stderr,"trampoline: Out of virtual memory!\n"); abort(); }
      /* Fill it with free trampolines. */
      { char** last = &freelist;
        char* page_end = page + pagesize;
        while (page+TRAMP_TOTAL_LENGTH <= page_end)
          { *last = page; last = (char**)page;
            page = (char*)(((long)page + TRAMP_TOTAL_LENGTH + TRAMP_ALIGN-1) & -TRAMP_ALIGN);
          }
        *last = NULL;
    } }
  function = freelist; freelist = *(char**)freelist;
#else
  { char* room = (char*) malloc(sizeof(void*) + TRAMP_TOTAL_LENGTH + TRAMP_ALIGN-1);
    if (!room)
      { fprintf(stderr,"trampoline: Out of virtual memory!\n"); abort(); }
    function = (char*)(((long)room + sizeof(void*) + TRAMP_ALIGN-1) & -TRAMP_ALIGN);
    ((char**)function)[-1] = room; /* backpointer for free_trampoline() */
  }
#endif

  /* 2. Fill out the trampoline */
  data = function + TRAMP_LENGTH;
  /* Knowing that data = function + TRAMP_LENGTH, we could certainly optimize
   * the trampolines a little bit more, using PC relative addressing modes.
   * But I doubt it's really worth it.
   */
#ifdef __i386__
#if BINFMT_ELF
  /* function:
   *    subl $16,%esp			83 EC 10
   *    movl $<data>,(%esp)		C7 04 24 <data>
   *    jmp <address>			E9 <address>-<here>
   * here:
   *    nop				90
   */
  *(long *)  (function + 0) = 0xC710EC83;
  *(short *) (function + 4) = 0x2404;
  *(long *)  (function + 6) = (long) data;
  *(char *)  (function +10) = 0xE9;
  *(long *)  (function +11) = (long) address - (long) (function + 15);
  *(char *)  (function +15) = 0x90;   /* nop, for alignment */
#define is_tramp(function)  \
  *(unsigned long *)  (function + 0) == 0xC710EC83 && \
  *(unsigned short *) (function + 4) == 0x2404 && \
  *(unsigned char *)  (function +10) == 0xE9
#define tramp_address(function)  \
  *(long *)  (function +11) + (long) (function + 15)
#define tramp_data(function)  \
  *(long *)  (function + 6)
#else
  /* function:
   *    movl $<data>,%ecx		B9 <data>
   *    jmp <address>			E9 <address>-<here>
   * here:
   *    nop				90
   *    nop				90
   */
  *(char *)  (function + 0) = 0xB9;
  *(long *)  (function + 1) = (long) data;
  *(char *)  (function + 5) = 0xE9;
  *(long *)  (function + 6) = (long) address - (long) (function + 10);
  *(short *) (function +10) = 0x9090;   /* nop nop, for alignment */
#define is_tramp(function)  \
  *(unsigned char *)  (function + 0) == 0xB9 && \
  *(unsigned char *)  (function + 5) == 0xE9
#define tramp_address(function)  \
  *(long *)  (function + 6) + (long) (function + 10)
#define tramp_data(function)  \
  *(long *)  (function + 1)
#endif
#endif
#ifdef __m68k__
#if BINFMT_ELF
  /* function:
   *    movel #<data>,sp@-		2F 3C <data>
   *    jmp <address>			4E F9 <address>
   *    nop				4E 71
   */
  *(short *) (function + 0) = 0x2F3C;
  *(long *)  (function + 2) = (long) data;
  *(short *) (function + 6) = 0x4EF9;
  *(long *)  (function + 8) = (long) address;
  *(short *) (function +12) = 0x4E71;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0x2F3C && \
  *(unsigned short *) (function + 6) == 0x4EF9 && \
  *(unsigned short *) (function +12) == 0x4E71
#else
#ifdef __NetBSD__
  /* function:
   *    movel #<data>,a1		22 7C <data>
   *    jmp <address>			4E F9 <address>
   *    nop				4E 71
   */
  *(short *) (function + 0) = 0x227C;
  *(long *)  (function + 2) = (long) data;
  *(short *) (function + 6) = 0x4EF9;
  *(long *)  (function + 8) = (long) address;
  *(short *) (function +12) = 0x4E71;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0x227C && \
  *(unsigned short *) (function + 6) == 0x4EF9 && \
  *(unsigned short *) (function +12) == 0x4E71
#else
  /* function:
   *    movel #<data>,a0		20 7C <data>
   *    jmp <address>			4E F9 <address>
   *    nop				4E 71
   */
  *(short *) (function + 0) = 0x207C;
  *(long *)  (function + 2) = (long) data;
  *(short *) (function + 6) = 0x4EF9;
  *(long *)  (function + 8) = (long) address;
  *(short *) (function +12) = 0x4E71;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0x207C && \
  *(unsigned short *) (function + 6) == 0x4EF9 && \
  *(unsigned short *) (function +12) == 0x4E71
#endif
#endif
#define tramp_address(function)  \
  *(long *)  (function + 8)
#define tramp_data(function)  \
  *(long *)  (function + 2)
#endif
#if defined(__mips__) || defined(__mipsn32__) && !defined(__mips64__)
#if BINFMT_ELF
  /* function:
   *    subu $29,$29,16			27 BD FF F0
   *    lw $2,24($25)			8F 22 00 18
   *    lw $25,28($25)			8F 39 00 1C
   *    sw $2,0($29)			AF A2 00 00
   *    jr $25				03 20 00 08
   *    nop				00 00 00 00
   *    .word <data>			<data>
   *    .word <address>			<address>
   */
  *(unsigned int *) (function + 0) = 0x27BDFFF0;
  *(unsigned int *) (function + 4) = 0x8F220018;
  *(unsigned int *) (function + 8) = 0x8F39001C;
  *(unsigned int *) (function +12) = 0xAFA20000;
  *(unsigned int *) (function +16) = 0x03200008;
  *(unsigned int *) (function +20) = 0x00000000;
  *(unsigned int *) (function +24) = (unsigned int) data;
  *(unsigned int *) (function +28) = (unsigned int) address;
#define is_tramp(function)  \
  *(unsigned int *) (function + 0) == 0x27BDFFF0 && \
  *(unsigned int *) (function + 4) == 0x8F220018 && \
  *(unsigned int *) (function + 8) == 0x8F39001C && \
  *(unsigned int *) (function +12) == 0xAFA20000 && \
  *(unsigned int *) (function +16) == 0x03200008 && \
  *(unsigned int *) (function +20) == 0x00000000
#define tramp_address(function)  \
  *(unsigned int *) (function +28)
#define tramp_data(function)  \
  *(unsigned int *) (function +24)
#else
  /* function:
   *    lw $2,16($25)			8F 22 00 10
   *    lw $25,20($25)			8F 39 00 14
   *    j $25				03 20 00 08
   *    nop				00 00 00 00
   *    .word <data>			<data>
   *    .word <address>			<address>
   */
  *(unsigned int *) (function + 0) = 0x8F220010;
  *(unsigned int *) (function + 4) = 0x8F390014;
  *(unsigned int *) (function + 8) = 0x03200008;
  *(unsigned int *) (function +12) = 0x00000000;
  *(unsigned int *) (function +16) = (unsigned int) data;
  *(unsigned int *) (function +20) = (unsigned int) address;
#define is_tramp(function)  \
  *(unsigned int *) (function + 0) == 0x8F220010 && \
  *(unsigned int *) (function + 4) == 0x8F390014 && \
  *(unsigned int *) (function + 8) == 0x03200008 && \
  *(unsigned int *) (function +12) == 0x00000000
#define tramp_address(function)  \
  *(unsigned int *) (function +20)
#define tramp_data(function)  \
  *(unsigned int *) (function +16)
#endif
#endif
#ifdef __mips64old__
  /* function:
   *    dli $2,<data>			3C 02 hi16(hi32(<data>))
   *					34 42 lo16(hi32(<data>))
   *					00 02 14 38
   *					34 42 hi16(lo32(<data>))
   *					00 02 14 38
   *					34 42 lo16(lo32(<data>))
   *    dli $25,<address>		3C 19 hi16(hi32(<address>))
   *					37 39 lo16(hi32(<address>))
   *					00 19 CC 38
   *					37 39 hi16(lo32(<address>))
   *					00 19 CC 38
   *					37 39 lo16(lo32(<address>))
   *    j $25				03 20 00 08
   *    nop				00 00 00 00
   */
  /* What about big endian / little endian ?? */
  *(short *) (function + 0) = 0x3C02;
  *(short *) (function + 2) = (unsigned long) data >> 48;
  *(short *) (function + 4) = 0x3442;
  *(short *) (function + 6) = ((unsigned long) data >> 32) & 0xffff;
  *(int *)   (function + 8) = 0x00021438;
  *(short *) (function +12) = 0x3442;
  *(short *) (function +14) = ((unsigned long) data >> 16) & 0xffff;
  *(int *)   (function +16) = 0x00021438;
  *(short *) (function +20) = 0x3442;
  *(short *) (function +22) = (unsigned long) data & 0xffff;
  *(short *) (function +24) = 0x3C19;
  *(short *) (function +26) = (unsigned long) address >> 48;
  *(short *) (function +28) = 0x3739;
  *(short *) (function +30) = ((unsigned long) address >> 32) & 0xffff;
  *(int *)   (function +32) = 0x0019CC38;
  *(short *) (function +36) = 0x3739;
  *(short *) (function +38) = ((unsigned long) address >> 16) & 0xffff;
  *(int *)   (function +40) = 0x0019CC38;
  *(short *) (function +44) = 0x3739;
  *(short *) (function +46) = (unsigned long) address & 0xffff;
  *(int *)   (function +48) = 0x03200008;
  *(int *)   (function +52) = 0x00000000;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0x3C02 && \
  *(unsigned short *) (function + 4) == 0x3442 && \
  *(unsigned int *)   (function + 8) == 0x00021438 && \
  *(unsigned short *) (function +12) == 0x3442 && \
  *(unsigned int *)   (function +16) == 0x00021438 && \
  *(unsigned short *) (function +20) == 0x3442 && \
  *(unsigned short *) (function +24) == 0x3C19 && \
  *(unsigned short *) (function +28) == 0x3739 && \
  *(unsigned int *)   (function +32) == 0x0019CC38 && \
  *(unsigned short *) (function +36) == 0x3739 && \
  *(unsigned int *)   (function +40) == 0x0019CC38 && \
  *(unsigned short *) (function +44) == 0x3739 && \
  *(unsigned int *)   (function +48) == 0x03200008 && \
  *(unsigned int *)   (function +52) == 0x00000000
#define hilo(word3,word2,word1,word0)  \
  (((unsigned long) (word3) << 48) | ((unsigned long) (word2) << 32) | \
   ((unsigned long) (word1) << 16) | (unsigned long) (word0))
#define tramp_address(function)  \
  hilo(*(unsigned short *) (function +26), \
       *(unsigned short *) (function +30), \
       *(unsigned short *) (function +38), \
       *(unsigned short *) (function +46))
#define tramp_data(function)  \
  hilo(*(unsigned short *) (function + 2), \
       *(unsigned short *) (function + 6), \
       *(unsigned short *) (function +14), \
       *(unsigned short *) (function +22))
#endif
#ifdef __mips64__
#if BINFMT_ELF
  /* function:
   *    dsubu $29,$29,16		67 BD DD F0
   *    ld $2,24($25)			DF 22 00 18
   *    ld $25,32($25)			DF 39 00 20
   *    sd $2,0($29)			FF A2 00 00
   *    j $25				03 20 00 08
   *    nop				00 00 00 00
   *    .dword <data>			<data>
   *    .dword <address>		<address>
   */
  *(unsigned int *)  (function + 0) = 0x67BDDDF0;
  *(unsigned int *)  (function + 4) = 0xDF220018;
  *(unsigned int *)  (function + 8) = 0xDF390020;
  *(unsigned int *)  (function +12) = 0xFFA20000;
  *(unsigned int *)  (function +16) = 0x03200008;
  *(unsigned int *)  (function +20) = 0x00000000;
  *(unsigned long *) (function +24) = (unsigned long) data;
  *(unsigned long *) (function +32) = (unsigned long) address;
#define is_tramp(function)  \
  *(unsigned int *)  (function + 0) == 0x67BDDDF0 && \
  *(unsigned int *)  (function + 4) == 0xDF220018 && \
  *(unsigned int *)  (function + 8) == 0xDF390020 && \
  *(unsigned int *)  (function +12) == 0xFFA20000 && \
  *(unsigned int *)  (function +16) == 0x03200008 && \
  *(unsigned int *)  (function +20) == 0x00000000
#define tramp_address(function)  \
  *(unsigned long *) (function +32)
#define tramp_data(function)  \
  *(unsigned long *) (function +24)
#else
  /* function:
   *    ld $2,16($25)			DF 22 00 10
   *    ld $25,24($25)			DF 39 00 18
   *    j $25				03 20 00 08
   *    nop				00 00 00 00
   *    .dword <data>			<data>
   *    .dword <address>		<address>
   */
  *(unsigned int *)  (function + 0) = 0xDF220010;
  *(unsigned int *)  (function + 4) = 0xDF390018;
  *(unsigned int *)  (function + 8) = 0x03200008;
  *(unsigned int *)  (function +12) = 0x00000000;
  *(unsigned long *) (function +16) = (unsigned long) data;
  *(unsigned long *) (function +24) = (unsigned long) address;
#define is_tramp(function)  \
  *(unsigned int *)  (function + 0) == 0xDF220010 && \
  *(unsigned int *)  (function + 4) == 0xDF390018 && \
  *(unsigned int *)  (function + 8) == 0x03200008 && \
  *(unsigned int *)  (function +12) == 0x00000000
#define tramp_address(function)  \
  *(unsigned long *) (function +24)
#define tramp_data(function)  \
  *(unsigned long *) (function +16)
#endif
#endif
#if defined(__sparc__) && !defined(__sparc64__)
  /* function:
   *    sethi %hi(<data>),%g2		05000000 | (<data> >> 10)
   *    sethi %hi(<address>),%g1	03000000 | (<address> >> 10)
   *    jmp %g1+%lo(<address>)		81C06000 | (<address> & 0x3ff)
   *    or %g2,%lo(<data>),%g2		8410A000 | (<data> & 0x3ff)
   */
#define hi(word)  ((unsigned long) (word) >> 10)
#define lo(word)  ((unsigned long) (word) & 0x3ff)
  *(long *) (function + 0) = 0x05000000 | hi(data);
  *(long *) (function + 4) = 0x03000000 | hi(address);
  *(long *) (function + 8) = 0x81C06000 | lo(address);
  *(long *) (function +12) = 0x8410A000 | lo(data);
#define is_tramp(function)  \
  (*(long *) (function + 0) & 0xffc00000) == 0x05000000 && \
  (*(long *) (function + 4) & 0xffc00000) == 0x03000000 && \
  (*(long *) (function + 8) & 0xfffffc00) == 0x81C06000 && \
  (*(long *) (function +12) & 0xfffffc00) == 0x8410A000
#define hilo(hiword,loword)  (((hiword) << 10) | ((loword) & 0x3ff))
#define tramp_address(function)  \
  hilo(*(long *) (function + 4), *(long *) (function + 8))
#define tramp_data(function)  \
  hilo(*(long *) (function + 0), *(long *) (function +12))
#endif
#ifdef __sparc64__
  /* function:
   *    rd %pc,%g1			83414000
   *    ldx [%g1+24],%g2		C4586018
   *    jmp %g2				81C08000
   *    ldx [%g1+16],%g5		CA586010
   *    .long high32(<data>)		<data> >> 32
   *    .long low32(<data>)		<data> & 0xffffffff
   *    .long high32(<address>)		<address> >> 32
   *    .long low32(<address>)		<address> & 0xffffffff
   */
  *(int *)  (function + 0) = 0x83414000;
  *(int *)  (function + 4) = 0xC4586018;
  *(int *)  (function + 8) = 0x81C08000;
  *(int *)  (function +12) = 0xCA586010;
  *(long *) (function +16) = (long) data;
  *(long *) (function +24) = (long) address;
#define is_tramp(function)  \
  *(int *)  (function + 0) == 0x83414000 && \
  *(int *)  (function + 4) == 0xC4586018 && \
  *(int *)  (function + 8) == 0x81C08000 && \
  *(int *)  (function +12) == 0xCA586010
#define tramp_address(function)  \
  *(long *) (function +24)
#define tramp_data(function)  \
  *(long *) (function +16)
#endif
#ifdef __alpha__
#if BINFMT_ELF
  /* function:
   *    br $1,function..ng	00 00 20 C0
   * function..ng:
   *    ldq $2,20($1)		14 00 41 A4
   *    lda $30,-16($30)	F0 FF DE 23
   *    ldq $27,28($1)		1C 00 61 A7
   *    stq $2,0($30)		00 00 5E B4
   *    jmp $31,($27),0		00 00 FB 6B
   *    .quad <data>		<data>
   *    .quad <address>		<address>
   */
  { static int code [6] =
      { 0xC0200000, 0xA4410014, 0x23DEFFF0, 0xA761001C, 0xB45E0000, 0x6BFB0000 };
    int i;
    for (i=0; i<6; i++) { ((int *) function)[i] = code[i]; }
    ((long *) function)[3] = (long) data;
    ((long *) function)[4] = (long) address;
  }
#define is_tramp(function)  \
  ((int *) function)[0] == 0xC0200000 && \
  ((int *) function)[1] == 0xA4410014 && \
  ((int *) function)[2] == 0x23DEFFF0 && \
  ((int *) function)[3] == 0xA761001C && \
  ((int *) function)[4] == 0xB45E0000 && \
  ((int *) function)[5] == 0x6BFB0000
#define tramp_address(function)  \
  ((long *) function)[4]
#define tramp_data(function)  \
  ((long *) function)[3]
#else
  /* function:
   *    br $1,function..ng	00 00 20 C0
   * function..ng:
   *    ldq $27,20($1)		14 00 61 A7
   *    ldq $1,12($1)		0C 00 21 A4
   *    jmp $31,($27),0		00 00 FB 6B
   *    .quad <data>		<data>
   *    .quad <address>		<address>
   */
  { static int code [4] =
      { 0xC0200000, 0xA7610014, 0xA421000C, 0x6BFB0000 };
    int i;
    for (i=0; i<4; i++) { ((int *) function)[i] = code[i]; }
    ((long *) function)[2] = (long) data;
    ((long *) function)[3] = (long) address;
  }
#define is_tramp(function)  \
  ((int *) function)[0] == 0xC0200000 && \
  ((int *) function)[1] == 0xA7610014 && \
  ((int *) function)[2] == 0xA421000C && \
  ((int *) function)[3] == 0x6BFB0000
#define tramp_address(function)  \
  ((long *) function)[3]
#define tramp_data(function)  \
  ((long *) function)[2]
#endif
#endif
#ifdef __hppaold__
  /* function:
   *    ldil L'<data>,%r29		23A00000 | hi(<data>)
   *    ldil L'<address>,%r21		22A00000 | hi(<address>)
   *    ldo R'<data>(%r29),%r29		37BD0000 | lo(<data>)
   *    ldo R'<address>(%r21),%r21	36B50000 | lo(<address>)
   *    bb,>=,n %r21,30,function2	C7D5C012
   *    depi 0,31,2,%r21		D6A01C1E
   *    ldw 4(0,%r21),%r19		4AB30008
   *    ldw 0(0,%r21),%r21		4AB50000
   * function2:
   *    ldsid (0,%r21),%r1		02A010A1
   *    mtsp %r1,%sr0			00011820
   *    be,n 0(%sr0,%r21)		E2A00002
   *    nop				08000240
   */
  /* When decoding a 21-bit argument in an instruction, the hppa performs
   * the following bit manipulation:
   * assemble21: x[20]...x[0]
   *       --> x[0] x[11]...x[1] x[15]..x[14] x[20]...x[16] x[13]..x[12]
   * When encoding a 21-bit argument into an instruction, we need the
   * to perform the reverse permutation:
   * permute21:  y[20]...y[0]
   *       --> y[6]...y[2] y[8]..y[7] y[1]..y[0] y[19]...y[9] y[20]
   */
#define assemble21(x)  \
  ((((x) & 0x1) << 20) | (((x) & 0xFFE) << 8) | \
   (((x) & 0xC000) >> 7) | (((x) & 0x1F0000) >> 14) | (((x) & 0x3000) >> 12))
#define permute21(y)  \
  ((((y) & 0x7C) << 14) | (((y) & 0x180) << 7) | (((y) & 0x3) << 12) | \
   (((y) & 0xFFE00) >> 8) | (((y) & 0x100000) >> 20))
#define hi(word)  permute21((unsigned long) (word) >> 11)
#define lo(word)  (((unsigned long) (word) & 0x7FF) << 1)
  *(long *) (function + 0) = 0x23A00000 | hi(data);
  *(long *) (function + 4) = 0x22A00000 | hi(address);
  *(long *) (function + 8) = 0x37BD0000 | lo(data);
  *(long *) (function +12) = 0x36B50000 | lo(address);
  *(long *) (function +16) = 0xC7D5C012;
  *(long *) (function +20) = 0xD6A01C1E;
  *(long *) (function +24) = 0x4AB30008;
  *(long *) (function +28) = 0x4AB50000;
  *(long *) (function +32) = 0x02A010A1;
  *(long *) (function +36) = 0x00011820;
  *(long *) (function +40) = 0xE2A00002;
  *(long *) (function +44) = 0x08000240;
#define is_tramp(function)  \
  ((long) function & 3) == 0 && \
  (*(long *) (function + 0) & 0xffe00000) == 0x23A00000 && \
  (*(long *) (function + 4) & 0xffe00000) == 0x22A00000 && \
  (*(long *) (function + 8) & 0xfffff000) == 0x37BD0000 && \
  (*(long *) (function +12) & 0xfffff000) == 0x36B50000 && \
  *(long *) (function +16) == 0xC7D5C012 && \
  *(long *) (function +20) == 0xD6A01C1E && \
  *(long *) (function +24) == 0x4AB30008 && \
  *(long *) (function +28) == 0x4AB50000 && \
  *(long *) (function +32) == 0x02A010A1 && \
  *(long *) (function +36) == 0x00011820 && \
  *(long *) (function +40) == 0xE2A00002 && \
  *(long *) (function +44) == 0x08000240
#define hilo(hiword,loword)  \
  ((assemble21((unsigned long) (hiword)) << 11) | \
   (((unsigned long) (loword) & 0xFFE) >> 1) \
  )
#define tramp_address(function)  \
  hilo(*(long *) (function + 4), *(long *) (function +12))
#define tramp_data(function)  \
  hilo(*(long *) (function + 0), *(long *) (function + 8))
#endif
#ifdef __hppanew__
  /* function:
   *    .long   tramp_r
   *    .long   closure
   * closure:
   *    .long   <data>
   *    .long   <address>
   */
  { /* work around a bug in gcc 3.* */
    void* tramp_r_address = &tramp_r;
    *(long *) (function + 0) = ((long *) ((char*)tramp_r_address-2))[0];
    *(long *) (function + 4) = (long) (function + 8);
    *(long *) (function + 8) = (long) data;
    *(long *) (function +12) = (long) address;
  }
#define is_tramp(function)  \
  ((long *) function)[0] == ((long *) ((char*)tramp_r_address-2))[0]
#define tramp_address(function)  \
  ((long *) function)[3]
#define tramp_data(function)  \
  ((long *) function)[2]
#endif
#ifdef __arm__
#if BINFMT_ELF
  /* function:
   *	add	r12,pc,#16		E28FC010
   *	sub	sp,sp,#8		E24DD008
   *	str	r12,[sp]		E58DC000
   *	ldr	pc,[pc]			E59FF000
   * _data:
   *	.word	<data>
   * _function:
   *	.word	<address>
   */
  {
    ((long *) function)[0] = 0xE28FC010;
    ((long *) function)[1] = 0xE24DD008;
    ((long *) function)[2] = 0xE58DC000;
    ((long *) function)[3] = 0xE59FF000;
    ((long *) function)[4] = (long) data;
    ((long *) function)[5] = (long) address;
  }
#define is_tramp(function)  \
  ((long *) function)[0] == 0xE28FC010 && \
  ((long *) function)[1] == 0xE24DD008 && \
  ((long *) function)[2] == 0xE58DC000 && \
  ((long *) function)[3] == 0xE59FF000
#define tramp_address(function)  \
  ((long *) function)[5]
#define tramp_data(function)  \
  ((long *) function)[4]
#else
  /* function:
   *	add	r12,pc,#8		E28FC008
   *	ldr	pc,[pc]			E59FF000
   * _data:
   *	.word	<data>
   * _function:
   *	.word	<address>
   */
  {
    ((long *) function)[0] = 0xE28FC008;
    ((long *) function)[1] = 0xE59FF000;
    ((long *) function)[2] = (long) data;
    ((long *) function)[3] = (long) address;
  }
#define is_tramp(function)  \
  ((long *) function)[0] == 0xE28FC008 && \
  ((long *) function)[1] == 0xE59FF000
#define tramp_address(function)  \
  ((long *) function)[3]
#define tramp_data(function)  \
  ((long *) function)[2]
#endif
#endif
#ifdef __powerpcsysv4__
#if BINFMT_ELF
  /* function:
   *    {liu|lis} 11,hi16(<data>)		3D 60 hi16(<data>)
   *    {oril|ori} 11,11,lo16(<data>)		61 6B lo16(<data>)
   *    {liu|lis} 0,hi16(<address>)		3C 00 hi16(<address>)
   *    {oril|ori} 0,0,lo16(<address>)		60 00 lo16(<address>)
   *    addi 1,1,-16				38 21 FF F0
   *    stw 11,0(1)				91 61 00 00
   *    mtctr 0					7C 09 03 A6
   *    bctr					4E 80 04 20
   */
  *(short *) (function + 0) = 0x3D60;
  *(short *) (function + 2) = (unsigned long) data >> 16;
  *(short *) (function + 4) = 0x616B;
  *(short *) (function + 6) = (unsigned long) data & 0xffff;
  *(short *) (function + 8) = 0x3C00;
  *(short *) (function +10) = (unsigned long) address >> 16;
  *(short *) (function +12) = 0x6000;
  *(short *) (function +14) = (unsigned long) address & 0xffff;
  *(long *)  (function +16) = 0x3821FFF0;
  *(long *)  (function +20) = 0x91610000;
  *(long *)  (function +24) = 0x7C0903A6;
  *(long *)  (function +28) = 0x4E800420;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0x3D60 && \
  *(unsigned short *) (function + 4) == 0x616B && \
  *(unsigned short *) (function + 8) == 0x3C00 && \
  *(unsigned short *) (function +12) == 0x6000 && \
  *(unsigned long *)  (function +16) == 0x3821FFF0 && \
  *(unsigned long *)  (function +20) == 0x91610000 && \
  *(unsigned long *)  (function +24) == 0x7C0903A6 && \
  *(unsigned long *)  (function +28) == 0x4E800420
#else
#ifdef __NetBSD__
  /* function:
   *    {liu|lis} 13,hi16(<data>)		3D A0 hi16(<data>)
   *    {oril|ori} 13,13,lo16(<data>)		61 AD lo16(<data>)
   *    {liu|lis} 0,hi16(<address>)		3C 00 hi16(<address>)
   *    {oril|ori} 0,0,lo16(<address>)		60 00 lo16(<address>)
   *    mtctr 0					7C 09 03 A6
   *    bctr					4E 80 04 20
   */
  *(short *) (function + 0) = 0x3DA0;
  *(short *) (function + 2) = (unsigned long) data >> 16;
  *(short *) (function + 4) = 0x61AD;
  *(short *) (function + 6) = (unsigned long) data & 0xffff;
  *(short *) (function + 8) = 0x3C00;
  *(short *) (function +10) = (unsigned long) address >> 16;
  *(short *) (function +12) = 0x6000;
  *(short *) (function +14) = (unsigned long) address & 0xffff;
  *(long *)  (function +16) = 0x7C0903A6;
  *(long *)  (function +20) = 0x4E800420;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0x3DA0 && \
  *(unsigned short *) (function + 4) == 0x61AD && \
  *(unsigned short *) (function + 8) == 0x3C00 && \
  *(unsigned short *) (function +12) == 0x6000 && \
  *(unsigned long *)  (function +16) == 0x7C0903A6 && \
  *(unsigned long *)  (function +20) == 0x4E800420
#else
  /* function:
   *    {liu|lis} 11,hi16(<data>)		3D 60 hi16(<data>)
   *    {oril|ori} 11,11,lo16(<data>)		61 6B lo16(<data>)
   *    {liu|lis} 0,hi16(<address>)		3C 00 hi16(<address>)
   *    {oril|ori} 0,0,lo16(<address>)		60 00 lo16(<address>)
   *    mtctr 0					7C 09 03 A6
   *    bctr					4E 80 04 20
   */
  *(short *) (function + 0) = 0x3D60;
  *(short *) (function + 2) = (unsigned long) data >> 16;
  *(short *) (function + 4) = 0x616B;
  *(short *) (function + 6) = (unsigned long) data & 0xffff;
  *(short *) (function + 8) = 0x3C00;
  *(short *) (function +10) = (unsigned long) address >> 16;
  *(short *) (function +12) = 0x6000;
  *(short *) (function +14) = (unsigned long) address & 0xffff;
  *(long *)  (function +16) = 0x7C0903A6;
  *(long *)  (function +20) = 0x4E800420;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0x3D60 && \
  *(unsigned short *) (function + 4) == 0x616B && \
  *(unsigned short *) (function + 8) == 0x3C00 && \
  *(unsigned short *) (function +12) == 0x6000 && \
  *(unsigned long *)  (function +16) == 0x7C0903A6 && \
  *(unsigned long *)  (function +20) == 0x4E800420
#endif
#endif
#define hilo(hiword,loword)  \
  (((unsigned long) (hiword) << 16) | (unsigned long) (loword))
#define tramp_address(function)  \
  hilo(*(unsigned short *) (function +10), *(unsigned short *) (function +14))
#define tramp_data(function)  \
  hilo(*(unsigned short *) (function + 2), *(unsigned short *) (function + 6))
#endif
#ifdef __powerpcaix__
  /* function:
   *    .long .tramp_r
   *    .long .mytoc
   *    .long 0
   * .mytoc:
   *    .long <data>
   *    .long <address>
   */
  *(long *)  (function + 0) = ((long *) &tramp_r)[0];
  *(long *)  (function + 4) = (long) (function + 12);
  *(long *)  (function + 8) = 0;
  *(long *)  (function +12) = (long) data;
  *(long *)  (function +16) = (long) address;
#define is_tramp(function)  \
  ((long *) function)[0] == ((long *) &tramp_r)[0]
#define tramp_address(function)  \
  ((long *) function)[4]
#define tramp_data(function)  \
  ((long *) function)[3]
#endif
#ifdef __powerpc64aix__
  /* function:
   *    .quad .tramp_r
   *    .quad .mytoc
   *    .quad 0
   * .mytoc:
   *    .quad <data>
   *    .quad <address>
   */
  *(long *)  (function + 0) = ((long *) &tramp_r)[0];
  *(long *)  (function + 8) = (long) (function + 24);
  *(long *)  (function +16) = 0;
  *(long *)  (function +24) = (long) data;
  *(long *)  (function +32) = (long) address;
#define is_tramp(function)  \
  ((long *) function)[0] == ((long *) &tramp_r)[0]
#define tramp_address(function)  \
  ((long *) function)[4]
#define tramp_data(function)  \
  ((long *) function)[3]
#endif
#ifdef __powerpc64le__
#if BINFMT_ELF
  /* function:
   *    ld      r11,24(r12)		18 00 6C E9
   *    ld      r12,32(r12)		20 00 8C E9
   *    addi    r1,r1,-16		F0 FF 21 38
   *    std     r11,0(r1)		00 00 61 F9
   *    mtctr   r12			A6 03 89 7D
   *    bctr				20 04 80 4E
   *    .quad <data>
   *    .quad <address>
   */
  *(int *)  (function + 0) = 0xE96C0018;
  *(int *)  (function + 4) = 0xE98C0020;
  *(int *)  (function + 8) = 0x3821FFF0;
  *(int *)  (function +12) = 0xF9610000;
  *(int *)  (function +16) = 0x7D8903A6;
  *(int *)  (function +20) = 0x4E800420;
  *(long *) (function +24) = (long) data;
  *(long *) (function +32) = (long) address;
#define is_tramp(function)  \
  *(unsigned int *) (function + 0) == 0xE96C0018 && \
  *(unsigned int *) (function + 4) == 0xE98C0020 && \
  *(unsigned int *) (function + 8) == 0x3821FFF0 && \
  *(unsigned int *) (function +12) == 0xF9610000 && \
  *(unsigned int *) (function +16) == 0x7D8903A6 && \
  *(unsigned int *) (function +20) == 0x4E800420
#define tramp_address(function)  \
  ((long *) function)[4]
#define tramp_data(function)  \
  ((long *) function)[3]
#else
  /* function:
   *    ld      r11,16(r12)		10 00 6C E9
   *    ld      r12,24(r12)		18 00 8C E9
   *    mtctr   r12			A6 03 89 7D
   *    bctr				20 04 80 4E
   *    .quad <data>
   *    .quad <address>
   */
  *(int *)  (function + 0) = 0xE96C0010;
  *(int *)  (function + 4) = 0xE98C0018;
  *(int *)  (function + 8) = 0x7D8903A6;
  *(int *)  (function +12) = 0x4E800420;
  *(long *) (function +16) = (long) data;
  *(long *) (function +24) = (long) address;
#define is_tramp(function)  \
  *(unsigned int *) (function + 0) == 0xE96C0010 && \
  *(unsigned int *) (function + 4) == 0xE98C0018 && \
  *(unsigned int *) (function + 8) == 0x7D8903A6 && \
  *(unsigned int *) (function +12) == 0x4E800420
#define tramp_address(function)  \
  ((long *) function)[3]
#define tramp_data(function)  \
  ((long *) function)[2]
#endif
#endif
#ifdef __ia64__
  /* function:
   *    data8   tramp_r
   *    data8   closure
   * closure:
   *    data8   <address>
   *    data8   <data>
   */
  *(long *) (function + 0) = (long) &tramp_r;
  *(long *) (function + 8) = (long) (function + 16);
  *(long *) (function +16) = (long) address;
  *(long *) (function +24) = (long) data;
#define is_tramp(function)  \
  ((long *) function)[0] == (long) &tramp_r
#define tramp_address(function)  \
  ((long *) function)[2]
#define tramp_data(function)  \
  ((long *) function)[3]
#endif
#ifdef __x86_64__
#if BINFMT_ELF
  /* function:
   *    movabsq $<data>,%r10		49 BA <data>
   *    movabsq $<address>,%rax		48 B8 <address>
   *    subq $16,%rsp			48 83 EC 10
   *    movq %r10,(%rsp)		4C 89 14 24
   *    jmp *%rax			FF E0
   */
#else
  /* function:
   *    movabsq $<data>,%r10		49 BA <data>
   *    movabsq $<address>,%rax		48 B8 <address>
   *    jmp *%rax			FF E0
   */
#endif
  *(short *) (function + 0) = 0xBA49;
  *(short *) (function + 2) = (unsigned long) data & 0xffff;
  *(int *)   (function + 4) = ((unsigned long) data >> 16) & 0xffffffff;
  *(short *) (function + 8) = ((unsigned long) data >> 48) & 0xffff;
  *(short *) (function +10) = 0xB848;
  *(int *)   (function +12) = (unsigned long) address & 0xffffffff;
  *(int *)   (function +16) = ((unsigned long) address >> 32) & 0xffffffff;
#if BINFMT_ELF
  *(int *)   (function +20) = 0x10EC8348;
  *(int *)   (function +24) = 0x2414894C;
  *(short *) (function +28) = 0xE0FF;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0xBA49 && \
  *(unsigned short *) (function +10) == 0xB848 && \
  *(unsigned int *)   (function +20) == 0x10EC8348 && \
  *(unsigned int *)   (function +24) == 0x2414894C && \
  *(unsigned short *) (function +28) == 0xE0FF
#else
  *(short *) (function +20) = 0xE0FF;
#define is_tramp(function)  \
  *(unsigned short *) (function + 0) == 0xBA49 && \
  *(unsigned short *) (function +10) == 0xB848 && \
  *(unsigned short *) (function +20) == 0xE0FF
#endif
#define hilo(hiword,loword)  \
  (((unsigned long) (hiword) << 32) | (unsigned long) (loword))
#define himidlo(hishort,midword,loshort)  \
  (((unsigned long) (hishort) << 48) | (unsigned long) (midword) << 16 \
   | (unsigned long) (loshort))
#define tramp_address(function)  \
  hilo(*(unsigned int *) (function +16), *(unsigned int *) (function +12))
#define tramp_data(function)  \
  himidlo(*(unsigned short *) (function + 8), \
          *(unsigned int *)   (function + 4), \
          *(unsigned short *) (function + 2))
#endif
#ifdef __s390__
#if BINFMT_ELF
  /* function:
        bras    %r1,function+12		A7 15 00 06
        .long   <data>
        .long   <address>
     function+12:
        l       %r0,0(%r1)		58 00 10 00
        ahi     %r15,-8			A7 FA FF F8
        l       %r1,4(%r1)		58 10 10 04
        st      %r0,0(%r15)		50 00 F0 00
        br      %r1			07 F1
  */
  *(int *)   (function + 0) = 0xA7150006;
  *(int *)   (function + 4) = (unsigned int) data;
  *(int *)   (function + 8) = (unsigned int) address;
  *(int *)   (function +12) = 0x58001000;
  *(int *)   (function +16) = 0xA7FAFFF8;
  *(int *)   (function +20) = 0x58101004;
  *(int *)   (function +24) = 0x5000F000;
  *(short *) (function +28) = 0x07F1;
#define is_tramp(function)  \
  *(unsigned int *)   (function + 0) == 0xA7150006 && \
  *(unsigned int *)   (function +12) == 0x58001000 && \
  *(unsigned int *)   (function +16) == 0xA7FAFFF8 && \
  *(unsigned int *)   (function +20) == 0x58101004 && \
  *(unsigned int *)   (function +14) == 0x5000F000 && \
  *(unsigned short *) (function +28) == 0x07F1
#else
  /* function:
        bras    %r1,function+12		A7 15 00 06
        .long   <data>
        .long   <address>
     function+12:
        l       %r0,0(%r1)		58 00 10 00
        l       %r1,4(%r1)		58 10 10 04
        br      %r1			07 F1
  */
  *(int *)   (function + 0) = 0xA7150006;
  *(int *)   (function + 4) = (unsigned int) data;
  *(int *)   (function + 8) = (unsigned int) address;
  *(int *)   (function +12) = 0x58001000;
  *(int *)   (function +16) = 0x58101004;
  *(short *) (function +20) = 0x07F1;
#define is_tramp(function)  \
  *(unsigned int *)   (function + 0) == 0xA7150006 && \
  *(unsigned int *)   (function +12) == 0x58001000 && \
  *(unsigned int *)   (function +16) == 0x58101004 && \
  *(unsigned short *) (function +20) == 0x07F1
#endif
#define tramp_address(function)  \
  *(unsigned int *) (function + 8)
#define tramp_data(function)  \
  *(unsigned int *) (function + 4)
#endif
  /*
   * data:
   *    <data0>				<data0>
   *    <data1>				<data1>
   */
  *(long *)  (data + 0*sizeof(void*)) = (long) data0;
  *(long *)  (data + 1*sizeof(void*)) = (long) data1;

  /* 3. Set memory protection to "executable" */

#if !defined(CODE_EXECUTABLE) && defined(EXECUTABLE_VIA_MPROTECT)
  /* Call mprotect on the pages that contain the range. */
  { unsigned long start_addr = (unsigned long) function;
    unsigned long end_addr = (unsigned long) (function + TRAMP_LENGTH);
    start_addr = start_addr & -pagesize;
    end_addr = (end_addr + pagesize-1) & -pagesize;
   {unsigned long len = end_addr - start_addr;
#if defined(HAVE_MACH_VM)
    if (vm_protect(task_self(),start_addr,len,0,VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE) != KERN_SUCCESS)
#else
    if (mprotect((void*)start_addr, len, PROT_READ|PROT_WRITE|PROT_EXEC) < 0)
#endif
      { fprintf(stderr,"trampoline: cannot make memory executable\n"); abort(); }
  }}
#endif

  /* 4. Flush instruction cache */
  /* We need this because some CPUs have separate data cache and instruction
   * cache. The freshly built trampoline is visible to the data cache, but not
   * maybe not to the instruction cache. This is hairy.
   */
#if !(defined(__hppanew__) || defined(__powerpcaix__) || defined(__powerpc64aix__) || defined(__powerpc64le__) || defined(__ia64__))
  /* Only needed if we really set up machine instructions. */
#ifdef __i386__
#if defined(_WIN32)
  while (!FlushInstructionCache(GetCurrentProcess(),function,TRAMP_LENGTH))
    continue;
#endif
#endif
#ifdef __m68k__
#if defined(NeXT) && defined(__GNUC__)
  __asm__ __volatile__ ("trap #2");
#endif
#if defined(AMIGA)
  CacheClearE(function,TRAMP_LENGTH,CACRF_ClearI|CACRF_ClearD);
#endif
#if defined(apollo)
  cache_$clear();
#endif
#if defined(hpux)
  cachectl(CC_IPURGE,function,TRAMP_LENGTH);
#endif
#if defined(__NetBSD__) && defined(__GNUC__)
  { register unsigned long _beg __asm__ ("%a1") = (unsigned long) function;
    register unsigned long _len __asm__ ("%d1") = TRAMP_LENGTH;
    __asm__ __volatile__ (
      "move%.l %#0x80000004,%/d0\n\t" /* CC_EXTPURGE | C_IPURGE */
      "trap #12"                      /* kernel call ‘cachectl’ */
      :
      : "a" (_beg), "d" (_len)
      : "%a0", "%a1", "%d0", "%d1"    /* call-used registers */
      );
  }
#endif
#if defined(__linux__) && defined(__GNUC__)
  { register unsigned long _beg __asm__ ("%d1") = (unsigned long) function;
    register unsigned long _len __asm__ ("%d4") = TRAMP_LENGTH + 32;
    __asm__ __volatile__ (
      "move%.l %#123,%/d0\n\t"
      "move%.l %#1,%/d2\n\t"
      "move%.l %#3,%/d3\n\t"
      "trap %#0"
      :
      : "d" (_beg), "d" (_len)
      : "%d0", "%d2", "%d3"
      );
  }
#endif
#if defined(AUX) && defined(__GNUC__)
  /* sysm68k(105, addr, scope, cache, len) */
  __asm__ __volatile__ (
    "move%.l %1,%/sp@-\n\t"
    "move%.l %#3,%/sp@-\n\t"
    "move%.l %#1,%/sp@-\n\t"
    "move%.l %0,%/sp@-\n\t"
    "move%.l %#105,%/sp@-\n\t"
    "move%.l %#0,%/sp@-\n\t"
    "move%.l %#38,%/sp@-\n\t"
    "trap %#0\n\t"
    "add%.l %#24,%/sp"
    :
    : "r" (function), "g" ((int)TRAMP_LENGTH)
    : "%d0"
    );
#endif
#endif
#if defined(__mips__) || defined(__mipsn32__) || defined(__mips64__)
  cacheflush(function,TRAMP_LENGTH,ICACHE);
  /* gforth-0.3.0 uses BCACHE instead of ICACHE. Why?? */
#endif
#if defined(__sparc__) || defined(__sparc64__)
  /* This assumes that the trampoline fits in at most two cache lines. */
  __TR_clear_cache_2(function,function+TRAMP_LENGTH-1);
#endif
#ifdef __alpha__
  __TR_clear_cache();
#endif
#ifdef __hppa__
  /* This assumes that the trampoline fits in at most two cache lines. */
  __TR_clear_cache(function,function+TRAMP_LENGTH-1);
#endif
#ifdef __arm__
  __TR_clear_cache(function,function+TRAMP_LENGTH);
#endif
#if defined(__powerpc__) && !defined(__powerpc64__)
  __TR_clear_cache(function);
#endif
#endif

  /* 5. Return. */
  return (__TR_function) (function + TRAMP_BIAS);
}

void free_trampoline_r (__TR_function function)
{
#if TRAMP_BIAS
  function = (__TR_function)((char*)function - TRAMP_BIAS);
#endif
#if !defined(CODE_EXECUTABLE) && !defined(EXECUTABLE_VIA_MPROTECT)
  *(char**)function = freelist; freelist = (char*)function;
  /* It is probably not worth calling munmap() for entirely freed pages. */
#else
  free(((char**)function)[-1]);
#endif
}

int is_trampoline_r (void* function)
{
#if defined(is_tramp) && defined(tramp_data)
#ifdef __hppanew__
  void* tramp_r_address = &tramp_r;
  if (!(((long)function & 3) == (TRAMP_BIAS & 3))) return 0;
#endif
  return
   ((is_tramp(((char*)function - TRAMP_BIAS)))
    && ((tramp_data(((char*)function - TRAMP_BIAS))) == (long)((char*)function - TRAMP_BIAS + TRAMP_LENGTH))
    ? 1 : 0
   );
#else
  abort();
#endif
}

__TR_function trampoline_r_address (void* function)
{
#ifdef tramp_address
  return (__TR_function)(tramp_address(((char*)function - TRAMP_BIAS)));
#else
  abort();
#endif
}

void* trampoline_r_data0 (void* function)
{
#ifdef tramp_data
  return ((void**)((char*)function-TRAMP_BIAS+TRAMP_LENGTH))[0];
#else
  abort();
#endif
}

void* trampoline_r_data1 (void* function)
{
#ifdef tramp_data
  return ((void**)((char*)function-TRAMP_BIAS+TRAMP_LENGTH))[1];
#else
  abort();
#endif
}
