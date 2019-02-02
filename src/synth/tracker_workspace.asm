;;; Max. number of modules, modules & patterns when running with
;;; tracker, should be enough for any reasonable 4k tune.
%define TRACKER_MAX_MODULES 64
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

%define NUM_MODULES TRACKER_MAX_MODULES

;;; length of single tracker tick in audio frames
song_ticklen:
	dd 0

;;; length of pattern (number of rows)
num_rows:
	dd 0
;;; number of tracks
num_tracks:
	dd 0

;;; master high boost filter parametrs
master_hb_c1:
	dd 0
master_hb_c2:
	dd 0
master_hb_mix:
	dd 0

modules:
	times (MODULE_DATA_BYTES * TRACKER_MAX_MODULES) dd 0

order:
	times TRACKER_MAX_ORDER db 0

patterns:
	times (MAX_NUM_TRACKS * MAX_NUM_ROWS * TRACKER_MAX_PATTERNS) db 0

trigger_points:
	;; each track can trigger two instruments.
	;; triggering happens as follows:
	;;
	;;  - set pitch of osc at module <trigger point>
	;;  - reset phase of env at module <trigger point + 1>
	;;
	;; or alternatively, 4 independent triggers per instrument
	;; in non-compact mode
	times (MAX_NUM_TRACKS * 2 * 4) dd 0

module_skip_flags:
	times (MAX_NUM_TRACKS * TRACKER_MAX_MODULES) db 0
