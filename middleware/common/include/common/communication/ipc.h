//!
//! ipc.h
//!
//! Created on: Jan 4, 2016
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_H_


#include "common/communication/message.h"
#include "common/communication/device.h"

#include "common/flag.h"

namespace casual
{
   namespace common
   {

      namespace communication
      {
         namespace ipc
         {
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
               using Transport = communication::message::basic_transport< platform::ipc::message::size>;
            } // message


            using handle_type = platform::ipc::id::type;





            namespace native
            {
               enum class Flag : long
               {
                  non_blocking = platform::flag::value( platform::flag::ipc::no_wait)
               };

               bool send( handle_type id, const message::Transport& transport, common::Flags< Flag> flags);
               bool receive( handle_type id, message::Transport& transport, common::Flags< Flag> flags);

            } // native


            namespace policy
            {

               template< typename Prefix, typename Policy>
               struct basic_prefix
               {
                  template< typename... Args>
                  auto operator() ( Args&&... arguments) -> decltype( std::declval< Policy>()( std::forward< Args>( arguments)...))
                  {
                     Prefix prefix( arguments...);
                     return m_policy( std::forward< Args>( arguments)...);
                  }
                  Policy m_policy;
               };

               namespace prefix
               {
                  struct Signal
                  {
                     Signal();

                     template< typename... Args>
                     Signal( Args&&...) : Signal() {}
                  };

                  namespace ignore
                  {
                     struct Signal : common::signal::thread::scope::Block
                     {
                        template< typename... Args>
                        Signal( Args&&...) : common::signal::thread::scope::Block() {}
                     };
                  } // ignore

               } // prefix

               struct basic_blocking
               {
                  bool operator() ( inbound::Connector& ipc, message::Transport& transport);
                  bool operator() ( const outbound::Connector& ipc, const message::Transport& transport);
               };

               using Blocking = basic_prefix< prefix::Signal, basic_blocking>;

               namespace non
               {
                  struct basic_blocking
                  {
                     bool operator() ( inbound::Connector& ipc, message::Transport& transport);
                     bool operator() ( const outbound::Connector& ipc, const message::Transport& transport);
                  };

                  using Blocking = basic_prefix< prefix::Signal, basic_blocking>;
               } // non

               namespace ignore
               {
                  namespace signal
                  {
                     using Blocking = basic_prefix< prefix::ignore::Signal, basic_blocking>;

                     namespace non
                     {
                        using Blocking = basic_prefix< prefix::ignore::Signal, policy::non::basic_blocking>;
                     } // non

                  } // signal
               } // ignore

            } // policy


            namespace inbound
            {
               struct Connector
               {
                  using handle_type = ipc::handle_type;
                  using transport_type = ipc::message::Transport;
                  using blocking_policy = typename policy::Blocking;
                  using non_blocking_policy = typename policy::non::Blocking;


                  Connector();
                  ~Connector();

                  Connector( const Connector&) = delete;
                  Connector& operator = ( const Connector&) = delete;

                  Connector( Connector&& rhs) noexcept;
                  Connector& operator = ( Connector&& rhs) noexcept;

                  handle_type id() const;

                  friend void swap( Connector& lhs, Connector& rhs);

                  friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);

               private:

                  enum
                  {
                     cInvalid = -1
                  };

                  handle_type m_id = cInvalid;
               };


               using Device = communication::inbound::Device< Connector>;


               Device& device();
               handle_type id();

            } // inbound

            namespace outbound
            {
               struct Connector
               {
                  using handle_type = ipc::handle_type;
                  using transport_type = ipc::message::Transport;
                  using blocking_policy = typename policy::Blocking;
                  using non_blocking_policy = typename policy::non::Blocking;

                  Connector( handle_type id);

                  operator handle_type() const;

                  handle_type id() const;

                  friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);

               private:
                  handle_type m_id;
               };

               using Device = communication::outbound::Device< Connector>;
            } // outbound




            namespace blocking
            {
               using error_type = typename inbound::Device::error_type;

               template< typename M>
               void receive( inbound::Device& ipc, M& message, const error_type& handler = nullptr)
               {
                  ipc.receive( message, policy::Blocking{}, handler);
               }

               template< typename M>
               bool receive( inbound::Device& ipc, M& message, const Uuid& correlation, const error_type& handler = nullptr)
               {
                  return ipc.receive( message, correlation, policy::Blocking{}, handler);
               }

               inline communication::message::Complete next( inbound::Device& ipc, const error_type& handler = nullptr)
               {
                  return ipc.next( policy::Blocking{}, handler);
               }

               template< typename M>
               Uuid send( outbound::Device ipc, M&& message, const error_type& handler = nullptr)
               {
                  return ipc.send( message, policy::Blocking{}, handler);
               }

               namespace force
               {
                  template< typename M>
                  Uuid send( inbound::Device ipc, M&& message, const error_type& handler = nullptr)
                  {
                     Uuid result;

                     while( ! (result = send( ipc.connector().id(), message, handler)))
                     {
                        ipc.flush();
                     }

                     return result;
                  }
               } // force

            } // blocking

            namespace non
            {
               namespace blocking
               {
                  using error_type = typename inbound::Device::error_type;

                  template< typename M>
                  bool receive( inbound::Device& ipc, M& message, const error_type& handler = nullptr)
                  {
                     return ipc.receive( message, policy::non::Blocking{}, handler);
                  }

                  template< typename M>
                  bool receive( inbound::Device& ipc, M& message, const Uuid& correlation, const error_type& handler = nullptr)
                  {
                     return ipc.receive( message, correlation, policy::non::Blocking{}, handler);
                  }

                  inline communication::message::Complete next( inbound::Device& ipc, const error_type& handler = nullptr)
                  {
                     return ipc.next( policy::non::Blocking{}, handler);
                  }

                  template< typename M>
                  Uuid send( outbound::Device ipc, M&& message, const error_type& handler = nullptr)
                  {
                     return ipc.send( message, policy::non::Blocking{}, handler);
                  }


               } // blocking
            } // non

            template< typename M, typename Policy = policy::ignore::signal::Blocking>
            auto call(
                  outbound::Device destination,
                  M&& message,
                  Policy&& policy = policy::ignore::signal::Blocking{},
                  const error::type& handler = nullptr,
                  inbound::Device& device = ipc::inbound::device())
               -> decltype( common::message::reverse::type( std::forward< M>( message)))
            {
               auto correlation = destination.send( message, policy, handler);

               auto reply = common::message::reverse::type( std::forward< M>( message));
               device.receive( reply, correlation, policy, handler);

               return reply;
            }

            namespace broker
            {
               outbound::Device& device();

               handle_type id();

            } // broker

            bool exists( handle_type id);

            bool remove( handle_type id);
            bool remove( const process::Handle& owner);


            //!
            //! Uses the default inbound::device() for receive, since this is by far the
            //! most common use case.
            //!
            //! Can be instantiated with an error-handler, since we often want to use the same handler
            //! for every ipc-interaction within a module.
            //!
            struct Helper
            {

               inline Helper( std::function<void()> error_handler)
                  : m_error_handler{ std::move( error_handler)}{};
               inline Helper() : Helper( nullptr) {}


               template< typename M>
               auto blocking_send( common::platform::ipc::id::type id, M&& message) const
                -> decltype( common::communication::ipc::blocking::send( id, message, nullptr))
               {
                  return common::communication::ipc::blocking::send( id, message, m_error_handler);
               }

               template< typename M>
               auto non_blocking_send( common::platform::ipc::id::type id, M&& message) const
                -> decltype( common::communication::ipc::blocking::send( id, message, nullptr))
               {
                  return common::communication::ipc::non::blocking::send( id, message, m_error_handler);
               }

               template< typename M, typename... Args>
               auto blocking_receive( M& message, Args&&... args) const
                -> decltype( common::communication::ipc::blocking::receive(
                      common::communication::ipc::inbound::device(), message, std::forward< Args>( args)..., nullptr))
               {
                  return common::communication::ipc::blocking::receive(
                        common::communication::ipc::inbound::device(), message, std::forward< Args>( args)..., m_error_handler);
               }

               template< typename M, typename... Args>
               auto non_blocking_receive( M& message, Args&&... args) const
                -> decltype( common::communication::ipc::non::blocking::receive(
                      common::communication::ipc::inbound::device(), message, std::forward< Args>( args)..., nullptr))
               {
                  return common::communication::ipc::non::blocking::receive(
                        common::communication::ipc::inbound::device(), message, std::forward< Args>( args)..., m_error_handler);
               }

               template< typename... Args>
               common::communication::message::Complete blocking_next( Args&&... args) const
               {
                  return common::communication::ipc::inbound::device().next(
                        std::forward< Args>( args)...,
                        common::communication::ipc::policy::Blocking{},
                        m_error_handler);
               }

               template< typename... Args>
               common::communication::message::Complete non_blocking_next( Args&&... args) const
               {
                  return common::communication::ipc::inbound::device().next(
                        std::forward< Args>( args)...,
                        common::communication::ipc::policy::non::Blocking{},
                        m_error_handler);
               }

               Uuid blocking_push( outbound::Device ipc, const communication::message::Complete& complete) const
               {
                  return ipc.put( complete,
                        common::communication::ipc::policy::Blocking{},
                        m_error_handler);
               }

               Uuid non_blocking_push( outbound::Device ipc, const communication::message::Complete& complete) const
               {
                  return ipc.put( complete,
                        common::communication::ipc::policy::non::Blocking{},
                        m_error_handler);
               }


               inbound::Device& device() const { return inbound::device();}
               platform::ipc::id::type id() const { return inbound::device().connector().id();}

               inline const std::function<void()>& error_handler() const { return m_error_handler;}



            private:
               std::function<void()> m_error_handler;
            };

         } // ipc

      } // communication
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_H_
