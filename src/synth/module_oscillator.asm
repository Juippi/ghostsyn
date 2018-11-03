;;; Parameters & state shared between channels
%define OSC_PARAM_GAIN	   4
%define OSC_PARAM_FREQ_MOD 8
%define OSC_PARAM_ADD	   12
%define OSC_PARAM_DETUNE2  16 ; 2nd osc detune

;;; Per-channel state
%define OSC_STATE_CTR1	   0
%define OSC_STATE_CTR2	   4

module_oscillator:
	pusha

	fld1
	fld dword [esi + OSC_PARAM_ADD] ; st0: osc add, st1: 1

	;; freq modulation
	fld dword [esi + OSC_PARAM_FREQ_MOD]
	fadd st2		; 1
	fmulp                   ; st0: modulated add, st1: 1
	;; stereo factor
	fld dword [bss_stereo_factor]
	fmulp                   ; st0: modulated add, st1: 1

	fxch st0, st1
	fld dword [ebp + OSC_STATE_CTR1] ; st0: osc counter, st1: 1, st2: modulated add
	fadd st0, st2

;;; noisify osc 1 (for pure noise, osc 2 can be silenced with detune == 0
	test dword [esi], OSC_FLAG_NOISE
	jz _skip_noise
	fadd dword [fc1]	; just need some constant here
	fmul st0
	fadd st0
_skip_noise:

	fprem1

	fstp dword [ebp + OSC_STATE_CTR1] ; st0: 1, st1: modulated add

	fld dword [esi + OSC_PARAM_DETUNE2]
	fmulp st2, st0                   ; st0: 1, st1: modulated + detuned add

	fld dword [ebp + OSC_STATE_CTR2]
	fadd st0, st2
	fprem1
	fst dword [ebp + OSC_STATE_CTR2] ; st0: osc2, st1: 1, st2: modulated + detuned add

;;;
	;; sine shaper for osc2
	test dword [esi], OSC_FLAG_SINE
	jz _osc2_skip_sine
	fadd st0
 _osc2_skip_sine:
	;; half sine can sound good too, enable it
	test dword [esi], OSC_FLAG_HSIN
	jz _osc2_skip_hsin
	fldpi
	fmulp
	fsin
_osc2_skip_hsin:
;;;

	fld dword [ebp + OSC_STATE_CTR1]
	faddp

;;;
	;; sine shaper for osc1
	test dword [esi], OSC_FLAG_SINE
	jz _osc1_skip_sine
	fadd st0
_osc1_skip_sine:
	test dword [esi], OSC_FLAG_HSIN
	jz _osc1_skip_hsin
	fldpi
	fmulp
	fsin
_osc1_skip_hsin:
;;;

	fxch st0, st2
	fstp st0
	fstp st0

	fmul dword [esi + OSC_PARAM_GAIN]
	;; output in st0

	popa
osc_end:
	ret
