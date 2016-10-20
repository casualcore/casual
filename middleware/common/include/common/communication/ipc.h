//!
//! casual
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

               struct Blocking
               {
                  template< typename Connector>
                  bool receive( Connector&& ipc, message::Transport& transport)
                  {
                     return native::receive( ipc.id(), transport, {});
                  }

                  template< typename Connector>
                  bool send( Connector&& connector, const message::Transport& transport)
                  {
                     return native::send( std::forward< Connector>( connector), transport, {});
                  }

               };

               namespace non
               {
                  struct Blocking
                  {
                     template< typename Connector>
                     bool receive( Connector&& connector, message::Transport& transport)
                     {
                        return native::receive( connector.id(), transport, native::Flag::non_blocking);
                     }

                     template< typename Connector>
                     bool send( Connector&& connector, const message::Transport& transport)
                     {
                        return native::send( connector, transport, native::Flag::non_blocking);
                     }

                  };

               } // non

            } // policy


            namespace inbound
            {
               struct Connector
               {
                  using handle_type = ipc::handle_type;
                  using transport_type = ipc::message::Transport;
                  using blocking_policy = policy::Blocking;
                  using non_blocking_policy = policy::non::Blocking;


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
                  using blocking_policy = policy::Blocking;
                  using non_blocking_policy = policy::non::Blocking;

                  Connector( handle_type id);

                  operator handle_type() const;

                  handle_type id() const;

                  inline void reconnect() const { throw; }

                  friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);

               protected:
                  handle_type m_id;
               };

               using Device = communication::outbound::Device< Connector>;

               namespace instance
               {
                  template< process::instance::fetch::Directive directive>
                  struct Connector
                  {
                     using handle_type = ipc::handle_type;
                     using transport_type = ipc::message::Transport;
                     using blocking_policy = policy::Blocking;
                     using non_blocking_policy = policy::non::Blocking;

                     Connector( const Uuid& identity, std::string environment);

                     inline operator handle_type() const { return m_process.queue;}
                     inline handle_type id() const { return m_process.queue;}
                     inline const common::process::Handle& process() const { return m_process;}

                     void reconnect();

                     friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);

                  private:
                     common::process::Handle m_process;
                     Uuid m_identity;
                     std::string m_environment;
                  };

                  //!
                  //! Will wait until the instance is online, could block for ever.
                  //!
                  using Device = communication::outbound::Device< Connector< process::instance::fetch::Directive::wait>>;

                  namespace optional
                  {

                     //!
                     //! Will fail if the instance is offline.
                     //!
                     using Device = communication::outbound::Device< Connector< process::instance::fetch::Directive::direct>>;

                  } // optional
               } // instance

               namespace domain
               {
                  struct Connector : outbound::Connector
                  {
                     Connector();
                     void reconnect();
                  };
                  using Device = communication::outbound::Device< Connector>;
               } // domain

            } // outbound


            using error_type = typename outbound::Device::error_type;

            template< typename D, typename M, typename P>
            auto send( D& ipc, M&& message, P&& policy, const error_type& handler = nullptr)
             -> typename std::enable_if< ! std::is_same< D, platform::ipc::id::type>::value, Uuid>::type
            {
               return ipc.send( message, policy, handler);
            }

            template< typename M, typename P>
            Uuid send( platform::ipc::id::type ipc, M&& message, P&& policy, const error_type& handler = nullptr)
            {
               return outbound::Device{ ipc}.send( message, policy, handler);
            }


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

               template< typename D, typename M>
               Uuid send( D&& ipc, M&& message, const error_type& handler = nullptr)
               {
                  return ipc::send( std::forward< D>( ipc), message, policy::Blocking{}, handler);
               }

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

                  template< typename D, typename M>
                  Uuid send( D&& ipc, M&& message, const error_type& handler = nullptr)
                  {
                     return ipc::send( std::forward< D>( ipc), message, policy::non::Blocking{}, handler);
                  }
               } // blocking
            } // non

            template< typename D, typename M, typename Policy = policy::Blocking>
            auto call(
                  D&& destination,
                  M&& message,
                  Policy&& policy = policy::Blocking{},
                  const error::type& handler = nullptr,
                  inbound::Device& device = ipc::inbound::device())
               -> decltype( common::message::reverse::type( std::forward< M>( message)))
            {
               auto correlation = ipc::send( std::forward< D>( destination), message, policy, handler);

               auto reply = common::message::reverse::type( std::forward< M>( message));
               device.receive( reply, correlation, policy, handler);

               return reply;
            }

            namespace broker
            {
               outbound::instance::Device& device();
            } // broker

            namespace transaction
            {
               namespace manager
               {
                  outbound::instance::Device& device();
               } // manager
            } // transaction

            namespace gateway
            {
               namespace manager
               {
                  outbound::instance::Device& device();


                  namespace optional
                  {
                     //!
                     //! Can be missing. That is, this will not block
                     //! until the device is found (the gateway is online)
                     //!
                     //! @return device to gateway-manager
                     //!
                     outbound::instance::optional::Device& device();
                  } // optional
               } // manager
            } // gateway

            namespace queue
            {
               namespace broker
               {
                  outbound::instance::Device& device();

                  namespace optional
                  {
                     //!
                     //! Can be missing. That is, this will not block
                     //! until the device is found (the queue is online)
                     //!
                     //! @return device to queue-broker
                     //!
                     outbound::instance::optional::Device& device();
                  } // optional

               } // broker
            } // queue


            namespace domain
            {
               namespace manager
               {
                  outbound::domain::Device& device();
               } // manager
            } // domain

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


               template< typename D, typename M>
               auto blocking_send( D&& device, M&& message) const
                -> decltype( common::communication::ipc::blocking::send( device, message, nullptr))
               {
                  return common::communication::ipc::blocking::send( std::forward< D>( device), message, m_error_handler);
               }

               template< typename D, typename M>
               auto non_blocking_send( D&& device, M&& message) const
                -> decltype( common::communication::ipc::blocking::send( device, message, nullptr))
               {
                  return common::communication::ipc::non::blocking::send( std::forward< D>( device), message, m_error_handler);
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
               common::communication::message::Complete next( Args&&... args) const
               {
                  return common::communication::ipc::inbound::device().next(
                        std::forward< Args>( args)...,
                        m_error_handler);
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

               template< typename D, typename M>
               auto call( D&& ipc, M&& message) const -> decltype( common::message::reverse::type( std::forward< M>( message)))
               {
                  return ipc::call(
                        std::forward< D>( ipc),
                        std::forward< M>( message),
                        common::communication::ipc::policy::Blocking{},
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
