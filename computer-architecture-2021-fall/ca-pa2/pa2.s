	.file	"pa2.c"
	.text
	.globl	int_fp10
	.type	int_fp10, @function
int_fp10:
.LFB0:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, -36(%rbp)
	movw	$0, -22(%rbp)
	cmpl	$0, -36(%rbp)
	jne	.L2
	movzwl	-22(%rbp), %eax
	jmp	.L3
.L2:
	cmpl	$0, -36(%rbp)
	jns	.L4
	movl	$127, %eax
	jmp	.L5
.L4:
	movl	$0, %eax
.L5:
	movl	%eax, -8(%rbp)
	movl	$0, -20(%rbp)
	movl	$0, -16(%rbp)
	cmpl	$0, -8(%rbp)
	je	.L6
	movl	-36(%rbp), %eax
	negl	%eax
	jmp	.L7
.L6:
	movl	-36(%rbp), %eax
.L7:
	movl	%eax, -12(%rbp)
	jmp	.L8
.L9:
	shrl	-12(%rbp)
	addl	$1, -20(%rbp)
.L8:
	cmpl	$1, -12(%rbp)
	ja	.L9
	cmpl	$0, -8(%rbp)
	je	.L10
	movl	-36(%rbp), %eax
	negl	%eax
	jmp	.L11
.L10:
	movl	-36(%rbp), %eax
.L11:
	movl	%eax, -12(%rbp)
	movl	-20(%rbp), %eax
	movl	$1, %edx
	movl	%eax, %ecx
	sall	%cl, %edx
	movl	%edx, %eax
	subl	$1, %eax
	movl	%eax, -4(%rbp)
	movl	-12(%rbp), %eax
	andl	-4(%rbp), %eax
	movl	%eax, %edx
	movl	$32, %eax
	subl	-20(%rbp), %eax
	movl	%eax, %ecx
	sall	%cl, %edx
	movl	%edx, %eax
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	andl	$134217728, %eax
	testl	%eax, %eax
	je	.L12
	movl	-16(%rbp), %eax
	andl	$268435456, %eax
	testl	%eax, %eax
	jne	.L13
	movl	-16(%rbp), %eax
	andl	$134217727, %eax
	testl	%eax, %eax
	je	.L12
.L13:
	movl	-16(%rbp), %eax
	shrl	$28, %eax
	addl	$1, %eax
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
	andl	$16, %eax
	testl	%eax, %eax
	je	.L15
	movl	$0, -16(%rbp)
	addl	$1, -20(%rbp)
	jmp	.L15
.L12:
	shrl	$28, -16(%rbp)
.L15:
	addl	$15, -20(%rbp)
	cmpl	$30, -20(%rbp)
	jbe	.L16
	movl	-8(%rbp), %eax
	sall	$9, %eax
	orw	$496, %ax
	jmp	.L3
.L16:
	movl	-16(%rbp), %eax
	orw	%ax, -22(%rbp)
	movl	-20(%rbp), %eax
	sall	$4, %eax
	orw	%ax, -22(%rbp)
	movl	-8(%rbp), %eax
	sall	$9, %eax
	orw	%ax, -22(%rbp)
	movzwl	-22(%rbp), %eax
.L3:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE0:
	.size	int_fp10, .-int_fp10
	.globl	fp10_int
	.type	fp10_int, @function
fp10_int:
.LFB1:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	movl	%edi, %eax
	movw	%ax, -20(%rbp)
	movl	$0, -16(%rbp)
	movzwl	-20(%rbp), %eax
	shrw	$9, %ax
	movzwl	%ax, %eax
	andl	$1, %eax
	movl	%eax, -12(%rbp)
	movzwl	-20(%rbp), %eax
	shrw	$4, %ax
	movzwl	%ax, %eax
	andl	$31, %eax
	movl	%eax, -8(%rbp)
	movzwl	-20(%rbp), %eax
	andl	$15, %eax
	movl	%eax, -4(%rbp)
	cmpl	$14, -8(%rbp)
	ja	.L18
	movl	$0, %eax
	jmp	.L19
.L18:
	cmpl	$31, -8(%rbp)
	jne	.L20
	movl	$-2147483648, %eax
	jmp	.L19
.L20:
	subl	$15, -8(%rbp)
	movl	-4(%rbp), %eax
	orl	$16, %eax
	movl	%eax, -16(%rbp)
	cmpl	$4, -8(%rbp)
	jbe	.L21
	movl	-8(%rbp), %eax
	subl	$4, %eax
	movl	%eax, %ecx
	sall	%cl, -16(%rbp)
	jmp	.L22
.L21:
	movl	$4, %eax
	subl	-8(%rbp), %eax
	movl	%eax, %ecx
	sarl	%cl, -16(%rbp)
.L22:
	cmpl	$0, -12(%rbp)
	je	.L23
	movl	-16(%rbp), %eax
	negl	%eax
	jmp	.L24
.L23:
	movl	-16(%rbp), %eax
.L24:
	movl	%eax, -16(%rbp)
	movl	-16(%rbp), %eax
.L19:
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE1:
	.size	fp10_int, .-fp10_int
	.globl	float_fp10
	.type	float_fp10, @function
float_fp10:
.LFB2:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r13
	pushq	%r12
	pushq	%rbx
	.cfi_offset 13, -24
	.cfi_offset 12, -32
	.cfi_offset 3, -40
	movss	%xmm0, -28(%rbp)
	leaq	-28(%rbp), %rax
	movl	(%rax), %ebx
	movl	%ebx, %eax
	testl	%eax, %eax
	jns	.L26
	movl	$127, %eax
	jmp	.L27
.L26:
	movl	$0, %eax
.L27:
	movl	%eax, %r13d
	movl	%ebx, %eax
	shrl	$23, %eax
	movzbl	%al, %r12d
	andl	$8388607, %ebx
	sall	$9, %r13d
	sall	$9, %ebx
	cmpl	$255, %r12d
	jne	.L28
	testl	%ebx, %ebx
	je	.L28
	movl	%r13d, %eax
	orw	$511, %ax
	jmp	.L29
.L28:
	cmpl	$112, %r12d
	ja	.L30
	movl	$113, %eax
	subl	%r12d, %eax
	movl	%eax, %ecx
	shrl	%cl, %ebx
	leal	-81(%r12), %eax
	movl	$1, %edx
	movl	%eax, %ecx
	sall	%cl, %edx
	movl	%edx, %eax
	orl	%eax, %ebx
	movl	$112, %r12d
.L30:
	movl	%ebx, %eax
	andl	$134217728, %eax
	testl	%eax, %eax
	je	.L31
	movl	%ebx, %eax
	andl	$402653183, %eax
	testl	%eax, %eax
	je	.L31
	shrl	$28, %ebx
	addl	$1, %ebx
	movl	%ebx, %eax
	shrl	$4, %eax
	addl	%eax, %r12d
	andl	$15, %ebx
	jmp	.L32
.L31:
	shrl	$28, %ebx
.L32:
	cmpl	$142, %r12d
	jbe	.L33
	movl	%r13d, %eax
	orw	$496, %ax
	jmp	.L29
.L33:
	subl	$112, %r12d
	sall	$4, %r12d
	movl	%r12d, %edx
	movl	%ebx, %eax
	orl	%eax, %edx
	movl	%edx, %ebx
	movl	%r13d, %eax
	orl	%eax, %ebx
	movl	%ebx, %eax
.L29:
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%rbp
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE2:
	.size	float_fp10, .-float_fp10
	.globl	fp10_float
	.type	fp10_float, @function
fp10_float:
.LFB3:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$48, %rsp
	movl	%edi, %eax
	movw	%ax, -36(%rbp)
	movq	%fs:40, %rax
	movq	%rax, -8(%rbp)
	xorl	%eax, %eax
	movl	$0, -28(%rbp)
	movzwl	-36(%rbp), %eax
	shrw	$9, %ax
	movzwl	%ax, %eax
	andl	$1, %eax
	movl	%eax, -16(%rbp)
	movzwl	-36(%rbp), %eax
	shrw	$4, %ax
	movzwl	%ax, %eax
	andl	$31, %eax
	movl	%eax, -12(%rbp)
	movzwl	-36(%rbp), %eax
	andl	$15, %eax
	movl	%eax, -24(%rbp)
	cmpl	$31, -12(%rbp)
	jne	.L35
	cmpl	$0, -24(%rbp)
	je	.L36
	movl	-16(%rbp), %eax
	sall	$31, %eax
	orl	$2147483647, %eax
	movl	%eax, -28(%rbp)
	leaq	-28(%rbp), %rax
	movss	(%rax), %xmm0
	jmp	.L42
.L36:
	movl	-16(%rbp), %eax
	sall	$31, %eax
	orl	$2139095040, %eax
	movl	%eax, -28(%rbp)
	leaq	-28(%rbp), %rax
	movss	(%rax), %xmm0
	jmp	.L42
.L35:
	movl	$0, -20(%rbp)
	cmpl	$0, -12(%rbp)
	jne	.L38
	cmpl	$0, -24(%rbp)
	je	.L39
	sall	$28, -24(%rbp)
	jmp	.L40
.L41:
	addl	$1, -20(%rbp)
	sall	-24(%rbp)
.L40:
	movl	-24(%rbp), %eax
	testl	%eax, %eax
	jns	.L41
	sall	-24(%rbp)
	shrl	$28, -24(%rbp)
	jmp	.L38
.L39:
	movl	-16(%rbp), %eax
	sall	$31, %eax
	movl	%eax, -28(%rbp)
	leaq	-28(%rbp), %rax
	movss	(%rax), %xmm0
	jmp	.L42
.L38:
	movl	-12(%rbp), %eax
	subl	-20(%rbp), %eax
	addl	$112, %eax
	movl	%eax, -12(%rbp)
	movl	-24(%rbp), %eax
	sall	$19, %eax
	movl	%eax, %edx
	movl	-28(%rbp), %eax
	orl	%edx, %eax
	movl	%eax, -28(%rbp)
	movl	-12(%rbp), %eax
	sall	$23, %eax
	movl	%eax, %edx
	movl	-28(%rbp), %eax
	orl	%edx, %eax
	movl	%eax, -28(%rbp)
	movl	-16(%rbp), %eax
	sall	$31, %eax
	movl	%eax, %edx
	movl	-28(%rbp), %eax
	orl	%edx, %eax
	movl	%eax, -28(%rbp)
	leaq	-28(%rbp), %rax
	movss	(%rax), %xmm0
.L42:
	movq	-8(%rbp), %rax
	xorq	%fs:40, %rax
	je	.L43
	call	__stack_chk_fail@PLT
.L43:
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE3:
	.size	fp10_float, .-fp10_float
	.ident	"GCC: (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	 1f - 0f
	.long	 4f - 1f
	.long	 5
0:
	.string	 "GNU"
1:
	.align 8
	.long	 0xc0000002
	.long	 3f - 2f
2:
	.long	 0x3
3:
	.align 8
4:
