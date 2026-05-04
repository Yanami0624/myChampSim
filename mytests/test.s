	.file	"test.cpp"
	.text
#APP
	.globl _ZSt21ios_base_library_initv
#NO_APP
	.section	.text._ZNKSt5ctypeIcE8do_widenEc,"axG",@progbits,_ZNKSt5ctypeIcE8do_widenEc,comdat
	.align 2
	.p2align 4
	.weak	_ZNKSt5ctypeIcE8do_widenEc
	.type	_ZNKSt5ctypeIcE8do_widenEc, @function
_ZNKSt5ctypeIcE8do_widenEc:
.LFB1810:
	.cfi_startproc
	endbr64
	movl	%esi, %eax
	ret
	.cfi_endproc
.LFE1810:
	.size	_ZNKSt5ctypeIcE8do_widenEc, .-_ZNKSt5ctypeIcE8do_widenEc
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB2420:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movl	$400, %edi
	pushq	%rbx
	.cfi_def_cfa_offset 24
	.cfi_offset 3, -24
	subq	$8, %rsp
	.cfi_def_cfa_offset 32
	call	malloc@PLT
	movl	$262144, %ecx
	movq	$262144, 392(%rax)
	leaq	384(%rax), %rdx
	.p2align 4,,10
	.p2align 3
.L4:
	subq	$1, %rcx
	movq	%rdx, %rsi
	subq	$8, %rdx
	movq	%rcx, 8(%rdx)
	cmpq	%rax, %rsi
	jne	.L4
	movq	(%rax), %rsi
	leaq	_ZSt4cout(%rip), %rdi
	call	_ZNSo9_M_insertIxEERSoT_@PLT
	movq	%rax, %rbx
	movq	(%rax), %rax
	movq	-24(%rax), %rax
	movq	240(%rbx,%rax), %rbp
	testq	%rbp, %rbp
	je	.L11
	cmpb	$0, 56(%rbp)
	je	.L6
	movzbl	67(%rbp), %eax
.L7:
	movq	%rbx, %rdi
	movsbl	%al, %esi
	call	_ZNSo3putEc@PLT
	movq	%rax, %rdi
	call	_ZNSo5flushEv@PLT
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 24
	xorl	%eax, %eax
	popq	%rbx
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	ret
.L6:
	.cfi_restore_state
	movq	%rbp, %rdi
	call	_ZNKSt5ctypeIcE13_M_widen_initEv@PLT
	movq	0(%rbp), %rax
	leaq	_ZNKSt5ctypeIcE8do_widenEc(%rip), %rcx
	movq	48(%rax), %rdx
	movl	$10, %eax
	cmpq	%rcx, %rdx
	je	.L7
	movl	$10, %esi
	movq	%rbp, %rdi
	call	*%rdx
	jmp	.L7
.L11:
	call	_ZSt16__throw_bad_castv@PLT
	.cfi_endproc
.LFE2420:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
