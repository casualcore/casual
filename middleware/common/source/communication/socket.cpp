//!
//! casual
//!

#include "common/communication/socket.h"
#include "common/communication/log.h"
#include "common/exception/system.h"
#include "common/exception/handle.h"

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
               namespace check
               {
                  template< typename R>
                  auto result( R&& result)
                  {
                     if( result == -1)
                        exception::system::throw_from_errno();

                     return result;
                  }
               } // check

               auto duplicate( const socket::descriptor_type descriptor)
               {
                  Trace trace( "common::communication::tcp::local::socket::duplicate");

                  // We block all signals while we're trying to duplicate the descriptor
                  //common::signal::thread::scope::Block block;

                  const auto copy = socket::descriptor_type( check::result( ::dup( descriptor.value())));

                  common::log::line( log, "descriptors - original: ", descriptor, " , copy:", copy);

                  return copy;
               }
               
            } // <unnamed>
         } // local
         Socket::Socket( descriptor_type descriptor) noexcept 
            : m_descriptor( std::move( descriptor)) {}

         Socket::~Socket() noexcept
         {
            if( *this)
            {
               try
               {  
                  //local::socket::check::result( ::shutdown( m_descriptor, SHUT_RDWR));
                  if( ::close( m_descriptor.value()) == -1)
                     exception::system::throw_from_errno( "failed to close socket");

                  common::log::line( log, "Socket::close - descriptor: ", m_descriptor);
               }
               catch( ...)
               {
                  exception::handle();
               }
            }
         }


         Socket::Socket( const Socket& other)
            : m_descriptor{ local::duplicate( other.m_descriptor)}
         {
         }

         void Socket::option( int level, int optname, const void *optval, size_type optlen)
         {
            local::check::result( ::setsockopt( m_descriptor.value(), level, optname, optval, optlen));
         }

         void Socket::set( socket::option::File option)
         {
            auto flags = ::fcntl( m_descriptor.value(), F_GETFL);
            local::check::result(::fcntl( m_descriptor.value(), F_SETFL, flags | cast::underlying( option)));
         }

         void Socket::unset( socket::option::File option)
         {
            auto flags = ::fcntl( m_descriptor.value(), F_GETFL);
            local::check::result(::fcntl( m_descriptor.value(), F_SETFL, flags & ~cast::underlying( option)));
         }

         Socket& Socket::operator = ( const Socket& other)
         {
            Socket copy{ other};
            return *this = std::move( copy);
         }

         Socket::Socket( Socket&&) noexcept = default;
         Socket& Socket::operator =( Socket&&) noexcept = default;


         Socket::descriptor_type Socket::descriptor() const noexcept
         {
            return m_descriptor;
         }

         std::error_code Socket::error() const
         {
            int optval{};
            socklen_t optlen = sizeof( optval);
            local::check::result( ::getsockopt( m_descriptor.value(), SOL_SOCKET, SO_ERROR, &optval, &optlen));

            return std::make_error_code( std::errc( optval));
         }

         Socket::descriptor_type Socket::release() noexcept
         {
            common::log::line( log, "Socket::release - descriptor: ", m_descriptor);

            return std::exchange( m_descriptor, {});
         }

         namespace socket
         {
            Socket duplicate( descriptor_type descriptor)
            {
               return Socket{ local::duplicate( descriptor)};
            }

         } // socket

      } // communication
   } // common
} // casual

