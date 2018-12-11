;;; esi
%define FLT_PARAM_INPUT      4
%define FLT_PARAM_CUTOFF     8
%define FLT_PARAM_FB         12

;;; ebp
%define FLT1_STATE_Y	   0
%define FLT2_STATE_Y	   4

;;; filter constants
fc1:
	dd 3.296875		; TODO: close to PI, good enough?
pw:
	dd 1.5			; filter peak width

module_filter_v2:
	pusha

	;; Scale feedback with cutoff for increased stability & more usable sound
	fld dword [esi + FLT_PARAM_CUTOFF]
	fabs			; safety

	fld dword [esi + FLT_PARAM_FB]

	;; fld st0			; st0 == st1 == cutoff
	;; fmul dword [esi + FLT_PARAM_FB]
	;; fldl2e
	;; fmulp
	;; st0: scaled feedback, st1: flt cutoff

	;; add input to feedback
	fld dword [ebp + FLT1_STATE_Y]
	fld dword [ebp + FLT2_STATE_Y]
	fsubp			; flt1_out - flt2_out
	fmulp			; *= feedback
	fsin			; waveshape feedback signal
	fadd dword [esi + FLT_PARAM_INPUT]
	;; st0: (input + feedback), st1: flt cutoff

	;; 1st LP filter
	fmul st1		; st0 = st0 * st1
	fld1
	fsub st2
	fmul dword [ebp + FLT1_STATE_Y]
	faddp
	fst dword [ebp + FLT1_STATE_Y]

	add ebp, 4		; advance ebp to 2nd filter state
	;; cutoff of 2nd filter
	fld st1
	fmul dword [pw]
	fstp st2

	;; 2nd LP filter
	fmul st1
	fld1
	fsub st2
	fmul dword [ebp + FLT1_STATE_Y] ; actually FLT2_STATE_Y now
	faddp
	fst dword [ebp + FLT1_STATE_Y]

	;; TODO: LP/HP switch
	fxch st0, st1
	fstp st0

	popa
	ret
