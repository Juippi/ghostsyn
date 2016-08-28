;;; stereo delay with feedback
;;;
;;; TODO: configurable stereo delay diff
;;; TODO: lp filter for feedback?
;;; idea: instead of adding single values from past, iterate through buffer,
;;; accumulate sin(diff) * val[diff] (or sin^2 or sin^n for less lp filtering)
%define DELAY_PARAM_IN       8
%define DELAY_PARAM_LEN      12
%define DELAY_PARAM_FB       16
;; %define DELAY_PARAM_WET      20

%define DELAY_STATE_POS      0

module_delay:
	pusha
	fld dword [esi + DELAY_PARAM_IN]
	;; update delay buf ptr
	mov eax, [ebp + DELAY_STATE_POS]
	add eax, 4

	mov ebx, dword [esi + DELAY_PARAM_LEN]
	test edi, 4
	jnz .foo
	add ebx, 2000 		; delay stereo separationings
.foo:

	xor edx, edx
	div ebx
	mov [ebp + DELAY_STATE_POS], edx
	add edx, 4

	fld dword [ebp + edx]
	fld dword [esi + DELAY_PARAM_FB]
	fmulp
	faddp

	fst dword [ebp + edx]

delay_end:
	popa
	ret
