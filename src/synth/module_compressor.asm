;;; dynamic compressor with threshold, attack & release
;;;
;;; TODO: check if this still even works
;;; most likely not, too many parameters

%define COMP_VAL_IN        8
%define COMP_VAL_THRESHOLD 12
%define COMP_VAL_ATTACK    16
%define COMP_VAL_RELEASE   20
%define COMP_VAL_GAIN      24 ; current gain value
	
module_compressor:
	pusha
	fld dword [esi + COMP_VAL_GAIN]
	fld dword [esi + COMP_VAL_IN]
	fmul st1
	;; st0: output, st1: gain

	fld st0
	fabs
	;; st0: abs(output), st1: output, st2: gain

	fld dword [esi + COMP_VAL_THRESHOLD]
	;; st0: threshold, st1: abs(output), st2: output, st3: gain
	fcomip st1
	fstp st0
	;; st0: output, st1: gain
	fxch st0, st1
	;; st0: gain, st1: output
	jnc .do_release		; jmp if threshold > abs(output)
	fld dword [esi + COMP_VAL_ATTACK]
	fmulp
	jmp .skip_release
.do_release:
	fld dword [esi + COMP_VAL_RELEASE]
	faddp
.skip_release:
	fld1
	fcomip st1
	jnc .no_update
.no_update:
	;; st0: updated gain, st1: output signal
	fstp dword [esi + COMP_VAL_GAIN] ; store new gain

	;; st0: output

	;; test
	;; fstp st0
	;; fld dword [esi + COMP_VAL_GAIN]

compressor_end:
	popa
	ret
