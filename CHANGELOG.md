# History

## Version 2.2.0
*Thursday 18th December, 2014*

This release brings a number of nice enhancements. However the main
feature is the accurate timing for triggering FX. This means you can now
reliably use FX for accurate rhythmic purposes such as wobbling, slicing
and echoes.

## Breaking Changes

* `use_sample_pack_as` now uses a double underscore `__` as a separator
  between the user-specified alias and the sample name.

## API Changes

* Teach synth args to take prefixed maps: `play 50, {amp: 0.5}, {release: 2}, amp: 2`
* Don't round Floats when user specifically prints them to log with puts
* `with_fx` FX synths are now triggered using virtual time rather than
  real time. This means that FX can now be used for rhythmical purposes.
* Work on new `RingArray` datastructure. This is essentially an array
  that wraps its indexes so you can use indexes larger than the array size.
* New fn `ring` - `(ring 1, 2, 3)` creates a new ring array.
* New fn `knit` - `(knit :a1, 2, :c1, 1)` returns `(ring :a1, :a1, :c1)` 
* New fn `bools` - `(bools 1, 0, 1)` returns `(ring true, false, true)`
* New fn `range` - `(range 70, 100, 10)` returns `(ring 70, 70, 90, 100)`
* New fn `is_sample_loaded?` - to detect whether a specific sample has been loaded

## Synth & FX

* Fixed regression in `:tb303` synth - sound is reverted to v2.0 behaviour
* New Synth - `:square` - Pure square wave

## GUI

* Help system now autodocks on close
* Preferences are now remembered across sessions
* On Raspberry Pi, previous volume and audio output options are forced
  on boot.

## New Samples

* `bd_tex` - Bass drum

## Bug fixes

* `one_in` now returns false if num is < 1
* Ensure `live_loop`'s no-sleep detector works within nested `with_fx` blocks
* `chord` now returns a ring.

## Version 2.1.1
*Tuesday 25th November, 2014*

* Windows version no longer needs special firewall exceptions to run
* Added license information to info window
* Minor grammar and spelling tweaks to tutorial


## Version 2.1
*Friday 21st November, 2014*

The focus of release is very much on technical improvements, efficiency
and general polish. 

The most obvious and exciting change is the introduction of the
`live_loop` which will change the way you work with Sonic Pi. For more
information on `live_loop` take a look at the new section in the
tutorial. Another very exciting development is that v2.1 marks the
official support for Windows thanks to the exellent work by Jeremy
Weatherford. Finally, this release is also the first release where Sonic
Pi has a Core Team of developers. Please give a warm welcome to Xavier
Riley, Jeremy Weatherford and Joseph Wilk.


### API Changes
* New fn `live_loop` - A loop for live coding
* New fn `inc` - increment
* New fn `dec` - decrement
* New fn `quantise` - quantise a value to resolution
* New fn `factor?` - Factor test
* New fn `at` - Run a block at the given times
* New fn `degree` - for resolving a note in a scale by degree such as `:i`, `:iv`
* New fn `chord_degree` - Construct chords based on scale degrees
* New TL fn `use_sample_bpm` - for changing the BPM based on a sample's duration
* New fn `rest?` - Determine if note or args is a rest
* New fn `vt` - Get virtual time
* New fn `set_control_delta!` - Set control delta globally
* `wait` now handles both `sleep` and `sync` functionality 
* Allow first arg to `play` to be a proc or lambda. In which case simple call it and use the result as the note
* Teach `play` to accept a single map argument (in which case it will extract `:note` key out if it exists.
* Fns `play` and `synth` now treat 'notes' `nil`, `:r` and `:rest` as rests and don't trigger any synths. 


### GUI Modifications

* Updated and improved tutorial
* Much improved autocompletion
* Add HPF, LPF, mono forcer and stereo swapping preferences to new studio section for use when performing with Sonic Pi through an external PA.
* Shortcuts overhauled - now supports basic Emacs-style Ctrl-* navigation.
* Shortcuts Alt-[ and Alt-] now cycle through workspaces
* Shortcuts now work when toolbar is hidden
* Font sizes for individual workspaces are now stored between sessions
* Ctl-Mouse-wheel zooms font on Windows
* Links are now clickable (opening external browser)
* Entries  in docsystem's synth arg table are now clickable and will take focus down to arg documentation
* Stop users accidentally clearing entire workspace if they type quickly after hitting run
* Hitting F1 or `C-i` over a function name now opens up the doc system at the relevant place

### Bugs/Improvements
* Reworked examples.
* Much improved efficiency in many areas - especially for Raspberry Pi.
* Avoid occasional clicking sound when stopping runs
* Note Cb is now correctly resolved to be a semitone lower than C
* Non RP systems now start with more audio busses (1024)
* Array#sample and Array#shuffle are now correctly seeded with thread local random generator
* Log files are now placed into ~/.sonic-pi/log
* Chords and scales now wrap around when accessed from indexes outside of their range.
* `rand_i` and `rrand_i` now correctly return integers rather than floats
* rrand arguments now correctly handle a range of 0 (i.e. min == max)
* Line offset in error messages is now correct
* When saving files on Windows, CRLF line endings are used
* Stop users defining functions with same name as core API fns


### Synths, Samples & FX
* New samples (bass drums, snares and loops)
* Allow `mod_range` param to have negative values (for oscillating with lower notes)
* Change slide mechanism to default to linear sliding with support for changing the curve type. All modifiable args now have corresponding  `_slide_shape` and `_slide_curve` args.
* Improve TB303 synth - now supports separate cutoff ADSR envelopes. New args:
  - `cutoff_attack`, 
  - `cutoff_sustain`, 
  - `cutoff_decay`, 
  - `cutoff_release`, 
  - `cutoff_min_slide`, 
  - `cutoff_attack_level`, 
  - `cutoff_sustain_level`,
  - `cutoff_env_curve`


## Version 2.0.1
*Tuesday 9th September, 2014*

* Fix recording functionality
* Improve documentation content and layout
* Close off OSC server from external clients
* Add History, Contributors and Community pages to info window
* Improve startup speed on OS X
* Re-work and add to shortcuts for key actions:
  - on RP they are all `alt-*` prefixed
  - on OS X they are all `cmd-*` prefixed
* Improve highlighting of log messages (`cue`/`sync` messages are more clearly highlighted)
* Log now communicates when a run has completed executing
* Fix bug encountered when stopping threads in super fast loops (stopped comms with server)

## Version 2.0
*Tuesday 2nd September, 2014*

* Complete rewrite since v1.0
* Support for Live Coding - redefining behaviour whilst music is playing
* New timing system - timing now guaranteed to be accurate
* Many new synths
* New chainable studio FX system 
* Support for sample playback
* Inclusion of over 70 CC0 licensed samples from http://freesound.org
* Support for controlling and modifying synth, fx and sample playback
  arguments after they have been triggered
* Completely re-designed GUI
* Help system with full documentation, tutorial and many examples
* Record functionality (for recording performances/pieces)
* Suport for controlling system audio settings on RP

