//!
//! terminal.h
//!
//! Created on: Dec 22, 2014
//!     Author: Lazan
//!

#ifndef COMMON_TERMINAL_H_
#define COMMON_TERMINAL_H_

#include "common/move.h"

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
            enum class Align
            {
               left,
               right
            };

            struct Directives
            {

               Directives( bool porcelain = false, bool colors = false, bool headers = false)
                  : porcelain( porcelain), colors( colors), headers( headers) {}

               bool porcelain;
               bool colors;
               bool headers;
            };

            template< typename T>
            struct formatter
            {
               using value_type = T;


               template< typename... Columns>
               formatter( Directives directives, Columns&&... columns)
                  : m_directives( directives),
                  m_columns( initialize( std::forward< Columns>( columns)...))
               {

               }

               template< typename Iter>
               void calculate_width( Iter first, Iter last)
               {
                  for( auto& column : m_columns)
                  {
                     column.calculate_width( first, last);
                  }
               }

               template< typename Iter>
               std::ostream& print( std::ostream& out, Iter first, Iter last)
               {

                  if( m_directives.porcelain)
                  {
                     for( ; first != last; ++first)
                     {
                        auto current = std::begin( m_columns);

                        for( ; current != std::end( m_columns); ++current)
                        {
                           current->print( out, *first, false, false);
                           if( current + 1 != std::end( m_columns))
                           {
                              out << '|';
                           }
                        }
                        out << std::endl;
                     }
                  }
                  else
                  {

                     calculate_width( first, last);

                     if( m_directives.headers)
                     {
                        {
                           out << std::setfill( ' ');

                           auto current = std::begin( m_columns);

                           for( ; current != std::end( m_columns); ++current)
                           {
                              out << std::left << std::setw( current->width()) << current->name();
                              if( current + 1 != std::end( m_columns))
                              {
                                 out << "  ";
                              }
                           }

                           out << std::endl;
                        }

                        {
                           out << std::setfill( ' ');

                           auto current = std::begin( m_columns);

                           for( ; current != std::end( m_columns); ++current)
                           {
                              out << std::string( current->width(), '-');
                              if( current + 1 != std::end( m_columns))
                              {
                                 out << "  ";
                              }
                           }

                           out << std::endl;
                        }
                     }

                     for( ; first != last; ++first)
                     {
                        auto current = std::begin( m_columns);

                        for( ; current != std::end( m_columns); ++current)
                        {
                           current->print( out, *first, m_directives.colors);
                           if( current + 1 != std::end( m_columns))
                           {
                              out << "  ";
                           }
                        }
                        out << std::endl;
                     }
                  }

                  return out;
               }


               template< typename R>
               std::ostream& print( std::ostream& out, R&& range)
               {
                  return print( out, std::begin( range), std::end( range));
               }


               struct base_column
               {
                  virtual ~base_column() = default;
                  virtual std::string name() const = 0;
                  virtual std::size_t width( const value_type& value) const = 0;
                  virtual void print( std::ostream& out, const value_type& value, std::size_t size, bool color) const = 0;
               };


               struct column_holder
               {
                  column_holder( std::unique_ptr< base_column> column)
                     : m_column( std::move( column)), m_width( m_column->name().size()) {}

                  template< typename Iter>
                  void calculate_width( Iter first, Iter last)
                  {
                     for( ; first != last; ++first)
                     {
                        m_width = std::max( m_width, m_column->width( *first));
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


                  std::size_t width( const value_type& value) const override { return m_implementation.width( value);}

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

                  std::unique_ptr< base_column> basic{ new basic_column< typename std::decay< C>::type>( std::forward< C>( column))};

                  result.emplace( std::begin( result), std::move( basic));
                  return result;
               }
               Directives m_directives;
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
               std::size_t width(  VT&& value) const
               {
                  std::ostringstream repsentation;
                  repsentation << binder( value);
                  return repsentation.str().size();
               }

               template< typename VT>
               void print( std::ostream& out, VT&& value, std::size_t width, bool color) const
               {
                  out << std::setfill( ' ');

                  if( color)
                  {
                     out << m_color.start() << std::setw( width) << m_align << binder( value) << m_color.end();
                  }
                  else
                  {
                     out << std::setw( width) << m_align << binder( value);
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
               -> name_column< default_column<B>>
            {
               return name_column< default_column<B>>{ std::move( name), std::move( binder), align, std::move( color)};
            }

            template< typename B>
            auto column( std::string name, B binder, common::terminal::color_t color)
               -> name_column< default_column<B>>
            {
               return column( std::move( name), std::move( binder), std::move( color), Align::left);
            }

            template< typename B>
            auto column( std::string name, B binder, Align align)
               -> name_column< default_column<B>>
            {
               return column( std::move( name), std::move( binder), common::terminal::color::no_color, align);
            }

            template< typename B>
            auto column( std::string name, B binder)
               -> name_column< default_column<B>>
            {
               return column( std::move( name), std::move( binder), common::terminal::color::no_color, Align::left);
            }

            template< typename C>
            auto custom_column( std::string name, C&& column)
               -> name_column< C>
            {
               return name_column< C>( std::move( name), std::move( column));
            }
         } // format


      } // terminal

   } // common
} // casual

#endif // TERMINAL_H_
