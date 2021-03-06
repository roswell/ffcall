/* Trampoline for x86_64 CPU */

/*
 * Copyright 2004-2017 Bruno Haible <bruno@clisp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Available registers: %rax, %r10, %r11. */

.globl tramp
	.type	tramp,@function
tramp:
	movabsq	$0x7355471143622155, %rax
	movabsq	%rax, 0x1234567813578765
	movabsq	$0xBABEBEC0DEA0FFAB, %rax
	jmp	*%rax
.Lfe1:
	.size	tramp,.Lfe1-tramp
