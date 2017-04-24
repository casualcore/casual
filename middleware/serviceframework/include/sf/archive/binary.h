//!
//! archive_binary.h
//!
//! Created on: Jun 2, 2013
//!     Author: Lazan
//!

#ifndef ARCHIVE_BINARY_H_
#define ARCHIVE_BINARY_H_


#include "sf/archive/basic.h"
#include "sf/buffer.h"

namespace casual
{
   namespace sf
   {
      namespace archive
      {
         namespace binary
         {
            namespace implementation
            {
               class Writer
               {
               public:

                  typedef buffer::binary::Stream buffer_type;

                  Writer( buffer_type& buffer) : m_buffer( buffer) {}

                  //! @{
                  //! No op
                  void container_end( const char*) { /*no op*/}
                  void serialtype_start( const char*) {  /*no op*/}
                  void serialtype_end( const char*) { /*no op*/}
                  //! @}


                  std::size_t container_start( const std::size_t size, const char*)
                  {
                     const auto elements = static_cast<common::network::byteorder::size::type>( size);
                     write( elements, nullptr);
                     return size;
                  }


                  template< typename T>
                  void write( T&& value, const char*)
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
                  void container_end( const char*) { /*no op*/}
                  void serialtype_end( const char*) { /*no op*/}
                  //! @}

                  bool serialtype_start( const char*) { return true;}

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char*)
                  {
                     common::network::byteorder::size::type elements;
                     read( elements, nullptr);
                     return std::make_tuple( elements, true);
                  }


                  template< typename T>
                  bool read( T& value, const char*)
                  {
                     m_buffer >> value;
                     return true;
                  }
               private:
                  buffer_type& m_buffer;
               };
            } // policy

            typedef basic_reader< implementation::Reader, policy::Relaxed> Reader;
            typedef basic_writer< implementation::Writer> Writer;


         } // binary
      } // archive
   } // sf
} // casual

#endif /* ARCHIVE_BINARY_H_ */
