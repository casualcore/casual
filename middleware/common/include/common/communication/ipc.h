//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/device.h"
#include "common/communication/socket.h"
#include "common/communication/log.h"

#include "casual/platform.h"
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
                     //! Which logical type of message this transport is carrying
                     //! @attention has to be the first bytes in the message
                     common::message::Type type;

                     using correlation_type = Uuid::uuid_type;

                     //! The message correlation id
                     correlation_type correlation;

                     //! which offset this transport message represent of the complete message
                     std::int64_t offset;

                     //! Size of payload in this transport message
                     std::int64_t count;

                     //! size of the logical complete message
                     std::int64_t size;

                     friend std::ostream& operator << ( std::ostream& out, const Header& value);
                  };

                  constexpr std::int64_t max_message_size = platform::ipc::transport::size;
                  constexpr std::int64_t header_size = sizeof( Header);
                  constexpr std::int64_t max_payload_size = max_message_size - header_size;

               } // transport

               struct Transport
               {
                  using correlation_type = Uuid::uuid_type;

                  using payload_type = std::array< char, transport::max_payload_size>;
                  using range_type = range::type_t< payload_type>;
                  using const_range_type = range::const_type_t< payload_type>;

                  struct message_t
                  {
                     transport::Header header;
                     payload_type payload;

                  } message{}; // note the {} which initialize the memory to 0:s


                  static_assert( transport::max_message_size - transport::max_payload_size < transport::max_payload_size, "Payload is to small");
                  static_assert( std::is_trivially_copyable< message_t>::value, "Message has to be trivially copyable");
                  static_assert( transport::header_size + transport::max_payload_size == transport::max_message_size, "something is wrong with padding");


                  inline Transport() = default;

                  inline Transport( common::message::Type type, size_type size)
                  {
                     message.header.type = type;
                     message.header.size = size;
                  }

                  //! @return the message type
                  inline common::message::Type type() const { return static_cast< common::message::Type>( message.header.type);}

                  inline const_range_type payload() const
                  {
                     return range::make( std::begin( message.payload), message.header.count);
                  }

                  inline const auto& correlation() const { return message.header.correlation;}
                  inline auto& correlation() { return message.header.correlation;}

                  //! @return payload size
                  inline size_type payload_size() const { return message.header.count;}

                  //! @return the offset of the logical complete message this transport
                  //!    message represent.
                  inline size_type payload_offset() const { return message.header.offset;}

                  //! @return the size of the complete logical message
                  inline size_type complete_size() const { return message.header.size;}

                  //! @return the total size of the transport message including header.
                  inline size_type size() const { return transport::header_size + payload_size();}

                  inline void* data() { return static_cast< void*>( &message);}
                  inline const void* data() const { return static_cast< const void*>( &message);}

                  inline void* header_data() { return static_cast< void*>( &message.header);}
                  inline void* payload_data() { return static_cast< void*>( &message.payload);}


                  //! Indication if this transport message is the last of the logical message.
                  //!
                  //! @return true if this transport message is the last of the logical message.
                  //! @attention this does not give any guarantees that no more transport messages will arrive...
                  inline bool last() const { return message.header.offset + message.header.count == message.header.size;}

                  auto begin() { return std::begin( message.payload);}
                  inline auto begin() const { return std::begin( message.payload);}
                  auto end() { return begin() + message.header.count;}
                  auto end() const { return begin() + message.header.count;}

                  template< typename R>
                  void assign( R&& range)
                  {
                     message.header.count = range.size();
                     assert( message.header.count <=  transport::max_payload_size);

                     algorithm::copy( range, std::begin( message.payload));
                  }

                  friend std::ostream& operator << ( std::ostream& out, const Transport& value);
               };


               inline size_type offset( const Transport& value) { return value.message.header.offset;}

            } // message

            namespace file
            {
               enum class Flag : int
               {  
                  read_only = O_RDONLY,
                  write_only = O_WRONLY,
                  no_block = O_NONBLOCK,
               };
               using Flags = common::Flags< file::Flag>;

               //! file handle abstraction
               struct Descriptor 
               {
                  Descriptor( const strong::ipc::id& ipc, file::Flags flags);
                  ~Descriptor();

                  Descriptor( Descriptor&&) noexcept;
                  Descriptor& operator = ( Descriptor&&) noexcept;

                  strong::file::descriptor::id blocking() const;
                  strong::file::descriptor::id non_blocking() const;

                  friend std::ostream& operator << ( std::ostream& out, const Descriptor& value);

               private:
                  strong::file::descriptor::id m_descriptor;
               };
            } // file

            namespace named
            {
               //! owns the lifetime of the fifo
               struct Pipe
               {
                  Pipe();
                  ~Pipe();

                  Pipe( Pipe&&) noexcept;
                  Pipe& operator = ( Pipe&&) noexcept;

                  inline const strong::ipc::id& ipc() const { return m_ipc;}
                  std::string path() const;

                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE_NAME( m_ipc, "ipc");
                  })

               private:
                  strong::ipc::id m_ipc;
               };
            } // named

            namespace handle
            {

               //! owns the lifetime of the named::Pipe. Only for reading
               struct Inbound
               {
                  Inbound();

                  Inbound( Inbound&&) noexcept = default;
                  Inbound& operator = ( Inbound&&) noexcept = default;

                  inline const file::Descriptor& descriptor() const { return m_descriptor;}
                  inline const auto& ipc() const { return m_pipe.ipc();}

                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE_NAME( m_pipe, "pipe");
                     CASUAL_SERIALIZE_NAME( m_descriptor, "descriptor");
                  })

               private:
                  named::Pipe m_pipe;
                  file::Descriptor m_descriptor;
                  file::Descriptor m_dummy_writer;
               };

               //! does not own the fifo. Only for writing
               struct Outbound
               {
                  Outbound( strong::ipc::id ipc);

                  Outbound( Outbound&&) noexcept = default;
                  Outbound& operator = ( Outbound&&) noexcept = default;

                  inline auto& descriptor() const { return m_descriptor;}
                  inline const strong::ipc::id& ipc() const { return m_ipc;}

                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE_NAME( m_ipc, "ipc");
                     CASUAL_SERIALIZE_NAME( m_descriptor, "descriptor");
                  })
               private:
                  file::Descriptor m_descriptor;
                  strong::ipc::id m_ipc;
               };

            } // handle

            namespace native
            {
               namespace blocking
               {
                  //! exposed for unittest
                  bool receive( handle::Inbound& handle, message::Transport& transport);
               } // blocking
               namespace non
               {
                  namespace blocking
                  {
                     //! exposed for unittest
                     bool receive( handle::Inbound& handle, message::Transport& transport);
                  } // blocking
               } // non
            } // native

            namespace policy
            {
               using cache_type = communication::inbound::cache_type;
               using cache_range_type = communication::inbound::cache_range_type;

               namespace blocking
               {
                  cache_range_type receive( handle::Inbound& handle, cache_type& cache);
                  Uuid send( const handle::Outbound& handle, const communication::message::Complete& complete);
               } // blocking


               struct Blocking
               {
                  template< typename Connector>
                  auto receive( Connector&& connector, cache_type& cache)
                  {
                     return policy::blocking::receive( connector.handle(), cache);
                  }

                  template< typename Connector>
                  auto send( Connector&& connector, const communication::message::Complete& complete)
                  {
                     return policy::blocking::send( connector.handle(), complete);
                  }
               };

               namespace non
               {
                  namespace blocking
                  {
                     cache_range_type receive( handle::Inbound& handle, cache_type& cache);
                     Uuid send( const handle::Outbound& handle, const communication::message::Complete& complete);
                  } // blocking

                  struct Blocking
                  {
                     template< typename Connector>
                     auto receive( Connector&& connector, cache_type& cache)
                     {
                        return policy::non::blocking::receive( connector.handle(), cache);
                     }

                     template< typename Connector>
                     auto send( Connector&& connector, const communication::message::Complete& complete)
                     {
                        return policy::non::blocking::send( connector.handle(), complete);
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

                  inline const auto& handle() const { return m_handle;}
                  inline auto& handle() { return m_handle;}

                  // for logging only
                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE_NAME( m_handle, "handle");
                  })

               private:
                  handle::Inbound m_handle;
               };

               template< typename S>
               using basic_device = communication::inbound::Device< Connector, S>;

               using Device = communication::inbound::Device< Connector>;


               Device& device();
               inline auto& handle() { return device().connector().handle();}
               inline auto ipc() { return device().connector().handle().ipc();}

            } // inbound

            namespace outbound
            {
               struct Connector
               {
                  using transport_type = message::Transport;
                  using blocking_policy = policy::Blocking;
                  using non_blocking_policy = policy::non::Blocking;

                  Connector( strong::ipc::id destination);

                  inline const auto& handle() const { return m_handle;}

                  inline void reconnect() const { throw; }
                  inline void clear() { m_handle = handle::Outbound{ strong::ipc::id{}};}

                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE_NAME( m_handle, "handle");
                  })

               protected:
                  handle::Outbound m_handle;
               };

               template< typename S>
               using basic_device = communication::outbound::Device< Connector, S>;

               using Device = communication::outbound::Device< Connector>;

            } // outbound



            template< typename D, typename M, typename P>
            auto send( D& device, M&& message, P&& policy)
             -> std::enable_if_t< ! std::is_same< std::decay_t< D>, strong::ipc::id>::value, Uuid>
            {
               return device.send( message, policy);
            }

            template< typename M, typename P>
            Uuid send( strong::ipc::id ipc, M&& message, P&& policy)
            {
               return outbound::Device{ ipc}.send( message, policy);
            }

            template< typename D, typename M, typename P>
            auto put( D&& device, M&& message, P&& policy) 
               -> decltype( device.put( message, policy))
            {
               return device.put( message, policy);
            }

            template< typename M, typename P>
            auto put( strong::ipc::id ipc, M&& message, P&& policy)
            {
               return put( outbound::Device{ ipc}, std::forward< M>( message), std::forward< P>( policy));
            }

            template< typename D, typename... Ts>
            auto next( D& device, Ts&&... ts) 
               -> decltype( device.next( std::forward< Ts>( ts)...))
            {
               return device.next( std::forward< Ts>( ts)...);
            }

            namespace blocking
            {
               template< typename S, typename M>
               auto receive( inbound::basic_device< S>& device, M& message)
               {
                  return device.receive( message, policy::Blocking{});
               }

               template< typename S, typename M>
               auto receive( inbound::basic_device< S>& device, M& message, const Uuid& correlation)
               {
                  return device.receive( message, correlation, policy::Blocking{});
               }

               template< typename D, typename... Ts>
               auto next( D& device, Ts&&... ts) 
                  -> decltype( device.next( std::forward< Ts>( ts)..., policy::Blocking{}))
               {
                  return device.next( std::forward< Ts>( ts)..., policy::Blocking{});
               }

               template< typename D, typename M>
               auto send( D&& device, M&& message)
               {
                  return ipc::send( std::forward< D>( device), message, policy::Blocking{});
               }

               namespace optional
               {
                  //! blocked send of the message, if callee is unreachable (i.e. the process has died)
                  //! `false` is returned
                  //! @returns true if sent, false if Unavailable
                  template< typename D, typename M>
                  bool send( D&& device, M&& message)
                  {
                     try 
                     {
                        blocking::send( std::forward< D>( device), std::forward< M>( message));
                        return true;
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        log::line( log, "failed to send message - target unavailable: ", device);
                        log::line( verbose::log, "message: ", message);
                        return false;
                     }
                  }
               } // optional

               template< typename D, typename M>
               auto put( D&& device, M&& message)
                  -> decltype( ipc::put( std::forward< D>( device), std::forward< M>( message), policy::Blocking{}))
               {
                  return ipc::put( std::forward< D>( device), std::forward< M>( message), policy::Blocking{});
               }

            } // blocking

            namespace non
            {
               namespace blocking
               {
                  template< typename S, typename M>
                  bool receive( inbound::basic_device< S>& device, M& message)
                  {
                     return device.receive( message, policy::non::Blocking{});
                  }

                  template< typename S, typename M>
                  bool receive( inbound::basic_device< S>& device, M& message, const Uuid& correlation)
                  {
                     return device.receive( message, correlation, policy::non::Blocking{});
                  }

                  template< typename S>
                  inline communication::message::Complete next( inbound::basic_device< S>& device)
                  {
                     return device.next( policy::non::Blocking{});
                  }

                  template< typename D, typename M>
                  Uuid send( D&& device, M&& message)
                  {
                     return ipc::send( std::forward< D>( device), message, policy::non::Blocking{});
                  }

                  template< typename D, typename M>
                  auto put( D&& device, M&& message)
                     -> decltype( ipc::put( std::forward< D>( device), std::forward< M>( message), policy::non::Blocking{}))
                  {
                     return ipc::put( std::forward< D>( device), std::forward< M>( message), policy::non::Blocking{});
                  }

                  namespace optional
                  {
                     //! tries to send the message, if callee is unreachable (i.e. the process has died)
                     //! the send is regarded as successfull
                     //! @returns true if successfull
                     template< typename D, typename M>
                     bool send( D&& device, M&& message)
                     {
                        try 
                        {
                           if( ! ipc::send( std::forward< D>( device), message, policy::non::Blocking{}))
                              return false;
                        }
                        catch( const exception::system::communication::Unavailable&)
                        {
                           /* no-op */
                        }
                        return true;
                     }
                  } // optional
               } // blocking
            } // non

            template< typename D, typename M, typename Device = inbound::Device>
            auto call(
                  D&& destination,
                  M&& message,
                  Device& device = inbound::device())
            {
               auto correlation = blocking::send( std::forward< D>( destination), message);

               auto reply = common::message::reverse::type( std::forward< M>( message));
               blocking::receive( device, reply, correlation);
               return reply;
            }


            bool remove( strong::ipc::id id);
            bool remove( const process::Handle& owner);

            bool exists( strong::ipc::id id);

            namespace dispatch
            {
               using Handler =  typename inbound::Device::handler_type;
            } // dispatch


         } // ipc
      } // communication
   } // common
} // casual