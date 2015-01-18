/*
 * archive_logger.h
 *
 *  Created on: 4 maj 2013
 *      Author: kristian
 */

#ifndef CASUAL_SF_ARCHIVE_TERMINAL_H_
#define CASUAL_SF_ARCHIVE_TERMINAL_H_

#include "sf/archive/basic.h"
#include "sf/platform.h"

#include "common/transcode.h"
#include "common/terminal.h"


#include <ostream>
//#include <tuple>

#include <iostream>

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
               class Implementation
               {
               public:

                  enum class Row
                  {
                     first,
                     empty,
                     ongoing,
                  };

                  Implementation( std::ostream& out);
                  Implementation( Implementation&&);
                  ~Implementation();

                  std::size_t container_start( const std::size_t size, const char* name);
                  void container_end( const char* name);
                  void serialtype_start( const char* name);
                  void serialtype_end( const char* name);

                  template<typename T>
                  void write( const T& value, const char*)
                  {
                     handleState();
                     m_output << value;
                  }

                  void write( const platform::binary_type& value, const char*);

               private:

                  void handleState();

                  void decrement();

                  std::ostream& m_output;
                  std::size_t m_depth = 0;
                  std::size_t m_row_start_depth = 0;
                  Row m_row = Row::first;
               };


               //!
               //! somestring|23|324.4|/bakbj/bllysl/bala|somevalue
               //!
               typedef basic_writer< Implementation> Writer;

            } // percelain



            struct Directive
            {
               enum class Align
               {
                  left,
                  right
               };

               using color_function = std::function< void(std::ostream&,const std::string&)>;

               Directive( std::string name, Align align, color_function color);
               Directive( std::string name, Align align);
               Directive( std::string name, color_function color);


               std::string name;
               Align align = Align::left;
               color_function color = nullptr;

               friend bool operator == ( const Directive& lhs, const std::string& rhs);
            };

            class Implementation
            {
            public:

               enum class State
               {
                  initializing,
                  first,
                  empty,
                  ongoing,
               };

               Implementation( std::ostream& out, std::vector< Directive> directives, bool header = true, bool colors = true);
               Implementation( std::ostream& out);
               Implementation( Implementation&&);
               ~Implementation();

               std::size_t container_start( const std::size_t size, const char* name);
               void container_end( const char* name);
               void serialtype_start( const char* name);
               void serialtype_end( const char* name);

               template<typename T>
               void write( const T& value, const char* name)
               {
                  handleState( name);

                  write_value( value);
                  ++m_current;
               }


            private:

               void handleState( const char* name);

               template<typename T>
               std::string to_string( const T& value)
               {
                  return std::to_string( value);
               }
               const std::string& to_string( const std::string& value);
               std::string to_string( const platform::binary_type& value);


               template<typename T>
               void write_value( const T& value)
               {
                  auto string = to_string( value);

                  m_current->size( string.size());
                  m_rows.back().push_back( std::move( string));
               }


               struct Column
               {
                  Column( std::string header);

                  void size( std::size_t size);
                  std::size_t size() const;
                  const std::string& header() const;

               private:
                  std::string m_header;
                  std::size_t m_size = 0;
               };

               void decrement();
               void finilize();


               std::ostream& m_output;
               bool m_header;
               bool m_colors;
               std::size_t m_depth = 0;
               std::size_t m_row_start_depth = 0;
               State m_state = State::initializing;

               std::vector< std::vector< std::string> > m_rows;
               std::vector< Column> m_columns;
               std::vector< Column>::iterator m_current;

               std::vector< Directive> m_directives;

            };

            typedef basic_writer< Implementation> Writer;

         } // terminal
      } // archive
   } // sf
} // casual



#endif /* ARCHIVE_LOGGER_H_ */
