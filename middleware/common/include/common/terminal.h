//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/move.h"
#include "common/algorithm.h"
#include "common/argument.h"

#include <string>
#include <ostream>
#include <iomanip>
#include <vector>
#include <sstream>

namespace casual
{
   namespace common
   {
      namespace terminal
      {
         namespace output
         {
            struct Directive
            {
               static Directive& instance();

               bool porcelain;
               bool color;
               bool header;
               std::streamsize precision;

               inline auto options()
               {
                  auto bool_completer = []( auto, bool ){
                     return std::vector< std::string>{ "true", "false"};
                  };
                  return argument::Option( std::tie( color), bool_completer, { "--color"}, "set/unset color")
                     + argument::Option( std::tie( header), bool_completer, { "--header"}, "set/unset header")
                     + argument::Option( std::tie( precision), { "--precision"}, "set number of decimal points used for output")
                     + argument::Option( std::tie( porcelain), bool_completer, { "--porcelain"}, "easy to parse output format");
               }

            private:
               Directive();
            };

            Directive& directive();


         } // output

         struct color_t
         {
            color_t( std::string color);

            struct proxy_t
            {
               proxy_t( std::ostream& out);
               ~proxy_t();

               proxy_t( proxy_t&&);
               proxy_t& operator = ( proxy_t&&);


               template< typename T>
               std::ostream& operator << ( T&& value)
               {
                  return *m_out << std::forward< T>( value);
               }

            private:
               std::ostream* m_out;
               move::Moved m_moved;
            };


            const std::string& start() const;
            const std::string& end() const;

            const std::string& escape() const;
            std::string reset() const;

            friend proxy_t operator << ( std::ostream& out, const color_t& color);

         private:
            std::string m_color;

         };

         namespace color
         {
            extern color_t no_color;
            extern color_t red;
            extern color_t grey;
            extern color_t red;
            extern color_t green;
            extern color_t yellow;
            extern color_t blue;
            extern color_t magenta;
            extern color_t cyan;
            extern color_t white;

            struct Solid
            {
               Solid( color_t& color) : m_color( color) {}

               template< typename T>
               void operator () ( std::ostream& out, T&& value)
               {
                  out << m_color << value;
               }
            private:
               color_t& m_color;
            };

         } // color



         namespace format
         {
            namespace customize
            {
               struct Stream
               {
                  Stream( std::ostream& stream);
                  ~Stream();

               private:
                  std::ostream* m_stream;
                  std::ios::fmtflags m_flags;
                  std::streamsize m_precision;
               };
            }

            enum class Align
            {
               left,
               right
            };


            template< typename T>
            struct formatter
            {
               using value_type = T;

               template< typename... Columns>
               static auto construct( Columns&&... columns)
               {
                  return formatter{ "  ", initialize( std::forward< Columns>( columns)...)};
               }

               template< typename... Columns>
               static auto construct( std::string delimiter, Columns&&... columns)
               {
                  return formatter{ std::move( delimiter), initialize( std::forward< Columns>( columns)...)};
               }


               template< typename R>
               void calculate_width( R&& range, const std::ostream& out)
               {
                  for( auto& column : m_columns)
                  {
                     column.calculate_width( range, out);
                  }
               }

               void print_headers( std::ostream& out)
               {
                  if( output::directive().header)
                  {
                     auto print_delimiter = [&](){
                        out << m_delimiter;
                     };

                     out << std::setfill( ' ');
                     {
                        auto print_name = [&out]( const column_holder& c){
                           out << std::left << std::setw( c.width()) << c.name();
                        };

                        algorithm::for_each_interleave( m_columns, print_name, print_delimiter);
                        out << '\n';
                     }

                     {
                        auto print_row = [&out]( const column_holder& c){
                           out << std::string( c.width(), '-');
                        };
                        algorithm::for_each_interleave( m_columns, print_row, print_delimiter);
                        out << '\n';
                     }
                  }
               }



               template< typename R>
               std::ostream& print_rows( std::ostream& out, R&& rows)
               {
                  if( output::directive().porcelain)
                  {
                     for( auto& row : rows)
                     {
                        auto print_delimiter = [&out](){
                           out << '|';
                        };

                        auto print_column = [&out,&row]( const column_holder& c){
                           c.print( out, row, false, false);
                        };

                        algorithm::for_each_interleave( m_columns, print_column, print_delimiter);
                        out << '\n';
                     }
                  }
                  else
                  {
                     for( auto& row : rows)
                     {
                        auto print_delimiter = [&](){
                           out << m_delimiter;
                        };

                        auto print_column = [&out,&row]( const column_holder& c){
                           c.print( out, row, output::directive().color);
                        };

                        algorithm::for_each_interleave( m_columns, print_column, print_delimiter);
                        out << '\n';
                     }
                  }
                  return out;
               }


               template< typename R>
               std::ostream& print( std::ostream& out, R&& range)
               {
                  customize::Stream stream( out);

                  if( ! output::directive().porcelain)
                  {
                     calculate_width( range, out);
                     print_headers( out);
                  }

                  print_rows( out, range);

                  return out;
               }

               template< typename Iter>
               std::ostream& print( std::ostream& out, Iter first, Iter last)
               {
                  return print( out, range::make( first, last));
               }


               struct base_column
               {
                  virtual ~base_column() = default;
                  virtual std::string name() const = 0;
                  virtual std::size_t width( const value_type& value, const std::ostream& out) const = 0;
                  virtual void print( std::ostream& out, const value_type& value, std::size_t size, bool color) const = 0;
               };


               struct column_holder
               {
                  column_holder( std::unique_ptr< base_column> column)
                     : m_column( std::move( column)), m_width( m_column->name().size()) {}

                  template< typename Range>
                  void calculate_width( Range&& range, const std::ostream& out)
                  {
                     for( auto& row : range)
                     {
                        m_width = std::max( m_width, m_column->width( row, out));
                     }
                  }

                  std::string name() const
                  {
                     return m_column->name();
                  }

                  void print( std::ostream& out, const value_type& value, bool color, bool width = true) const
                  {
                     m_column->print( out, value, width ? m_width : 0, color);
                  }


                  std::size_t width() const
                  {
                     return m_width;
                  }

                  std::unique_ptr< base_column> m_column;
                  std::size_t m_width;
               };


               template< typename I>
               struct basic_column : base_column
               {
                  using implementation_type = I;
                  basic_column( implementation_type implementation) : m_implementation( std::move( implementation)) {}

                  std::string name() const override { return m_implementation.name();}
                  void print( std::ostream& out, const value_type& value, std::size_t width, bool color) const override
                  {
                     m_implementation.print( out, value, width, color);
                  }


                  std::size_t width( const value_type& value, const std::ostream& out) const override { return m_implementation.width( value, out);}

                  I m_implementation;
               };



               using columns_type = std::vector< column_holder>;


               static columns_type initialize()
               {
                  return {};
               }

               template< typename C, typename... Columns>
               static columns_type initialize( C&& column, Columns&&... columns)
               {
                  auto result = initialize( std::forward< Columns>( columns)...);

                  auto basic = std::make_unique< basic_column< typename std::decay< C>::type>>( std::forward< C>( column));

                  result.emplace( std::begin( result), std::move( basic));
                  return result;
               }

            protected:
               formatter( std::string delimiter, columns_type columns)
                  : m_delimiter( std::move( delimiter)), 
                  m_columns( std::move( columns))
               {

               }
               
               std::string m_delimiter;
               columns_type m_columns;
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

               default_column( binder_type binder,
                     Align align = Align::left,
                     common::terminal::color_t color = common::terminal::color::red)
               : m_color( std::move( color)),
                 m_align( align == Align::left ? std::left : std::right),
                 binder( std::move( binder)) {}


               template< typename VT>
               std::size_t width( VT&& value, const std::ostream& out) const
               {
                  std::ostringstream repsentation;
                  repsentation.flags( out.flags());
                  repsentation.precision( out.precision());
                  repsentation << binder( value);
                  return repsentation.str().size();
               }

               template< typename VT>
               void print( std::ostream& out, VT&& value, std::size_t width, bool color) const
               {
                  std::ostringstream string_value;
                  string_value.precision( out.precision());
                  string_value.flags( out.flags());
                  string_value << binder( value);

                  out << std::setfill( ' ');

                  if( color)
                  {
                     out << m_color.start() << std::setw( width) << m_align << string_value.str() << m_color.end();
                  }
                  else
                  {
                     out << std::setw( width) << m_align << string_value.str();
                  }
               }

               common::terminal::color_t m_color;
               decltype( &std::left) m_align;

               //
               // gcc 4.8.2 does not overload const function operator...
               //
               mutable binder_type binder;
            };


            template< typename B>
            auto column( std::string name, B binder, common::terminal::color_t color, Align align)
            {
               return name_column< default_column<B>>{ std::move( name), std::move( binder), align, std::move( color)};
            }

            template< typename B>
            auto column( std::string name, B binder, common::terminal::color_t color)
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

            template< typename C>
            auto custom_column( std::string name, C&& column)
            {
               return name_column< C>( std::move( name), std::move( column));
            }
         } // format


      } // terminal

   } // common
} // casual


