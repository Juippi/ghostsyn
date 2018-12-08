global synth

%ifdef TRACKER_EMBED
global synth_update
%endif

section .data

;;; global vol, faded at the end
global_volume:
	dd 1.0
end_fade:
	dd 0.99998

;;; state for master high boost filter
;;; TODO: move to bss
master_hb_state:
	dd 0.0
	dd 0.0

;;; other constants
stereo_mod:
	dd 1.003	; adjustment for 2nd ch. TODO: could be osc param?
main_tune:
	dd 0.00018539226566085504 ; 440Hz tune for 44.1KHz

oct_semitones:
	dd 12		; number of notes per octave

	%define	MODULE_DATA_SIZE 5
	%define MODULE_DATA_BYTES MODULE_DATA_SIZE * 4
	%define MODULE_WORK_BYTES 65536 * 4

	%define NUM_TRACKS 8

%ifdef TRACKER_EMBED
	;; include empty data for tracker use
	%include "tracker_workspace.asm"
%else
	;; include tracker-generated asm version
	%include "song.asm"
%endif

;;; oscillator flags
	%define OSC_FLAG_SINE 0x10000
	%define OSC_FLAG_HSIN 0x20000
	%define OSC_FLAG_SQUARE 0x40000
	%define OSC_FLAG_NOISE 0x80000

	%define OSC_FLAG_STEREO  0x80

	;; module types (byte 0 of module type word)
	%define MODULE_TYPE_OSC        0x00
	%define MODULE_TYPE_FILTER     0x01
	%define MODULE_TYPE_ENV        0x02
	%define MODULE_TYPE_COMPRESSOR 0x03
	%define MODULE_TYPE_REVERB      0x04
	%define MODULE_TYPE_CHORUS      0x05
	;; special type used for masking unused modules in non-compact builds
	%define MODULE_TYPE_SILENCE       0x0f

	;; output ops (byte 1 of module type word)
	%define MODULE_OUT_OP_SET  0
	%define MODULE_OUT_OP_ADD  1
	%define MODULE_OUT_OP_MULT 2

;;; Element data area

	;; We replace .bss with g_buffer + N references in converted
	;; intro version, so define constants for it. These must match
	;; offsets of corresponding vars in bss (as they would be in
	;; a build without TRACKER_EMBED)
	%define BOFF_ORDER_POS     0
	%define BOFF_PATTERN_POS   4
	%define BOFF_STEREO_FACTOR 8
	%define BOFF_MASTER_OUT	   12
	%define BOFF_TEMP1         16
	%define BOFF_MODULES       20
	%define BOFF_WORK          20 + NUM_TRACKS * NUM_MODULES * MODULE_DATA_BYTES

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

bss_master_out:
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
bss_modules:
	resd NUM_TRACKS * NUM_MODULES * MODULE_DATA_BYTES / 4
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
;;; create copies of modules for each track

	pusha
%ifdef TRACKER_EMBED
	;; setup must be done only once after synth_update() has been called
	mov eax, [state_setup_done]
	test eax, eax
	jnz skip_setup
%endif

	lea ebx, [module_skip_flags]	; track idx for module masking

	lea edi, [bss_modules]	; module set for each track goes here
%ifdef TRACKER_EMBED
	mov ecx, [num_tracks]
%else
	mov ecx, NUM_TRACKS
%endif
setup_loop:
	push ecx

%if 0
;;; compact version, no module masking
	;; make one copy of the element set for each track (
	lea esi, [modules]
	mov ecx, MODULE_DATA_BYTES * NUM_MODULES
	rep movsb
%else
;;; per-module copy version with module masking for != 4k builds
	lea esi, [modules]
	mov ecx, NUM_MODULES
_module_loop:
	push ecx

	mov edx, edi		; save original edi (1st byte of module)

	mov ecx, MODULE_DATA_BYTES
	rep movsb

	;; if mask byte != 0, disable module
	mov al, [ebx]
	cmp al, 0
	je _no_mask
	mov dword [edx], MODULE_TYPE_SILENCE ; just outputs 0
_no_mask:

	inc ebx			; track idx for module masking
	pop ecx
	loop _module_loop
%endif

	pop ecx
	loop setup_loop

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
;;; Synth main loop
;;;
;;; registers:

synth_loop:
	push ecx

%ifndef TRACKER_EMBED
;;;
;;; special hack: end fade
;;; fade out at end

	cmp ecx, 0x20000
	jns .no_fade
	fld dword [global_volume]
	fmul dword [end_fade]
	fstp dword [global_volume]
.no_fade:
%endif

	fldz			; master out accumulator

;;;
;;; Track loop
;;;
;;; Run once for each track, increments pattern pointer once for each on
;;; new tick, so that a whole pattern row is consumed
;;;

	;; load first track's module & area pointer, will stay in esi for duration of loop
	lea esi, [bss_modules]
	;; work area pointer for module
	lea ebp, [bss_work]

%ifdef TRACKER_EMBED
	mov ecx, [num_tracks]
%else
	mov ecx, NUM_TRACKS
%endif
tracks_loop:
	push ecx

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
	mov eax, [bss_pattern_pos]
	add ebx, eax
	;; add channel

	xor eax, eax
	mov al, [ebx + patterns]
	cmp al, 0
	je notrig		 ; 0 -> emptry cell, don't trigger note at all

	;; trigger envelope

	;;
	;; Set osc pitch for channel
	;;
	pusha			; save eax for trigger control, edx tick ctr

	and al, 0x7f		; pitch, bits 0-7, bit 8 alt instr

	mov bl, 12
	div bl			; quotient to al, remainder to ah
	mov [bss_temp1], ah

	;; 2 fld:s would work here too because of reasons
	fild dword [bss_temp1]
	fild dword [oct_semitones]
	fdivp

	f2xm1
	fld1
	faddp

	;; octave
	mov ebx, 1
	mov cl, al
	shl ebx, cl
	mov [bss_temp1], ebx
	fild dword [bss_temp1]
	fmulp

	;; synth main tune
	fld dword [main_tune]
	fmulp

	popa			; restore eax for trigger ctr, edx tick ctr

	pop ecx			; get track no for trigger control
	push ecx		; keep stored for loop end

%if 0		  ; compact triggering
	;;
	;; trigger primary or alternate instrument for track
	;;
	pusha

	neg ecx
	add ecx, 4
	imul ecx, 8

	test al, 0x80
	jz no_alt_instr
	add ecx, 4
no_alt_instr:

	;; trigger point value is just a byte offset from start of source elemnt to start of dest.
	add ecx, trigger_points
	;; ecx == &trigger_points[track_idx]
	mov ecx, [ecx]

	add esi, ecx

	cmp al, 1
	je noteoff

	;; we trigger an envelope element, and set pitch of
	;; immediately following osc element
	mov dword [esi + ENV_PARAM_STAGE], ENV_STAGE_ATTACK
	fstp dword [esi + MODULE_DATA_BYTES + OSC_PARAM_ADD]

	;;
	;; TEST: only set one osc for testing
	fstp dword [esi + OSC_PARAM_ADD]

	jmp no_noteoff
noteoff:
	;; switch envelope to release phase
	mov dword [esi + ENV_PARAM_STAGE], 2
no_noteoff:

	popa

%else	      ; complex triggering
	;;
	;; flexible triggering: trigger up to 4 osc/env modules
	;;
	pusha

	neg ecx
%ifdef TRACKER_EMBED
	add ecx, [num_tracks]
%else
	add ecx, NUM_TRACKS
%endif
	imul ecx, (2 * 4 * 4)	; up to 4 triggers / instrument, 2 instruments per channel

	test al, 0x80
	jz no_alt_instr
	add ecx, (4 * 4)
no_alt_instr:

	mov edx, ecx
	add edx, trigger_points

	mov ecx, 4
trigger_loop:
	mov eax, [edx]

	;; lowest 2 bits of offset tell if we trigger osc, env or nothing
	;; we can zero them after that since element offset is always divisible by 4
	test al, 0x01
	jz no_trig_osc

	;; trigger oscillator (set pitch)
	and eax, 0xfffffffc
	add eax, esi
	fst dword [eax + OSC_PARAM_ADD]
	jmp trig_done
no_trig_osc:

	test al, 0x02
	jz no_trig_env

	;; trigger envelope (set stage to 1)
	and eax, 0xfffffffc
	add eax, esi
%ifdef ENABLE_ENVELOPE
	mov dword [eax + ENV_PARAM_STAGE], ENV_STAGE_ATTACK
%endif ;; ENABLE_ENVELOPE

	and eax, 0xfffffffc
no_trig_env:

trig_done:
	add edx, 4
	loop trigger_loop

	popa

	fstp st0		; remove note pitch from fpu stack
%endif
notrig:				; no trigger note
	;;
	;; end of triggering
	;;

	mov eax, [bss_pattern_pos]
	inc eax
%ifdef TRACKER_EMBED
	push ebx
	mov ebx, [num_rows]
	imul ebx, NUM_TRACKS
	cmp eax, ebx
	pop ebx
%else
	cmp eax, NUM_TRACKS * NUM_ROWS
%endif
	jne no_advance
	;; advance order
	mov eax, [bss_order_pos]
	inc eax
	mov [bss_order_pos], eax
	xor eax, eax
no_advance:
	mov [bss_pattern_pos], eax

notick:				; no tick starting


	;; process all modules
	mov ecx, NUM_MODULES

element_loop:
	push ecx

	;;; call modules according to types
	mov eax, dword [esi] ; load element type

	push ebp		; save ebp, in case incremented because of stereo

	;; stereo things
	test edi, 0x4
	fld1
	jz .ch_left

	;; if stereo effect not enabled in osc flags, run both channels with same param
	test al, OSC_FLAG_STEREO
	jz .mono_only

	fmul dword [stereo_mod]

	add ebp, STATE_BYTES_PER_CHANNEL
.mono_only:
.ch_left:
	fstp dword [bss_stereo_factor]

	and al, 0x7f		; strip stereo flag

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

	;; element output now in st0
	xor ebx, ebx
	mov bl, [esi + 3]
	test ebx, ebx
	jnz not_master_out

	;; add to master (TODO: keep master out in fpu stack?)
	faddp
	jmp was_master_out

not_master_out:
	add ebx, esi

	;; set, add or multiply target
	;; TODO: add ifdefs so that only ones used are compiled in

	cmp ah, MODULE_OUT_OP_SET
	je no_op

	fld dword [ebx]

	cmp ah, MODULE_OUT_OP_MULT
	jne no_op_mult
	fmulp
no_op_mult:

	cmp ah, MODULE_OUT_OP_ADD
	jne no_op_add
	faddp
no_op_add:
no_op:
	fstp dword [ebx]

was_master_out:

	;; next element
	add esi, MODULE_DATA_BYTES
	pop ebp			; restore ebp in case was adjusted for stereo
	add ebp, STATE_BYTES_PER_CHANNEL * 2

	;;
	;; end element loop
	;;
	pop ecx
	dec ecx
	jnz element_loop

	;;
	;; end tracks loop
	;;
	pop ecx
	dec ecx
	jnz tracks_loop
;;;
;;; End track loop
;;;

	;; decrement tick counter
	;; TODO: would this fit in a reg?
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

	fmul dword [global_volume]

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

	fstp dword [edi]	; store synth output
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
	mov eax, esp
	pusha

	mov esi, [eax + 4]	; patterns
	mov ecx, [eax + 8]
	mov edi, patterns
	rep movsb

	mov esi, [eax + 12]	; order
	mov ecx, [eax + 16]
	mov edi, order
	rep movsb

	mov esi, [eax + 20]     ; instruments
	mov ecx, [eax + 24]
	mov edi, modules
	rep movsb

	mov esi, [eax + 28]	; triggers
	mov ecx, [eax + 32]
	mov edi, trigger_points
	rep movsb

	;; module count, we only process this many
	mov esi, [eax + 36]
	;; mov [tracker_mod_count], esi ; TODO: this needs some extra logic (in init code)

	;; tracker tick len (tempo)
	mov esi, [eax + 40]
	mov [song_ticklen], esi

	;; number of rows in pattern
	mov esi, [eax + 44]
	mov [num_rows], esi
	;; number of tracks
	mov esi, [eax + 48]
	mov [num_tracks], esi

	;; start playback from this pattern row
	mov esi, [eax + 52]
	imul esi, NUM_TRACKS
	mov [bss_pattern_pos], esi

	;; master high boost filter params
	mov esi, [eax + 56]
	mov [master_hb_c1], esi
	mov esi, [eax + 60]
	mov [master_hb_c2], esi
	mov esi, [eax + 64]
	mov [master_hb_mix], esi

	;; per-channel module skip flags
	mov esi, [eax + 68]
	mov ecx, [eax + 72]
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
