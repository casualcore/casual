//!
//! terminal.cpp
//!
//! Created on: Dec 22, 2014
//!     Author: Lazan
//!

#include "common/terminal.h"


#include <iomanip>

namespace casual
{
   namespace common
   {
      namespace terminal
      {


            color_t::color_t( std::string color) : m_color( std::move( color))
            {

            }


            color_t::proxy_t::proxy_t( std::ostream& out) : m_out( &out) {}

            color_t::proxy_t::~proxy_t()
            {
               if( ! m_moved)
               {
                 // auto flags = m_out->flags();
                  //auto width = m_out->width( 0);
                  *m_out << "\033[0m";
                 // m_out->setf( flags);
                 // m_out->width( width);
               }
            }

            color_t::proxy_t::proxy_t( proxy_t&&) = default;
            color_t::proxy_t& color_t::proxy_t::operator = ( proxy_t&&) = default;


            const std::string& color_t::escape() const
            {
               return m_color;
            }

            std::string color_t::reset() const
            {
               return "\033[0m";
            }

            const std::string& color_t::start() const
            {
               return m_color;
            }

            const std::string& color_t::end() const
            {
               static const std::string reset = "\033[0m";

               return reset;
            }

            color_t::proxy_t operator << ( std::ostream& out, const color_t& color)
            {
               auto flags = out.flags();
               auto width = out.width();
               out.width( 0);
               out << color.m_color;
               out << std::setw( width);
               out.setf( flags);

               return color_t::proxy_t{ out};
            }


            namespace color
            {
               color_t no_color{ "\033[0m"};
               color_t grey{ "\033[0;30m"};
               color_t red{ "\033[0;31m"};
               color_t green{ "\033[0;32m"};
               color_t yellow{ "\033[0;33m"};
               color_t blue{ "\033[0;34m"};
               color_t magenta{ "\033[0;35m"};
               color_t cyan{ "\033[0;36m"};
               color_t white{ "\033[0;37m"};
            } // color


            namespace format
            {

            } // format
      } // terminal
   } // common
} // casual


