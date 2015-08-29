
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

Then just connect the outputs to audio outs, and MIDI port to your midi input.
(QJackctl can be used for this).

Editing
=======

Instrument format is JSON, see included patches for examples. Parameters are
multiplied by the decay parameter for each (over)sample. Filter may become
unstable if filter_feedback is set to > 1.0 (you don't want that).

TODO
====

* Realtime midi controls
* Add separate downsampling filter, reduce oversample factor
* Amplitude & filter envelopes
* Effects (stereo delay, reverb, flanger/chorus etc.)
* Figure out how to make better drum sounds
* DSSI support?    
* Editor GUI?
