;;;  AD envelope generator
;;;  - linear attack
;;;  - switch at defined gain level
;;;  - exponential decay
;;;
;;; TODO: N stage support

%define ENV_PARAM_ATT      8  ; linear attack add
%define ENV_PARAM_SWITCH   12 ; level for A -> D switch (or how about just doing at 1?)
%define ENV_PARAM_DECAY    16 ; multiplier for exp decay
%define ENV_PARAM_STAGE    20 ; env stage A: 0, D: 1

%define ENV_STATE_LEVEL    0  ; current level

%define ENV_STAGE_ATTACK   1
%define ENV_STAGE_DECAY    0

;;; small constant for denormal killing
small:
	db 0
	db 0
	db 0x80
	db 0

module_envelope:
	pusha
	fld dword [ebp + ENV_STATE_LEVEL]
	mov eax, [esi + ENV_PARAM_STAGE]
	cmp eax, ENV_STAGE_ATTACK
	jne .in_decay
	fld dword [esi + ENV_PARAM_ATT]
	faddp
	;; test for state switch
	fld dword [esi + ENV_PARAM_SWITCH]
	fcomip
	jnc .no_switch
	xor eax, eax		; mov eax, ENV_STAGE_DECAY
	mov [esi + ENV_PARAM_STAGE], eax
.no_switch:
	jmp .no_decay
.in_decay:
	fld dword [esi + ENV_PARAM_DECAY]
	;; faster decay after note off
	cmp eax, 0
	je .no_release
	fmul st1, st0
	fmul st1, st0
	fmul st1, st0
.no_release:	
	fmulp
.no_decay:

	;; denormal killling
	fld dword [small]
	faddp

	fst dword [ebp + ENV_STATE_LEVEL]

	;; output in st0
envelope_end:
	popa
	ret
