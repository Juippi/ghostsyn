;;; Max. number of modules, elements & patterns when running with
;;; tracker, should be enough for any reasonable 4k tune.
%define TRACKER_MAX_MODULES 64
%define TRACKER_MAX_ORDER 64
%define TRACKER_MAX_PATTERNS 64

;;; in tracker build, this is the max. number of modules we reserve
;;; space for, and tracker_mod_count stores the number of modules
;;; we actually run
%define NUM_MODULES 32

;;; tracker build has all module types always enabled
%define ENABLE_OSCILLATOR
%define ENABLE_FILTER
%define ENABLE_ENVELOPE
%define ENABLE_COMPRESSOR
%define ENABLE_CHORUS
%define ENABLE_DELAY
%define ENABLE_SILENCE	

song_ticklen:
	dd 0

elements:
	times (MODULE_DATA_BYTES * TRACKER_MAX_MODULES) dd 0

order:
	times TRACKER_MAX_ORDER db 0

patterns:
	times (NUM_TRACKS * NUM_ROWS * TRACKER_MAX_PATTERNS) db 0

trigger_points:
	;; each track can trigger two instruments.
	;; triggering happens as follows:
	;;
	;;  - set pitch of osc at module <trigger point>
	;;  - reset phase of env at module <trigger point + 1>
	;;
	;; or alternatively, 4 independent triggers per instrument
	;; in non-compact mode
	times (NUM_TRACKS * 2 * 4) dd 0

module_skip_flags:
	times (NUM_TRACKS * NUM_MODULES) db 0
