//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/terminal.h"

#include "common/environment.h"
#include "common/exception/guard.h"
#include "common/execute.h"
#include "common/string/compose.h"


#include <iomanip>

namespace casual
{
   namespace common::terminal
   {
      namespace output
      {
         namespace local
         {
            namespace
            {

               namespace deduce
               {
                  auto option( std::string value)
                  {
                     if( value == "auto")
                        return ::isatty( ::fileno( stdout)) == 1;
                     else
                        return string::from< bool>( value);
                  }
               }

               namespace global
               {
                  Directive& directive = Directive::instance();
               } // global

               namespace option
               {

                  auto trinary_completer()
                  { 
                     return []( bool, auto )
                     {
                        return std::vector< std::string>{ "true", "false", "auto"};
                     };
                  }

                     auto bool_completer()
                     {
                        return []( bool, auto)
                        {
                           return std::vector< std::string>{ "true", "false"};
                        };
                     }

                  auto header_default() 
                  {
                     return environment::variable::get( environment::variable::name::terminal::header).value_or( "true");
                  }

                  auto header( std::string& value)
                  {
                     return argument::Option( 
                        std::tie( value),
                        trinary_completer(), 
                        { "--header"}, 
                        string::compose( "set/unset header - if auto, headers are used if tty is bound to stdout (default: ", header_default(), ")"));
                  }

                  auto color( std::string& value)
                  {
                     return argument::Option( 
                        std::tie( value),
                        trinary_completer(), 
                        { "--color"},
                        string::compose( "set/unset color - if auto, colors are used if tty is bound to stdout (default: ", value, ")"));
                  }

                  auto porcelain( bool& value)
                  {
                     return argument::Option( 
                        std::tie( value),
                        bool_completer(), 
                        { "--porcelain"}, 
                        string::compose( "backward compatible, easy to parse output format (default: ", std::boolalpha, value, R"()

Format: `<column1>|<column2>|...|<columnN>`, with no `ws`. 
casual guarantees that any new columns will be appended to existing, to preserve compatibility within major versions.
Hence, column order can differ between `porcelain` and "regular".

@note To view `header` in porcelain-mode an explicit `--header true` must be used.
)"));

                  }
                  
               } // option

            } // <unnamed>
         } // local

         bool Directive::color() const
         {
            return ! m_porcelain && local::deduce::option( m_color);
         }

         bool Directive::header() const
         {
            if( ! m_header.empty())
               return local::deduce::option( m_header);
            else
               return local::deduce::option( local::option::header_default());
         }

         bool Directive::explict_header() const
         {
            return ! m_header.empty() && m_header != "auto" && local::deduce::option( m_header);
         }

         std::vector< argument::Option> Directive::options() &
         {
            auto default_description = []( const char* message, auto value)
            {
               return string::compose( message, " (default: ", std::boolalpha, value, ')');
            };

            return { 
               local::option::color( m_color),
               local::option::header( m_header),
               argument::Option( std::tie( m_precision), { "--precision"}, default_description( "set number of decimal points used for output", m_precision)),
               argument::Option( std::tie( m_block), local::option::bool_completer(), { "--block"}, default_description( "set/unset blocking - if false return control to user as soon as possible", m_block)),
               argument::Option( std::tie( m_verbose), local::option::bool_completer(), { "--verbose"}, default_description( "verbose output", m_verbose)),
               local::option::porcelain( m_porcelain)
            };
         }


         Directive& Directive::instance()
         {
            static Directive result;
            return result;
         }

         Directive::Directive()
            : m_color{ environment::variable::get( environment::variable::name::terminal::color).value_or( "true")},
               m_porcelain{ environment::variable::get< bool>( environment::variable::name::terminal::porcelain).value_or( false)},
               m_block{ environment::variable::get< bool>( environment::variable::name::terminal::precision).value_or( true)},
               m_verbose{ environment::variable::get< bool>( environment::variable::name::terminal::verbose).value_or( false)},
               m_precision{ environment::variable::get< int>( environment::variable::name::terminal::precision).value_or( 3)}
         {

         }

         void Directive::plain()
         {
            m_color = "false";
            m_header = "true";
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
         Color no_color{ value::no_color};
         Color black{ value::black};
         Color red{ value::red};
         Color green{ value::green};
         Color yellow{ value::yellow};
         Color blue{ value::blue};
         Color magenta{ value::magenta};
         Color cyan{ value::cyan};
         Color white{ value::white};
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

   } // common::terminal
} // casual


