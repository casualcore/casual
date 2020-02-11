//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/communication/message.h"

#include "common/message/dispatch.h"
#include "common/serialize/native/binary.h"
#include "common/serialize/native/complete.h"
#include "common/signal.h"
#include "common/exception/signal.h"
#include "common/exception/system.h"
#include "common/predicate.h"

#include "common/log.h"



namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace device
         {
            namespace handle
            {
               void error();
            } // handle
         } // device

         namespace inbound
         {
            using cache_type = std::vector< message::Complete>;
            using cache_range_type =  range::type_t< cache_type>;

            template< typename Connector, typename Deserialize = serialize::native::binary::create::Reader>
            struct Device
            {

               using connector_type = Connector;
               using complete_type = message::Complete;
               using message_type = typename complete_type::message_type_type;

               using blocking_policy = typename connector_type::blocking_policy;
               using non_blocking_policy = typename connector_type::non_blocking_policy;

               using deserialize_type = Deserialize;

               using handler_type = common::message::dispatch::basic_handler< deserialize_type>;

               template< typename... Args>
               Device( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

               ~Device()
               {
                  if( ! m_cache.empty() && log::category::warning)
                  {
                     log::Stream warning{ "warning"};
                     log::line( warning, "pending messages in cache - ", *this);
                  }
                  else if( log::debug)
                  {
                     log::Stream debug{ "casual.debug"};
                     log::line( debug, "device: ", *this);
                  }
               }


               Device( Device&&) = default;
               Device& operator = ( Device&&) = default;

               constexpr blocking_policy policy_blocking() const { return blocking_policy{};}
               constexpr non_blocking_policy policy_non_blocking() const { return non_blocking_policy{};}

               //! Creates a corresponding message-dispatch-handler to this
               //! inbound device
               template< typename... Args>
               static handler_type handler( Args&&... args)
               {
                  return { std::forward< Args>( args)...};
               }

               //! Tries to find the first logic complete message
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( P&& policy)
               {
                  return select(
                        std::forward< P>( policy),
                        []( auto& m){ return true;});
               }

               //! Tries to find the first logic complete message with a specific type
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( message_type type, P&& policy)
               {
                  return select(
                        std::forward< P>( policy),
                        [=]( const complete_type& m){ return m.type == type;});
               }

               //! Tries to find the first logic complete message with any of the types in @p types
               //! `types` has to be an _iterateable_ (container) that holds message::Type's
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename R, typename P>
               auto next( R&& types, P&& policy) 
                  // `types` is a temple to enable other forms of containers than std::vector
                  -> std::enable_if_t< traits::concrete::is_same< decltype( *std::begin( types)), message_type>::value, complete_type>
               {
                  return select(
                        std::forward< P>( policy),
                        [&]( const complete_type& m){ return ! common::algorithm::find( types, m.type).empty();});
               }

               //! Tries to find the logic complete message with correlation @p correlation
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( const Uuid& correlation, P&& policy)
               {
                  return select(
                        std::forward< P>( policy),
                        [&]( const complete_type& m){ return m.correlation == correlation;});
               }

               //! Tries to find a logic complete message a specific type and correlation
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( message_type type, const Uuid& correlation, P&& policy)
               {
                  return select(
                        std::forward< P>( policy),
                        [&]( const complete_type& m){ return m.type == type && m.correlation == correlation;});
               }

               //! Tries to find a logic complete message that fulfills the predicate
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename Policy, typename Predicate>
               complete_type select( Policy&& policy, Predicate&& predicate)
               {
                  auto found = find(
                        std::forward< Policy>( policy),
                        predicate::make_and(
                              []( const auto& m){ return m.complete();},
                              std::forward< Predicate>( predicate)));

                  if( found)
                  {
                     auto result = std::move( *found);
                     m_cache.erase( std::begin( found));
                     return result;
                  }
                  return {};

               }

               //! Tries to find a message with the same type as @p message
               //!
               //! @return true if we found one, and message is deserialized. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               template< typename M, typename P>
               bool receive( M& message, P&& policy)
               {
                  return deserialize(
                        this->next(
                              common::message::type( message),
                              std::forward< P>( policy)),
                        message);
               }

               //! Tries to find a message that has the same type and @p correlation
               //!
               //! @return true if we found one, and message is deserialized. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               template< typename M, typename P>
               bool receive( M& message, const Uuid& correlation, P&& policy)
               {
                  return deserialize(
                        this->next(
                              common::message::type( message),
                              correlation,
                              std::forward< P>( policy)),
                        message);
               }

               //! Discards any message that correlates.
               void discard( const Uuid& correlation)
               {
                  flush();

                  auto complete = algorithm::find_if( m_cache, [&]( const auto& m){ return m.correlation == correlation;});

                  if( complete)
                  {
                     if( ! complete->complete())
                        m_discarded.push_back( correlation);

                     m_cache.erase( complete.begin());
                  }
                  else
                  {
                     if( ! algorithm::find( m_discarded, correlation))
                        m_discarded.push_back( correlation);
                  }
               }

               //! push a complete message to the cache
               inline Uuid put( message::Complete&& message)
               {
                  // Make sure we consume messages from the real queue first.
                  flush();

                  m_cache.push_back( std::move( message));
                  return m_cache.back().correlation;
               }

               template< typename M>
               Uuid push( M&& message)
               {
                  return put( serialize::native::complete( std::forward< M>( message), serialize::native::create::reverse_t< deserialize_type>{}));
               }

               //! flushes the messages on the device into cache. (ie, make the device writable if it was full)
               void flush()
               {
                  // We don't want to handle any signals while we're flushing
                  signal::thread::scope::Block block;

                  auto count = platform::batch::flush;

                  while( next( message_type::flush_ipc, non_blocking_policy{}) && --count > 0)
                     ; // no op
               }

               //! Clear and discard all messages in cache and on the device.
               void clear()
               {
                  flush();
                  std::exchange( m_cache, {});
               }

               connector_type& connector() { return m_connector;}
               const connector_type& connector() const { return m_connector;}


               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( m_connector, "connector");
                  CASUAL_SERIALIZE_NAME( m_cache, "cache");
                  CASUAL_SERIALIZE_NAME( m_discarded, "discarded");
               })

            private:

               template< typename C, typename M>
               bool deserialize( C&& complete, M& message)
               {
                  if( complete)
                  {
                     serialize::native::complete( complete, message, deserialize_type{});
                     return true;
                  }
                  return false;
               }


               template< typename Policy, typename Predicate>
               cache_range_type find( Policy&& policy, Predicate&& predicate)
               {
                  while( true)
                  {
                     try
                     {
                        auto found = algorithm::find_if( m_cache, predicate);

                        while( ! found && ( found = policy.receive( m_connector, m_cache)))
                        {
                           // Check if we should discard the message
                           if( discard( *found))
                              m_cache.erase( std::begin( found));

                           // Try to find a massage that matches the predicate
                           found = algorithm::find_if( m_cache, predicate);
                        }
                        return found;
                     }
                     catch( ...)
                     {
                        communication::device::handle::error();
                     }
                  }
               }


               bool discard( const communication::message::Complete& complete)
               {
                  auto found = algorithm::find( m_discarded, complete.correlation);

                  if( found && complete.complete())
                  {
                     m_discarded.erase( std::begin( found));
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
            //! Doesn't do much. More for symmetry with inbound
            template< typename Connector, typename Serialize = serialize::native::binary::create::Writer>
            struct Device
            {
               using connector_type = Connector;

               using blocking_policy = typename connector_type::blocking_policy;
               using non_blocking_policy = typename connector_type::non_blocking_policy;

               using serialize_type = Serialize;

               template< typename... Args>
               Device( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

               constexpr blocking_policy policy_blocking() const { return blocking_policy{};}
               constexpr non_blocking_policy policy_non_blocking() const { return non_blocking_policy{};}

               connector_type& connector() { return m_connector;}
               const connector_type& connector() const { return m_connector;}


               template< typename Policy>
               Uuid put( const message::Complete& complete, Policy&& policy)
               {
                  return apply( policy, complete);
               }

               //! Tries to send a message to the connector @p message
               //!
               //! @return true if we found one, and message is deserialized. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               template< typename M, typename P>
               Uuid send( M&& message, P&& policy)
               {
                  if( ! message.execution)
                     message.execution = execution::id();

                  message::Complete complete( 
                     common::message::type( message), 
                     message.correlation ? message.correlation : uuid::make());

                  auto serialize = serialize_type()( complete.payload);
                  serialize << message;

                  return apply(
                        std::forward< P>( policy),
                        complete);
               }

               template< typename M>
               Uuid blocking_send( M&& message)
               {
                  return send( message, blocking_policy{});
               }

               template< typename M>
               Uuid non_blocking_send( M&& message)
               {
                  return send( message, non_blocking_policy{});
               }

               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( m_connector, "connector");
               })

            private:

               template< typename Policy>
               Uuid apply( Policy&& policy, const message::Complete& complete)
               {
                  while( true)
                  {
                     try
                     {
                        // Delegate the invocation to the policy
                        return policy.send( m_connector, complete);
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        // Let connector take a crack at resolving this problem...
                        m_connector.reconnect();
                     }
                     catch( ...)
                     {
                        device::handle::error(); 
                     }
                  }
               }
               connector_type m_connector;
            };

         } // outbound
      } // communication
   } // common
} // casual


