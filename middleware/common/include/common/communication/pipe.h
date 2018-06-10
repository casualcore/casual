//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/device.h"

#include "common/platform.h"
#include "common/strong/id.h"
#include "common/uuid.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/flag.h"
#include "common/file.h"


namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace pipe
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

                  constexpr std::int64_t max_message_size() { return platform::communication::pipe::transport::size;}
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

                  } message;


                  static_assert( transport::max_message_size() - transport::max_payload_size() < transport::max_payload_size(), "Payload is to small");
                  static_assert( std::is_trivially_copyable< message_t>::value, "Message has to be trivially copyable");
                  static_assert( ( transport::header_size() + transport::max_payload_size()) == transport::max_message_size(), "something is wrong with padding");


                  inline Transport() { memory::set( message);}

                  inline Transport( common::message::Type type, size_type complete_size) : Transport()
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
               explicit Handle( strong::pipe::id id);
               Handle( Handle&& other);
               Handle& operator = ( Handle&& other);
               ~Handle();

               inline strong::pipe::id id() const { return m_id;} 

            private:
               strong::pipe::id m_id;
            };

            namespace native
            {
               Handle create( const std::string& name);
               Handle open( const std::string& name);
               
               using cache_type = communication::inbound::cache_type;
               using cache_range_type = communication::inbound::cache_range_type;
               
               namespace blocking
               {
                  bool send( strong::pipe::id id, const message::Transport& transport);
                  bool receive( strong::pipe::id id, message::Transport& transport);
               } // blocking

               namespace non
               {
                  namespace blocking
                  {
                     bool send( strong::pipe::id id, const message::Transport& transport);
                     bool receive( strong::pipe::id id, message::Transport& transport);
                  } // blocking
               } // non
    
            } // native
            
            struct Address
            {
               static Address create();
               inline const common::Uuid& uuid() const { return m_uuid;}

               inline explicit operator bool () const { return ! m_uuid.empty();} 
               

               inline friend bool operator == ( const Address& lhs, const Address& rhs) { return lhs.m_uuid == rhs.m_uuid;}
               inline friend std::ostream& operator << ( std::ostream& out, const Address& rhs) { return out << rhs.uuid();}

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & m_uuid;
               })
            private:
               common::Uuid m_uuid;
            };

            std::string file( const Address& address);


            namespace policy
            {
               using cache_type = communication::inbound::cache_type;
               using cache_range_type = communication::inbound::cache_range_type;

               namespace blocking
               {
                  cache_range_type receive( strong::pipe::id id, cache_type& cache);
                  Uuid send( strong::pipe::id id, const communication::message::Complete& complete);
               } // blocking

               struct Blocking
               {

                  template< typename Connector>
                  cache_range_type receive( Connector&& connector, cache_type& cache)
                  {
                     return policy::blocking::receive( connector.id(), cache);
                  }

                  template< typename Connector>
                  Uuid send( Connector&& connector, const communication::message::Complete& complete)
                  {
                     return policy::blocking::send( std::forward< Connector>( connector), complete);
                  }


               };

               namespace non
               {
                  namespace blocking
                  {
                     cache_range_type receive( strong::pipe::id id, cache_type& cache);
                     Uuid send( strong::pipe::id id, const communication::message::Complete& complete);
                  } // blocking

                  struct Blocking
                  {
                     template< typename Connector>
                     cache_range_type receive( Connector&& connector, cache_type& cache)
                     {
                        return policy::non::blocking::receive( connector.id(), cache);
                     }

                     template< typename Connector>
                     Uuid send( Connector&& connector, const communication::message::Complete& complete)
                     {
                        return policy::non::blocking::send( std::forward< Connector>( connector), complete);
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

                  Connector( const Connector&) = delete;
                  Connector& operator = ( const Connector&) = delete;

                  Connector( Connector&& rhs) noexcept;
                  Connector& operator = ( Connector&& rhs) noexcept;

                  inline const Address& address() const { return m_address;}
                  inline strong::pipe::id id() const { return m_id;}

                  friend void swap( Connector& lhs, Connector& rhs);
                  friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);

               private:
                  pipe::Address m_address;
                  strong::pipe::id m_id;
               };

               template< typename S>
               using basic_device = communication::inbound::Device< Connector, S>;

               using Device = communication::inbound::Device< Connector>;


               Device& device();
               strong::pipe::id id();

            } // inbound

            namespace outbound
            {
               struct Connector
               {
                  using transport_type = message::Transport;
                  using blocking_policy = policy::Blocking;
                  using non_blocking_policy = policy::non::Blocking;

                  Connector( const pipe::Address& address);
                  ~Connector();

                  Connector( Connector&& rhs) noexcept = default;
                  Connector& operator = ( Connector&& rhs) noexcept = default;

                  inline strong::pipe::id id() const { return m_id;}
                  inline operator strong::pipe::id() const { return id();}


                  inline void reconnect() const { throw; }

                  friend std::ostream& operator << ( std::ostream& out, const Connector& rhs);

               protected:
                  strong::pipe::id m_id;
               };

               template< typename S>
               using basic_device = communication::outbound::Device< Connector, S>;

               using Device = communication::outbound::Device< Connector>;

           
               namespace domain
               {
                  struct Connector : outbound::Connector
                  {
                     Connector();
                     void reconnect();
                  };
                  using Device = communication::outbound::Device< Connector>;

                  namespace optional
                  {

                     struct Connector : outbound::Connector
                     {
                        Connector();
                        void reconnect();
                     };

                     //!
                     //! Will not wait and try to connect to domain
                     //!
                     using Device = communication::outbound::Device< Connector>;

                  } // optional
               } // domain

            } // outbound


            using error_type = typename outbound::Device::error_type;

            template< typename D, typename M, typename P>
            auto send( D& device, M&& message, P&& policy, const error_type& handler = nullptr)
             -> std::enable_if_t< ! std::is_same< std::decay_t< D>, pipe::Address>::value, Uuid>
            {
               return device.send( message, policy, handler);
            }

            template< typename M, typename P>
            Uuid send( const Address& address, M&& message, P&& policy, const error_type& handler = nullptr)
            {
               return outbound::Device{ address}.send( message, policy, handler);
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
                  return pipe::send( std::forward< D>( device), message, policy::Blocking{}, handler);
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
                     return pipe::send( std::forward< D>( device), message, policy::non::Blocking{}, handler);
                  }
               } // blocking
            } // non

            template< typename D, typename M, typename Policy = policy::Blocking, typename Device = inbound::Device>
            auto call(
                  D&& destination,
                  M&& message,
                  Policy&& policy = policy::Blocking{},
                  const error::type& handler = nullptr,
                  Device& device = pipe::inbound::device())
            {
               auto correlation = pipe::send( std::forward< D>( destination), message, policy, handler);

               auto reply = common::message::reverse::type( std::forward< M>( message));
               device.receive( reply, correlation, policy, handler);

               return reply;
            }

         } // pipe
      } // communication
   } // common
} // casual