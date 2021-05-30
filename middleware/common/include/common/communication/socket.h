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

               template< bool on = true> 
               struct reuse_address : on_base< SO_REUSEADDR, on> {}; 

               template< bool on = true> 
               struct keepalive : on_base< SO_KEEPALIVE, on> {};

               struct linger : base< SO_LINGER> 
               {
                  linger() = default;
                  linger( std::chrono::seconds time) : m_time( time) {}

                  auto value()
                  {
                     // using linger defined in sys/socket.h
                     return ::linger{ 1, static_cast< int>( m_time.count())};
                  }
                  std::chrono::seconds m_time{};
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

            Socket() noexcept = default;
            explicit Socket( strong::socket::id descriptor) noexcept;
            ~Socket() noexcept;

            Socket( Socket&&) noexcept;
            Socket& operator = ( Socket&&) noexcept;

            inline bool empty() const noexcept { return m_descriptor.empty();}
            inline explicit operator bool () const noexcept { return ! empty();}

            //! Releases the responsibility of the socket
            //!
            //! @return descriptor
            strong::socket::id release() noexcept;

            strong::socket::id descriptor() const noexcept;

            //! return SO_ERROR from getsockopt
            std::errc error() const;

            template< typename Option>
            void set( Option&& option)
            {
               auto&& value = option.value();
               Socket::set_option( option.level(), option.option(), &value, sizeof( std::decay_t< decltype( value)>));
            }

            template< typename Option>
            auto get( Option&& option) const
            {
               auto result = option.value();
               Socket::get_option( option.level(), option.option(), &result, sizeof( std::decay_t< decltype( result)>));
               return result;
            }

            void set( socket::option::File option);
            void unset( socket::option::File option);

            friend std::ostream& operator << ( std::ostream& out, const Socket& value);

            inline auto tie() const { return std::tie( m_descriptor);}

         private:

            void set_option( int level, int optname, const void *optval, platform::size::type optlen);
            void get_option( int level, int optname, void* optval, platform::size::type optlen) const;

            strong::socket::id m_descriptor;
         };

         namespace socket
         {
            //! Duplicates the descriptor
            //!
            //! @param descriptor to be duplicated
            //! @return socket that owns the descriptor
            Socket duplicate( strong::socket::id descriptor);

         } // socket
      
      } // communication
   } // common
} // casual

