;;; chorus/flanger effect
;;; (TODO: does this need to have stereo modulation, or are stereo oscs enough?)

;;; esi
%define CHO_VAL_IN	8
%define CHO_VAL_TIME	12
%define CHO_VAL_MOD_ADD	16
%define CHO_VAL_MODAMP  20

;;; ebp
%define CHO_STATE_DELAY_POS	0
%define CHO_STATE_DELAY_MOD_CTR	4
%define CHO_STATE_BUF		8	; delay buffer start offset

half:
	dd 0.5

module_chorus:
	pusha

	fld dword [esi + CHO_VAL_IN] ; input

	mov eax, [ebp + CHO_STATE_DELAY_POS]

	;; add delay time modulation to eax here
	fld dword [ebp + CHO_STATE_DELAY_MOD_CTR]
	;; st0: delay ctr
	fadd dword [esi + CHO_VAL_MOD_ADD]
	;; st0: updated delay ctr
	fst dword [ebp + CHO_STATE_DELAY_MOD_CTR]

	fsin
	fld1
	faddp
	;; st0: sin(updated delay ctr) + 1

	fmul dword [esi + CHO_VAL_MODAMP]
	;; st0: delay mod value

	;; covert to int, add to eax
	fistp dword [esi + CHO_VAL_TIME] ; TODO: use generic temp location
	add eax, [esi + CHO_VAL_TIME]
	and eax, 0x7ffc		; % 32768
	;; modulated delay time now in eax

	fadd dword [ebp + eax + CHO_STATE_BUF] ; delay buf
	;; st0: delay in + input

	fmul dword [half]
	;; st0: .5 * dly + .5 * in

	fst dword [ebp + eax + CHO_STATE_BUF] ; write output to delay buf

	;; advance delay buf ptr
	mov eax, [ebp + CHO_STATE_DELAY_POS]
	add eax, 4
	and eax, 0x7ffc		; % 32768
	mov [ebp + CHO_STATE_DELAY_POS], eax

	;; st0: output
	popa
	ret
