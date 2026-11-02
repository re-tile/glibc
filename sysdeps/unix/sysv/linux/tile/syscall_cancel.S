/* Cancellable syscall wrapper.  Linux/tile version.
   Copyright (C) 2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <sysdep.h>
#include <arch/abi.h>
#include <descr-const.h>

/* long int [r0] __syscall_cancel_arch (int *cancelhandling [r0],
					__syscall_arg_t nr   [r1],
					__syscall_arg_t arg1 [r2],
					__syscall_arg_t arg2 [r3],
					__syscall_arg_t arg3 [r4],
					__syscall_arg_t arg4 [r5],
					__syscall_arg_t arg5 [r6],
					__syscall_arg_t arg6 [r7])  */

	.text
ENTRY (__syscall_cancel_arch)

	.globl __syscall_cancel_arch_start
__syscall_cancel_arch_start:

	/* if (*cancelhandling & CANCELED_BITMASK)
	     __syscall_do_cancel ()  */
	ld4s	r10, r0
	andi	r10, r10, TCB_CANCELED_BITMASK
	bnez	r10, 0f

	/* Issue a 6 argument syscall, the nr [r1] being the syscall
	   number.  */
	{ move TREG_SYSCALL_NR_NAME, r1; move r0, r2 }
	{ move r1, r3; move r2, r4 }
	{ move r3, r5; move r4, r6 }
	move	r5, r7
	swint1

	.globl __syscall_cancel_arch_end
__syscall_cancel_arch_end:

	/* The kernel returns the raw negative errno in r0; r1 is only a
	   derived positive-errno convenience flag (computed from r0 in the
	   syscall iret path), so r0 is already in the convention the caller
	   expects and no error folding is needed here.  */
	jrp	lr

0:
	j	__syscall_do_cancel

END (__syscall_cancel_arch)
