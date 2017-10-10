//!
//! casual
//!


#ifndef CASUAL_SF_ARCHIVE_LOG_H_
#define CASUAL_SF_ARCHIVE_LOG_H_

#include "sf/archive/basic.h"
#include "sf/platform.h"


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
               Implementation( Implementation&&) = default;


               ~Implementation();

               platform::size::type container_start( const platform::size::type size, const char* name);
               void container_end( const char*);

               void serialtype_start( const char* name);
               void serialtype_end( const char*);

               template<typename T>
               void write( const T& value, const char* name)
               {
                  write( std::to_string( value), name);
               }


               void write( std::string&& value, const char* name);
               void write( const bool& value, const char* name);
               void write( const std::string& value, const char* name);
               void write( const std::wstring& value, const char* name);
               void write( const platform::binary::type& value, const char* name);


            private:

               void add( const char* name);

               void flush();


               enum class Type
               {
                  value,
                  container,
                  composite
               };

               struct buffer_type
               {
                  buffer_type( platform::size::type indent, const char* name) : indent( indent), name( name) {}
                  buffer_type( buffer_type&&) = default;

                  platform::size::type indent;
                  std::string name;
                  std::string value;
                  platform::size::type size = 0;
                  Type type = Type::value;
               };

               std::ostream& m_output;
               std::vector< buffer_type> m_buffer;
               platform::size::type m_indent = 1;
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

         } // log
      } // archive
   } // sf
} // casual



#endif /* ARCHIVE_LOGGER_H_ */
