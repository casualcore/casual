//!
//! terminal.cpp
//!
//! Created on: Jan 2, 2015
//!     Author: Lazan
//!

#include "sf/archive/terminal.h"


#include "common/algorithm.h"

#include <iomanip>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace terminal
         {

            namespace percelain
            {

               Implementation::Implementation( std::ostream& out)
               : m_output( out)
               {

               }

               Implementation::Implementation( Implementation&&) = default;


               Implementation::~Implementation()
               {
                  m_output.flush();
               }

               void Implementation::handle_start( const char* const name) { /* no-op */;}

               void Implementation::handle_end( const char* const name) { /* no-op */;}

               std::size_t Implementation::handle_container_start( const std::size_t size)
               {
                  ++m_depth;
                  return size;
               }

               void Implementation::handle_container_end()
               {
                  decrement();
               }

               void Implementation::handle_serialtype_start()
               {
                  ++m_depth;
               }

               void Implementation::handle_serialtype_end()
               {
                  decrement();
               }


               void Implementation::write( const platform::binary_type& value)
               {
                  m_output << common::transcode::base64::encode( value);
               }

               void Implementation::handleState()
               {
                  switch( m_row)
                  {
                     case Row::first:
                        m_row_start_depth = m_depth;
                        m_row = Row::ongoing;
                        break;
                     case Row::empty:
                        m_row = Row::ongoing;
                        break;
                     case Row::ongoing:
                        m_output << '|';
                        break;
                  }

               }


               void Implementation::decrement()
               {
                  if( m_depth-- == m_row_start_depth)
                  {
                     m_output << '\n';
                     m_row = Row::empty;
                  }
               }

            } // percelain



            Directive::Directive( std::string name, Align align, color_function color)
              : name( std::move( name)), align( align), color( std::move( color)) {}

            Directive::Directive( std::string name, Align align)
              : Directive( std::move( name), align, nullptr) {}

            Directive::Directive( std::string name, color_function color)
            : Directive( std::move( name), Align::left, std::move( color)) {}


            bool operator == ( const Directive& lhs, const std::string& rhs)
            {
               return lhs.name == rhs;
            }




            Implementation::Implementation( std::ostream& out, std::vector< Directive> directives, bool header, bool colors)
                  : m_output( out), m_header( header), m_colors( colors), m_directives( std::move( directives)) {}

            Implementation::Implementation( std::ostream& out)
                  : Implementation( out, {}, true, true) {}

            Implementation::Implementation( Implementation&&) = default;


            Implementation::~Implementation() { m_output.flush();}

            void Implementation::handle_start( const char* const name) { m_name = name; ++m_depth;}

            void Implementation::handle_end( const char* const name) { decrement();}
            std::size_t Implementation::handle_container_start( const std::size_t size) { return size;}
            void Implementation::handle_container_end() { /* no-op */;}
            void Implementation::handle_serialtype_start() { /* no-op */;}
            void Implementation::handle_serialtype_end() { /* no-op */;}



            void Implementation::handleState()
            {
               switch( m_state)
               {
                  case State::initializing:
                     m_row_start_depth = m_depth - 2;
                     m_rows.push_back( {});
                     m_state = State::first;
                  case State::first:
                     m_columns.emplace_back( m_name);
                     m_current = std::end( m_columns) - 1;

                     break;
                  case State::empty:
                     m_rows.push_back( {});
                     m_current = std::begin( m_columns);
                     m_state = State::ongoing;
                     break;

                  default:
                     // no-op
                     break;
               }
            }

            const std::string& Implementation::to_string( const std::string& value)
            {
               return value;
            }

            std::string Implementation::to_string( const platform::binary_type& value)
            {
               return common::transcode::base64::encode( value);
            }


            Implementation::Column::Column( std::string header) : m_header( std::move( header)) {}

            void Implementation::Column::size( std::size_t size)
            {
               if( size > m_size)
               {
                  m_size = size;
               }
            }
            std::size_t Implementation::Column::size() const
            {
               if( m_header.size() > m_size)
               {
                  return m_header.size();
               }
               return m_size;
            }

            const std::string& Implementation::Column::header() const { return m_header;}




            void Implementation::decrement()
            {
               --m_depth;

               if( m_depth == m_row_start_depth)
               {
                  m_state = State::empty;
               }

               if( m_depth == 0)
               {
                  finilize();
               }
            }

            void Implementation::finilize()
            {
               decltype( m_columns) columns;
               std::swap( columns, m_columns);

               if( m_header)
               {
                  m_output << std::setfill( ' ');
                  for( auto& column : columns)
                  {
                     m_output << std::left;
                     auto found = common::range::find( m_directives, column.header());

                     if( found && found->align == Directive::Align::right)
                     {
                        m_output << std::right;
                     }

                     m_output << std::setw( column.size()) << column.header() << "  ";
                  }
                  m_output << std::endl;

                  m_output << std::left << std::setfill( '-');
                  for( auto& column : columns)
                  {
                     m_output << std::setw( column.size()) << "" << "  ";
                  }
                  m_output << std::endl;

               }

               m_output << std::setfill( ' ');

               decltype( m_rows) rows;
               std::swap( rows, m_rows);

               for( auto& row : rows)
               {
                  auto column = std::begin( columns);

                  for( auto& value : row)
                  {
                     m_output << std::left << std::setw( column->size());

                     auto found = common::range::find( m_directives, column->header());

                     if( found)
                     {
                        if( found->align == Directive::Align::right)
                        {
                           m_output << std::right;
                        }
                        if( m_colors && found->color)
                        {
                           found->color( m_output, value);
                        }
                        else
                        {
                           m_output << value;
                        }
                     }
                     else
                     {
                        m_output << value;
                     }

                     m_output << "  ";

                     ++column;
                  }

                  m_output << std::endl;
               }

               m_name = nullptr;
               m_depth = 0;
               m_row_start_depth = 0;
               m_state = State::initializing;
            }

         } // terminal
      } // archive
   } // sf
} // casual
