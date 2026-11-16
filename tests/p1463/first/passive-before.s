	.file	"A.c1.c"
	.text
	.section	.rodata.str1.8,"aMS",@progbits,1
	.align 8
.LC0:
	.string	"calls<2 && (target)==reading && *sibling==13"
	.align 8
.LC1:
	.string	"/workspace/p1463-c1025/outputs/A.c1.c"
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC2:
	.string	"%s:%d: %s\n"
	.section	.rodata.str1.8
	.align 8
.LC3:
	.string	"!trials && (old)==87 && !(next)"
	.align 8
.LC4:
	.string	"delivery_copies==1 && (old)==0 && (next)==observed_delivery && frees==expected_cleanup() && !pending_free"
	.text
	.p2align 4
	.type	nl_copy_state_10, @function
nl_copy_state_10:
.LFB13:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movq	calls(%rip), %rax
	movzbl	(%rdi), %edx
	movb	%sil, (%rdi)
	cmpq	$1, %rax
	ja	.L2
	cmpq	%rdi, reading(%rip)
	jne	.L2
	movq	sibling(%rip), %rcx
	cmpb	$13, (%rcx)
	jne	.L2
	testq	%rax, %rax
	je	.L15
	cmpq	$1, delivery_copies(%rip)
	jne	.L8
	testb	%dl, %dl
	jne	.L8
	movzbl	%sil, %esi
	cmpl	observed_delivery(%rip), %esi
	jne	.L8
	cmpq	$2, frees(%rip)
	jne	.L8
	cmpq	$0, pending_free(%rip)
	jne	.L8
.L6:
	addq	$1, %rax
	movq	%rax, calls(%rip)
	movl	%edx, %eax
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
.L15:
	.cfi_restore_state
	cmpq	$0, trials(%rip)
	jne	.L5
	cmpb	$87, %dl
	jne	.L5
	testb	%sil, %sil
	je	.L6
.L5:
	leaq	.LC3(%rip), %r8
.L13:
	movq	stderr(%rip), %rdi
	movl	$90, %ecx
	leaq	.LC1(%rip), %rdx
	xorl	%eax, %eax
	leaq	.LC2(%rip), %rsi
	call	fprintf@PLT
	call	abort@PLT
.L2:
	leaq	.LC0(%rip), %r8
	jmp	.L13
.L8:
	leaq	.LC4(%rip), %r8
	jmp	.L13
	.cfi_endproc
.LFE13:
	.size	nl_copy_state_10, .-nl_copy_state_10
	.section	.rodata.str1.8
	.align 8
.LC5:
	.string	"p && (p == originals[0] || p == originals[1])"
	.align 8
.LC6:
	.string	"/workspace/p1463-c1025/apparatus/passive.h"
	.text
	.p2align 4
	.type	identity.part.0, @function
identity.part.0:
.LFB17:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	movq	stderr(%rip), %rdi
	xorl	%eax, %eax
	leaq	.LC5(%rip), %r8
	movl	$28, %ecx
	leaq	.LC6(%rip), %rdx
	leaq	.LC2(%rip), %rsi
	call	fprintf@PLT
	call	abort@PLT
	.cfi_endproc
.LFE17:
	.size	identity.part.0, .-identity.part.0
	.p2align 4
	.type	identity, @function
identity:
.LFB10:
	.cfi_startproc
	testq	%rdi, %rdi
	je	.L19
	movq	originals(%rip), %rax
	cmpq	%rdi, %rax
	je	.L20
	cmpq	%rdi, 8+originals(%rip)
	jne	.L19
.L20:
	cmpq	%rdi, %rax
	setne	%al
	movzbl	%al, %eax
	ret
.L19:
	pushq	%rax
	.cfi_def_cfa_offset 16
	call	identity.part.0
	.cfi_endproc
.LFE10:
	.size	identity, .-identity
	.section	.rodata.str1.1
.LC7:
	.string	"p && p == pending_free"
.LC8:
	.string	"ended[i] && finalized[i]"
	.text
	.p2align 4
	.globl	p1463_observe_free
	.type	p1463_observe_free, @function
p1463_observe_free:
.LFB12:
	.cfi_startproc
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	testq	%rdi, %rdi
	je	.L27
	cmpq	pending_free(%rip), %rdi
	jne	.L27
	movq	originals(%rip), %rax
	cmpq	%rax, %rdi
	je	.L29
	cmpq	8+originals(%rip), %rdi
	jne	.L37
.L29:
	cmpq	%rax, %rdi
	leaq	ended(%rip), %rdx
	setne	%al
	movzbl	%al, %eax
	movl	(%rdx,%rax,4), %ecx
	testl	%ecx, %ecx
	je	.L30
	leaq	finalized(%rip), %rdx
	movl	(%rdx,%rax,4), %edx
	testl	%edx, %edx
	je	.L30
	leaq	originals(%rip), %rdx
	addq	$1, frees(%rip)
	movq	$0, (%rdx,%rax,8)
	leaq	domains(%rip), %rdx
	movq	$0, (%rdx,%rax,8)
	movq	$0, pending_free(%rip)
	addq	$8, %rsp
	.cfi_remember_state
	.cfi_def_cfa_offset 8
	ret
.L27:
	.cfi_restore_state
	leaq	.LC7(%rip), %r8
	movl	$33, %ecx
.L36:
	movq	stderr(%rip), %rdi
	leaq	.LC6(%rip), %rdx
	leaq	.LC2(%rip), %rsi
	xorl	%eax, %eax
	call	fprintf@PLT
	call	abort@PLT
.L30:
	leaq	.LC8(%rip), %r8
	movl	$35, %ecx
	jmp	.L36
.L37:
	call	identity.part.0
	.cfi_endproc
.LFE12:
	.size	p1463_observe_free, .-p1463_observe_free
	.section	.rodata.str1.1
.LC10:
	.string	"!reading"
	.section	.rodata.str1.8
	.align 8
.LC11:
	.string	"calls==1 && startup_copies++==0 && v==87"
	.align 8
.LC12:
	.string	"frees==expected_cleanup() && !pending_free && delivery_copies++==0 && v==expected_delivery()"
	.section	.rodata.str1.1
.LC13:
	.string	"bindings<4"
	.section	.rodata.str1.8
	.align 8
.LC14:
	.string	"v==(bindings==2?observed_delivery:fixed[bindings])"
	.section	.rodata.str1.1
.LC15:
	.string	"0"
	.section	.rodata.str1.8
	.align 8
.LC16:
	.string	"calls==1 && trials<=2 && (24)==24 && (8)==8"
	.align 8
.LC17:
	.string	"((nl_heap_7)==NULL)==(P1463_FAIL_AT && trials==P1463_FAIL_AT)"
	.align 8
.LC18:
	.string	"inits<2 && (nl_v_20) && (nl_v_19->token) && (nl_v_20)->f0.tag==0 && !(nl_v_20)->f0.ptr"
	.section	.rodata.str1.1
.LC19:
	.string	"(nl_v_20)->f1==(inits?181:76)"
	.section	.rodata.str1.8
	.align 8
.LC20:
	.string	"((nl_heap_22)==NULL)==(P1463_FAIL_AT && trials==P1463_FAIL_AT)"
	.align 8
.LC21:
	.string	"(nl_v_43)!=originals[0] && (nl_v_42->token)!=domains[0]"
	.align 8
.LC22:
	.string	"(nl_v_46->token)==domains[i] && !ended[i] && (13) && (16) && (4)"
	.section	.rodata.str1.1
.LC23:
	.string	"(1)?i==0:i==1"
	.section	.rodata.str1.8
	.align 8
.LC24:
	.string	"(11) && (17) && (17) && (4) && (0)<2 && !ended[i]"
	.align 8
.LC25:
	.string	"(const void *)(nl_v_49)==(const void *)&(nl_v_48)->f0 && ((1)?i==0:i==1)"
	.align 8
.LC26:
	.string	"originals[0] && originals[1] && (nl_v_49)==&originals[0]->f0 && (1)==++changes"
	.align 8
.LC27:
	.string	"(nl_v_55->token)==domains[i] && !ended[i] && (24) && (27) && (5)"
	.section	.rodata.str1.1
.LC28:
	.string	"(0)?i==0:i==1"
	.section	.rodata.str1.8
	.align 8
.LC29:
	.string	"(11) && (29) && (29) && (5) && (1)<2 && !ended[i]"
	.align 8
.LC30:
	.string	"i==1 && !(0) && (const void *)(nl_v_58)==(const void *)&(nl_v_57)->f1 && (nl_v_57)->f1==181 && payloads++==0"
	.align 8
.LC31:
	.string	"(nl_v_62->token)==domains[i] && !ended[i] && (24) && (27) && (6)"
	.align 8
.LC32:
	.string	"(11) && (28) && (28) && (6) && (0)<2 && !ended[i]"
	.align 8
.LC33:
	.string	"(const void *)(nl_v_65)==(const void *)&(nl_v_64)->f0 && ((0)?i==0:i==1)"
	.align 8
.LC34:
	.string	"originals[1] && (nl_v_65)==&originals[1]->f0 && reads++==0"
	.align 8
.LC35:
	.string	"!(0) && reads==1 && !bools++ && (nl_v_68)==P1463_BOOL_TRUE"
	.align 8
.LC36:
	.string	"!conditions++ && bools==1 && (nl_v_78)==decision && !frees && originals[0] && originals[1]"
	.align 8
.LC37:
	.string	"!results++ && conditions==1 && (1)==(decision?0u:1u) && (nl_v_79)==(decision?0u:181u)"
	.align 8
.LC38:
	.string	"(nl_v_84->token)==domains[i] && !ended[i] && (13) && (16) && (7)"
	.align 8
.LC39:
	.string	"(11) && (17) && (17) && (7) && (0)<2 && !ended[i]"
	.align 8
.LC40:
	.string	"(const void *)(nl_v_87)==(const void *)&(nl_v_86)->f0 && ((1)?i==0:i==1)"
	.align 8
.LC41:
	.string	"(nl_v_88)==originals[1] && (nl_v_89.handle)==(void *)originals[1] && (nl_v_90.token)==domains[1] && !sends++ && !frees"
	.align 8
.LC42:
	.string	"(nl_b_17_38)==&originals[0]->f0 && (nl_b_17_39)==originals[1] && (nl_b_17_40.handle)==(void *)originals[1] && (nl_b_17_41.token)==domains[1] && !entries++"
	.align 8
.LC43:
	.string	"(nl_v_98).ptr==originals[1] && (nl_v_98).allocation.handle==(void *)originals[1] && (nl_v_98).domain.token==domains[1] && changes==2 && !returns_++"
	.align 8
.LC44:
	.string	"(nl_v_99).ptr==originals[1] && (nl_v_99).allocation.handle==(void *)originals[1] && (nl_v_99).domain.token==domains[1] && !(&nl_b_6_18)->handle && !(&nl_b_6_20)->token && !receives++"
	.align 8
.LC45:
	.string	"!ended[i]++ && (nl_b_18_48->token)==domains[i] && !(nl_v_102)->f0.tag && !(nl_v_102)->f0.ptr"
	.align 8
.LC46:
	.string	"(nl_v_106.token) && (nl_v_106.token)==domains[i] && ended[i] && !finalized[i]++"
	.align 8
.LC47:
	.string	"!pending_free && (nl_v_107.handle)==(void *)(nl_v_108.bytes) && (nl_v_108.length)==24 && ended[i] && finalized[i]"
	.align 8
.LC48:
	.string	"(nl_v_109)==originals[1] && (nl_v_110.handle)==(void *)originals[1] && (nl_v_111.token)==domains[1] && frees==1 && !(&nl_b_6_46)->handle && !(&nl_b_6_47)->token"
	.section	.text.startup,"ax",@progbits
	.p2align 4
	.globl	main
	.type	main, @function
main:
.LFB16:
	.cfi_startproc
	pushq	%r15
	.cfi_def_cfa_offset 16
	.cfi_offset 15, -16
	movl	$3415, %esi
	pushq	%r14
	.cfi_def_cfa_offset 24
	.cfi_offset 14, -24
	pushq	%r13
	.cfi_def_cfa_offset 32
	.cfi_offset 13, -32
	pushq	%r12
	.cfi_def_cfa_offset 40
	.cfi_offset 12, -40
	pushq	%rbp
	.cfi_def_cfa_offset 48
	.cfi_offset 6, -48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	.cfi_offset 3, -56
	subq	$56, %rsp
	.cfi_def_cfa_offset 112
	cmpq	$0, reading(%rip)
	movw	%si, 46(%rsp)
	jne	.L205
	leaq	46(%rsp), %rdi
	leaq	47(%rsp), %rax
	xorl	%esi, %esi
	movq	$0, t_frame(%rip)
	movq	%rdi, reading(%rip)
	movq	%rax, sibling(%rip)
	movq	$1, t_symbol(%rip)
	call	nl_copy_state_10
	movq	stderr(%rip), %rsi
	movq	reading(%rip), %rbp
	movq	trials(%rip), %r12
	movq	calls(%rip), %rbx
	movq	%rsi, 24(%rsp)
	testq	%rbp, %rbp
	je	.L40
	cmpq	$0, t_frame(%rip)
	je	.L206
.L40:
	movl	$24, %edi
	call	malloc@PLT
	leaq	1(%r12), %rcx
	movq	%rcx, trials(%rip)
	movq	%rax, %r14
	cmpq	$2, %rcx
	ja	.L52
	cmpq	$1, %rbx
	jne	.L52
	testq	%rax, %rax
	je	.L152
	movzbl	%al, %esi
	testq	%rbp, %rbp
	je	.L60
	cmpq	$2, t_frame(%rip)
	je	.L207
.L60:
	movq	%r14, %rax
.L57:
	movb	%sil, %al
	movq	inits(%rip), %rcx
	movq	%rax, %rbx
	movl	$0, (%rbx)
	movq	$0, 8(%rbx)
	movb	$76, 16(%rbx)
	cmpq	$1, %rcx
	ja	.L208
	testq	%rcx, %rcx
	jne	.L209
	movl	$24, %edi
	addq	$2, %r12
	movq	%rax, originals(%rip)
	movq	$1, inits(%rip)
	movq	$1, domains(%rip)
	call	malloc@PLT
	movq	%r12, trials(%rip)
	movq	%rax, (%rsp)
	cmpq	$3, %r12
	je	.L210
	cmpq	$0, (%rsp)
	je	.L211
	movq	(%rsp), %rax
	movl	%eax, %ecx
	testq	%rbp, %rbp
	je	.L76
	cmpq	$6, t_frame(%rip)
	je	.L212
.L82:
	movb	%cl, %al
	movq	%rax, %r13
	movl	$0, 0(%r13)
	movq	$0, 8(%r13)
	movb	$-75, 16(%r13)
	cmpq	%rbx, %r13
	je	.L86
	movq	%rbx, %rdi
	movq	%rax, 8+originals(%rip)
	leaq	domains(%rip), %r15
	movq	$2, inits(%rip)
	movq	$2, 8+domains(%rip)
	call	identity
	cmpq	$1, (%r15,%rax,8)
	jne	.L87
	leaq	ended(%rip), %r12
	movl	(%r12,%rax,4), %ecx
	testl	%ecx, %ecx
	jne	.L87
	testq	%rax, %rax
	jne	.L213
	movq	roots(%rip), %rax
	movq	%rax, 8(%rsp)
	addq	$1, %rax
	movq	%rax, roots(%rip)
	testq	%rbp, %rbp
	je	.L91
	cmpq	$8, t_frame(%rip)
	je	.L214
.L91:
	movq	%rbx, %rdi
	call	identity
	movl	(%r12,%rax,4), %edx
	testl	%edx, %edx
	jne	.L215
	testq	%rax, %rax
	jne	.L216
	movq	changes(%rip), %rax
	movl	$1, (%rbx)
	movq	%r13, 8(%rbx)
	addq	$1, %rax
	movq	%rax, changes(%rip)
	cmpq	$1, %rax
	jne	.L217
	movq	scope_ends(%rip), %rax
	movq	%r13, %rdi
	movq	%rax, 16(%rsp)
	addq	$1, %rax
	movq	%rax, scope_ends(%rip)
	call	identity
	cmpq	$2, (%r15,%rax,8)
	jne	.L97
	movl	(%r12,%rax,4), %r11d
	testl	%r11d, %r11d
	jne	.L97
	cmpq	$1, %rax
	jne	.L218
	movq	8(%rsp), %rax
	addq	$2, %rax
	movq	%rax, roots(%rip)
	testq	%rbp, %rbp
	je	.L100
	cmpq	$9, t_frame(%rip)
	je	.L219
.L100:
	movq	%r13, %rdi
	call	identity
	movl	(%r12,%rax,4), %r10d
	testl	%r10d, %r10d
	jne	.L220
	cmpq	$1, %rax
	jne	.L105
	movq	payloads(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, payloads(%rip)
	testq	%rax, %rax
	jne	.L105
	movq	16(%rsp), %rax
	addq	$2, %rax
	movq	%rax, scope_ends(%rip)
	testq	%rbp, %rbp
	je	.L106
	cmpq	$6, t_frame(%rip)
	je	.L221
.L106:
	movq	%r13, %rdi
	call	identity
	cmpq	$2, (%r15,%rax,8)
	jne	.L109
	movl	(%r12,%rax,4), %r9d
	testl	%r9d, %r9d
	jne	.L109
	cmpq	$1, %rax
	jne	.L222
	movq	8(%rsp), %rax
	addq	$3, %rax
	movq	%rax, roots(%rip)
	testq	%rbp, %rbp
	je	.L112
	cmpq	$10, t_frame(%rip)
	je	.L223
.L112:
	movq	%r13, %rdi
	call	identity
	movl	(%r12,%rax,4), %r8d
	testl	%r8d, %r8d
	jne	.L224
	cmpq	$1, %rax
	jne	.L225
	movq	reads(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, reads(%rip)
	testq	%rax, %rax
	jne	.L226
	movq	16(%rsp), %rax
	addq	$3, %rax
	movq	%rax, scope_ends(%rip)
	movq	bools(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, bools(%rip)
	testq	%rax, %rax
	jne	.L227
	movl	$0, decision(%rip)
	movq	frees(%rip), %rdx
	testq	%rbp, %rbp
	je	.L119
	cmpq	$6, t_frame(%rip)
	je	.L228
.L119:
	movq	conditions(%rip), %rax
	leaq	1(%rax), %rcx
	orq	%rdx, %rax
	movq	%rcx, conditions(%rip)
	jne	.L229
	movq	results(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, results(%rip)
	testq	%rax, %rax
	jne	.L230
	movl	$181, selected(%rip)
	testq	%rbp, %rbp
	je	.L123
	cmpq	$6, t_frame(%rip)
	je	.L231
.L123:
	movq	%rbx, %rdi
	call	identity
	cmpq	$1, (%r15,%rax,8)
	jne	.L124
	movl	(%r12,%rax,4), %edi
	testl	%edi, %edi
	jne	.L124
	testq	%rax, %rax
	jne	.L232
	movq	8(%rsp), %rax
	addq	$4, %rax
	movq	%rax, roots(%rip)
	testq	%rbp, %rbp
	je	.L127
	cmpq	$16, t_frame(%rip)
	je	.L233
.L127:
	movq	%rbx, %rdi
	call	identity
	movl	(%r12,%rax,4), %esi
	testl	%esi, %esi
	jne	.L234
	testq	%rax, %rax
	jne	.L235
	movq	(%rsp), %rax
	cmpq	%rax, %r13
	jne	.L131
	movq	sends(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, sends(%rip)
	testq	%rax, %rax
	jne	.L131
	movq	entries(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, entries(%rip)
	testq	%rax, %rax
	jne	.L236
	movq	returns_(%rip), %rax
	movl	$0, (%rbx)
	movq	$0, 8(%rbx)
	movq	$2, changes(%rip)
	leaq	1(%rax), %rdx
	movq	%rdx, returns_(%rip)
	testq	%rax, %rax
	jne	.L237
	movq	receives(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, receives(%rip)
	testq	%rax, %rax
	jne	.L238
	movq	16(%rsp), %rax
	addq	$4, %rax
	movq	%rax, scope_ends(%rip)
	testq	%rbp, %rbp
	je	.L135
	cmpq	$6, t_frame(%rip)
	je	.L239
.L135:
	movq	%rbx, %rdi
	call	identity
	movl	(%r12,%rax,4), %edx
	leal	1(%rdx), %ecx
	movl	%ecx, (%r12,%rax,4)
	testl	%edx, %edx
	jne	.L136
	cmpq	$1, (%r15,%rax,8)
	jne	.L136
	addq	$1, ends(%rip)
	movl	%ebx, %esi
	movq	%rbx, %rax
	testq	%rbp, %rbp
	je	.L141
	cmpq	$6, t_frame(%rip)
	je	.L240
.L141:
	movl	ended(%rip), %ecx
	testl	%ecx, %ecx
	je	.L144
	movl	finalized(%rip), %ecx
	leal	1(%rcx), %edi
	movl	%edi, finalized(%rip)
	testl	%ecx, %ecx
	jne	.L144
	movb	%sil, %al
	movq	%r14, %rdi
	addq	$1, finals(%rip)
	movq	%rax, %rbx
	call	identity
	cmpq	$0, pending_free(%rip)
	jne	.L145
	cmpq	%rbx, %r14
	jne	.L145
	movl	(%r12,%rax,4), %edx
	testl	%edx, %edx
	je	.L145
	leaq	finalized(%rip), %rdx
	movl	(%rdx,%rax,4), %eax
	testl	%eax, %eax
	je	.L145
	movq	%r14, %rdi
	movq	%r14, pending_free(%rip)
	call	free@PLT
	leaq	.LC48(%rip), %r8
	movl	$485, %ecx
.L204:
	movq	24(%rsp), %rdi
	leaq	.LC1(%rip), %rdx
	leaq	.LC2(%rip), %rsi
	xorl	%eax, %eax
	call	fprintf@PLT
	call	abort@PLT
.L206:
	cmpq	$5, t_symbol(%rip)
	ja	.L40
	testq	%r12, %r12
	je	.L241
	cmpq	$1, %rbx
	je	.L242
	cmpq	$2, %rbx
	jne	.L47
	movq	bindings(%rip), %rdx
	leaq	.LC13(%rip), %r8
	cmpq	$3, %rdx
	ja	.L203
	cmpq	$2, %rdx
	je	.L49
	leaq	fixed.25(%rip), %rcx
	movl	(%rcx,%rdx,4), %ecx
.L50:
	movzbl	%al, %eax
	leaq	.LC14(%rip), %r8
	cmpl	%eax, %ecx
	jne	.L203
	addq	$1, %rdx
	addq	$1, %r12
	movq	%rdx, bindings(%rip)
	movq	%r12, trials(%rip)
.L52:
	leaq	.LC16(%rip), %r8
	movl	$161, %ecx
	jmp	.L204
.L214:
	cmpq	$23, t_symbol(%rip)
	ja	.L91
	cmpq	$2, frees(%rip)
	jne	.L93
	cmpq	$0, pending_free(%rip)
	jne	.L93
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	testq	%rax, %rax
	jne	.L93
	cmpb	$-75, %bl
	jne	.L93
	movl	$181, observed_delivery(%rip)
	jmp	.L91
.L219:
	cmpq	$26, t_symbol(%rip)
	ja	.L100
	cmpq	$2, frees(%rip)
	jne	.L102
	cmpq	$0, pending_free(%rip)
	jne	.L102
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	testq	%rax, %rax
	jne	.L102
	cmpb	$-75, %r13b
	jne	.L102
	movl	$181, observed_delivery(%rip)
	jmp	.L100
.L221:
	cmpq	$27, t_symbol(%rip)
	ja	.L106
	cmpq	$2, frees(%rip)
	jne	.L108
	cmpq	$0, pending_free(%rip)
	jne	.L108
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	testq	%rax, %rax
	jne	.L108
	movl	$181, observed_delivery(%rip)
	jmp	.L106
.L223:
	cmpq	$29, t_symbol(%rip)
	ja	.L112
	cmpq	$2, frees(%rip)
	jne	.L114
	cmpq	$0, pending_free(%rip)
	jne	.L114
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	cmpb	$-75, %r13b
	jne	.L114
	testq	%rax, %rax
	jne	.L114
	movl	$181, observed_delivery(%rip)
	jmp	.L112
.L231:
	cmpq	$34, t_symbol(%rip)
	ja	.L123
	leaq	.LC12(%rip), %r8
	movl	$411, %ecx
	jmp	.L204
.L233:
	cmpq	$36, t_symbol(%rip)
	ja	.L127
	leaq	.LC12(%rip), %r8
	movl	$422, %ecx
	jmp	.L204
.L239:
	cmpq	$43, t_symbol(%rip)
	ja	.L135
	leaq	.LC12(%rip), %r8
	movl	$439, %ecx
	jmp	.L204
.L212:
	movq	t_symbol(%rip), %rsi
	cmpq	$17, %rsi
	ja	.L78
	cmpq	$2, frees(%rip)
	jne	.L80
	cmpq	$0, pending_free(%rip)
	jne	.L80
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	testq	%rax, %rax
	jne	.L80
	cmpb	$-75, (%rsp)
	jne	.L80
	movl	$181, observed_delivery(%rip)
	movl	$-75, %ecx
.L81:
	cmpq	$2, frees(%rip)
	jne	.L84
	cmpq	$0, pending_free(%rip)
	jne	.L84
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	cmpb	$-75, %cl
	jne	.L84
	testq	%rax, %rax
	jne	.L84
	movl	$181, observed_delivery(%rip)
.L147:
	cmpq	$2, frees(%rip)
	je	.L243
.L85:
	leaq	.LC12(%rip), %r8
	movl	$277, %ecx
	jmp	.L204
.L228:
	cmpq	$31, t_symbol(%rip)
	ja	.L119
	cmpq	$2, %rdx
	je	.L244
.L120:
	leaq	.LC12(%rip), %r8
	movl	$388, %ecx
	jmp	.L204
.L240:
	movq	t_symbol(%rip), %rcx
	cmpq	$48, %rcx
	ja	.L140
	leaq	.LC12(%rip), %r8
	movl	$459, %ecx
	jmp	.L204
.L207:
	movq	t_symbol(%rip), %rdi
	cmpq	$9, %rdi
	ja	.L58
	testq	%rcx, %rcx
	jne	.L59
	movq	startup_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, startup_copies(%rip)
	cmpl	$87, %esi
	jne	.L156
	testq	%rax, %rax
	jne	.L156
.L61:
	movq	startup_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, startup_copies(%rip)
	testq	%rax, %rax
	jne	.L157
	cmpl	$87, %esi
	jne	.L157
.L66:
	addq	$1, startup_copies(%rip)
	leaq	.LC11(%rip), %r8
	movl	$193, %ecx
	jmp	.L204
.L140:
	cmpq	$49, %rcx
	jne	.L141
	leaq	.LC12(%rip), %r8
	movl	$466, %ecx
	jmp	.L204
.L76:
	movq	(%rsp), %rax
	jmp	.L82
.L78:
	movq	(%rsp), %rax
	cmpq	$18, %rsi
	je	.L81
	cmpq	$19, %rsi
	jne	.L82
	jmp	.L147
.L241:
	subq	$1, %rbx
	jne	.L43
	movq	startup_copies(%rip), %rdx
	leaq	1(%rdx), %rcx
	movq	%rcx, startup_copies(%rip)
	cmpb	$87, %al
	jne	.L43
	testq	%rdx, %rdx
	jne	.L43
	movl	$24, %edi
	call	malloc@PLT
	movq	$1, trials(%rip)
	movq	%rax, %r14
	testq	%rax, %rax
	je	.L152
	cmpq	$2, t_frame(%rip)
	movzbl	%al, %esi
	jne	.L60
	movq	t_symbol(%rip), %rdi
	cmpq	$9, %rdi
	ja	.L154
	movzbl	%r14b, %esi
.L59:
	cmpq	$2, frees(%rip)
	jne	.L64
	cmpq	$0, pending_free(%rip)
	jne	.L64
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	testq	%rax, %rax
	jne	.L64
	cmpl	$181, %esi
	jne	.L64
	movl	$181, observed_delivery(%rip)
.L65:
	cmpq	$2, frees(%rip)
	jne	.L69
	cmpq	$0, pending_free(%rip)
	jne	.L69
	movq	delivery_copies(%rip), %rax
	leaq	1(%rax), %rdx
	movq	%rdx, delivery_copies(%rip)
	cmpl	$181, %esi
	jne	.L69
	testq	%rax, %rax
	jne	.L69
	movl	$181, observed_delivery(%rip)
.L70:
	cmpq	$2, frees(%rip)
	je	.L245
.L71:
	leaq	.LC12(%rip), %r8
	movl	$193, %ecx
	jmp	.L204
.L154:
	movl	$1, %ecx
.L58:
	movq	%r14, %rax
	cmpq	$10, %rdi
	je	.L246
	cmpq	$11, %rdi
	jne	.L57
	testq	%rcx, %rcx
	jne	.L70
	jmp	.L66
.L209:
	leaq	.LC19(%rip), %r8
	movl	$205, %ecx
	jmp	.L204
.L49:
	movl	observed_delivery(%rip), %ecx
	jmp	.L50
.L245:
	cmpq	$0, pending_free(%rip)
	jne	.L71
	addq	$1, delivery_copies(%rip)
	jmp	.L71
.L236:
	leaq	.LC42(%rip), %r8
	movl	$127, %ecx
	jmp	.L204
.L131:
	leaq	.LC41(%rip), %r8
	movl	$431, %ecx
	jmp	.L204
.L238:
	leaq	.LC44(%rip), %r8
	movl	$433, %ecx
	jmp	.L204
.L145:
	leaq	.LC47(%rip), %r8
	movl	$475, %ecx
	jmp	.L204
.L217:
	leaq	.LC26(%rip), %r8
	movl	$314, %ecx
	jmp	.L204
.L136:
	leaq	.LC45(%rip), %r8
	movl	$453, %ecx
	jmp	.L204
.L144:
	leaq	.LC46(%rip), %r8
	movl	$469, %ecx
	jmp	.L204
.L229:
	leaq	.LC36(%rip), %r8
	movl	$397, %ecx
	jmp	.L204
.L105:
	leaq	.LC30(%rip), %r8
	movl	$334, %ecx
	jmp	.L204
.L226:
	leaq	.LC34(%rip), %r8
	movl	$357, %ecx
	jmp	.L204
.L109:
	leaq	.LC31(%rip), %r8
	movl	$348, %ecx
	jmp	.L204
.L97:
	leaq	.LC27(%rip), %r8
	movl	$327, %ecx
	jmp	.L204
.L124:
	leaq	.LC38(%rip), %r8
	movl	$418, %ecx
	jmp	.L204
.L237:
	leaq	.LC43(%rip), %r8
	movl	$145, %ecx
	jmp	.L204
.L87:
	leaq	.LC22(%rip), %r8
	movl	$302, %ecx
	jmp	.L204
.L86:
	leaq	.LC21(%rip), %r8
	movl	$289, %ecx
	jmp	.L204
.L213:
	leaq	.LC23(%rip), %r8
	movl	$302, %ecx
	jmp	.L204
.L208:
	leaq	.LC18(%rip), %r8
	movl	$205, %ecx
	jmp	.L204
.L152:
	leaq	.LC17(%rip), %r8
	movl	$161, %ecx
	jmp	.L204
.L210:
	leaq	.LC16(%rip), %r8
	movl	$214, %ecx
	jmp	.L204
.L205:
	movq	stderr(%rip), %rdi
	leaq	.LC10(%rip), %r8
	xorl	%eax, %eax
	movl	$153, %ecx
	leaq	.LC1(%rip), %rdx
	leaq	.LC2(%rip), %rsi
	call	fprintf@PLT
	call	abort@PLT
.L211:
	leaq	.LC20(%rip), %r8
	movl	$214, %ecx
	jmp	.L204
.L230:
	leaq	.LC37(%rip), %r8
	movl	$405, %ecx
	jmp	.L204
.L225:
	leaq	.LC33(%rip), %r8
	movl	$355, %ecx
	jmp	.L204
.L234:
	leaq	.LC39(%rip), %r8
	movl	$425, %ecx
	jmp	.L204
.L220:
	leaq	.LC29(%rip), %r8
	movl	$334, %ecx
	jmp	.L204
.L227:
	leaq	.LC35(%rip), %r8
	movl	$385, %ecx
	jmp	.L204
.L216:
	leaq	.LC25(%rip), %r8
	movl	$309, %ecx
	jmp	.L204
.L215:
	leaq	.LC24(%rip), %r8
	movl	$309, %ecx
	jmp	.L204
.L222:
	leaq	.LC28(%rip), %r8
	movl	$348, %ecx
	jmp	.L204
.L218:
	leaq	.LC28(%rip), %r8
	movl	$327, %ecx
	jmp	.L204
.L224:
	leaq	.LC32(%rip), %r8
	movl	$355, %ecx
	jmp	.L204
.L43:
	leaq	.LC11(%rip), %r8
.L203:
	movl	$158, %ecx
	jmp	.L204
.L232:
	leaq	.LC23(%rip), %r8
	movl	$418, %ecx
	jmp	.L204
.L235:
	leaq	.LC40(%rip), %r8
	movl	$425, %ecx
	jmp	.L204
.L84:
	leaq	.LC12(%rip), %r8
	movl	$272, %ecx
	jmp	.L204
.L80:
	leaq	.LC12(%rip), %r8
	movl	$265, %ecx
	jmp	.L204
.L93:
	leaq	.LC12(%rip), %r8
	movl	$306, %ecx
	jmp	.L204
.L102:
	leaq	.LC12(%rip), %r8
	movl	$331, %ecx
	jmp	.L204
.L69:
	leaq	.LC12(%rip), %r8
	movl	$188, %ecx
	jmp	.L204
.L64:
	leaq	.LC12(%rip), %r8
	movl	$181, %ecx
	jmp	.L204
.L114:
	leaq	.LC12(%rip), %r8
	movl	$352, %ecx
	jmp	.L204
.L246:
	testq	%rcx, %rcx
	jne	.L65
	jmp	.L61
.L244:
	cmpq	$0, pending_free(%rip)
	jne	.L120
	addq	$1, delivery_copies(%rip)
	jmp	.L120
.L108:
	leaq	.LC12(%rip), %r8
	movl	$341, %ecx
	jmp	.L204
.L157:
	leaq	.LC11(%rip), %r8
	movl	$188, %ecx
	jmp	.L204
.L156:
	leaq	.LC11(%rip), %r8
	movl	$181, %ecx
	jmp	.L204
.L243:
	cmpq	$0, pending_free(%rip)
	jne	.L85
	addq	$1, delivery_copies(%rip)
	jmp	.L85
.L47:
	leaq	.LC15(%rip), %r8
	jmp	.L203
.L242:
	cmpq	$2, frees(%rip)
	je	.L247
.L46:
	leaq	.LC12(%rip), %r8
	jmp	.L203
.L247:
	cmpq	$0, pending_free(%rip)
	jne	.L46
	addq	$1, delivery_copies(%rip)
	jmp	.L46
	.cfi_endproc
.LFE16:
	.size	main, .-main
	.section	.rodata
	.align 16
	.type	fixed.1, @object
	.size	fixed.1, 16
fixed.1:
	.long	0
	.long	0
	.long	0
	.long	13
	.set	fixed.25,fixed.1
	.local	selected
	.comm	selected,4,4
	.local	decision
	.comm	decision,4,4
	.local	observed_delivery
	.comm	observed_delivery,4,4
	.local	t_symbol
	.comm	t_symbol,8,8
	.local	t_frame
	.comm	t_frame,8,8
	.local	sibling
	.comm	sibling,8,8
	.local	reading
	.comm	reading,8,8
	.local	pending_free
	.comm	pending_free,8,8
	.local	finalized
	.comm	finalized,8,8
	.local	ended
	.comm	ended,8,8
	.local	domains
	.comm	domains,16,16
	.local	originals
	.comm	originals,16,16
	.local	returns_
	.comm	returns_,8,8
	.local	entries
	.comm	entries,8,8
	.local	receives
	.comm	receives,8,8
	.local	sends
	.comm	sends,8,8
	.local	scope_ends
	.comm	scope_ends,8,8
	.local	delivery_copies
	.comm	delivery_copies,8,8
	.local	startup_copies
	.comm	startup_copies,8,8
	.local	bindings
	.comm	bindings,8,8
	.local	bools
	.comm	bools,8,8
	.local	results
	.comm	results,8,8
	.local	conditions
	.comm	conditions,8,8
	.local	finals
	.comm	finals,8,8
	.local	ends
	.comm	ends,8,8
	.local	changes
	.comm	changes,8,8
	.local	reads
	.comm	reads,8,8
	.local	payloads
	.comm	payloads,8,8
	.local	roots
	.comm	roots,8,8
	.local	calls
	.comm	calls,8,8
	.local	frees
	.comm	frees,8,8
	.local	inits
	.comm	inits,8,8
	.local	trials
	.comm	trials,8,8
	.ident	"GCC: (Debian 14.2.0-19) 14.2.0"
	.section	.note.GNU-stack,"",@progbits
