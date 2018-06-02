//!
//! casual
//!

#include "common/communication/domain.h"

#include "common/signal.h"
#include "common/platform.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace domain
         {
            namespace local
            {
               namespace
               {
                  namespace socket
                  {
                     ::sockaddr_un address( const Address& source)
                     {
                        ::sockaddr_un result = {};

                        auto&& name = source.name();
                        result.sun_family = AF_UNIX;
                        algorithm::copy_max( name, result.sun_path);

                        return result;
                     }
                  } // socket

                  void bind( const Socket& socket, const ::sockaddr_un& address)
                  {
                     // bind client to client_filename
                     ::bind( socket.descriptor().underlaying(), reinterpret_cast< const ::sockaddr*>( &address), sizeof( address));
                  }

                  namespace check
                  {
                     void error( common::code::system last_error)
                     {
                        using system = common::code::system;
                        switch( last_error)
                        {
                           case system::interrupted:
                           {
                              common::signal::handle();

                              //
                              // we got a signal we don't have a handle for
                              // We fall through
                              //

                           } // @fallthrough
                           default:
                           {
                              exception::system::throw_from_errno();
                           }
                        }
                     }

                     auto result( int result)
                     {
                        if( result == -1)
                        {
                           check::error( common::code::last::system::error());
                        }
                        return result;
                     }
                  } // check

               } // <unnamed>
            } // local

            namespace native
            {
               Socket create()
               {
                  return Socket{ Socket::descriptor_type{ ::socket( AF_UNIX, SOCK_SEQPACKET, 0)}};
               }

               void bind( const Socket& socket, const Address& address)
               {
                  auto destination_address = local::socket::address( address);

                  local::bind( socket, destination_address);
               }

               void listen( const Socket& socket)
               {
                  local::check::result( ::listen( socket.descriptor().underlaying(), platform::communication::domain::backlog));
               }
               Socket accept( const Socket& socket)
               {
                  //local::check::result( ::accept( socket.descriptor().underlaying(), platform::communication::domain::backlog));
                  return {};
               }

               Uuid send( const Socket& socket, const communication::message::Complete& complete, common::Flags< Flag> flags)
               {
                  return {};
               }

               communication::message::Complete receive( const Socket& socket, common::Flags< Flag> flags)
               {
                  return {};
               }
            } // native

            void connect( const Socket& socket, const Address& address)
            {
               auto destination_address = local::socket::address( address);

               local::bind( socket, destination_address);
               
            }
            
         } // domain
      } // communication
   } // common
} // casual