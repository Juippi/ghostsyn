;;; dynamic compressor with threshold, attack & release
;;; independent channels (TODO: stereo link would be nice too)
;;; 

%define COMP_VAL_IN        4
%define COMP_VAL_THRESHOLD 8
%define COMP_VAL_ATTACK    12
%define COMP_VAL_RELEASE   16

%define COMP_STATE_GAIN    0
	
module_compressor:
	pusha
	fld dword [ebp + COMP_STATE_GAIN]
	fld dword [esi + COMP_VAL_IN]
	fmul st1
	;; st0: output, st1: gain

	fld st0
	fabs
	;; st0: abs(output), st1: output, st2: gain

	fld dword [esi + COMP_VAL_THRESHOLD]
	;; st0: threshold, st1: abs(output), st2: output, st3: gain
	fcomip st1	; set carry if abs(output) < threshold
	fstp st0	; pop abs(output)
	;; st0: output, st1: gain
	fxch st0, st1
	;; st0: gain, st1: output
	jnc .do_release		; jmp if threshold > abs(output)

	;; if output is above threshold, apply comp. attack (should be < 1.0 to reduce gain)
	fld dword [esi + COMP_VAL_ATTACK]
	fmulp
	jmp .skip_release

.do_release:
	;; otherwise, apply comp. release (increase gain) if gain < 1
	fld1
	fcomip st1		; set cf if gain > 1
	jc .skip_release

	fld dword [esi + COMP_VAL_RELEASE]
	faddp

.skip_release:
	;; st0: updated gain, st1: output signal
	fstp dword [ebp + COMP_STATE_GAIN] ; store new gain (if not > 1)

	;; st0: output
	popa
	ret
