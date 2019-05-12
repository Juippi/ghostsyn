;;; delay/reverb
;;; TODO: with delay len modulation this could serve as chorus/flanger too

;;; st0: input
;;; st1: num. taps
;;; st2: feedback
;;; st3: lp flt coeff

;;; esi
;; %define REV_VAL_IN	 4
;; %define REV_VAL_TAPS     8	; no. of taps in delay line, 1 creates a simple delay
;; %define REV_VAL_FEEDBACK 12
;; %define REV_VAL_LP	 16	; LP filter coefficient (for delay line input)

;;; ebp
%define REV_STATE_DELAY_POS	0
%define REV_STATE_FLT		4
%define REV_STATE_BUF		8	; delay buffer start offset

%define REV_MAGIC 19391		; magic constant for generating delay tap locations
%define DELAY_LEN_MASK 0x1fffc	; for 32768 sample delay buffer (4 bytes / sample)

module_reverb:
        pusha

        ;; advance delay pointer
        mov ebx, [ebp + REV_STATE_DELAY_POS]
        add ebx, 4
        and ebx, DELAY_LEN_MASK
        mov [ebp + REV_STATE_DELAY_POS], ebx

        ;; sample delayline at multiple positions
        fld st1
        fistp dword [bss_temp1]
        mov ecx, [bss_temp1]

        fldz			      ; delay line output accumulator
        ;; stack + 1

        xor eax, eax		; keeps delay line sample offset during loop
delay_tap_loop:
        ;; calculate offset of next tab ("pseudorandom")
        imul eax, REV_MAGIC
        add eax, REV_MAGIC

        push ebx		; store original ringbuffer pos
        add ebx, eax
        and ebx, DELAY_LEN_MASK
        fadd dword [ebp + ebx + REV_STATE_BUF] ; sample delay line
        fchs				       ; reverse phase
        pop ebx

        ;; stereo effect: bss_stereo_factor is a float, but works just
        ;; as well here for spreading tap locations depending on channel
        add eax, [bss_stereo_factor]

        loop delay_tap_loop

%if 0
        ;; LP filter for delay line input
        fld1
        ;; stack + 2
        fsub st5
        fmulp			; x * (1 - lp_coeff)
        ;; stack + 1

        fld dword [ebp + REV_STATE_FLT]
        ;; stack + 2
        fmul st4 ; y^-1 * lp_coeff
        faddp
        ;; stack + 1
%endif

        fst dword [ebp + REV_STATE_FLT] ; store output for lp filter

        fmul st3                ; feedback
        fadd st1                ; input

        fst dword [ebp + ebx + REV_STATE_BUF] ; store output to delayline

        popa
reverb_done:
        ret
