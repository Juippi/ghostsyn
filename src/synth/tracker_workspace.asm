;;; Max. number of modules, modules & patterns when running with
;;; tracker, should be enough for any reasonable 4k tune.
;; %define TRACKER_MAX_MODULES 64
%define TRACKER_MAX_MODULES 24 ; test

%define TRACKER_MAX_ORDER 64
%define TRACKER_MAX_PATTERNS 64

%define MAX_NUM_ROWS 128 ; reserve space for max. this many rows per pattern
%define MAX_NUM_TRACKS 8 ; reserve space for max. this many tracks in a pattern

;;; tracker build has all module types and features always enabled
%define ENABLE_OSCILLATOR
%define ENABLE_FILTER
%define ENABLE_FILTER_MODE_HP
%define ENABLE_ENVELOPE
%define ENABLE_COMPRESSOR
%define ENABLE_CHORUS
%define ENABLE_REVERB
%define ENABLE_SILENCE

%define NUM_TRACKS MAX_NUM_TRACKS
%define NUM_MODULES TRACKER_MAX_MODULES

;;; size of constant & accumulator area
;;; reserve enough constants for distinct values for max. no. of modules
;;; plus one accumulator for each module's output, should be enough
%define WORKBUF_DWORDS TRACKER_MAX_MODULES * (4 + 1)
%define WORKBUF_BYTES WORKBUF_DWORDS * 4

;;; length of single tracker tick in audio frames
song_ticklen:
        dd 0

;;; length of pattern (number of rows)
num_rows:
        dd 0
;;; number of tracks
num_tracks:
        dd 0

;;; no. of constantants in workbuf that should not be zeroed for new sample
const_bytes:
        dd 0

;;; master high boost filter parametrs
master_hb_c1:
        dd 0
master_hb_c2:
        dd 0
master_hb_mix:
        dd 0

order:
        times TRACKER_MAX_ORDER db 0

patterns:
        times (MAX_NUM_TRACKS * MAX_NUM_ROWS * TRACKER_MAX_PATTERNS) db 0

trigger_points:
        ;; write note pitch (add) in these positions when triggering instrument
        times (MAX_NUM_TRACKS * 2 * 4) dd 0

;;; modules must immediately precede workbuf (they're copied as one chunk
;;; in synth init)
modules:
        times (MODULE_DATA_BYTES * TRACKER_MAX_MODULES) db 0

;;; workbuf structure:
;;;
;;; for each track:
;;;
;;; modules <2 * dd / module>, not zeroed, keeps state shared between channels
;;; constant values <32 * dd>, not zeroed
;;; accumulators, zeroed for each sample <32 * dd>
;;;
;;; separate bss buf for per-channel state
;;;
;;; before sample: zero all accumulators
;;;
;;; TODO: add module flag to always add output to accumulators of LAST track/channel,
;;; instead of the current

workbuf:
        times WORKBUF_BYTES db 0

module_skip_flags:
;;; HACK: we always reserve extra space here because tracker_data.cpp
;;; has 64 for max_num_modules
        times (MAX_NUM_TRACKS * TRACKER_MAX_MODULES * 64) db 0

%define MODULES_TOTAL_SIZE MODULES_DATA_BYTES * TRACKER_MAX_MODULES + WORKBUF_BYTES
