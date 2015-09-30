
Linux port of the synth from the FreeBSD 4k intro Ghosts of Mars
(https://github.com/trilkk/faemiyah-demoscene_2015-08_4k-intro_ghosts_of_mars,
http://www.pouet.net/prod.php?which=66046), with JACK audio and MIDI support.
Some extra features (DC filter, dynamic compressor, extra instrument parameters)
added.

Building
========

Requirements:

* CMake & make (for building)
* libjack
* libjsoncpp

```
 $ cd src/ 
 $ cmake .
 $ make
```
 
Use
===

$ ghostsyn <patch for MIDI ch 1> <patch for MIDI ch 2> ...

There's a bunch of example patches in src/patches. The easiest way to get
started is just to load them all on different midi channels with

$ cd src && ./ghostsyn patches/*.patch

Then just connect the outputs to audio outs, and MIDI port to your midi input.
(QJackctl can be used for this).

Realtime controls
-----------------

Pitch bend works, mod wheel adjusts filter cutoff.

Editing
=======

Instrument format is JSON, see included patches for examples. Parameters are
multiplied by the decay parameter for each (over)sample. Filter may become
unstable if filter_feedback is set to > 1.0 (you don't want that).

TODO
====

* More realtime controls, control value smoothing
* Add separate downsampling filter, reduce oversample factor
* Amplitude & filter envelopes
* Effects (stereo delay, reverb, flanger/chorus etc.)
* Figure out how to make better drum sounds
* DSSI support?    
* Editor GUI?
