//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_DEVICE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_DEVICE_H_


#include "common/communication/message.h"

#include "common/message/dispatch.h"
#include "common/marshal/binary.h"
#include "common/marshal/complete.h"

#include "common/internal/trace.h"



namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace error
         {



            using type = std::function<void()>;

            namespace handler
            {
               struct Default
               {
                  void operator () () const { throw;}
               };

               namespace callback
               {
                  namespace on
                  {
                     //!
                     //! A common policy that does a callback when a
                     //! process terminates
                     //!
                     struct Terminate
                     {
                        using callback_type = std::function<void(const process::lifetime::Exit&)>;

                        inline Terminate( callback_type callback) : m_callback{ std::move( callback)} {};

                        inline void operator () () const
                        {
                           try
                           {
                              throw;
                           }
                           catch( const exception::signal::child::Terminate& exception)
                           {
                              auto terminated = process::lifetime::ended();
                              for( auto& death : terminated)
                              {
                                 m_callback( death);
                              }
                           }
                        }

                     protected:
                        std::function<void(const process::lifetime::Exit&)> m_callback;
                     };

                  } // on
               } // callback

            } // handler

         } // error

         namespace inbound
         {

            template< typename Connector, typename Unmarshal = marshal::binary::create::Input>
            struct Device
            {

               using connector_type = Connector;
               using complete_type = message::Complete;
               using message_type = typename complete_type::message_type_type;
               using transport_type = typename connector_type::transport_type;

               using blocking_policy = typename connector_type::blocking_policy;
               using non_blocking_policy = typename connector_type::non_blocking_policy;

               using unmarshal_type = Unmarshal;
               using error_type = std::function<void()>;

               using handler_type = common::message::dispatch::basic_handler< Unmarshal>;

               template< typename... Args>
               Device( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

               Device( Device&&) = default;
               Device& operator = ( Device&&) = default;



               //!
               //! Creates a corresponding message-dispatch-handler to this
               //! inbound device
               //!
               template< typename... Args>
               static handler_type handler( Args&&... args)
               {
                  return { std::forward< Args>( args)...};
               }



               //!
               //! Tries to find the first logic complete message
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               //!
               template< typename P>
               complete_type next( P&& policy, const error_type& handler = nullptr)
               {
                  return find_complete(
                        std::forward< P>( policy),
                        handler);
               }

               //!
               //! Tries to find the first logic complete message with a specific type
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               //!
               template< typename P>
               complete_type next( message_type type, P&& policy, const error_type& handler = nullptr)
               {
                  return find_complete(
                        std::forward< P>( policy),
                        handler,
                        [=]( const complete_type& m){ return m.type == type;});
               }

               //!
               //! Tries to find the first logic complete message with any of the types in @p types
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               //!
               template< typename P>
               complete_type next( const std::vector< message_type>& types, P&& policy, const error_type& handler = nullptr)
               {
                  return find_complete(
                        std::forward< P>( policy),
                        handler,
                        [&]( const complete_type& m){ return ! common::range::find( types, m.type).empty();});
               }

               //!
               //! Tries to find the logic complete message with correlation @p correlation
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               //!
               template< typename P>
               complete_type next( const Uuid& correlation, P&& policy, const error_type& handler = nullptr)
               {
                  return find_complete(
                        std::forward< P>( policy),
                        handler,
                        [&]( const complete_type& m){ return m.correlation == correlation;});
               }


               //!
               //! Tries to find a message whith the same type as @p message
               //!
               //! @return true if we found one, and message is unmarshaled. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               //!
               template< typename M, typename P>
               bool receive( M& message, P&& policy, const error_type& handler = nullptr)
               {
                  return unmarshal(
                        this->next(
                              common::message::type( message),
                              std::forward< P>( policy),
                              handler),
                        message);
               }

               //!
               //! Tries to find a message that has @p correlation
               //!
               //! @return true if we found one, and message is unmarshaled. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               //!
               template< typename M, typename P>
               bool receive( M& message, const Uuid& correlation, P&& policy, const error_type& handler = nullptr)
               {
                  return unmarshal(
                        this->next(
                              correlation,
                              std::forward< P>( policy),
                              handler),
                        message);
               }




               //!
               //! Discards any message that correlates.
               //!
               void discard( const Uuid& correlation)
               {
                  auto complete = range::find_if( m_cache, [&]( const complete_type& m){ return m.correlation == correlation;});

                  if( complete)
                  {
                     if( ! complete->complete())
                     {
                        m_discarded.push_back( correlation);
                     }
                     m_cache.erase( complete.begin());
                  }
                  else
                  {
                     auto found = range::find( m_discarded, correlation);

                     if( ! found)
                     {
                        m_discarded.push_back( correlation);
                     }
                  }
               }

               //!
               //! push a complete message to the cache
               //!
               //! @param message
               //!
               //! @return
               //!
               inline Uuid put( message::Complete&& message)
               {
                  //
                  // Make sure we consume messages from the real queue first.
                  //
                  flush();

                  m_cache.push_back( std::move( message));
                  return m_cache.back().correlation;
               }

               template< typename M>
               Uuid push( M&& message)
               {
                  return put( marshal::complete( std::forward< M>( message), marshal::create::reverse_t< unmarshal_type>{}));
               }


               //!
               //! flushes the messages on the device into cache. (ie, make the device writable if it was full)
               //!
               void flush()
               {
                  //
                  // We don't want to handle any signals while we're flushing
                  //
                  signal::thread::scope::Block block;

                  while( next( message_type::flush_ipc, non_blocking_policy{}))
                  {
                     ;
                  }
               }

               //!
               //! Clear and discard all messages in cache and on the device.
               //!
               void clear()
               {
                  flush();
                  cache_type empty;
                  std::swap( empty, m_cache);
               }

               connector_type& connector() { return m_connector;}
               const connector_type& connector() const { return m_connector;}


               inline friend std::ostream& operator << ( std::ostream& out, const Device& device)
               {
                  return out << "{ connector: " << device.m_connector << ", cache: "
                        << range::make( device.m_cache) << ", discarded: " << range::make( device.m_discarded) << "}";
               }

            private:

               using cache_type = std::vector< complete_type>;
               using range_type =  range::type_t< cache_type>;

               template< typename C, typename M>
               bool unmarshal( C&& complete, M& message)
               {
                  if( complete)
                  {
                     marshal::complete( complete, message, unmarshal_type{});
                     return true;
                  }
                  return false;
               }


               template< typename Policy, typename Predicate>
               range_type find( Policy&& policy, const error_type& handler, Predicate&& predicate)
               {

                  while( true)
                  {
                     try
                     {
                        transport_type transport;

                        auto found = range::find_if( m_cache, predicate);

                        while( ! found && policy.receive( m_connector, transport))
                        {
                           //
                           // Check if the message should be discarded
                           //
                           if( ! discard( transport))
                           {
                              cache( transport);
                              found = range::find_if( m_cache, predicate);
                           }
                        }
                        return found;
                     }
                     catch( ...)
                     {
                        //
                        // Delegate the errors to the handler, if provided
                        //
                        if( ! handler)
                        {
                           throw;
                        }
                        handler();
                     }
                  }
               }

               template< typename Policy, typename... Predicates>
               complete_type find_complete( Policy&& policy, const error_type& handler, Predicates&&... predicates)
               {

                  auto found = find(
                        std::forward< Policy>( policy),
                        handler,
                        chain::And::link(
                              []( const message::Complete& m){ return m.complete();},
                              std::forward< Predicates>( predicates)...));

                  if( found)
                  {
                     auto result = std::move( *found);
                     m_cache.erase( std::begin( found));
                     return result;
                  }
                  return {};
               }


               void cache( transport_type& transport)
               {

                  auto found = range::find_if( m_cache,
                        [&]( const complete_type& m)
                        {
                           return transport.type() == m.type && transport.message.header.correlation == m.correlation;
                        });

                  if( found)
                  {
                     found->add( transport);
                  }
                  else
                  {
                     m_cache.emplace_back( transport);
                  }
               }

               bool discard( transport_type& transport)
               {
                  auto found = range::find( m_discarded, transport.message.header.correlation);

                  if( found)
                  {
                     //
                     // If transport is the last part in the message, we don't need to
                     // discard any more transports...
                     // we can't really be sure since messages could come out of order
                     // (although, on ipc-queue order is guaranteed)
                     //
                     // TODO: figure out if there is a way to determine this for all possible devices.
                     //
                     if( transport.last())
                     {
                        m_discarded.erase( std::begin( found));
                     }
                     return true;
                  }
                  return false;
               }

               cache_type m_cache;
               std::vector< Uuid> m_discarded;
               connector_type m_connector;
            };

         } // inbound

         namespace outbound
         {

            //!
            //! Doesn't do much. More for symmetry with inbound
            //!
            template< typename Connector, typename Marshal = marshal::binary::create::Output>
            struct Device
            {
               using connector_type = Connector;
               using complete_type = message::Complete;
               using message_type = typename complete_type::message_type_type;
               using transport_type = typename connector_type::transport_type;
               using blocking_policy = typename connector_type::blocking_policy;
               using non_blocking_policy = typename connector_type::non_blocking_policy;

               using marshal_type = Marshal;

               using error_type = std::function<void()>;

               template< typename... Args>
               Device( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}


               connector_type& connector() { return m_connector;}
               const connector_type& connector() const { return m_connector;}


               template< typename Policy>
               Uuid put( const message::Complete& message, Policy&& policy, const error_type& handler = nullptr)
               {
                  using transport_type = typename Device::transport_type;
                  transport_type transport{ message.type, message.payload.size()};

                  message.correlation.copy( transport.correlation());


                  auto part_begin = std::begin( message.payload);

                  do
                  {
                     auto part_end = std::distance( part_begin, std::end( message.payload)) > transport.max_payload_size() ?
                           part_begin + transport.max_payload_size() : std::end( message.payload);

                     transport.assign( part_begin, part_end);
                     transport.message.header.offset = std::distance( std::begin( message.payload), part_begin);

                     //
                     // send the physical message
                     //
                     if( ! apply( policy, transport, handler))
                     {
                        return uuid::empty();
                     }

                     part_begin = part_end;
                  }
                  while( part_begin != std::end( message.payload));

                  return message.correlation;
               }

               //!
               //! Tries to send a message to the connector @p message
               //!
               //! @return true if we found one, and message is unmarshaled. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               //!
               template< typename M, typename P>
               Uuid send( M&& message, P&& policy, const error_type& handler = nullptr)
               {
                  if( ! message.execution)
                  {
                     message.execution = execution::id();
                  }

                  message::Complete complete( message.type(), message.correlation ? message.correlation : uuid::make());

                  auto marshal = marshal_type()( complete.payload);
                  marshal << message;

                  return put(
                        complete,
                        std::forward< P>( policy),
                        handler);
               }

               template< typename M>
               Uuid blocking_send( M&& message, const error_type& handler = nullptr)
               {
                  return send( message, blocking_policy{}, handler);
               }

               template< typename M>
               Uuid non_blocking_send( M&& message, const error_type& handler = nullptr)
               {
                  return send( message, non_blocking_policy{}, handler);
               }

               inline friend std::ostream& operator << ( std::ostream& out, const Device& device)
               {
                  return out << "{ connector: " << device.m_connector << "}";
               }

            private:

               template< typename Policy>
               bool apply( Policy&& policy, const transport_type& transport, const error_type& handler)
               {
                  while( true)
                  {
                     try
                     {
                        //
                        // Delegate the invocation to the policy
                        //
                        return policy.send( m_connector, transport);
                     }
                     catch( const exception::communication::Unavailable&)
                     {
                        //
                        // Let connector take a crack at resolving this problem...
                        //
                        m_connector.reconnect();
                     }
                     catch( ...)
                     {
                        //
                        // Delegate the errors to the handler, if provided
                        //
                        if( ! handler)
                        {
                           throw;
                        }
                        handler();
                     }
                  }
               }
               connector_type m_connector;
            };

         } // outbound



      } // communication
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_DEVICE_H_
