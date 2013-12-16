/*
 * archive_logger.h
 *
 *  Created on: 4 maj 2013
 *      Author: kristian
 */

#ifndef ARCHIVE_LOGGER_H_
#define ARCHIVE_LOGGER_H_

#include "sf/basic_archive.h"


#include <ostream>
#include <tuple>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace log
         {


            class Implementation
            {
            public:
               Implementation();
               Implementation( std::ostream& out);



               ~Implementation();

               void handle_start( const char* const name);

               void handle_end( const char* const name);

               std::size_t handle_container_start( const std::size_t size);

               void handle_container_end();

               void handle_serialtype_start();

               void handle_serialtype_end();

               template<typename T>
               void write( const T& value)
               {
                  write( std::to_string( value));
               }

               void write( std::string&& value);
               void write( const std::string& value);
               void write( const std::wstring& value);
               void write( const common::binary_type& value);


            private:

               void flush();


               enum class Type
               {
                  value,
                  container,
                  composite
               };

               struct buffer_type
               {
                  buffer_type( std::size_t indent, const char* name) : indent( indent), name( name) {}
                  buffer_type( buffer_type&&) = default;

                  std::size_t indent;
                  std::string name;
                  std::string value;
                  Type type = Type::value;
               };

               std::ostream& m_output;
               std::vector< buffer_type> m_buffer;
               std::size_t m_indent = 1;
            };


            //!
            //! |-someStuff
            //! ||-name...........[blabla]
            //! ||-someOtherName..[foo]
            //! ||-composite
            //! |||-foo..[slkjf]
            //! |||-bar..[42]
            //! ||-
            //!
            typedef basic_writer< Implementation> Writer;

         } // logger
      } // archive
   } // sf
} // casual



#endif /* ARCHIVE_LOGGER_H_ */
