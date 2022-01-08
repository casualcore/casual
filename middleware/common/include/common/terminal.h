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

               bool color() const;
               inline auto porcelain() const { return m_porcelain;}
               bool header() const;
               inline auto precision() const { return m_precision;}
               inline auto block() const { return m_block;}
               inline auto verbose() const { return m_verbose;}
               inline void verbose( bool value) { m_verbose = value;}

               //! sets no color 
               void plain();

               using options_type = decltype( std::declval< argument::Option>() + std::declval< argument::Option>());

               options_type options() &;

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
            explicit Color( const char* color) : m_color{ color} {}

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

         private:
            const char* m_color;

         };

         namespace color
         {
            namespace value
            {
               constexpr auto no_color = "\033[0m";
               constexpr auto grey = "\033[0;30m";
               constexpr auto red = "\033[0;31m";
               constexpr auto green = "\033[0;32m";
               constexpr auto yellow = "\033[0;33m";
               constexpr auto blue = "\033[0;34m";
               constexpr auto magenta = "\033[0;35m";
               constexpr auto cyan = "\033[0;36m";
               constexpr auto white = "\033[0;37m";
            } // value

            extern Color no_color;
            extern Color red;
            extern Color grey;
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
                  out << m_color << value;
               }
            private:
               Color& m_color;
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
                     column.calculate_width( range, out);
               }

               void print_headers( std::ostream& out)
               {
                  if( output::directive().header())
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
                  if( output::directive().porcelain())
                  {
                     for( auto& row : rows)
                     {
                        auto print_delimiter = [&out](){
                           out << '|';
                        };

                        auto print_column = [&out,&row]( const column_holder& c){
                           c.print( out, row, false);
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
                           c.print( out, row);
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

                  if( ! output::directive().porcelain())
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


               struct concept
               {
                  virtual ~concept() = default;
                  virtual std::string name() const = 0;
                  virtual std::size_t width( const value_type& value, const std::ostream& out) const = 0;
                  virtual void print( std::ostream& out, const value_type& value, std::size_t size) const = 0;
               };


               struct column_holder
               {
                  column_holder( std::unique_ptr< concept> column)
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

                  void print( std::ostream& out, const value_type& value, bool width = true) const
                  {
                     m_column->print( out, value, width ? m_width : 0);
                  }


                  std::size_t width() const
                  {
                     return m_width;
                  }

                  std::unique_ptr< concept> m_column;
                  std::size_t m_width;
               };


               template< typename I>
               struct basic_column : concept
               {
                  using implementation_type = I;
                  basic_column( implementation_type implementation) : m_implementation( std::move( implementation)) {}

                  std::string name() const override { return m_implementation.name();}
                  void print( std::ostream& out, const value_type& value, std::size_t width) const override
                  {
                     m_implementation.print( out, value, width);
                  }

                  std::size_t width( const value_type& value, const std::ostream& out) const override { return m_implementation.width( value, out);}

                  I m_implementation;
               };

               using columns_type = std::vector< column_holder>;

               static columns_type initialize()
               {
                  return {};
               }

               template< typename C, typename... Cs>
               static columns_type initialize( C&& column, Cs&&... columns)
               {
                  auto result = initialize( std::forward< Cs>( columns)...);

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
                     common::terminal::Color color = common::terminal::color::red)
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
                  return std::move( repsentation.str()).size();
               }

               template< typename VT>
               void print( std::ostream& out, VT&& value, std::size_t width) const
               {
                  std::ostringstream string_value;
                  string_value.precision( out.precision());
                  string_value.flags( out.flags());
                  string_value << binder( value);

                  out << std::setfill( ' ');

                  out << m_color << std::setw( width) << m_align << string_value.str();
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
                  return name_column< C>( std::move( name), std::move( column));
               }
            } // custom

         } // format

         namespace formatter
         {
            namespace key
            {
               //! returns a formatter for `std::tuple< std::string, std::string>`, with column-names
               //! 'key' and 'value'. 
               inline auto value()
               {
                  // TODO maintainence: we use this in cli::information, but should it be declared here? If so,
                  // should other "general" formatters be here too? 
                  // The whole 'terminal' stuff is not that good to begin with, make this dission if and when we
                  // rewrite the 'terminal' stuff.
                  auto get_first = []( auto& pair) -> const std::string& { return std::get< 0>( pair);};
                  auto get_second = []( auto& pair) -> const std::string& { return std::get< 1>( pair);};

                  return terminal::format::formatter< std::tuple< std::string, std::string>>::construct(
                     terminal::format::column( "key", get_first, terminal::color::yellow, terminal::format::Align::left),
                     terminal::format::column( "value", get_second, terminal::color::no_color, terminal::format::Align::left)
                  );
               }
            } // key

         } // formatter

      } // terminal
   } // common
} // casual
