;;;  AD envelope generator
;;;  - linear attack
;;;  - switch at defined gain level
;;;  - exponential decay
;;;

;;; params in stack upon entry

;;; st0: env stage: st0 != 0 -> Attack, st0 == 0 -> Decay
;;; st1: a->d switch level
;;; st2: attack add
;;; st3: exp decay multiplier

;;; TODO should one param be optionally integer too, and delivered
;;; here in a register in addition to pushing it to fpu stack
;;; or just 3 float params & 1 int param always?

;;; ebp
%define ENV_STATE_LEVEL    0  ; current level

module_envelope:
        pusha

        ;; update zero flag according to value of env state in st0
        fldz
        ;; stack +1
        fcomip st1
        ;; stack +0

        ;; load envelope level
        fld dword [ebp + ENV_STATE_LEVEL]
        ;; stack +1

        jz .no_attack
        fadd st3                ; add linear attack
        jmp .no_decay
.no_attack:
        fmul st4                ; apply exp. decay
        ;; add small constant to prevent denormals during exp. decay
        ;; fadd dword [small]
.no_decay:
        ;; store new env level
        fst dword [ebp + ENV_STATE_LEVEL]

        ;; check if we need to switch to decay
        fld st2                 ; load env a->d switch level
        fcomip
        jnc no_switch
decay_switch:
        ;; switch to decay
        fldz
        ;; stack +2
        fxch st2
        fstp st0
        ;; stack +1
no_switch:

        ;; output in st0
        popa
envelope_end:
        ret
