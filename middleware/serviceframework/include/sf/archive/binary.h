//!
//! casual
//!

#ifndef ARCHIVE_BINARY_H_
#define ARCHIVE_BINARY_H_


#include "sf/archive/basic.h"

#include "common/memory.h"

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
               struct Base
               {
                  using memory_type = platform::binary::type;

                  Base( memory_type& buffer) : m_memory( buffer) {}

                  inline auto size() const { return memory().size();}

               protected:

                  inline platform::binary::type& memory() { return m_memory.get();}
                  inline const platform::binary::type& memory() const { return m_memory.get();}

                  std::reference_wrapper< memory_type> m_memory;
               };

               class Writer : Base
               {
               public:

                  using Base::Base;

                  //! @{
                  //! No op
                  void container_end( const char*) { /*no op*/}
                  void serialtype_start( const char*) {  /*no op*/}
                  void serialtype_end( const char*) { /*no op*/}
                  //! @}


                  std::size_t container_start( std::size_t size, const char*)
                  {
                     write( size, nullptr);
                     return size;
                  }


                  template< typename T>
                  void write( T&& value, const char*)
                  {
                     store( std::forward< T>( value));
                  }
               private:
                  template< typename Range>
                  void append( Range source)
                  {
                     auto current_size = size();

                     memory().resize( current_size + source.size());

                     auto destination = common::range::make( std::begin( memory()) + current_size, source.size());

                     common::memory::copy( source, destination);
                  }

                  template< typename T>
                  void store( const T& value)
                  {
                     append( common::memory::range::make( value));
                  }

                  void store( const platform::binary::type& value)
                  {
                     //
                     // TODO: Write the size as some-common_size_type
                     //
                     store( value.size());
                     append( common::range::make( value));
                  }

                  void store( const std::string& value)
                  {
                     //
                     // TODO: Write the size as some-common_size_type
                     //
                     store( value.size());
                     append( common::range::make( value));
                  }

               };


               class Reader : Base
               {
               public:
                  using Base::Base;

                  //! @{
                  //! No op
                  void container_end( const char*) { /*no op*/}
                  void serialtype_end( const char*) { /*no op*/}
                  //! @}

                  bool serialtype_start( const char*) { return true;}

                  std::tuple< std::size_t, bool> container_start( std::size_t size, const char*)
                  {
                     read( size, nullptr);
                     return std::make_tuple( size, true);
                  }


                  template< typename T>
                  bool read( T& value, const char*)
                  {
                     load( value);
                     return true;
                  }

               private:

                  template< typename Range>
                  void consume( Range destination)
                  {
                     if( m_offset + destination.size() > size())
                     {
                        throw exception::Validation( "Attempt to read out of bounds  ro: " + std::to_string( m_offset) + " size: " + std::to_string( size()));
                     }

                     auto source = common::range::make( std::begin( memory()) + m_offset, destination.size());

                     m_offset += common::memory::copy( source, destination);
                  }

                  template< typename T>
                  void load( T& value)
                  {
                     consume( common::memory::range::make( value));
                  }

                  void load( std::string& value)
                  {
                     //
                     // TODO: Read the size as some-common_size_type
                     //
                     auto size = value.size();
                     load( size);
                     value.resize( size);
                     consume( common::range::make( value));
                  }

                  void load( platform::binary::type& value)
                  {
                     //
                     // TODO: Read the size as some-common_size_type
                     //
                     auto size = value.size();
                     load( size);
                     value.resize( size);
                     consume( common::range::make( value));
                  }

                  platform::binary::size::type m_offset = 0;

               };
            } // implementation

            typedef basic_reader< implementation::Reader, policy::Relaxed> Reader;
            typedef basic_writer< implementation::Writer> Writer;

         } // binary
      } // archive
   } // sf
} // casual

#endif /* ARCHIVE_BINARY_H_ */
