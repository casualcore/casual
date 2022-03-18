//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/socket.h"
#include "common/communication/log.h"

#include "common/result.h"
#include "common/flag.h"


std::ostream& operator << ( std::ostream& out, const ::linger& value)
{
   if( value.l_onoff == 0)
      return out << "{}";
   return out << "{ time: " << value.l_linger << "s}";
}

namespace casual
{
   namespace common::communication
   {
      namespace socket::option
      {
         std::string_view description( File value) noexcept
         {
            switch( value)
            {
               case File::no_block: return "no_block";
               case File::close_in_child: return "close_in_child";
            }
            return "<unknown>";
         }
         
      } // socket::option

      Socket::Socket( strong::socket::id descriptor) noexcept 
         : m_descriptor( std::move( descriptor)) 
      {
         if( *this)
            common::log::line( log, "Socket::adopted - descriptor: ", m_descriptor);
      }

      Socket::~Socket() noexcept
      {
         if( *this)
         {
            if( posix::log::result( ::close( m_descriptor.value()), "failed to close socket"))
               common::log::line( log, "Socket::close - descriptor: ", m_descriptor);
         }
      }

      Socket::Socket( Socket&& other) noexcept
         : m_descriptor{ std::exchange( other.m_descriptor, {})}
      {}

      Socket& Socket::operator =( Socket&& other) noexcept
      {
         m_descriptor = std::exchange( other.m_descriptor, m_descriptor);
         return *this;
      }

      void Socket::set( socket::option::File option)
      {
         auto flags = ::fcntl( m_descriptor.value(), F_GETFL);
         posix::result( ::fcntl( m_descriptor.value(), F_SETFL, flags | cast::underlying( option)), "failed to set file option: ", option);
      }

      void Socket::unset( socket::option::File option)
      {
         auto flags = ::fcntl( m_descriptor.value(), F_GETFL);
         posix::result( ::fcntl( m_descriptor.value(), F_SETFL, flags & ~cast::underlying( option)), "failed to unset file option: ", option);
      }


      strong::socket::id Socket::descriptor() const noexcept
      {
         return m_descriptor;
      }

      std::optional< std::errc> Socket::error() const
      {
         int optval{};
         socklen_t optlen = sizeof( optval);
         posix::result( ::getsockopt( m_descriptor.value(), SOL_SOCKET, SO_ERROR, &optval, &optlen));

         if( optval == 0)
            return {};

         return std::errc( optval);
      }

      strong::socket::id Socket::release() noexcept
      {
         common::log::line( log, "Socket::release - descriptor: ", m_descriptor);

         return std::exchange( m_descriptor, {});
      }

      std::ostream& operator << ( std::ostream& out, const Socket& value)
      {
         if( ! out)
            return out;

         if( ! value.m_descriptor)
            return out << "{ descriptor: " << value.m_descriptor << '}';

         auto flags = ::fcntl( value.m_descriptor.value(), F_GETFL);

         return out << "{ descriptor: " << value.m_descriptor
            << ", blocking: " << std::boolalpha << ! common::has::flag< O_NONBLOCK>( flags)
            << ", reuse: " << ( value.get( socket::option::reuse_address<false>{}) != 0)
            << ", keepalive: " << ( value.get( socket::option::keepalive<false>{}) != 0)
            << ", linger: " << value.get( socket::option::linger{})
            << "}";
      }


      void Socket::set_option( int level, int optname, const void *optval, platform::size::type optlen)
      {
         posix::result( ::setsockopt( m_descriptor.value(), level, optname, optval, optlen), "setsockopt");
      }

      void Socket::get_option( int level, int optname, void* optval, platform::size::type optlen) const
      {
         ::socklen_t socklen = optlen;
         posix::result( ::getsockopt( m_descriptor.value(), level, optname, optval, &socklen), "getsockopt");
      }

   } // common::communication
} // casual

