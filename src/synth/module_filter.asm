%define FLT_PARAM_INPUT      8
%define FLT_PARAM_CUTOFF     12
%define FLT_PARAM_FB_GAIN    16

%define FLT_STATE_FB1	   0
%define FLT_STATE_FB2	   4

;;; filter constants
fc1:
       dd 3.296875

module_filter:
	pusha
	;; feedback coefficient
	fld dword [esi + FLT_PARAM_FB_GAIN]
	fld dword [esi + FLT_PARAM_CUTOFF]
	fmul dword [fc1]
	fmulp
	;; st0: fb_coeff

	fld dword [ebp + FLT_STATE_FB1]
	fld dword [ebp + FLT_STATE_FB2]
	fsubp
	fmulp
	;; st0: feedback
	fsin

	fld dword [esi + FLT_PARAM_INPUT]  ; input
	fld dword [esi + FLT_PARAM_CUTOFF] ; cutoff
	fabs				   ; safety measure: negative cutoff makes unstable
	fmulp
	;; st0: (in * cutoff), st1: feedback

	fld dword [ebp + FLT_STATE_FB1]
	fld1
	fld dword [esi + FLT_PARAM_CUTOFF] ; cutoff
	fabs			; safety
	fsubp st1		    ; 1 - cutoff
	fmulp
	;; st0: flt_p1 * (1 - cutoff), st1: (in * cutoff), st2: feedback

	faddp
	faddp
	fst dword [ebp + FLT_STATE_FB1]
	;; st0: flt_p1
	;; TODO: kill denormals here?

	fld dword [esi + FLT_PARAM_CUTOFF]
	fabs			; safety
	fadd st0
	fld1
	fsub st0, st1
	;; st0: (1 - flt_co * 2), st1: (flt_co * 2), st2: flt_p1

	fmul dword [ebp + FLT_STATE_FB2]
	fxch st0, st1
	fmul st0, st2
	;; st0: (1 - flt_co * 2) * flt_p1, st1: (flt_co * 2) * flt_p1, st2: flt_p

	faddp
	fst dword [ebp + FLT_STATE_FB2]

	fxch st0, st1
	fstp st0

	;; output in st0
filter_end:
	popa
	ret
