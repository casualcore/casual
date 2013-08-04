/*
 * archive_logger.h
 *
 *  Created on: 4 maj 2013
 *      Author: kristian
 */

#ifndef ARCHIVE_LOGGER_H_
#define ARCHIVE_LOGGER_H_

#include "sf/basic_archive.h"


#include <sstream>

namespace casual
{
   namespace sf
   {

      namespace archive
      {
         namespace logger
         {

            class Implementation
            {
            public:
               Implementation();
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
                  value_start();
                  write_value( value);
                  value_end();
               }

            private:

               void value_start();
               void value_end();

               template<typename T>
               void write_value( const T& value)
               {
                  m_buffer << value;
               }

               void write_value( const std::string& value);
               void write_value( const std::wstring& value);
               void write_value( const common::binary_type& value);

            private:
               std::ostringstream m_buffer;
               const char* m_name = 0;
               std::size_t m_indent = 0;
            };


            typedef basic_writer< Implementation> Writer;

         } // logger
      } // archive
   } // sf
} // casual



#endif /* ARCHIVE_LOGGER_H_ */
