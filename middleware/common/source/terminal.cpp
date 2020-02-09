//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/terminal.h"

#include "common/environment.h"
#include "common/exception/handle.h"
#include "common/execute.h"


#include <iomanip>

namespace casual
{
   namespace common
   {
      namespace terminal
      {
         namespace output
         {
            namespace local
            {
               namespace
               {
                  template< typename String, typename V> 
                  auto get( String&& environment, V value)
                  {
                     try 
                     {
                        return environment::variable::get( std::forward< String>( environment), value);
                     }
                     catch( ...)
                     {
                        exception::handle();
                     }
                     return value;
                  }

                  namespace global
                  {
                     Directive& directive = Directive::instance();
                  } // global
               } // <unnamed>
            } // local


            Directive::options_type Directive::options() &
            {
               auto bool_completer = []( auto, bool ){
                  return std::vector< std::string>{ "true", "false"};
               };

               auto default_description = []( const char* message, auto value)
               {
                  return string::compose( message, " (default: ", value, ')');
               };

               return argument::Option( std::tie( m_color), bool_completer, { "--color"}, default_description( "set/unset color", m_color))
                  + argument::Option( std::tie( m_header), bool_completer, { "--header"}, default_description( "set/unset header", m_header))
                  + argument::Option( std::tie( m_precision), { "--precision"}, default_description( "set number of decimal points used for output", m_precision))
                  + argument::Option( std::tie( m_block), bool_completer, { "--block"}, default_description( "set/unset blocking - if false return control to user as soon as possible", m_block))
                  + argument::Option( std::tie( m_porcelain), bool_completer, { "--porcelain"}, default_description( "easy to parse output format", m_porcelain));
            }


            Directive& Directive::instance()
            {
               static Directive result;
               return result;
            }

            Directive::Directive()
               : m_color{ local::get( environment::variable::name::terminal::color(), true)},
                  m_porcelain{ local::get( environment::variable::name::terminal::porcelain(), false)},
                  m_header{ local::get( environment::variable::name::terminal::header(), true)},
                  m_block{ local::get( environment::variable::name::terminal::precision(), true)},
                  m_precision{ local::get( environment::variable::name::terminal::precision(), 3)}
                  
            {

            }

            void Directive::plain()
            {
               m_color = false;
               m_header = true;
               m_precision = 3;
            }

            Directive& directive() 
            { 
               return local::global::directive;
            }


         } // output

         Color::Proxy::Proxy( std::ostream& out) : m_active( &out) {}
         Color::Proxy::Proxy() = default;

         Color::Proxy::~Proxy()
         {
            if( m_active && output::directive().color())
            {
               *m_active.value << "\033[0m";
            }
         }

         Color::Proxy::Proxy( Proxy&&) = default;
         Color::Proxy& Color::Proxy::operator = ( Proxy&&) = default;

         Color::Proxy operator << ( std::ostream& out, const Color& color)
         {
            if( output::directive().color())
            {
               auto flags = out.flags();
               auto width = out.width();
               out.width( 0);
               
               out << color.m_color;
               
               out << std::setw( width);
               out.setf( flags);
            }

            return Color::Proxy{ out};
         }

         namespace color
         {
            Color no_color{ "\033[0m"};
            Color grey{ "\033[0;30m"};
            Color red{ "\033[0;31m"};
            Color green{ "\033[0;32m"};
            Color yellow{ "\033[0;33m"};
            Color blue{ "\033[0;34m"};
            Color magenta{ "\033[0;35m"};
            Color cyan{ "\033[0;36m"};
            Color white{ "\033[0;37m"};
         } // color


         namespace format
         {
            namespace customize
            {
               namespace local
               {
                  namespace
                  {
                     auto flags()
                     {
                        return std::ios::dec
                           | std::ios::fixed 
                           | std::ios::boolalpha;
                     }

                  } // <unnamed>
               } // local

               Stream::Stream( std::ostream& stream)
                  : m_stream( &stream),
                     m_flags( stream.flags( local::flags())),
                     m_precision( stream.precision( output::directive().precision()))
               {

               }

               Stream::~Stream()
               {
                  m_stream->flags( m_flags);
                  m_stream->precision( m_precision);
               }
            }

         } // format
      } // terminal
   } // common
} // casual


