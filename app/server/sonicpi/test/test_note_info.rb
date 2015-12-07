#--
# This file is part of Sonic Pi: http://sonic-pi.net
# Full project source: https://github.com/samaaron/sonic-pi
# License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
#
# Copyright 2013, 2014, 2015 by Sam Aaron (http://sam.aaron.name).
# All rights reserved.
#
# Permission is granted for use, copying, modification, and
# distribution of modified versions of this work as long as this
# notice is included.
#++

require 'test/unit'
require_relative "../../core"
require_relative "../lib/sonicpi/mods/sound"

module SonicPi
  module Mods
    module Sound
      module_function :note_info
    end
  end
  class NoteInfoTester < Test::Unit::TestCase

    def test_resolution_of_octave
      assert_equal(7, Mods::Sound.note_info(:C7).octave)
      assert_equal(7, Mods::Sound.note_info("C7").octave)
      assert_equal(7, Mods::Sound.note_info(96).octave)

      assert_equal(7, Mods::Sound.note_info(96, octave: 7).octave)
      assert_equal(7, Mods::Sound.note_info(:C, octave: 7).octave)
      assert_equal(7, Mods::Sound.note_info("C", octave: 7).octave)
    end

  end

end
