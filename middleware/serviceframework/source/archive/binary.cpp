//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/archive/binary.h"

#include "serviceframework/archive/policy.h"

#include "common/memory.h"
#include "common/network/byteorder.h"

namespace casual
{
   namespace serviceframework
   {
      namespace archive
      {
         namespace binary
         {
            namespace local
            {
               namespace
               {
                  namespace implementation
                  {
                     using size_type = common::platform::size::type;

                     class Writer 
                     {
                     public:

                        using memory_type = platform::binary::type;

                        Writer( memory_type& buffer) : m_memory( buffer) {}
                        ~Writer() = default;

                        //! @{
                        //! No op
                        void container_end( const char*) { /*no op*/}
                        void serialtype_start( const char*) {  /*no op*/}
                        void serialtype_end( const char*) { /*no op*/}
                        //! @}


                        size_type container_start( const size_type size, const char*)
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
                        template< typename T>
                        void append( const T& value)
                        {
                           common::memory::append( value, memory());
                        }

                        template< typename T>
                        void store( const T& value)
                        {
                           append( value);
                        }

                        void store( const platform::binary::type& value)
                        {
                           // Write the size as size_type
                           store( common::range::size( value));
                           common::algorithm::append( value, memory());
                        }

                        void store( const std::string& value)
                        {
                           // Write the size as size_type
                           store( value.size());
                           common::algorithm::append( value, memory());
                        }

                        inline size_type size() const { return m_memory.get().size();}
                        inline platform::binary::type& memory() { return m_memory.get();}
                        
                        
                        std::reference_wrapper< memory_type> m_memory;
                     };


                     class Reader
                     {
                     public:
                        
                        using memory_type = const platform::binary::type;

                        Reader( memory_type& buffer) : m_memory( buffer) {}
                        ~Reader() = default;

                        //! @{
                        //! No op
                        void container_end( const char*) { /*no op*/}
                        void serialtype_end( const char*) { /*no op*/}
                        //! @}

                        bool serialtype_start( const char*) { return true;}

                        std::tuple< size_type, bool> container_start( size_type size, const char*)
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


                        template< typename T>
                        void load( T& value)
                        {
                           m_offset = common::memory::copy( memory(), m_offset, value);
                        }

                        template< typename T> 
                        void load_container( T& value) 
                        {
                           size_type size{};
                           load( size);

                           auto source = common::range::make( std::begin( memory()) + m_offset, size);
                           common::algorithm::copy( source, value);
                           m_offset += size;
                        }

                        void load( std::string& value)
                        {
                           load_container( value);
                        }

                        void load( platform::binary::type& value)
                        {
                           load_container( value);
                        }

                        size_type m_offset = 0;

                        inline size_type size() const { return memory().size();}
                        inline memory_type& memory() const { return m_memory.get();}
                        
                        std::reference_wrapper< memory_type> m_memory;

                     };
                  } // implementation

               } // <unnamed>
            } // local

            archive::Reader reader( const common::platform::binary::type& source)
            {
               return archive::Reader::emplace< archive::policy::Relaxed< local::implementation::Reader>>( source);
            }

            archive::Writer writer( common::platform::binary::type& destination)
            {
               return archive::Writer::emplace< local::implementation::Writer>( destination);
            }

         } // binary
      } // archive
   } // serviceframework
} // casual