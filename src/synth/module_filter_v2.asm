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

module_filter_v2:
	pusha

	;; Scale feedback with cutoff for increased stability & more usable sound
	fld dword [esi + FLT_PARAM_FB]
	fld dword [esi + FLT_PARAM_CUTOFF]
	fmul dword [fc1]
	fmulp

	;; add input to feedback
	fld dword [ebp + FLT1_STATE_Y]
	fld dword [ebp + FLT2_STATE_Y]
	fsubp			; flt1_out - flt2_out
	fmulp			; *= feedback
	fsin			; waveshape feedback signal
	fadd dword [esi + FLT_PARAM_INPUT]
	;; st0: (input + feedback)

	;; 1st LP filter
	fmul dword [esi + FLT_PARAM_CUTOFF]
	fabs			; safety
	fld1
	fsub dword [esi + FLT_PARAM_CUTOFF]
	fabs			; safety
	fmul dword [ebp + FLT1_STATE_Y]
	faddp
	fst dword [ebp + FLT1_STATE_Y]

	;; 2nd LP filter
	fld dword [esi + FLT_PARAM_CUTOFF]	
	fadd dword [esi + FLT_PARAM_CUTOFF]
	fmulp
	fld1
	fsub dword [esi + FLT_PARAM_CUTOFF]
	fsub dword [esi + FLT_PARAM_CUTOFF]
	fmul dword [ebp + FLT2_STATE_Y]
	faddp
	fst dword [ebp + FLT2_STATE_Y]

	;; TODO: LP/HP switch

	popa
	ret
