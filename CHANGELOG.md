# History

## Version 2.4 - 'Defrost'
*Wednesday 11th February, 2015*
[(view commits)](https://github.com/samaaron/sonic-pi/commits/v2.4.0)

A quick release following `v2.3` to address an issue with the GUI
freezing on specific CPUs. However, although this release has had a
small development cycle, it ships with three fantastic features. Firstly
we now have the `spread` fn which provides an amazing way to create
interesting rhythms with very little code. Secondly we can now use
`cutoff:` on any sample massively increasing their timbral range and
finally we have three exciting new synths for you to play with. Have
fun!

### Breaking Changes

* Unfortunately 5 pre-release synths accidentally slipped into
  v2.3. Three of them have been polished up and are in this release (one
  with major changes including a name change). However, the other two
  have been removed.

### New

* New fn `spread` for creating rings of Euclidean distributions. Great
  for quickly creating interesting rhythms.
* GUI now automatically appends a : to the FX param autocomplete list  
* Synths and FX now raise an exception if any of their non-modulatable
  params are modulated. This is disabled when the pref 'check synth
  args' is unchecked.
* GUI now renders pretty UTF-8 └─ ├─ characters when printing in the log
  on RP.
* Improve docstrings for sample player.

### Synths & FX
* New Synth `:dark_ambience`, an ambient bass trying to escape the
  darkness.
* New Synth `:hollow`, a hollow breathy sound.
* New Synth `:growl`, a deep rumbling growl.
* Sampler synths now sport built-in `rlpf` and `normaliser` FX. These
  are disabled by default (i.e. won't affect sound of the sample) and
  can by enabled via the new `cutoff:`, `res:` and `norm:` params. 

### Bug Fixes

* Fix insanely obsure bug which caused the GUI to freeze on certain
  platforms (32 bit Windows and RP2 with 2G/2G kernel).
* Remove DC Bias offset from Prophet synth (see
  http://en.wikipedia.org/wiki/DC_bias)


## Version 2.3 - 'Bitcrush'
*Wednesday 28th January, 2015*
[(view commits)](https://github.com/samaaron/sonic-pi/commits/v2.3.0)


### Breaking Changes

* Playing chords with the fn `chord` now divides the amplitiude of each
  resulting synth by the number of notes in the chord. This ensures the
  resulting amplitude isn't excessive and is normalised.
* Chords now evaluate their args once and those args are used for all
  synth triggers. This means random values are only generated once and
  are similar across all notes in the chord. Previous behaviour can be
  obtained by calling play multiple times with no interleaved sleeps.
* Ensure each new thread's random number generator is unique yet seeded
  in a deterministic manner. This stops random vals across `at` from
  being identical.
* `range` is now exclusive: `(range 1, 5) #=> (ring 1, 2, 3, 4)`

### New

* New fn `density` for compressing and repeating time Dr Who style. For
  example, wrapping some code with a call to density of 2 will double
  the bpm for that block as well as repeating it twice. This ensures the
  block takes the same amount of time to execute while doing double the
  work.
* New fns `with_bpm_mul` and `use_bpm_mul` which will multiply the
  current bpm by a specified amount. Useful for slowing down and
  speeding up the execution of a specific thread or live_loop.
* New fn `rdist` - generate a random number with a centred distribution
* New examples: `square skit`, `shufflit` and `tilburg`

### Improvements

* Teach control to ignore nil nodes i.e. `control nil, amp: 3` will do
  nothing.
* Teach Float#times to yield a float to the block. For example,
  `3.4.times {|v| puts v}` will yield `0.0`, `1.0` and `2.0`.
* Synth, Sample and FX args now handle bools and nil correctly. `true`
  resolves to `1.0` and `false`, `nil` resolve to `0.0`. This allows you
  to write code such as: `play :e3, invert_wave: true`
* Teach `at` to handle varying block arities differently. See docs for
  more detail. Original behaviour is preserved and only extended. 
* App now checks for updates (at most once every 2 weeks). This may be
  disabled in the prefs.
* Teach `:reverb` FX to extend its kill delay time with larger room
  sizes to reduce the chance of clipping.

### Synths & FX

* New FX `bitcrusher` - for crunching and destroying those hi-fi sounds.
* New FX `flanger` - a classic swhooshing effect typically used with
  vocals and guitars.
* New FX `ring` - ring modulation for that friendly Dalek sound
* New FX `bpf` - a band pass filter
* New FX `rbpf` - a resonant band pass filter
* New FX `nbpf` - a normalised band pass filter
* New FX `nrbpf` - a normalised resonant band pass filter

### New Samples

* `perc_snap` - a finger snap
* `perc_snap2` - another finger snap
* `bd_ada` - a bass drum
* `guit_em9` - a lovely guitar arpegio over Em9

### Bug Fixes

* Namespace `live_loop` fn and thread names to stop them clashing with
  standard user defined threads and fns.
* GUI no longer crashes when you start a line with a symbol.
* `with_fx` now returns the result of the block
* Kill zombie scsynth servers on startup for better crash recovery.
* Handle paths with UTF8 characters gracefully
* Force sample rate for output and input to 44k on OS X. This stops
  scsynth from crashing when output and input sample rates are
  different.

## Version 2.2 - 'Slicer'
 *Thursday 18th December, 2014*
[(view commits)](https://github.com/samaaron/sonic-pi/commits/v2.2.0)

This release brings a number of nice enhancements. However the main
feature is the accurate timing for triggering FX. This means you can now
reliably use FX for accurate rhythmic purposes such as wobbling, slicing
and echoes.

### Breaking Changes

* `use_sample_pack_as` now uses a double underscore `__` as a separator
  between the user-specified alias and the sample name.

### API Changes

* Teach synth args to take prefixed maps: `play 50, {amp: 0.5}, {release: 2}, amp: 2`
* Don't round Floats when user specifically prints them to log with puts
* `with_fx` FX synths are now triggered using virtual time rather than
  real time. This means that FX can now be used for rhythmical purposes.
* Work on new `RingArray` datastructure. This is essentially an array
  that wraps its indexes so you can use indexes larger than the array size.
* New fn `ring` - `(ring 1, 2, 3)` creates a new ring array.
* New fn `knit` - `(knit :a1, 2, :c1, 1)` returns `(ring :a1, :a1, :c1)` 
* New fn `bools` - `(bools 1, 0, 1)` returns `(ring true, false, true)`
* New fn `range` - `(range 70, 100, 10)` returns `(ring 70, 80, 90, 100)`
* New fn `sample_loaded?` - to detect whether a specific sample has been loaded

### Synths & FX

* Fixed regression in `:tb303` synth - sound is reverted to v2.0 behaviour
* New Synth - `:square` - Pure square wave

### GUI

* Help system now autodocks on close
* Preferences are now remembered across sessions
* On Raspberry Pi, previous volume and audio output options are forced
  on boot.

### New Samples

* `bd_tek` - Bass drum

### Bug fixes

* `one_in` now returns false if num is < 1
* Ensure `live_loop`'s no-sleep detector works within nested `with_fx` blocks
* `chord` now returns a ring.

## Version 2.1.1
*Tuesday 25th November, 2014*
[(view commits)](https://github.com/samaaron/sonic-pi/commits/v2.1.1)

* Windows version no longer needs special firewall exceptions to run
* Added license information to info window
* Minor grammar and spelling tweaks to tutorial


## Version 2.1 - 'Core'
*Friday 21st November, 2014*
[(view commits)](https://github.com/samaaron/sonic-pi/commits/v2.1.0)

The focus of release is very much on technical improvements, efficiency
and general polish. 

The most obvious and exciting change is the introduction of the
`live_loop` which will change the way you work with Sonic Pi. For more
information on `live_loop` take a look at the new section in the
tutorial. Another very exciting development is that v2.1 marks the
official support for Windows thanks to the excellent work by Jeremy
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
[(view commits)](https://github.com/samaaron/sonic-pi/commits/v2.0.1)

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

## Version 2.0 - 'Phoenix'
*Tuesday 2nd September, 2014*
[(view commits)](https://github.com/samaaron/sonic-pi/commits/v2.0.0)

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
* Support for controlling system audio settings on RP

