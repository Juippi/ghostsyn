 ;;; synth test stuff

%define NUM_TRACKS 1
%define NUM_ROWS 16

%define ENABLE_OSCILLATOR
%define ENABLE_ENVELOPE
%define ENABLE_FILTER
%define ENABLE_REVERB
%define ENABLE_SILENCE

%define SONG_TICKLEN 10000

order:
        db 0, 0, 0, 0, 0, 0, 0, 0

patterns:
        db 20, 0, 30, 20, 0, 30, 20, 0
        db 0, 0, 0, 0, 0, 0, 0, 0

%define NUM_MODULES 2
        
modules:
        ;; 0
        db 2                    ; type (env)
        db 0                    ; flags
        db 0                    ; input (unused)
        db 44                   ; output (accum 6)

        db 12                   ; exp decay multiplier
        db 4                    ; attack add (const 2)
        db 4                    ; a-> switch level (const 2)
        db 16                   ; env state (const 5)

        ;; 1
        db 0                    ; type (osc)
        db 0                    ; flags
        db 0                    ; input (unused)
        db 24                   ; output (1st accumulator)

        db 0                    ; osc2 detune (1st constant)
        db 0                    ; freq mod (1st constant)
        db 44                   ; gain (accum 6)
        db 8                    ; add (3rd constant)

%define WORKBUF_LEN 12           ; total, in dwords
        
%define NUM_MODULE_CONSTANTS 6   ; 6, in dwords
%define CONST_BYTES NUM_MODULE_CONSTANTS * 4
        
%define WORKBUF_BYTES WORKBUF_LEN * 4

%define MODULES_TOTAL_SIZE (MODULE_DATA_SIZE * NUM_MODULES + WORKBUF_BYTES)

        ;; TODO: what's the proper ratio of consts/accumulators? 50-50 or 75-25?
workbuf:
        ;; 0
        dd 0.0                  ; const 1
        ;; 4
        dd 0.2                  ; const 2
        ;; 8
        dd 0.01                 ; const 3
        ;; 12
        dd 0.9998               ; const 4
        ;; 16
        dd 0                    ; const 5
        ;; 20
        dd 0                    ; const 6
        ;; 24
        dd 0                    ; accum 1
        ;; 28
        dd 0                    ; accum 2
        ;; 32
        dd 0                    ; accum 3
        ;; 26
        dd 0                    ; accum 4
        ;; 40
        dd 0                    ; accum 5
        ;; 44
        dd 0                    ; accum 6

module_skip_flags:
        db 0, 0, 0, 0, 0, 0, 0, 0

trigger_points:
        ;; ch instr 0
        dd 8 + 1                 ; set const 3 (osc add) to pitch
        dd 16 + 1               ; set const 5 (env state) to attack

        db 0, 0
        ;; ch instr 1
        dd 0, 0, 0, 0

