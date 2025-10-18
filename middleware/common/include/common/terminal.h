//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/move.h"
#include "common/algorithm.h"
#include "casual/argument.h"

#include <string>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>

namespace casual
{
   namespace common::terminal
   {

      namespace output
      {
         struct Directive
         {
            static Directive& instance();

            bool color() const;
            inline auto porcelain() const { return m_porcelain;}
            bool header() const;
            //! user has used --header true explicitly
            //! @note this is to enable print header with porcelain -> not break backward compatible
            bool explicit_header() const;
            inline auto precision() const { return m_precision;}
            inline auto block() const { return m_block;}
            inline auto verbose() const { return m_verbose;}
            inline void verbose( bool value) { m_verbose = value;}

            //! sets no color 
            void plain();


            std::vector< argument::Option> options() &;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_color);
               CASUAL_SERIALIZE( m_porcelain);
               CASUAL_SERIALIZE( m_header);
               CASUAL_SERIALIZE( m_block);
               CASUAL_SERIALIZE( m_verbose);
               CASUAL_SERIALIZE( m_precision);
            )

         private:
            std::string m_color;
            bool m_porcelain;
            std::string m_header;
            bool m_block;
            bool m_verbose;
            std::streamsize m_precision;

            Directive();
         };

         Directive& directive();


      } // output

      struct Color
      {
         explicit Color( std::string_view color) : m_color{ color} {}

         struct Proxy
         {
            explicit Proxy( std::ostream& out);
            Proxy();

            ~Proxy();
            Proxy( Proxy&&);
            Proxy& operator = ( Proxy&&);

            template< typename T>
            std::ostream& operator << ( T&& value)
            {
               return stream::write( *m_active.value, std::forward< T>( value));
            }

         private:
            using Active = move::basic_active< std::ostream*>;
            Active m_active;
         };

         friend Proxy operator << ( std::ostream& out, const Color& color);

         auto value() const { return m_color; }

      private:
         std::string_view m_color;

      };

      namespace color
      {
         namespace value
         {
            constexpr std::string_view no_color = "\033[0m";
            constexpr std::string_view black = "\033[0;30m";
            constexpr std::string_view red = "\033[0;31m";
            constexpr std::string_view green = "\033[0;32m";
            constexpr std::string_view yellow = "\033[0;33m";
            constexpr std::string_view blue = "\033[0;34m";
            constexpr std::string_view magenta = "\033[0;35m";
            constexpr std::string_view cyan = "\033[0;36m";
            constexpr std::string_view white = "\033[0;37m";
         } // value

         extern Color no_color;
         extern Color red;
         extern Color black;
         extern Color red;
         extern Color green;
         extern Color yellow;
         extern Color blue;
         extern Color magenta;
         extern Color cyan;
         extern Color white;

         struct Solid
         {
            Solid( Color& color) : m_color( color) {}

            template< typename T>
            void operator () ( std::ostream& out, T&& value)
            {
               stream::write( out, m_color, value);
            }
         private:
            Color& m_color;
         };

      } // color



      namespace format
      {
         namespace ostream
         {
            struct scope
            {
               scope( std::ostream& stream);
               ~scope();

            private:
               std::ostream* m_stream;
               std::ios::fmtflags m_flags;
               std::streamsize m_precision;
            };
         } // ostream

         enum class Align
         {
            left,
            right
         };

         //! explicit type to denote a delimiter
         struct Delimiter
         {
            explicit Delimiter( std::string_view value) : value( value) {}
            std::string_view value;
         };

         template< typename B>
         struct name_column : public B
         {
            template< typename... Args>
            name_column( std::string name, Args&& ...args) : B( std::forward< Args>( args)...), m_name( std::move( name)) {}

            const std::string& name() const { return m_name;}

         private:
            std::string m_name;
         };


         template< typename B>
         struct default_column
         {
            using binder_type = B;

            default_column( binder_type binder, Align align = Align::left, common::terminal::Color color = common::terminal::color::red)
               : m_color( std::move( color)),
                  m_align( align == Align::left ? std::left : std::right),
                  binder( std::move( binder)) 
            {}


            template< typename VT>
            std::size_t width( VT&& value, const std::ostream& out) const
            {
               std::ostringstream representation;
               representation.flags( out.flags());
               representation.precision( out.precision());
               stream::write( representation, binder( value));
               return std::move( representation.str()).size();
            }

            template< typename VT>
            void print( std::ostream& out, VT&& value, std::size_t width) const
            {
               std::ostringstream string_value;
               string_value.precision( out.precision());
               string_value.flags( out.flags());
               stream::write( string_value, binder( value));

               out << std::setfill( ' ');

               out << m_color << std::setw( width) << m_align << std::move( string_value).str();
            }

            common::terminal::Color m_color;
            decltype( &std::left) m_align;

            // gcc 4.8.2 does not overload const function operator...
            mutable binder_type binder;
         };


         template< typename B>
         auto column( std::string name, B binder, common::terminal::Color color, Align align)
         {
            return name_column< default_column<B>>{ std::move( name), std::move( binder), align, std::move( color)};
         }

         template< typename B>
         auto column( std::string name, B binder, common::terminal::Color color)
         {
            return column( std::move( name), std::move( binder), std::move( color), Align::left);
         }

         template< typename B>
         auto column( std::string name, B binder, Align align)
         {
            return column( std::move( name), std::move( binder), common::terminal::color::no_color, align);
         }

         template< typename B>
         auto column( std::string name, B binder)
         {
            return column( std::move( name), std::move( binder), common::terminal::color::no_color, Align::left);
         }
         
         namespace custom
         {
            template< typename C>
            auto column( std::string name, C&& column)
            {
               return name_column< C>( std::move( name), std::forward< C>( column));
            }
         } // custom


         inline std::string guard_empty( std::string value)
         {
            if( value.empty() && ! terminal::output::directive().porcelain())
               return "-";

            return value;
         }

         namespace detail
         {
            struct Bookkeeping
            {
               std::size_t width = 0;
            };

            auto calculate_width( std::ostream& out, concepts::range auto&& range, const auto&... columns)
            {
               std::array< detail::Bookkeeping, sizeof...( columns)> bookkeeping;

               // first calculate widths for each column name. We know that we need at least that width
               {
                  std::size_t index = 0;
                  ( ( bookkeeping[ index++].width = columns.name().size()), ...);
               }

               // then possibly expand widths based on column data

               auto calculate = [ &out]( const auto& row, std::size_t index, auto& bookkeeping, const auto& column)
               {
                  bookkeeping[ index].width = std::max( bookkeeping[ index].width, column.width( row, out));
               };

               for( auto& row : range)
               {
                  std::size_t index = 0;
                  ( ( calculate( row, index++, bookkeeping, columns)), ...);
               }

               return bookkeeping;
            }

            void print_headers( std::ostream& out, Delimiter delimiter, const auto& bookkeeping, const auto&... columns)
            {
               if( ! output::directive().header())
                  return;

               {
                  auto print_names = [ &]( std::size_t index, const auto& column)
                  {
                     out << std::left << std::setw( bookkeeping[ index].width) << column.name();

                     // if not last index, print delimiter
                     if( index < bookkeeping.size() - 1)
                        out << delimiter.value;
                  };

                  std::size_t index = 0;
                  ( ( print_names( index++, columns)), ...);

                  out << '\n';
               }

               {
                  auto print_separators = [ &]( const auto& meta)
                  {
                     out << std::string( meta.width, '-');
                  };

                  auto print_delimiter = [ &]() { out << delimiter.value;};

                  algorithm::for_each_interleave( bookkeeping, print_separators, print_delimiter);

                  out << '\n';
               }
            }

            void print_porcelain_headers( std::ostream& out, const auto&... columns)
            {
               constexpr auto column_count = sizeof...( columns);

               auto print_name = [&]( std::size_t index, const auto& column)
               {
                  if( index < column_count - 1)
                     out << column.name() << '|';
                  else
                     out << column.name();
               };

               std::size_t index = 0;
               ( ( print_name( index++, columns)), ...);

               out << '\n';
            }

            void print_rows( std::ostream& out, Delimiter delimiter, concepts::range auto&& range, const auto& bookkeeping, const auto&... columns)
            {
               auto print_row = [&]( const auto& row)
               {
                  auto print_column = [&]( std::size_t index, const auto& column)
                  {
                     column.print( out, row, bookkeeping[ index].width);

                     // if not last index, print delimiter
                     if( index < bookkeeping.size() - 1)
                        out << delimiter.value;
                  };

                  std::size_t index = 0;
                  ( ( print_column( index++, columns)), ...);

                  out << '\n';
               };

               std::ranges::for_each( range, print_row);
            }

            void print_porcelain_rows( std::ostream& out, concepts::range auto&& range, const auto&... columns)
            {
               auto print_row = [&]( const auto& row)
               {
                  auto print_column = [&]( std::size_t index, const auto& column)
                  {
                     column.print( out, row, false);

                     // if not last index, print delimiter
                     if( index < sizeof...( columns) - 1)
                        out << '|';
                  };

                  std::size_t index = 0;
                  ( ( print_column( index++, columns)), ...);

                  out << '\n';
               };

               std::ranges::for_each( range, print_row);
            }

         } // detail


         void print( std::ostream& out, Delimiter delimiter, concepts::range auto&& range, auto&&... columns)
         {
            static_assert( sizeof...( columns) > 0, "at least one column must be specified");

            // set user defined stream settings
            ostream::scope scope( out);

            if( ! output::directive().porcelain())
            {
               auto bookkeeping = detail::calculate_width( out, range, columns...);
               detail::print_headers( out, delimiter, bookkeeping, columns...);
               detail::print_rows( out, delimiter, range, bookkeeping, columns...);
            }
            else
            {
               if( output::directive().explicit_header())
                  detail::print_porcelain_headers( out, columns...);
               
               detail::print_porcelain_rows( out, range, columns...);
            }
         }

         void print( std::ostream& out, concepts::range auto&& range, auto&&... columns)
         {
            print( out, Delimiter{ "  "}, range, columns...);
         }

         void print( Delimiter delimiter, concepts::range auto&& range, auto&&... columns)
         {
            print( std::cout, delimiter, range, columns...);
         }


         void print( concepts::range auto&& range, auto&&... columns)
         {
            print( std::cout, range, columns...);
         }

         namespace pair
         {
            void print( std::ostream& out, concepts::range auto&& range)
            {
               auto get_first = []( auto& pair) -> const std::string& { return std::get< 0>( pair);};
               auto get_second = []( auto& pair) -> const std::string& { return std::get< 1>( pair);};

               format::print( out, range,
                  terminal::format::column( "key", get_first, terminal::color::yellow, terminal::format::Align::left),
                  terminal::format::column( "value", get_second, terminal::color::no_color, terminal::format::Align::left)
               );
            }

            void print( concepts::range auto&& range)
            {
               print( std::cout, std::forward< decltype( range)>( range));
            }

         } // pair

      } // format
      
   } // common::terminal
} // casual
