//!
//! casual
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
   namespace common
   {
      namespace communication
      {
         namespace local
         {
            namespace
            {
               auto duplicate( strong::socket::id descriptor)
               {
                  Trace trace( "common::communication::tcp::local::socket::duplicate");

                  // We block all signals while we're trying to duplicate the descriptor
                  //common::signal::thread::scope::Block block;

                  const auto copy = strong::socket::id( posix::result( ::dup( descriptor.value()), "duplicate socket"));

                  common::log::line( log, "descriptors - original: ", descriptor, " , copy:", copy);

                  return copy;
               }
               
            } // <unnamed>
         } // local
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
            posix::result( ::fcntl( m_descriptor.value(), F_SETFL, flags | cast::underlying( option)), "fcntl");
         }

         void Socket::unset( socket::option::File option)
         {
            auto flags = ::fcntl( m_descriptor.value(), F_GETFL);
            posix::result( ::fcntl( m_descriptor.value(), F_SETFL, flags & ~cast::underlying( option)), "fcntl");
         }


         strong::socket::id Socket::descriptor() const noexcept
         {
            return m_descriptor;
         }

         std::errc Socket::error() const
         {
            int optval{};
            socklen_t optlen = sizeof( optval);
            posix::result( ::getsockopt( m_descriptor.value(), SOL_SOCKET, SO_ERROR, &optval, &optlen));

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

         namespace socket
         {
            Socket duplicate( strong::socket::id descriptor)
            {
               return Socket{ local::duplicate( descriptor)};
            }

         } // socket

      } // communication
   } // common
} // casual

