//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/strong/id.h"
#include "common/compare.h"

#include <fcntl.h>
#include <sys/socket.h>

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace socket
         {
            using descriptor_type = strong::socket::id;

            namespace option
            {
               template< int option_value>
               struct base 
               {
                  constexpr static auto level() { return SOL_SOCKET;}
                  constexpr static auto option() { return option_value;}
               };

               template< int option, bool on> 
               struct on_base : base< option> 
               {
                  constexpr static int value() { return on ? 1 : 0;}
               };

               template< bool on> 
               struct reuse_address : on_base< SO_REUSEADDR, on> {}; 

               struct linger : base< SO_LINGER> 
               {
                  linger( std::chrono::seconds time) : m_time( time) {}

                  auto value()
                  {
                     // using linger defined in sys/socket.h
                     return ::linger{ 1, static_cast< int>( m_time.count())};
                  }
                  std::chrono::seconds m_time;
               };

               enum class File 
               {
                  no_block = O_NONBLOCK,
               };

            } // option

         } // socket

         class Socket : compare::Equality< Socket>
         {
         public:

            using descriptor_type = socket::descriptor_type;
            using size_type = platform::size::type;

            Socket() noexcept = default;
            explicit Socket( descriptor_type descriptor) noexcept;
            ~Socket() noexcept;

            Socket( const Socket&);
            Socket& operator = ( const Socket&);

            Socket( Socket&&) noexcept;
            Socket& operator = ( Socket&&) noexcept;

            inline bool empty() const noexcept { return m_descriptor.empty();}
            inline explicit operator bool () const noexcept { return ! empty();}

            //! Releases the responsibility of the socket
            //!
            //! @return descriptor
            descriptor_type release() noexcept;

            descriptor_type descriptor() const noexcept;

            //! return SO_ERROR from getsockopt
            std::errc error() const;

            template< typename Option>
            void set( Option&& option)
            {
               auto&& value = option.value();
               Socket::option( option.level(), option.option(), &value, sizeof( std::decay_t< decltype( value)>));
            }

            void set( socket::option::File option);
            void unset( socket::option::File option);

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_descriptor, "descriptor");
            )

            inline auto tie() const { return std::tie( m_descriptor);}

         private:

            void option( int level, int optname, const void *optval, size_type optlen);

            descriptor_type m_descriptor;
         };

         namespace socket
         {
            //! Duplicates the descriptor
            //!
            //! @param descriptor to be duplicated
            //! @return socket that owns the descriptor
            Socket duplicate( descriptor_type descriptor);

         } // socket
      
      } // communication
   } // common
} // casual

