	.file	"test.c"
	.text
	.globl	fibonacci
	.type	fibonacci, @function
fibonacci:
	pushq	%rbp
	movq	%rsp, %rbp
	movq	%rdi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movq	-16(%rbp), %rdx
	movq	-8(%rbp), %rax
	addq	%rdx, %rax
	popq	%rbp
	ret
	.size	fibonacci, .-fibonacci
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$32, %rsp
	movl	$1, -32(%rbp)
	movq	$0, -24(%rbp)
	movq	$1, -16(%rbp)
	movl	$1000, -28(%rbp)
	movq	$1, -8(%rbp)
.L4:
	movq	-24(%rbp), %rdx
	movq	-16(%rbp), %rax
	movq	%rdx, %rsi
	movq	%rax, %rdi
	call	fibonacci
	movq	%rax, -8(%rbp)
	movq	-16(%rbp), %rax
	movq	%rax, -24(%rbp)
	movq	-8(%rbp), %rax
	movq	%rax, -16(%rbp)
	addl	$1, -32(%rbp)
	jmp	.L4
	.size	main, .-main
	.ident	"GCC: (Ubuntu 6.5.0-2ubuntu1~16.04) 6.5.0 20181026"
	.section	.note.GNU-stack,"",@progbits
