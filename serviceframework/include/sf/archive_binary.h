//!
//! archive_binary.h
//!
//! Created on: Jun 2, 2013
//!     Author: Lazan
//!

#ifndef ARCHIVE_BINARY_H_
#define ARCHIVE_BINARY_H_


#include "sf/basic_archive.h"
#include "sf/buffer.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace binary
         {
            namespace policy
            {
               class Writer
               {
               public:

                  typedef buffer::binary::Stream buffer_type;

                  Writer( buffer_type& buffer) : m_buffer( buffer) {}

                  //! @{
                  //! No op
                  void handle_start( const char* name) { /*no op*/}
                  void handle_end( const char* name) {  /*no op*/}
                  void handle_container_end() { /*no op*/}
                  void handle_serialtype_start() {  /*no op*/}
                  void handle_serialtype_end() { /*no op*/}
                  //! @}


                  std::size_t handle_container_start( std::size_t size)
                  {
                     write( size);
                     return size;
                  }


                  template< typename T>
                  void write( T&& value)
                  {
                     m_buffer << std::forward< T>( value);
                  }
               private:
                  buffer_type& m_buffer;
               };


               class Reader
               {
               public:

                  typedef buffer::binary::Stream buffer_type;

                  Reader( buffer_type& buffer) : m_buffer( buffer) {}

                  //! @{
                  //! No op
                  void handle_start( const char* name) { /*no op*/}
                  void handle_end( const char* name) {  /*no op*/}
                  void handle_container_end() { /*no op*/}
                  void handle_serialtype_start() {  /*no op*/}
                  void handle_serialtype_end() { /*no op*/}
                  //! @}


                  std::size_t handle_container_start( std::size_t size)
                  {
                     read( size);
                     return size;
                  }


                  template< typename T>
                  void read( T& value)
                  {
                     m_buffer >> value;
                  }
               private:
                  buffer_type& m_buffer;
               };
            } // policy

            typedef basic_reader< policy::Reader> Reader;
            typedef basic_writer< policy::Writer> Writer;


         } // binary
      } // archive
   } // sf
} // casual

#endif /* ARCHIVE_BINARY_H_ */
