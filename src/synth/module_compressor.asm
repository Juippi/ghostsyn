;;; dynamic compressor with threshold, attack & release
;;; independent channels (TODO: stereo link would be nice too)
;;;

;;; st0: input
;;; st1: threshold
;;; st2: attack
;;; st3: release

;;; ebp
%define COMP_STATE_GAIN    0

module_compressor:
        pusha

        fld st0                 ; input
        ;; stack + 1 - st0: input
        fmul dword [ebp + COMP_STATE_GAIN]
        ;; stack + 1 - st0: output

        fld st0
        fabs
        ;; stack + 2 - st0: abs(output), st1: output

        fld st3                 ; threshold
        ;; stack + 3
        ;; st0: threshold, st1: abs(output), st2: output
        fcomip st1	; don't set carry if threshold >= abs(output)
        ;; stack + 2
        fstp st0	; pop abs(output)
        ;; stack + 1 - st0: output
        fld dword [ebp + COMP_STATE_GAIN]
        ;; stack + 2 - st0: gain, st1: output
        jnc do_release		; jmp if threshold >= abs(output)

        ;; if output is above threshold, apply comp. attack (should be < 1.0 to reduce gain)
        fmul st4                ; attack
        jmp skip_release

do_release:
        ;; otherwise, apply comp. release (increase gain) if gain < 1
        fld1
        ;; stack + 3
        fcomip st1		; don't set cf if 1 >= gain
        ;; stack + 2
        jc skip_release
        fadd st5                ; release
skip_release:

        ;; st0: updated gain, st1: output signal
        fstp dword [ebp + COMP_STATE_GAIN] ; store new gain (if not > 1)
        ;; stack + 1

        ;; st0: output
        popa
compressor_done:
        ret
