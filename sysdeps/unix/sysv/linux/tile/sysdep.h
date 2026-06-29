/* Copyright (C) 2011-2018 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Chris Metcalf <cmetcalf@tilera.com>, 2011.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include <asm/unistd.h>
#include <sysdeps/tile/sysdep.h>
#include <sysdeps/unix/sysdep.h>
#include <sysdeps/unix/sysv/linux/sysdep.h>
#include <sys/syscall.h>

#undef SYS_ify
#define SYS_ify(syscall_name)	__NR_##syscall_name

/* The TILE-Gx kernel exports its vDSO functions under the LINUX_2.6
   version (the chip predates the per-arch version bumps newer ports use).
   The generic dl-vdso-setup machinery binds these from these macros.  */
#if defined SHARED && !defined __ASSEMBLER__
# define VDSO_NAME	"LINUX_2.6"
# define VDSO_HASH	61765110

# define HAVE_GETTIMEOFDAY_VSYSCALL	"__vdso_gettimeofday"
# define HAVE_CLOCK_GETTIME_VSYSCALL	"__vdso_clock_gettime"
#endif


#ifdef __ASSEMBLER__

/* The actual implementation of doing a syscall. */
#define DO_CALL(syscall_name, args)                     \
  moveli TREG_SYSCALL_NR_NAME, SYS_ify(syscall_name);	\
  swint1

/* TILE Linux returns the result in r0 (or a negative errno).
   The kernel "owns" the code to decide if a given value is an error,
   and puts errno in r1 if so, or otherwise zero.  */
#define	PSEUDO(name, syscall_name, args)		\
  ENTRY	(name);						\
  DO_CALL(syscall_name, args);				\
  BNEZ r1, 0f

#define ret  jrp lr

#ifndef SHARED
/* For static code, on error jump to __syscall_error directly. */
# define SYSCALL_ERROR_NAME __syscall_error
#elif IS_IN (libc) || IS_IN (libpthread)
/* Use the internal name for libc/libpthread shared objects. */
# define SYSCALL_ERROR_NAME __GI___syscall_error
#else
/* Otherwise, on error do a full PLT jump. */
# define SYSCALL_ERROR_NAME plt(__syscall_error)
#endif

#undef PSEUDO_END
#define	PSEUDO_END(name)				\
0:							\
  j SYSCALL_ERROR_NAME;					\
  END (name)

#undef PSEUDO_NOERRNO
#define	PSEUDO_NOERRNO(name, syscall_name, args)	\
  ENTRY	(name);						\
  DO_CALL(syscall_name, args)

#define ret_NOERRNO  jrp lr

#undef PSEUDO_END_NOERRNO
#define	PSEUDO_END_NOERRNO(name) \
  END (name)

/* Convenience wrappers. */
#define SYSCALL__(name, args)   PSEUDO (__##name, name, args)
#define SYSCALL(name, args)   PSEUDO (name, name, args)

#else /* not __ASSEMBLER__ */

#include <errno.h>

/* The Y2038 time64 syscall wrappers (sigtimedwait, ppoll, ...) use
   struct __timespec64 in their signatures.  For a 64-bit-time arch it is
   just an alias for struct timespec, defined here; pulling it in via the
   syscall sysdep guarantees the alias is visible before any wrapper body,
   which the tile include chain does not otherwise reach.  */
#include <struct___timespec64.h>

/* Likewise pull in the C11 atomic_load_relaxed / atomic_store_relaxed
   helpers; several syscall-adjacent files (arc4random, ...) use them
   without an explicit <atomic.h>, relying on a transitive include the
   tile chain does not otherwise provide.  */
#if !IS_IN (rtld)
# include <atomic.h>
#endif

/* The TILE-Gx Linux kernel signals a failed system call by setting r1
   to the (positive) errno, leaving the raw result in r0.  Modern glibc
   wants INTERNAL_SYSCALL to return the negative errno directly, as the
   generic mechanisms here (INTERNAL_SYSCALL_ERROR_P, INTERNAL_SYSCALL_ERRNO,
   INLINE_SYSCALL in sysdeps/unix/sysv/linux/sysdep.h) test for that.  We
   therefore fold the r1 flag into the return value, exactly as the mips
   port does with its $a3 error register.  */

#undef INTERNAL_SYSCALL
#define INTERNAL_SYSCALL(name, nr, args...)             \
  internal_syscall##nr (SYS_ify (name), args)

#undef INTERNAL_SYSCALL_NCS
#define INTERNAL_SYSCALL_NCS(number, nr, args...)       \
  internal_syscall##nr (number, args)

#define internal_syscall0(num, dummy...)                                \
  ({                                                                    \
    long _sys_result, _sys_err, __SYSCALL_CLOBBER_DECLS;                \
    __asm__ __volatile__ (                                              \
      "swint1"                                                          \
      : "=R00" (_sys_result), "=R01" (_sys_err), __SYSCALL_CLOBBER_OUTPUTS \
      : "R10" (num)                                                     \
      : __SYSCALL_CLOBBERS);                                            \
    _sys_err != 0 ? -_sys_err : _sys_result;                            \
  })

#define internal_syscall1(num, arg0)                                    \
  ({                                                                    \
    long _sys_result, _sys_err, __SYSCALL_CLOBBER_DECLS;                \
    __asm__ __volatile__ (                                              \
      "swint1"                                                          \
      : "=R00" (_sys_result), "=R01" (_sys_err), __SYSCALL_CLOBBER_OUTPUTS \
      : "R10" (num), "R00" (arg0)                                       \
      : __SYSCALL_CLOBBERS);                                            \
    _sys_err != 0 ? -_sys_err : _sys_result;                            \
  })

#define internal_syscall2(num, arg0, arg1)                              \
  ({                                                                    \
    long _sys_result, _sys_err, __SYSCALL_CLOBBER_DECLS;                \
    __asm__ __volatile__ (                                              \
      "swint1"                                                          \
      : "=R00" (_sys_result), "=R01" (_sys_err), __SYSCALL_CLOBBER_OUTPUTS \
      : "R10" (num), "R00" (arg0), "R01" (arg1)                         \
      : __SYSCALL_CLOBBERS);                                            \
    _sys_err != 0 ? -_sys_err : _sys_result;                            \
  })

#define internal_syscall3(num, arg0, arg1, arg2)                        \
  ({                                                                    \
    long _sys_result, _sys_err, __SYSCALL_CLOBBER_DECLS;                \
    __asm__ __volatile__ (                                              \
      "swint1"                                                          \
      : "=R00" (_sys_result), "=R01" (_sys_err), __SYSCALL_CLOBBER_OUTPUTS \
      : "R10" (num), "R00" (arg0), "R01" (arg1), "R02" (arg2)           \
      : __SYSCALL_CLOBBERS);                                            \
    _sys_err != 0 ? -_sys_err : _sys_result;                            \
  })

#define internal_syscall4(num, arg0, arg1, arg2, arg3)                  \
  ({                                                                    \
    long _sys_result, _sys_err, __SYSCALL_CLOBBER_DECLS;                \
    __asm__ __volatile__ (                                              \
      "swint1"                                                          \
      : "=R00" (_sys_result), "=R01" (_sys_err), __SYSCALL_CLOBBER_OUTPUTS \
      : "R10" (num), "R00" (arg0), "R01" (arg1), "R02" (arg2),          \
        "R03" (arg3)                                                    \
      : __SYSCALL_CLOBBERS);                                            \
    _sys_err != 0 ? -_sys_err : _sys_result;                            \
  })

#define internal_syscall5(num, arg0, arg1, arg2, arg3, arg4)            \
  ({                                                                    \
    long _sys_result, _sys_err, __SYSCALL_CLOBBER_DECLS;                \
    __asm__ __volatile__ (                                              \
      "swint1"                                                          \
      : "=R00" (_sys_result), "=R01" (_sys_err), __SYSCALL_CLOBBER_OUTPUTS \
      : "R10" (num), "R00" (arg0), "R01" (arg1), "R02" (arg2),          \
        "R03" (arg3), "R04" (arg4)                                      \
      : __SYSCALL_CLOBBERS);                                            \
    _sys_err != 0 ? -_sys_err : _sys_result;                            \
  })

#define internal_syscall6(num, arg0, arg1, arg2, arg3, arg4, arg5)      \
  ({                                                                    \
    long _sys_result, _sys_err, __SYSCALL_CLOBBER_DECLS;                \
    __asm__ __volatile__ (                                              \
      "swint1"                                                          \
      : "=R00" (_sys_result), "=R01" (_sys_err), __SYSCALL_CLOBBER_OUTPUTS \
      : "R10" (num), "R00" (arg0), "R01" (arg1), "R02" (arg2),          \
        "R03" (arg3), "R04" (arg4), "R05" (arg5)                        \
      : __SYSCALL_CLOBBERS);                                            \
    _sys_err != 0 ? -_sys_err : _sys_result;                            \
  })

#undef __SYSCALL_CLOBBERS
#define __SYSCALL_CLOBBERS                                      \
  "r6",  "r7",                                                  \
    "r8",  "r9",        "r11", "r12", "r13", "r14", "r15",      \
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",     \
    "r24", "r25", "r26", "r27", "r28", "r29", "memory"

/* gcc doesn't seem to allow an input operand to be clobbered, so we
   fake it with dummy outputs. */
#define __SYSCALL_CLOBBER_DECLS                                         \
  _clobber_r2, _clobber_r3, _clobber_r4, _clobber_r5, _clobber_r10

#define __SYSCALL_CLOBBER_OUTPUTS                                       \
  "=R02" (_clobber_r2), "=R03" (_clobber_r3), "=R04" (_clobber_r4),     \
    "=R05" (_clobber_r5), "=R10" (_clobber_r10)

#endif /* __ASSEMBLER__  */
