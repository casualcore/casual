//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_TCP_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_TCP_H_


#include "common/communication/message.h"
#include "common/communication/device.h"

#include "common/platform.h"
#include "common/flag.h"


#include <string>

namespace casual
{
   namespace common
   {

      namespace communication
      {
         namespace tcp
         {

            struct Address
            {
               struct Host : std::string
               {
                  Host( std::string s) : std::string{ std::move( s)} {}
               };

               struct Port : public std::string
               {
                  Port( std::string s) : std::string{ std::move( s)} {}
                  Port( long port) : std::string{ std::to_string( port)} {}
               };

               Address( const std::string& address);
               Address( Port port);
               Address( Host host, Port port);


               friend std::ostream& operator << ( std::ostream& out, const Address& value);

               std::string host;
               std::string port;
            };

            namespace socket
            {
               using descriptor_type = platform::tcp::descriptor::type;

               namespace address
               {

                  //!
                  //! @return The address to which the socket is bound to on local host
                  //!
                  Address host( descriptor_type descriptor);

                  //!
                  //! @return The address of the peer connected to the socket
                  //!
                  Address peer( descriptor_type descriptor);

               } // address

            } // socket

            class Socket
            {
            public:

               using descriptor_type = socket::descriptor_type;

               Socket() noexcept;
               ~Socket() noexcept;

               Socket( const Socket&);
               Socket& operator = ( const Socket&);

               Socket( Socket&&) noexcept;
               Socket& operator = ( Socket&&) noexcept;

               explicit operator bool () const noexcept;

               //!
               //! Releases the responsibility of the socket
               //!
               //! @return descriptor
               //!
               descriptor_type release() noexcept;

               descriptor_type descriptor() const noexcept;

               friend Socket adopt( socket::descriptor_type descriptor);
               friend std::ostream& operator << ( std::ostream& out, const Socket& value);


            private:
               Socket( descriptor_type descriptor) noexcept;

               descriptor_type m_descriptor = -1;
               move::Moved m_moved;

            };

            Socket adopt( socket::descriptor_type descriptor);

            //!
            //! Duplicates the descriptor
            //!
            //! @param descriptor to be duplicated
            //! @return socket that owns the descriptor
            //!
            Socket duplicate( socket::descriptor_type descriptor);

            Socket connect( const Address& address);

            namespace retry
            {
               //!
               //! Keeps trying to connect
               //!
               //! @param address to connect to
               //! @param sleep sleep pattern
               //! @return created socket
               //!
               Socket connect( const Address& address, process::pattern::Sleep sleep);
            } // retry


            class Listener
            {
            public:
               Listener( Address adress);

               Socket operator() () const;

            private:
               Socket m_listener;
            };




            //
            // Forwards
            //
            namespace inbound
            {
               struct Connector;
            } // inbound

            namespace outbound
            {
               struct Connector;
            } // inbound


            namespace message
            {
               using Transport = communication::message::basic_transport< platform::tcp::message::size>;

            } // message


            namespace native
            {
               enum class Flag : long
               {
                  non_blocking = platform::flag::value( platform::flag::tcp::no_wait)
               };

               bool send( const Socket& socket, const message::Transport& transport, common::Flags< Flag> flags);
               bool receive( const Socket& socket, message::Transport& transport, common::Flags< Flag> flags);

            } // native


            namespace policy
            {
               struct basic_blocking
               {
                  bool receive( const inbound::Connector& ipc, message::Transport& transport);
                  bool send( const outbound::Connector& tcp, const message::Transport& transport);
               };

               using Blocking = basic_blocking;

               namespace non
               {
                  struct basic_blocking
                  {
                     bool receive( const inbound::Connector& ipc, message::Transport& transport);
                     bool send( const outbound::Connector& tcp, const message::Transport& transport);
                  };

                  using Blocking = basic_blocking;
               } // non

            } // policy

            struct base_connector
            {
               using handle_type = tcp::Socket;
               using transport_type = tcp::message::Transport;
               using blocking_policy = policy::Blocking;
               using non_blocking_policy = policy::non::Blocking;

               base_connector() noexcept;
               base_connector( tcp::Socket&& socket) noexcept;
               base_connector( const tcp::Socket& socket);

               base_connector( const base_connector&) = delete;
               base_connector& operator = ( const base_connector&) = delete;

               const handle_type& socket() const;

               friend std::ostream& operator << ( std::ostream& out, const base_connector& rhs);

            private:
               handle_type m_socket;
            };

            namespace inbound
            {
               struct Connector : base_connector
               {
                  using base_connector::base_connector;
               };

               using Device = communication::inbound::Device< Connector>;

            } // inbound

            namespace outbound
            {
               struct Connector : base_connector
               {
                  using base_connector::base_connector;

                  inline void reconnect() const { throw; }
               };

               using Device = communication::outbound::Device< Connector>;
            } // outbound


         } // tcp
      } // communication
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_TCP_H_
