;;; params in stack upon entry

;;; st0: add
;;; st1: gain
;;; st2: freq mod
;;; st3: osc2 detune

;;; state (ebp)
%define OSC_STATE_CTR1	   0
%define OSC_STATE_CTR2	   4

module_oscillator:
        pusha

%ifdef OSC_ENABLE_FREQ_MOD
;;; freq mod
        fld1
        ;; stack + 1
        fadd st3
        fmul dword [bss_stereo_factor] ; stereo detune
        fmul st1
        ;; st0 now osc add before possible osc2 detune
%else
        fld st0
        fmul dword [bss_stereo_factor] ; stereo detune
%endif

;;; oscillator 1
        fld1
        ;; stack + 2 - st0: 1, st1: osc add
        fld dword [ebp + OSC_STATE_CTR1]
        ;; stack + 3 - st0: ctr1, st1: 1, st2: osc add
        fadd st2
        fprem1                  ; st0 = st0 % 1
        fst dword [ebp + OSC_STATE_CTR1]
;;; oscillator 2
        fxch st0, st2
        ;; stack + 3 - st0: osc add, st1: 1, st2: ctr1
        fmul st6                ; osc2 detune
        fadd dword [ebp + OSC_STATE_CTR2]
        fprem1
        fst dword [ebp + OSC_STATE_CTR2]
        ;; stack + 3 - st0: ctr2, st1: 1, st2: ctr1

;;; stack cleanup
        fxch st0, st1           ; move 1 to top
        fstp st0                ; and remove
        ;; stack + 2
        faddp                   ; sum osc1 and osc2 for output
        ;; stack + 1

;;; osc shape
%ifdef OSC_ENABLE_SINE
        mov eax, [esi]
        test eax, OSC_FLAG_SINE
        jz osc_no_sin
        fsin
osc_no_sin:
%endif
        fmul st2                ; gain

        ;; output in st0, params remain in st1 - st4

        popa
osc_end:
        ret
