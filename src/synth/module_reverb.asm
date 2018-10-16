;;; delay/reverb
;;; TODO: with delay len modulation this could serve as chorus/flanger too

;;; esi
%define REV_VAL_IN	8
%define REV_VAL_TAPS    12	; no. of taps in delay line, 1 creates a simple delay
%define REV_VAL_FEEDBACK 16
%define REV_VAL_LP	20	; LP filter coefficient (for delay line input)

;;; ebp
%define REV_STATE_DELAY_POS	0
%define REV_STATE_FLT		4
%define REV_STATE_BUF		8	; delay buffer start offset

%define REV_MAGIC 19391		; magic constant for generating delay tap locations
%define DELAY_LEN_MASK 0x1fffc	; for 32768 sample delay buffer (4 bytes / sample)

module_reverb:
	pusha

	fld dword [esi + REV_VAL_IN] ; input

	;; advance delay pointer
	mov ebx, [ebp + REV_STATE_DELAY_POS]
	add ebx, 4
	and ebx, DELAY_LEN_MASK
	mov [ebp + REV_STATE_DELAY_POS], ebx

	;; sample delayline at multiple positions
	mov ecx, [esi + REV_VAL_TAPS] ; number of delay taps
	fldz			      ; delay line output accumulator

	xor eax, eax		; keeps delay line sample offset during loop
delay_tap_loop:
	;; calculate offset of next tab ("pseudorandom")
	imul eax, REV_MAGIC
	add eax, REV_MAGIC

	push ebx		; store original ringbuffer pos
	add ebx, eax
	and ebx, DELAY_LEN_MASK
	fadd dword [ebp + ebx + REV_STATE_BUF] ; sample delay line
	fneg				       ; reverse phase
	pop ebx

	;; stereo effect: bss_stereo_factor is a float, but works just
	;; as well here for spreading tap locations depending on channel
	add eax, [bss_stereo_factor]

	loop delay_tap_loop

	;; LP filter for delay line input
	fld1
	fsub dword [esi + REV_VAL_LP]
	fmulp			; x * (1 - lp_coeff)
	
	fld dword [ebp + REV_STATE_FLT]
	fmul dword [esi + REV_VAL_LP] ; y^-1 * lp_coeff
	faddp

	fst dword [ebp + REV_STATE_FLT] ; store output for lp filter

	fmul dword [esi + REV_VAL_FEEDBACK]
	faddp

	fst dword [ebp + ebx + REV_STATE_BUF] ; store output to delayline
	
	popa
	ret
