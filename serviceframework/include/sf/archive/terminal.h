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


#include <ostream>
#include <tuple>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace terminal
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

               inline Implementation( std::ostream& out) : m_output( out) {}
               inline  Implementation( Implementation&&) = default;


               inline ~Implementation() { m_output.flush();}

               inline void handle_start( const char* const name) { /* no-op */;}

               inline void handle_end( const char* const name) { /* no-op */;}

               inline std::size_t handle_container_start( const std::size_t size) { ++m_depth; return size;}

               inline void handle_container_end() { decrement();}

               inline void handle_serialtype_start() { ++m_depth;}

               inline void handle_serialtype_end() { decrement();}

               template<typename T>
               void write( const T& value)
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
                  m_output << value;
               }

               inline void write( const platform::binary_type& value)
               {
                  m_output << common::transcode::base64::encode( value);
               }

            private:

               inline void decrement()
               {
                  if( m_depth-- == m_row_start_depth)
                  {
                     m_output << '\n';
                     m_row = Row::empty;
                  }
               }

               std::ostream& m_output;
               std::size_t m_depth = 0;
               std::size_t m_row_start_depth = 0;
               Row m_row = Row::first;
            };


            //!
            //! somestring|23|324.4|/bakbj/bllysl/bala|somevalue
            //!
            typedef basic_writer< Implementation> Writer;

         } // terminal
      } // archive
   } // sf
} // casual



#endif /* ARCHIVE_LOGGER_H_ */
