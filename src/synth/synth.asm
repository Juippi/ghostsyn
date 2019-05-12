global synth

%ifdef TRACKER_EMBED
global synth_update
%endif

;;; %define ENABLE_MASTER_HIGHBOOST
;;; %define ENABLE_END_FADE
%define OSC_ENABLE_FREQ_MOD
%define ENABLE_END_MOD

section .data

;;; global vol, faded at the end
%ifdef ENABLE_END_FADE
global_volume:
        dd 1.0
end_fade:
        dd 0.99998
%endif

%ifdef ENABLE_MASTER_HIGHBOOST
;;; state for master high boost filter
;;; TODO: move to bss
master_hb_state:
        dd 0.0
        dd 0.0
%endif ;; ENABLE_MASTER_HIGHBOOST

;;; other constants
stereo_mod:
        dd 1.003	; adjustment for 2nd ch. TODO: could be osc param?
main_tune:
        ;; dd 0.00018539226566085504 ; 440Hz tune for 44.1KHz
        dd 0.0007415690626434202 ; transpose trick 2 octaves
;;; filter constants
pw:
        dd 1.5			; filter peak width

;;; envelope constants
oct_semitones:                  ; dword constant, partially overlaps with following
        db 12
        db 0
;; small:        ;;  small constant for denormal killing
        ;; db 0
        ;; db 0
        ;; db 0x80
        ;; db 0
;;; custom global freqmod effect at the end
%ifdef ENABLE_END_MOD
end_mod_add_ch:
        ;; ~0.00000002
        db 0
        db 0
        db 0xa0
        db 0x32
end_mod_add:
        ;; ~0.000003
        db 0
        db 0
        db 0x80
        db 0x36
end_mod:
        db 0
        db 0
        db 0x58
        db 0x3f
%endif

%include "constants.asm"

%ifdef TRACKER_EMBED
        ;; include empty data for tracker use
%include "tracker_workspace.asm"
%else
        ;; include tracker-generated asm version
%include "song.asm"
%endif

;;; Module structure (8 bytes/module)
;;;
;;; 0 - module type & flags
;;; 1 - module output dst idx in routing arrah
;;; 2 - module input/param 1 idx in routing array
;;; 3 - module input/param 2 idx in routing array
;;; 4 - module input/param 3 idx in routing array
;;; 5 - module input/param 4 idx in routing array

;;; module types
%define MODULE_TYPE_OSC        0x00
%define MODULE_TYPE_FILTER     0x01
%define MODULE_TYPE_ENV        0x02
%define MODULE_TYPE_COMPRESSOR 0x03
%define MODULE_TYPE_REVERB      0x04
%define MODULE_TYPE_CHORUS      0x05

;;; special type used for masking unused modules in non-compact builds
%define MODULE_TYPE_SILENCE       0x0f

;;; Stereo flag. If set, L and R channel has independent state at *ebp for
;;; channels (but param area at *esi is shared). If not set, both param area
;;; and state are shared.
%define MODULE_FLAG_STEREO  0x80

;;; Oscillator flags
%define OSC_FLAG_SINE 0x100
%define OSC_FLAG_HSIN 0x200
%define OSC_FLAG_SQUARE 0x400
%define OSC_FLAG_NOISE 0x800

;;; Filter flags
%define FLT_FLAG_HP      0x40

;;; Element data area

;;; We replace .bss with g_buffer + N references in converted
;;; intro version, so define constants for it. These must match
;;; offsets of corresponding vars in bss (as they would be in
;;; a build without TRACKER_EMBED)

%define BOFF_ORDER_POS     0
%define BOFF_PATTERN_POS   4
%define BOFF_STEREO_FACTOR 8
%define BOFF_TEMP1         12
%define BOFF_MODULES       16
%define BOFF_WORKBUF_ACC1  16 + NUM_MODULES * MODULE_DATA_BYTES + CONST_BYTES
%define BOFF_WORK          16 + NUM_TRACKS * (NUM_MODULES * MODULE_DATA_BYTES + WORKBUF_BYTES)

section .bss

%ifdef TRACKER_EMBED
state_setup_done: resd 1
state_tick_ctr:	  rest 1
%endif

g_buffer:			; for non-intro builds from AT&T syntax

bss_order_pos:
        resd 1
bss_pattern_pos:
        resd 1
bss_stereo_factor:
        ;; extra modulation source that varies per channel, to create stereo diff
        resd 1
        ;; misc temp vars
bss_temp1:
        resd 1

;; copy of all modules per track, + work area per channel
%define STATE_BYTES_PER_CHANNEL MODULE_WORK_BYTES * 2

;;;
;;; Work area structure:
;;;
;;; NUM_TRACKS * [element set]
;;; NUM_TRACKS * NUM_MODULES * [work area]
;;;
;;; must use resd instead of resb here because the at&t converted doesn't
;;; understand resb
%ifdef TRACKER_EMBED
bss_modules:
        resb NUM_TRACKS * (NUM_MODULES * MODULE_DATA_BYTES + WORKBUF_BYTES)
%else
bss_modules:
        resb NUM_MODULES * MODULE_DATA_BYTES
        resb CONST_BYTES
bss_workbuf_acc1:
        resb (NUM_TRACKS * (NUM_MODULES * MODULE_DATA_BYTES + WORKBUF_BYTES) - (NUM_MODULES * MODULE_DATA_BYTES + CONST_BYTES))

%endif

bss_work:
        ;; add some extra for work space of poss. last delay element
        resd STATE_BYTES_PER_CHANNEL * 2 * NUM_TRACKS * NUM_MODULES / 4

section .text

%include "module_oscillator.asm"

%ifdef ENABLE_FILTER
%include "module_filter_v2.asm"
%endif ;; ENABLE_FILTER

%ifdef ENABLE_ENVELOPE
%include "module_envelope.asm"
%endif ;; ENABLE_ENVELOPE

%ifdef ENABLE_COMPRESSOR
%include "module_compressor.asm"
%endif ;; ENABLE_COMPRESSOR

%ifdef ENABLE_REVERB
%include "module_reverb.asm"
%endif ;; ENABLE_REVERB

%ifdef ENABLE_CHORUS
%include "module_chorus.asm"
%endif ;; ENABLE_CHORUS

;;;
;;; synth main
;;;
;;; regparm convention, args:
;;; eax: output addr
;;; edx: no. of frames to render (stereo floats, 8 bytes each)
;;; ecx: outptr for playback state (tracker build only)
synth:
        pusha

;;; store playback state for tracker
%ifdef TRACKER_EMBED
        pusha
        mov ebx, [bss_order_pos]
        xor eax, eax
        mov al, [order + ebx]
        mov [ecx], eax
        mov eax, [bss_pattern_pos]

        ;; eax = eax / NUM_TRACKS
        push edx
        xor edx, edx
%ifdef TRACKER_EMBED
        mov edi, [num_tracks]
%else
        mov edi, NUM_TRACKS
%endif
        idiv edi
        pop edx

        mov [ecx + 4], eax
        popa
%endif

        ;; reorder register args to proper places
        ;; TODO: make this unnecessary
        mov edi, eax
        mov ecx, edx

;;;
;;; Synth initial setup
;;;
;;; create copies of modules data for each track

        pusha
%ifdef TRACKER_EMBED
        ;; setup must be done only once after synth_update() has been called
        mov eax, [state_setup_done]
        test eax, eax
        jnz skip_setup
%endif

;;; make one copy of module data set for each track
        lea edi, [bss_modules]       ; module data is copied here

%ifdef TRACKER_EMBED
        mov ecx, [num_tracks]
%else
        mov ecx, NUM_TRACKS
%endif

setup_loop:
        push ecx
        lea esi, [modules]
%ifdef TRACKER_EMBED
        mov ecx, (MODULE_DATA_BYTES * TRACKER_MAX_MODULES + WORKBUF_BYTES)
%else
        mov ecx, (MODULE_DATA_BYTES * NUM_MODULES + WORKBUF_BYTES)
%endif
        rep movsb

        pop ecx
        loop setup_loop

;;;
;;; end synth initial setup
;;;

%ifdef TRACKER_EMBED
skip_setup:
        mov eax, 1
        mov [state_setup_done], eax
%endif
        popa

%ifdef TRACKER_EMBED
        mov edx, [state_tick_ctr]
%else
        xor edx, edx		; tick ctr
%endif

;;;
;;; Synth main loop, running once per output sample
;;; (ie. twice per stereo frame)
;;;
;;; registers:
;;;  edi - output write dst
;;;  edx - tick counter

synth_loop:
        push ecx

;;; special hack: end fade
;;; fade out at end
;;; TODO: perhaps this could be made optional?
%ifndef TRACKER_EMBED
%ifdef ENABLE_END_FADE
        cmp ecx, 0x20000
        jns .no_fade
        fld dword [global_volume]
        fmul dword [end_fade]
        fstp dword [global_volume]
.no_fade:
%endif

%ifdef ENABLE_END_MOD
        cmp ecx, 0x60000
        jns no_end_mod
        fld dword [end_mod_add]
        fadd dword [end_mod_add_ch]
        fst dword [end_mod_add]
        fadd dword [end_mod]
        fstp dword [end_mod]
no_end_mod:
%endif

%endif                          ; TRACKER_EMBED

;;;
;;; Zero the accumulators in our signal array
;;;
        pusha

%ifdef TRACKER_EMBED
        lea edi, [bss_modules + NUM_MODULES * MODULE_DATA_BYTES]
        add edi, [const_bytes]
%else
        lea edi, [bss_workbuf_acc1]
%endif

        xor eax, eax
        mov ecx, NUM_TRACKS
zero_loop:
        push ecx

%ifdef TRACKER_EMBED
        mov ecx, [const_bytes]
        shr ecx, 2              ; bytes -> dwords
        neg ecx
        add ecx, WORKBUF_DWORDS
        rep stosd
%else
        mov ecx, (WORKBUF_DWORDS - NUM_MODULE_CONSTANTS)
        rep stosd
%endif

%ifdef TRACKER_EMBED
        add edi, (NUM_MODULES * MODULE_DATA_BYTES)
        add edi, [const_bytes]
%else
        add edi, (NUM_MODULES * MODULE_DATA_BYTES + CONST_BYTES)
%endif

        pop ecx
        loop zero_loop

        popa
;;;
;;; Track loop
;;;
;;; Run once for each track, increments pattern pointer once for each on
;;; new tick, so that a whole pattern row is consumed
;;;
;;; registers:
;;;  esi - module data start address (start of current module in loop)
;;;  edi - module workbuf (constants & accumulators) start address
;;;  ebp - module work area start address
;;;

        ;; ptr to current module data
        lea esi, [bss_modules]
        ;; ptr to current module's per-channel work area
        lea ebp, [bss_work]

        push edi                ; save edi, used for ptr in tracks_loop

        ;; master out accumulator
        fldz

%ifdef TRACKER_EMBED
        mov ecx, [num_tracks]
%else
        mov ecx, NUM_TRACKS
%endif
tracks_loop:
        push ecx

        ;; points to current track's const & accum area
        lea edi, [esi + MODULE_DATA_BYTES * NUM_MODULES]
break2:
        ;;
        ;; song & pattern stuff here
        ;;
        test edx, edx
        jnz notick

        ;;
        ;; start of tick, can trigger new notes
        ;;

        ;; get pattern start
        mov ebx, [bss_order_pos]
        mov bl, [ebx + order]
%ifdef TRACKER_EMBED
        imul ebx, [num_rows]
        imul ebx, [num_tracks]
%else
        imul ebx, NUM_TRACKS * NUM_ROWS
%endif
        ;; add pattern offset
        add ebx, [bss_pattern_pos]

        ;; get note from pattern
        ;; don't zero eax here, we're only using al in pitch
        ;; calculation logic
        mov al, [ebx + patterns]
        cmp al, 0
        je notrig		 ; 0 -> emptry cell, don't trigger note at all

        ;;
        ;; Calculate osc pitch for current note
        ;;
        pusha			; save eax for trigger control, edx tick ctr

        mov cl, al
        shr cl, 4
        and cl, 0x07
        and al, 0x0f		; pitch, bits 0-7, bit 8 alt instr

        mov [bss_temp1], al
        fild dword [bss_temp1]
        fidiv dword [oct_semitones]

        ;; would work too, because even though both are integers, their ratio when
        ;; interpreted as floats is the same
        ;; fld dword [bss_temp1]
        ;; fdiv dword [oct_semitones]

        ;; 2^x for base pitch
        f2xm1
        fld1
        faddp

        ;; octave
        xor ebx, ebx
        inc ebx
        shl ebx, cl
        push ebx
        fimul dword [esp]
        pop ebx

        ;; synth main tune
        fmul dword [main_tune]

        popa			; restore eax for trigger ctr, edx tick ctr

        pop ecx			; get track no. for trigger control
        push ecx		; keep stored for loop end

        ;;
        ;; flexible triggering: trigger up to 4 osc/env modules
        ;;
        pusha

        ;; TODO: could just dec ecx and skip the following add, saves a couple
        ;; of bytes but requires reordering triggers in data
        neg ecx
%ifdef TRACKER_EMBED
        add ecx, [num_tracks]
%else
        add ecx, NUM_TRACKS
%endif
        ;; mult by 8: up to 4 triggers / instrument, 2 instruments per channel
        sal ecx, 3

        test al, 0x80
        jz no_alt_instr
        add ecx, 4
no_alt_instr:
        and al, 0x7f

        lea edx, [ecx + trigger_points]
        ;; edx now ptr to trigger point list for the instr for current note

        mov ecx, 4
trigger_loop:
        xor ebx, ebx
        mov bl, [edx]

        ;; TODO: how to do a nice note off?
        ;; cmp al, 0x01
        ;; jz no_trig          ; note off should not set osc pitch

        ;; trigger element
        ;; - for oscillator, this can set osc pitch (add / sample)
        ;; - for envelope, this can set env state to value between 0 and 1 (attack)
        ;; fst dword [esi + ebx * 4 + MODULE_DATA_BYTES * NUM_MODULES]
        fst dword [esi + ebx * 4+ MODULE_DATA_BYTES * NUM_MODULES] ; intel2gas hack, syntax rule breaks with space!

trig_done:
        inc edx
        loop trigger_loop

        popa

        fstp st0		; remove note pitch from fpu stack

notrig:				; no trigger note
        ;;
        ;; end of triggering
        ;;

        mov eax, [bss_pattern_pos]
%ifdef TRACKER_EMBED
        add eax, [num_rows]
        push ebx
        mov ebx, [num_rows]
        imul ebx, [num_tracks]
        add ebx, [num_rows]
        dec ebx
        cmp eax, ebx
        pop ebx
%else
        add eax, NUM_ROWS
        cmp eax, NUM_TRACKS * NUM_ROWS + NUM_ROWS - 1
%endif
        jne no_advance
        ;; advance order
        mov eax, [bss_order_pos]
        inc eax
        mov [bss_order_pos], eax
        xor eax, eax

no_advance:

        ;; if not, we may need to jump to the start of the next row
%ifdef TRACKER_EMBED
        push ebx
        mov ebx, [num_rows]
        imul ebx, [num_tracks]
        cmp eax, ebx
        jb no_nexttrack
        sub eax, ebx
        inc eax
no_nexttrack:
        pop ebx
%else
        cmp eax, NUM_TRACKS * NUM_ROWS
        jb no_nexttrack
        sub eax, NUM_TRACKS * NUM_ROWS
        inc eax
no_nexttrack:
%endif
        ;; save new pattern pos
        mov [bss_pattern_pos], eax

notick:				; no tick starting

        ;; process all modules
        mov ecx, NUM_MODULES

module_loop:
        push ecx

        ;; set zero flag with channel idx
        ;; mov eax, [orig_edi]
        mov eax, [esp + 8]      ; we have edi in stack behind 2 loop ctrs at this point
        test eax, 0x4

        ;; load element type & other properties
        mov eax, dword [esi]

        push ebp		; save ebp, in case incremented because of stereo

        fld1
        jz ch_left              ; jump acc. to ch idx

        ;; if stereo effect not enabled in osc flags, run both channels with same param
        test al, MODULE_FLAG_STEREO
        jz mono_only

        fmul dword [stereo_mod]
        add ebp, STATE_BYTES_PER_CHANNEL

mono_only:
ch_left:
        fstp dword [bss_stereo_factor]

        and al, 0x3f		; strip flags

        ;; Each module will run with the 4 parameters in stack (we push them here
        ;; and pop 4 after return from module).
        ;; Note that they'll be in the FPU stack in reverse order; last param in st0 etc.
breakx:
        xor ebx, ebx
        mov bl, [esi + 2]
        fld dword [edi + ebx * 4]

        xor ebx, ebx
        mov bl, [esi + 3]
        fld dword [edi + ebx * 4]

        xor ebx, ebx
        mov bl, [esi + 4]
        fld dword [edi + ebx * 4]

        xor ebx, ebx
        mov bl, [esi + 5]
        fld dword [edi + ebx * 4]
before_osc:

%ifdef ENABLE_OSCILLATOR
        cmp al, MODULE_TYPE_OSC
        jne no_osc
        call module_oscillator
no_osc:
%endif

%ifdef ENABLE_FILTER
        cmp al, MODULE_TYPE_FILTER
        jne no_filter
        call module_filter_v2
no_filter:
%endif

%ifdef ENABLE_ENVELOPE
        cmp al, MODULE_TYPE_ENV
        jne no_envelope
        call module_envelope
no_envelope:
%endif

%ifdef ENABLE_COMPRESSOR
        cmp al, MODULE_TYPE_COMPRESSOR
        jne no_compressor
        call module_compressor
no_compressor:
%endif

%ifdef ENABLE_REVERB
        cmp al, MODULE_TYPE_REVERB
        jne no_reverb
        call module_reverb
no_reverb:
        %endif

%ifdef ENABLE_CHORUS
        cmp al, MODULE_TYPE_CHORUS
        jne no_chorus
        call module_chorus
no_chorus:
%endif

%ifdef ENABLE_SILENCE
        cmp al, MODULE_TYPE_SILENCE
        jne no_silence
        fldz			; silence to master out
no_silence:
%endif

elem_done:
        ;; element output now in st0

        ;; get output array idx for module output
        mov bl, [esi + 1]
        cmp bl, 0
        jz to_master

        ;; TODO: here we could have an option to always add to last track's
        ;; workbuf instead: could use it for master out, feeding multiple tracks
        ;; to single nonlinear module etc.

        ;; add output to routing array destination

        fadd dword [edi + ebx * 4]
store_module_out:
        fstp dword [edi + ebx * 4]
        jmp not_to_master
to_master:
        faddp st5, st0
not_to_master:

        ;; pop module params off the fpu stack into their places
        ;; (allow module to modify them to enable modules to keep state that's common
        ;; for both channels)
        xor ebx, ebx
        mov bl, [esi + 5]
        fstp dword [edi + ebx * 4]

        xor ebx, ebx
        mov bl, [esi + 4]
        fstp dword [edi + ebx * 4]

        xor ebx, ebx
        mov bl, [esi + 3]
        fstp dword [edi + ebx * 4]

        xor ebx, ebx
        mov bl, [esi + 2]
        fstp dword [edi + ebx * 4]

        ;; next module
        add esi, MODULE_DATA_BYTES
        pop ebp			; restore ebp in case was adjusted for stereo
        add ebp, STATE_BYTES_PER_CHANNEL * 2 ; need to skip both channels' state

        ;;
        ;; end element loop
        ;;
        pop ecx
        dec ecx
        jnz module_loop

        add esi, WORKBUF_BYTES  ; skip constant/accumulator area

        ;;
        ;; end tracks loop
        ;;
        pop ecx
        dec ecx
        jnz tracks_loop
;;;
;;; End track loop
;;;
        pop edi                 ; restore edi (output write dst)

        ;; decrement tick counter
        dec edx			; update tick counter
        jns .no_newtick
%ifdef TRACKER_EMBED
        mov edx, [song_ticklen]
%else
        mov edx, SONG_TICKLEN
%endif
.no_newtick:

%ifdef TRACKER_EMBED
        mov [state_tick_ctr], edx
%endif

%ifdef ENABLE_END_FADE
        fmul dword [global_volume]
%endif

;;; amplitude mod effect at song end
%ifdef ENABLE_END_MOD
        fmul dword [pw]         ; 1.5
        fld dword [end_mod]
        fcos
        fmulp
%endif 

%ifdef ENABLE_MASTER_HIGHBOOST
        ;; high freq boost
        push edx

        mov edx, edi
        and edx, 0x04
        add edx, master_hb_state

        fld st0
        fld st0

        fmul dword [master_hb_c1]
        fld dword [edx]
        fmul dword [master_hb_c2]
        faddp

        fst dword [edx]
        fsubp
        fmul dword [master_hb_mix]
        faddp

        pop edx
        ;; end high freq boost
%endif ;; ENABLE_MASTER_HIGHBOOST

write_master_out:
        fstp dword [edi]	; store synth master output
        add edi, 4

        pop ecx
        dec ecx
        jnz synth_loop

        popa
        ret

%ifdef TRACKER_EMBED
;;;
;;; update synth data from memory
;;;
;;; normal args:
;;;  ptr to patterns
;;;  pattern bytes
;;;  ptr to order
;;;  order bytes
;;;  ptr to instruments
;;;  instrument bytes
;;;  ptr to trigger offsets
;;;  trigger offsets
synth_update:
        mov ebx, esp
        pusha

        mov esi, [ebx + 4]	; patterns
        mov ecx, [ebx + 8]
        mov edi, patterns
        rep movsb

        mov esi, [ebx + 12]	; order
        mov ecx, [ebx + 16]
        mov edi, order
        rep movsb

        ;; fill module array bu no-op silence outputting modules first
        ;; TODO. this causes them to run as oscillators, which breaks editor audio!
        ;; TODO. must ensure they're silent no-ops instead
        mov edi, modules
        mov ecx, NUM_MODULES
modules_init_loop:
        push ecx
        mov al, MODULE_TYPE_SILENCE
        stosb
        mov al, 0
        mov ecx, MODULE_DATA_BYTES - 1
        rep stosb
        pop ecx
        loop modules_init_loop

        mov esi, [ebx + 20]     ; modules
        mov ecx, [ebx + 24]
        mov edi, modules
        rep movsb

        mov esi, [ebx + 28]	; triggers
        mov ecx, [ebx + 32]
        mov edi, trigger_points
        rep movsb

        ;; zero workbuf just in case
        xor eax, eax
        mov edi, workbuf
        mov ecx, WORKBUF_BYTES
        rep stosb

        mov esi, [ebx + 36]	; initial values / constans for workbuf
        mov ecx, [ebx + 40]
        mov edi, workbuf
        rep movsb

        ;; no. of constants in workbuf
        mov esi, [ebx + 44]
        mov [const_bytes], esi

        ;; module count, we only process this many
        mov esi, [ebx + 48]
        ;; mov [tracker_mod_count], esi ; TODO: this needs some extra logic (in init code)

        ;; tracker tick len (tempo)
        mov esi, [ebx + 52]
        mov [song_ticklen], esi

        ;; number of rows in pattern
        mov esi, [ebx + 56]
        mov [num_rows], esi
        ;; number of tracks
        mov esi, [ebx + 60]
        mov [num_tracks], esi

        ;; start playback from this pattern row
        mov esi, [ebx + 64]
        imul esi, [num_tracks]
        mov [bss_pattern_pos], esi

        ;; master high boost filter params
        mov esi, [ebx + 68]
        mov [master_hb_c1], esi
        mov esi, [ebx + 72]
        mov [master_hb_c2], esi
        mov esi, [ebx + 76]
        mov [master_hb_mix], esi

        ;; per-channel module skip flags
        mov esi, [ebx + 80]
        mov ecx, [ebx + 84]
        mov edi, module_skip_flags
        rep movsb

        xor eax, eax
        mov [bss_order_pos], eax
        mov [state_tick_ctr], eax ; this only exists in tracker build

        ;; reset synth state by zeroing work area
        mov edi, bss_work
        mov ecx, STATE_BYTES_PER_CHANNEL * 2 * NUM_TRACKS * NUM_MODULES
        rep stosb

        ;; need to set up work area again
        xor eax, eax
        mov [state_setup_done], eax

        popa
        ret

%endif
