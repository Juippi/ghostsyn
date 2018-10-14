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

%define REV_MAGIC 7391		; magic constant for generating delay tap locations
%define DELAY_LEN_MASK 0x1ffff	; for 32768 sample delay buffer (4 bytes / sample)

module_reverb:
	pusha

	fld dword [esi + REV_VAL_IN] ; input

	;; advance delay pointer
	mov ebx, [ebp + REV_STATE_DELAY_POS]
	add ebx, 4
	and ebx, DELAY_LEN_MASK
	mov [ebp + REV_STATE_DELAY_POS], ebx

	push ebx		; store pos. of next delay input value

	;; sample delayline at multiple positions

	mov ecx, [esi + REV_VAL_TAPS] ; number of delay taps
	fldz			      ; delay line output accumulator

	.delay_tap_loop
	fadd dword [ebp + ebx + REV_STATE_BUF] ; sample delay line
	imul ebx, REV_MAGIC
	add ebx, REV_MAGIC
	and ebx, DELAY_LEN_MASK
	loop .delay_tap_loop

	;; TODO: lp filter for delay line output

	fmul dword [esi + REV_VAL_FEEDBACK]
	faddp

	pop ebx				      ; restore pos. of next delay input value
	fst dword [ebp + ebx + REV_STATE_BUF] ; store output to delayline
	
	popa
	ret
