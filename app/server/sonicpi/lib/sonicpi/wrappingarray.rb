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

module SonicPi
  class WrappingArray < Array
    def [](idx, len=nil)
      return self.to_a[idx, len] if len

      idx = idx.to_i % size if idx.is_a? Numeric
      self.to_a[idx]
    end

    def slice(idx, len=nil)
      return self.to_a.slice(idx, len) if len

      idx = idx.to_i % size if idx.is_a? Numeric
      self.to_a.slice(idx)
    end
  end
end
