//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_H_


#include "common/communication/message.h"
#include "common/communication/device.h"

#include "common/platform.h"

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
               namespace transport
               {
                  struct Header
                  {
                     using correalation_type = Uuid::uuid_type;

                     //!
                     //! The message correlation id
                     //!
                     correalation_type correlation;

                     //!
                     //! which offset this transport message represent of the complete message
                     //!
                     std::int64_t offset;

                     //!
                     //! Size of payload in this transport message
                     //!
                     std::int64_t count;

                     //!
                     //! size of the logical complete message
                     //!
                     std::int64_t complete_size;
                  };

                  constexpr std::int64_t max_message_size() { return platform::ipc::message::size;}
                  constexpr std::int64_t header_size() { return sizeof( Header);}
                  constexpr std::int64_t max_payload_size() { return max_message_size() - header_size();}

               } // transport

               struct Transport
               {
                  using correalation_type = Uuid::uuid_type;

                  using payload_type = std::array< char, transport::max_payload_size()>;
                  using range_type = range::type_t< payload_type>;
                  using const_range_type = range::const_type_t< payload_type>;

                  struct message_t
                  {
                     //!
                     //! Which logical type of message this transport is carrying
                     //!
                     //! @attention has to be the first bytes in the message
                     //!
                     platform::ipc::message::type type;

                     transport::Header header;
                     payload_type payload;

                  } message;


                  static_assert( transport::max_message_size() - transport::max_payload_size() < transport::max_payload_size(), "Payload is to small");
                  static_assert( std::is_pod< message_t>::value, "Message has be a POD");
                  //static_assert( sizeof( message_t) - header_size() == max_payload_size(), "something is wrong with padding");


                  inline Transport() { memory::set( message);}

                  inline Transport( common::message::Type type, std::size_t complete_size) : Transport()
                  {
                     Transport::type( type);
                     message.header.complete_size = complete_size;
                  }

                  inline Transport( common::message::Type type) : Transport() { Transport::type( type);}



                  //!
                  //! @return the message type
                  //!
                  inline common::message::Type type() const { return static_cast< common::message::Type>( message.type);}

                  //!
                  //! Sets the message type
                  //!
                  //! @param type type to set
                  //!
                  inline void type( common::message::Type type) { message.type = static_cast< platform::ipc::message::type>( type);}


                  inline const_range_type payload() const
                  {
                     return range::make( std::begin( message.payload), message.header.count);
                  }


                  inline const correalation_type& correlation() const { return message.header.correlation;}
                  inline correalation_type& correlation() { return message.header.correlation;}

                  //!
                  //! @return payload size
                  //!
                  inline std::size_t pyaload_size() const { return message.header.count;}

                  //!
                  //! @return the offset of the logical complete message this transport
                  //!    message represent.
                  //!
                  inline std::size_t pyaload_offset() const { return message.header.offset;}


                  //!
                  //! @return the size of the complete logical message
                  //!
                  inline std::size_t complete_size() const { return message.header.complete_size;}


                  //!
                  //! @return the total size of the transport message including header.
                  //!
                  inline std::size_t size() const { return transport::header_size() + pyaload_size();}


                  //!
                  //! Indication if this transport message is the last of the logical message.
                  //!
                  //! @return true if this transport message is the last of the logical message.
                  //!
                  //! @attention this does not give any guarantees that no more transport messages will arrive...
                  //!
                  inline bool last() const { return message.header.offset + message.header.count == message.header.complete_size;}


                  auto begin() { return std::begin( message.payload);}
                  inline auto begin() const { return std::begin( message.payload);}
                  auto end() { return begin() + message.header.count;}
                  auto end() const { return begin() + message.header.count;}

                  template< typename Iter>
                  void assign( Iter first, Iter last)
                  {
                     message.header.count = std::distance( first, last);
                     assert( first <= last && message.header.count <=  transport::max_payload_size());

                     std::copy( first, last, std::begin( message.payload));
                  }

                  friend std::ostream& operator << ( std::ostream& out, const Transport& value);
               };


               inline std::size_t offset( const Transport& value) { return value.message.header.offset;}

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
               using cache_type = communication::inbound::cache_type;
               using cache_range_type =  communication::inbound::cache_range_type;

               cache_range_type receive( handle_type id, cache_type& cache, common::Flags< native::Flag> flags);
               Uuid send( handle_type id, const communication::message::Complete& complete, common::Flags< native::Flag> flags);

               struct Blocking
               {

                  template< typename Connector>
                  cache_range_type receive( Connector&& connector, cache_type& cache)
                  {
                     return policy::receive( connector.id(), cache, {});
                  }

                  template< typename Connector>
                  Uuid send( Connector&& connector, const communication::message::Complete& complete)
                  {
                     return policy::send( std::forward< Connector>( connector), complete, {});
                  }


               };

               namespace non
               {
                  struct Blocking
                  {
                     template< typename Connector>
                     cache_range_type receive( Connector&& connector, cache_type& cache)
                     {
                        return policy::receive( connector.id(), cache, native::Flag::non_blocking);
                     }

                     template< typename Connector>
                     Uuid send( Connector&& connector, const communication::message::Complete& complete)
                     {
                        return policy::send( std::forward< Connector>( connector), complete, native::Flag::non_blocking);
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

               template< typename S>
               using basic_device = communication::inbound::Device< Connector, S>;

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
                    
                     template< process::instance::fetch::Directive d> 
                     friend std::ostream& operator << ( std::ostream& out, const Connector< d>& rhs);

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
             -> std::enable_if_t< ! std::is_same< D, platform::ipc::id::type>::value, Uuid>
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

               template< typename S, typename M>
               bool receive( inbound::basic_device< S>& ipc, M& message, const error_type& handler = nullptr)
               {
                  return ipc.receive( message, policy::Blocking{}, handler);
               }

               template< typename S, typename M>
               bool receive( inbound::basic_device< S>& ipc, M& message, const Uuid& correlation, const error_type& handler = nullptr)
               {
                  return ipc.receive( message, correlation, policy::Blocking{}, handler);
               }

               template< typename S>
               inline communication::message::Complete next( inbound::basic_device< S>& ipc, const error_type& handler = nullptr)
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

                  template< typename S, typename M>
                  bool receive( inbound::basic_device< S>& ipc, M& message, const error_type& handler = nullptr)
                  {
                     return ipc.receive( message, policy::non::Blocking{}, handler);
                  }

                  template< typename S, typename M>
                  bool receive( inbound::basic_device< S>& ipc, M& message, const Uuid& correlation, const error_type& handler = nullptr)
                  {
                     return ipc.receive( message, correlation, policy::non::Blocking{}, handler);
                  }

                  template< typename S>
                  inline communication::message::Complete next( inbound::basic_device< S>& ipc, const error_type& handler = nullptr)
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

            namespace receive
            {
               enum class Flag : char
               {
                  blocking,
                  non_blocking
               };

               template< typename D, typename M, typename... Args>
               bool message( D& device, M& message, Flag flag, Args&&... args)
               {
                  if( flag == Flag::blocking)
                  {
                     return blocking::receive( device, message, std::forward< Args>( args)...);
                  }
                  else
                  {
                     return non::blocking::receive( device, message, std::forward< Args>( args)...);
                  }
               }

            } // receive

            template< typename D, typename M, typename Policy = policy::Blocking>
            auto call(
                  D&& destination,
                  M&& message,
                  Policy&& policy = policy::Blocking{},
                  const error::type& handler = nullptr,
                  inbound::Device& device = ipc::inbound::device())
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



               template< typename... Args>
               static auto handler( Args&&... args) -> decltype( common::communication::ipc::inbound::device().handler())
               {
                  return { std::forward< Args>( args)...};
               }

               template< typename D, typename M>
               auto blocking_send( D&& device, M&& message) const
               {
                  return common::communication::ipc::blocking::send( std::forward< D>( device), message, m_error_handler);
               }

               template< typename D, typename M>
               auto non_blocking_send( D&& device, M&& message) const
               {
                  return common::communication::ipc::non::blocking::send( std::forward< D>( device), message, m_error_handler);
               }

               template< typename M, typename... Args>
               auto blocking_receive( M& message, Args&&... args) const
               {
                  return common::communication::ipc::blocking::receive(
                        common::communication::ipc::inbound::device(), message, std::forward< Args>( args)..., m_error_handler);
               }

               template< typename M, typename... Args>
               auto non_blocking_receive( M& message, Args&&... args) const
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

            namespace dispatch
            {
               using Handler =  typename inbound::Device::handler_type;
            } // dispatch

         } // ipc

      } // communication
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_IPC_H_
