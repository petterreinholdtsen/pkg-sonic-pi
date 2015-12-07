#--
# This file is part of Sonic Pi: http://sonic-pi.net
# Full project source: https://github.com/samaaron/sonic-pi
# License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
#
# Copyright 2013, 2014 by Sam Aaron (http://sam.aaron.name).
# All rights reserved.
#
# Permission is granted for use, copying, modification, distribution,
# and distribution of modified versions of this work as long as this
# notice is included.
#++
require_relative "../../core"

require 'test/unit'
require 'osc-ruby'

require_relative "../lib/sonicpi/oscencode"
require_relative "../lib/sonicpi/oscdecode"

module SonicPi
  class OSCTester < Test::Unit::TestCase

    def test_basic_address_encoding
      encoder = OscEncode.new
      decoder = OscDecode.new

      address = "/foo"

      m = encoder.encode_single_message(address)
      d_address, d_args = decoder.decode_single_message(m)
      assert_equal(address, d_address)
      assert_equal([], d_args)
    end

    def test_args_encoding
      encoder = OscEncode.new
      decoder = OscDecode.new

      address = "/foobar"
      args = ["beans"]

      m = encoder.encode_single_message(address, args)
      m2 = OSC::Message.new(address, *args).encode
      assert_equal(m, m2)
      d_address, d_args = decoder.decode_single_message(m)
      assert_equal(address, d_address)
      assert_equal(args, d_args)
    end

    def test_args_encoding_multiple
      encoder = OscEncode.new
      decoder = OscDecode.new

      address = "/feooblah"

      args_to_test = [[1], [-1], [100], [-100], [1.0, 1.0], [0, 1], [0, 0, 0], [1, 0, 1, 1, 0, 1], [1, 0.0, 1.0, 0], [1.0, 1, 1], [-1, -1, -1], [1, 0, -1], ["eggs", "foo","bar", "beans", 0, -1, 2.0, -2000]]

      args_to_test.each do |args|
        m = encoder.encode_single_message(address, args)
        m2 = OSC::Message.new(address, *args).encode
        assert_equal(m, m2)
        d_address, d_args = decoder.decode_single_message(m)
        assert_equal(address, d_address)
        assert_equal(args, d_args)
        end
    end
  end
end
