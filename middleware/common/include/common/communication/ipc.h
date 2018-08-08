//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/device.h"
#include "common/communication/socket.h"

#include "common/platform.h"
#include "common/strong/id.h"
#include "common/uuid.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/flag.h"
#include "common/file.h"



#include <sys/un.h>

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace ipc
         {
            using size_type = platform::size::type;

            namespace message
            {
               namespace transport
               {
                  struct Header
                  {
                     //!
                     //! Which logical type of message this transport is carrying
                     //!
                     //! @attention has to be the first bytes in the message
                     //!
                     common::message::Type type;

                     using correlation_type = Uuid::uuid_type;

                     //!
                     //! The message correlation id
                     //!
                     correlation_type correlation;

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

                  constexpr std::int64_t max_message_size() { return platform::ipc::transport::size;}
                  constexpr std::int64_t header_size() { return sizeof( Header);}
                  constexpr std::int64_t max_payload_size() { return max_message_size() - header_size();}

               } // transport

               struct Transport
               {
                  using correlation_type = Uuid::uuid_type;

                  using payload_type = std::array< char, transport::max_payload_size()>;
                  using range_type = range::type_t< payload_type>;
                  using const_range_type = range::const_type_t< payload_type>;

                  struct message_t
                  {

                     transport::Header header;
                     payload_type payload;

                  } message{}; // note the {} which initialize the memory to 0:s


                  static_assert( transport::max_message_size() - transport::max_payload_size() < transport::max_payload_size(), "Payload is to small");
                  static_assert( std::is_trivially_copyable< message_t>::value, "Message has to be trivially copyable");
                  static_assert( ( transport::header_size() + transport::max_payload_size()) == transport::max_message_size(), "something is wrong with padding");


                  inline Transport() = default;

                  inline Transport( common::message::Type type, size_type complete_size)
                  {
                     message.header.type = type;
                     message.header.complete_size = complete_size;
                  }
                  //!
                  //! @return the message type
                  //!
                  inline common::message::Type type() const { return static_cast< common::message::Type>( message.header.type);}

                  inline const_range_type payload() const
                  {
                     return range::make( std::begin( message.payload), message.header.count);
                  }

                  inline const correlation_type& correlation() const { return message.header.correlation;}
                  inline correlation_type& correlation() { return message.header.correlation;}

                  //!
                  //! @return payload size
                  //!
                  inline size_type payload_size() const { return message.header.count;}

                  //!
                  //! @return the offset of the logical complete message this transport
                  //!    message represent.
                  //!
                  inline size_type payload_offset() const { return message.header.offset;}


                  //!
                  //! @return the size of the complete logical message
                  //!
                  inline size_type complete_size() const { return message.header.complete_size;}


                  //!
                  //! @return the total size of the transport message including header.
                  //!
                  inline size_type size() const { return transport::header_size() + payload_size();}


                  inline void* data() { return static_cast< void*>( &message);}
                  inline const void* data() const { return static_cast< const void*>( &message);}

                  inline void* header_data() { return static_cast< void*>( &message.header);}
                  inline void* payload_data() { return static_cast< void*>( &message.payload);}


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

                  template< typename R>
                  void assign( R&& range)
                  {
                     message.header.count = range.size();
                     assert( message.header.count <=  transport::max_payload_size());

                     algorithm::copy( range, std::begin( message.payload));
                  }

                  friend std::ostream& operator << ( std::ostream& out, const Transport& value);
               };


               inline size_type offset( const Transport& value) { return value.message.header.offset;}

            } // message


            struct Handle
            {

               explicit Handle( Socket&& socket, strong::ipc::id ipc);
               Handle( Handle&& other) noexcept;
               Handle& operator = ( Handle&& other) noexcept;
               ~Handle();

               inline const Socket& socket() const { return m_socket;}
               inline strong::ipc::id ipc() const { return m_ipc;}

               inline explicit operator bool () const { return ! m_socket.empty();}

               friend std::ostream& operator << ( std::ostream& out, const Handle& rhs);
            private:
               Socket m_socket;
               strong::ipc::id m_ipc;
            };

            static_assert( sizeof( Handle) == sizeof( Socket) + sizeof( strong::ipc::id), "padding problem");

            struct Address
            {
               explicit Address( strong::ipc::id id);

               inline const ::sockaddr_un& native() const noexcept { return m_native;}

               inline const ::sockaddr* native_pointer() const noexcept { return reinterpret_cast< const ::sockaddr*>( &m_native);}
               inline auto native_size() const noexcept { return sizeof( m_native);}

               friend std::ostream& operator << ( std::ostream& out, const Address& rhs);

            private:
               ::sockaddr_un m_native = {};
            };

            namespace native
            {
               namespace detail
               {
                  namespace create
                  {
                     namespace domain
                     {
                        Socket socket();
                     } // domain
                  } // create
                  namespace outbound
                  {
                     const Socket& socket();
                  } // outbound
               } // detail

               enum class Flag : int
               {
                  none = 0,
                  non_blocking = platform::flag::value( platform::flag::tcp::no_wait)
               };

               bool send( const Socket& socket, const Address& destination, const message::Transport& transport, Flag flag);
               bool receive( const Handle& handle, message::Transport& transport, Flag flag);

               namespace blocking
               {
                  bool send( const Socket& socket, const Address& destination, const message::Transport& transport);
                  bool receive( const Handle& handle, message::Transport& transport);
               } // blocking

               namespace non
               {
                  namespace blocking
                  {
                     bool send( const Socket& socket, const Address& destination, const message::Transport& transport);
                     bool receive( const Handle& handle, message::Transport& transport);
                  } // blocking
               } // non
            } // native


            namespace policy
            {
               using cache_type = communication::inbound::cache_type;
               using cache_range_type = communication::inbound::cache_range_type;

               namespace blocking
               {
                  cache_range_type receive( Handle& handle, cache_type& cache);
                  Uuid send( const Socket& socket, const Address& destination, const communication::message::Complete& complete);
               } // blocking


               struct Blocking
               {

                  template< typename Connector>
                  cache_range_type receive( Connector&& connector, cache_type& cache)
                  {
                     return policy::blocking::receive( connector.handle(), cache);
                  }

                  template< typename Connector>
                  Uuid send( Connector&& connector, const communication::message::Complete& complete)
                  {
                     return policy::blocking::send( connector.socket(), connector.destination(), complete);
                  }
               };

               namespace non
               {
                  namespace blocking
                  {
                     cache_range_type receive( Handle& handle, cache_type& cache);
                     Uuid send( const Socket& socket, const Address& destination, const communication::message::Complete& complete);
                  } // blocking

                  struct Blocking
                  {
                     template< typename Connector>
                     cache_range_type receive( Connector&& connector, cache_type& cache)
                     {
                        return policy::non::blocking::receive( connector.handle(), cache);
                     }

                     template< typename Connector>
                     Uuid send( Connector&& connector, const communication::message::Complete& complete)
                     {
                        return policy::non::blocking::send( connector.socket(), connector.destination(), complete);
                     }
                  };

               } // non

            } // policy


            namespace inbound
            {
               struct Connector
               {
                  using transport_type = message::Transport;
                  using blocking_policy = policy::Blocking;
                  using non_blocking_policy = policy::non::Blocking;


                  Connector();
                  ~Connector();

                  Connector( Connector&&) noexcept = default;
                  Connector& operator = ( Connector&&) noexcept = default;

                  inline const Handle& handle() const { return m_handle;}
                  inline Handle& handle() { return m_handle;}
                  inline auto descriptor() const { return m_handle.socket().descriptor();}

                  friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);
               private:
                  Handle m_handle;
               };

               template< typename S>
               using basic_device = communication::inbound::Device< Connector, S>;

               using Device = communication::inbound::Device< Connector>;


               Device& device();
               inline Handle& handle() { return device().connector().handle();}
               inline strong::ipc::id ipc() { return device().connector().handle().ipc();}

            } // inbound

            namespace outbound
            {

               struct Connector
               {
                  using transport_type = message::Transport;
                  using blocking_policy = policy::Blocking;
                  using non_blocking_policy = policy::non::Blocking;

                  Connector( strong::ipc::id destination);
                  ~Connector() = default;

                  inline const Address& destination() const { return m_destination;}
                  inline const Socket& socket() const { return native::detail::outbound::socket();}

                  inline void reconnect() const { throw; }

                  friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);
               protected:
                  Address m_destination;
               };

               template< typename S>
               using basic_device = communication::outbound::Device< Connector, S>;

               using Device = communication::outbound::Device< Connector>;

            } // outbound



            using error_type = typename outbound::Device::error_type;

            template< typename D, typename M, typename P>
            auto send( D& device, M&& message, P&& policy, const error_type& handler = nullptr)
             -> std::enable_if_t< ! std::is_same< std::decay_t< D>, strong::ipc::id>::value, Uuid>
            {
               return device.send( message, policy, handler);
            }

            template< typename M, typename P>
            Uuid send( strong::ipc::id ipc, M&& message, P&& policy, const error_type& handler = nullptr)
            {
               return outbound::Device{ ipc}.send( message, policy, handler);
            }



            namespace blocking
            {
               using error_type = typename inbound::Device::error_type;

               template< typename S, typename M>
               bool receive( inbound::basic_device< S>& device, M& message, const error_type& handler = nullptr)
               {
                  return device.receive( message, policy::Blocking{}, handler);
               }

               template< typename S, typename M>
               bool receive( inbound::basic_device< S>& device, M& message, const Uuid& correlation, const error_type& handler = nullptr)
               {
                  return device.receive( message, correlation, policy::Blocking{}, handler);
               }

               template< typename S>
               inline communication::message::Complete next( inbound::basic_device< S>& device, const error_type& handler = nullptr)
               {
                  return device.next( policy::Blocking{}, handler);
               }

               template< typename D, typename M>
               Uuid send( D&& device, M&& message, const error_type& handler = nullptr)
               {
                  return ipc::send( std::forward< D>( device), message, policy::Blocking{}, handler);
               }

            } // blocking

            namespace non
            {
               namespace blocking
               {
                  using error_type = typename inbound::Device::error_type;

                  template< typename S, typename M>
                  bool receive( inbound::basic_device< S>& device, M& message, const error_type& handler = nullptr)
                  {
                     return device.receive( message, policy::non::Blocking{}, handler);
                  }

                  template< typename S, typename M>
                  bool receive( inbound::basic_device< S>& device, M& message, const Uuid& correlation, const error_type& handler = nullptr)
                  {
                     return device.receive( message, correlation, policy::non::Blocking{}, handler);
                  }

                  template< typename S>
                  inline communication::message::Complete next( inbound::basic_device< S>& device, const error_type& handler = nullptr)
                  {
                     return device.next( policy::non::Blocking{}, handler);
                  }

                  template< typename D, typename M>
                  Uuid send( D&& device, M&& message, const error_type& handler = nullptr)
                  {
                     return ipc::send( std::forward< D>( device), message, policy::non::Blocking{}, handler);
                  }
               } // blocking
            } // non

            template< typename D, typename M, typename Policy = policy::Blocking, typename Device = inbound::Device>
            auto call(
                  D&& destination,
                  M&& message,
                  Policy&& policy = policy::Blocking{},
                  const error::type& handler = nullptr,
                  Device& device = inbound::device())
            {
               auto correlation = send( std::forward< D>( destination), message, policy, handler);

               auto reply = common::message::reverse::type( std::forward< M>( message));
               device.receive( reply, correlation, policy, handler);

               return reply;
            }


            bool remove( strong::ipc::id id);
            bool remove( const process::Handle& owner);

            bool exists( strong::ipc::id id);

            namespace dispatch
            {
               using Handler =  typename inbound::Device::handler_type;
            } // dispatch

            struct Helper
            {
               inline Helper(std::function<void()> error_handler)
                     : m_error_handler{std::move(error_handler)} {};
               inline Helper() : Helper(nullptr) {}

               template< typename... Args>
               static auto handler( Args&&... args) -> decltype( common::communication::ipc::inbound::device().handler())
               {
                  return {std::forward<Args>(args)...};
               }

               template< typename D, typename M>
               auto blocking_send( D&& device, M &&message) const
               {
                  return common::communication::ipc::blocking::send( std::forward<D>(device), message, m_error_handler);
               }

               template< typename D, typename M>
               auto non_blocking_send( D&& device, M &&message) const
               {
                  return common::communication::ipc::non::blocking::send( std::forward<D>(device), message, m_error_handler);
               }

               template< typename M, typename... Args>
               auto blocking_receive( M& message, Args &&... args) const
               {
                  return common::communication::ipc::blocking::receive(
                        common::communication::ipc::inbound::device(), message, std::forward<Args>(args)..., m_error_handler);
               }

               template< typename M, typename... Args>
               auto non_blocking_receive( M& message, Args &&... args) const
               {
                  return common::communication::ipc::non::blocking::receive(
                        common::communication::ipc::inbound::device(), message, std::forward<Args>(args)..., m_error_handler);
               }

               template< typename... Args>
               common::communication::message::Complete next(Args &&... args) const
               {
                  return common::communication::ipc::inbound::device().next(
                        std::forward<Args>(args)...,
                        m_error_handler);
               }

               template< typename... Args>
               common::communication::message::Complete blocking_next( Args &&... args) const
               {
                  return common::communication::ipc::inbound::device().next(
                        std::forward<Args>(args)...,
                        common::communication::ipc::policy::Blocking{},
                        m_error_handler);
               }

               template< typename... Args>
               common::communication::message::Complete non_blocking_next( Args &&... args) const
               {
                  return common::communication::ipc::inbound::device().next(
                        std::forward<Args>(args)...,
                        common::communication::ipc::policy::non::Blocking{},
                        m_error_handler);
               }

               Uuid blocking_push(outbound::Device ipc, const communication::message::Complete &complete) const
               {
                  return ipc.put(complete,
                                 common::communication::ipc::policy::Blocking{},
                                 m_error_handler);
               }
         
               Uuid non_blocking_push(outbound::Device ipc, const communication::message::Complete &complete) const
               {
                  return ipc.put(complete,
                                 common::communication::ipc::policy::non::Blocking{},
                                 m_error_handler);
               }

               template< typename D, typename M>
               auto call(D &&ipc, M &&message) const -> decltype(common::message::reverse::type(std::forward<M>(message)))
               {
                  return ipc::call(
                     std::forward<D>(ipc),
                     std::forward<M>(message),
                     common::communication::ipc::policy::Blocking{},
                     m_error_handler);
               }

               inbound::Device &device() const { return inbound::device();}
               decltype( auto) ipc() const { return inbound::ipc();}

               inline const std::function<void()> &error_handler() const { return m_error_handler;}

            private:
               std::function<void()> m_error_handler;

            };

         } // ipc
      } // communication
   } // common
} // casual