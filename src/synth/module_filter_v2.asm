;;; params in stack upon entry

;;; st0: input
;;; st1: cutoff
;;; st2: feedback gain
;;; st3: <unused>

;;; ebp
%define FLT1_STATE_Y	   0
%define FLT2_STATE_Y	   4

module_filter_v2:
        pusha

        fld st1                 ; cutoff
        ;; stack + 1
        ;; fabs			; safety

        fld st3                 ; feedback
        ;; stack + 2

        ;; Could scale feedback according to cutoff for more 'classical'
        ;; sound, but let's skip that this time
        ;; fld st0			; st0 == st1 == cutoff
        ;; fmul dword [esi + FLT_PARAM_FB]
        ;; fldl2e
        ;; fmulp
        ;; st0: scaled feedback, st1: flt cutoff

        ;; add input to feedback
        fld dword [ebp + FLT1_STATE_Y]
        ;; stack + 3
        fsub dword [ebp + FLT2_STATE_Y] ; flt1_out - flt2_out
        fmulp			; *= feedback
        ;; stack + 2
        fsin			; waveshape feedback signal

        fadd st2                ; input
        ;; stack + 2 - st0: (input + feedback), st1: flt cutoff

        ;; 1st LP filter
        fmul st1		; st0 = st0 * st1
        fld1
        ;; stack + 3
        fsub st2
        fmul dword [ebp + FLT1_STATE_Y]
        faddp
        ;; stack + 2
        fst dword [ebp + FLT1_STATE_Y]

        add ebp, 4		; advance ebp to 2nd filter state
        ;; cutoff of 2nd filter
        fld st1
        ;; stack + 3
        fmul dword [pw]
        fstp st2
        ;; stack + 2

        ;; 2nd LP filter
        fmul st1
        fld1
        ;; stack + 3
        fsub st2
        fmul dword [ebp + FLT1_STATE_Y] ; actually FLT2_STATE_Y now
        faddp
        ;; stack + 2
        fst dword [ebp + FLT1_STATE_Y]

        ;; get rid of cutoff in st1, leave 2nd flt output to st0
        fxch st0, st1
        fstp st0
        ;; stack + 1

%ifdef ENABLE_FILTER_MODE_HP
        ;; if HP flag set, turn filter into hp (& notch) by subtracting original input
        mov al, [esi]
        test al, FLT_FLAG_HP
        jz .no_hp
        fsub st1
.no_hp:
%endif

        popa
        ret
