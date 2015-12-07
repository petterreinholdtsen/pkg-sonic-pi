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

require 'cgi'
require_relative 'util'
require_relative 'markdown_converter'


module SonicPi
  module DocSystem

    def self.included(base)

      class << base
        include SonicPi::Util

        @@docs ||= {}

        def docs
          @@docs
        end

        # Generates HTML for Lang part of help system
        def docs_html_map
          hv_face = "face=\"HelveticaNeue-Light,Helvetica Neue Light,Helvetica Neue\""
          res = {}

          extract_comments = lambda do |s|
            code = ""
            comments = ""
            s.each_line do |l|
              m = l.match /(.*?)[^&]?(#.*)/
              if m

                code << CGI.escapeHTML(m[1]) << "\n"
                comments << CGI.escapeHTML(m[2]) << "\n"
              else
                code << CGI.escapeHTML(l)
                comments << " \n"
              end
            end
            [code, comments]
          end
          @@docs.each do |k, v|
            unless(v[:hide])
              html = ""
              html << '<p> <span style="font-size:25px; color:white;background-color:deeppink;">'

              summary = (v[:summary] || v[:name]).to_s
              summary[0] = summary[0].capitalize
              html << "<font #{hv_face}>" << summary << "</font></span></p>\n"
              html << "<h2><font color=\"deeppink\"><pre>#{v[:name]}<font color=\"#3c3c3c\">"
              name_size = v[:name].size
              req_args = []
              raise "no args defined for #{v[:name]}" unless v[:args]
              v[:args].each do |arg|
                n, t = *arg
                req_args << "#{n} <font color=\"gray\">(#{t})</font>"
              end
              html << " #{req_args.join(', ')}</pre></font></h2>\n"


              if v[:opts] && !v[:opts].empty?

                html << "<p>"

                html << "<table cellpadding=\"2\">\n"

                bg_colour_k = "#5e5e5e"
                fnt_colour_k = "white"

                bg_colour_v = "#E8E8E8"
                fnt_colour_v = "#5e5e5e"

                html << "<tr><td bgcolor=\"dodgerblue\"><font color=\"white\">Options</font></td/<td></td></tr>"
                cnt = 0
                v[:opts].each do |opt_name, opt_doc|


                  html << "<tr><td bgcolor=\"#{bg_colour_k}\"><pre><font color=\"#{fnt_colour_k}\">#{opt_name.to_s}:</font></pre></td><td bgcolor=\"#{bg_colour_v}\"><font color=\"#{fnt_colour_v}\"> #{MarkdownConverter.convert(opt_doc)}</font></td></tr>"
                  cnt += 1
                end
                html << "</table></p>"
              end
              html << MarkdownConverter.convert(v[:doc])
              html << "<p><font size=\"3\", #{hv_face}>\n"

              html << "<span style=\"color:white;background-color:darkorange;\">"
              html << "Introduced in " << v[:introduced].to_s << "\n</span></p>\n"

              html << "<table cellpadding=\"8\">\n"
              html << " <tr>\n   <th></th><th></th><th></th>\n </tr>\n"

              v[:examples].each_with_index do |e, idx|

                background_colour = idx.even? ? "#F8F8F8" : "#E8E8E8"
                key_bg_colour = idx.even? ? "#74ACFF" : "#B2D1FF"

                html << " <tr bgcolor=\"#{background_colour}\">\n"
                html << "  <td bgcolor=\"#{key_bg_colour}\"><h3><pre>Example #{idx} </pre></h3></td>\n"

                code, comments = *extract_comments.call(e.strip)

                html << "   <td><pre>\n#{code << "\n\n\n"}</pre></td>\n"
                html << "   <td><pre>\n#{comments << "\n\n\n"}</pre></td></tr>\n"
              end
              html << "</table>"
              res[k.to_s] = html
            end
          end
          res
        end

        def doc(*info)
          args_h = resolve_synth_opts_hash_or_array(info)
          @@docs[args_h[:name]] = args_h
        end

        def ring_fns
          @@docs.values.select do |info|
            info[:returns] == :ring
          end
        end
      end
    end
  end
end
