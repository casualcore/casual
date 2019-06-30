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
                     //! A common policy that does a callback when a
                     //! process terminates
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
            using cache_type = std::vector< message::Complete>;
            using cache_range_type =  range::type_t< cache_type>;

            template< typename Connector, typename Deserialize = serialize::native::binary::create::Input>
            struct Device
            {

               using connector_type = Connector;
               using complete_type = message::Complete;
               using message_type = typename complete_type::message_type_type;

               using blocking_policy = typename connector_type::blocking_policy;
               using non_blocking_policy = typename connector_type::non_blocking_policy;

               using deserialize_type = Deserialize;
               using error_type = std::function<void()>;

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
               complete_type next( P&& policy, const error_type& handler = nullptr)
               {
                  return select(
                        std::forward< P>( policy),
                        []( auto& m){ return true;},
                        handler);
               }

               //! Tries to find the first logic complete message with a specific type
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( message_type type, P&& policy, const error_type& handler = nullptr)
               {
                  return select(
                        std::forward< P>( policy),
                        [=]( const complete_type& m){ return m.type == type;},
                        handler);
               }

               //! Tries to find the first logic complete message with any of the types in @p types
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( const std::vector< message_type>& types, P&& policy, const error_type& handler = nullptr)
               {
                  return select(
                        std::forward< P>( policy),
                        [&]( const complete_type& m){ return ! common::algorithm::find( types, m.type).empty();},
                        handler);
               }

               //! Tries to find the logic complete message with correlation @p correlation
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( const Uuid& correlation, P&& policy, const error_type& handler = nullptr)
               {
                  return select(
                        std::forward< P>( policy),
                        [&]( const complete_type& m){ return m.correlation == correlation;},
                        handler);
               }

               //! Tries to find a logic complete message a specific type and correlation
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               complete_type next( message_type type, const Uuid& correlation, P&& policy, const error_type& handler = nullptr)
               {
                  return select(
                        std::forward< P>( policy),
                        [&]( const complete_type& m){ return m.type == type && m.correlation == correlation;},
                        handler);
               }

               //! Tries to find a logic complete message that fulfills the predicate
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename Policy, typename Predicate>
               complete_type select( Policy&& policy, Predicate&& predicate, const error_type& handler = nullptr)
               {
                  auto found = find(
                        std::forward< Policy>( policy),
                        handler,
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
               bool receive( M& message, P&& policy, const error_type& handler = nullptr)
               {
                  return deserialize(
                        this->next(
                              common::message::type( message),
                              std::forward< P>( policy),
                              handler),
                        message);
               }

               //! Tries to find a message that has the same type and @p correlation
               //!
               //! @return true if we found one, and message is deserialized. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               template< typename M, typename P>
               bool receive( M& message, const Uuid& correlation, P&& policy, const error_type& handler = nullptr)
               {
                  return deserialize(
                        this->next(
                              common::message::type( message),
                              correlation,
                              std::forward< P>( policy),
                              handler),
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
                     {
                        m_discarded.push_back( correlation);
                     }
                     m_cache.erase( complete.begin());
                  }
                  else
                  {
                     auto found = algorithm::find( m_discarded, correlation);

                     if( ! found)
                     {
                        m_discarded.push_back( correlation);
                     }
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
                  {
                     ;
                  }
               }

               //! Clear and discard all messages in cache and on the device.
               void clear()
               {
                  flush();
                  std::exchange( m_cache, {});
               }

               connector_type& connector() { return m_connector;}
               const connector_type& connector() const { return m_connector;}


               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
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
               cache_range_type find( Policy&& policy, const error_type& handler, Predicate&& predicate)
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
                           {
                              m_cache.erase( std::begin( found));
                           }

                           // Try to find a massage that matches the predicate
                           found = algorithm::find_if( m_cache, predicate);
                        }
                        return found;
                     }
                     catch( ...)
                     {
                        // Delegate the errors to the handler, if provided
                        if( ! handler)
                        {
                           throw;
                        }
                        handler();
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
            template< typename Connector, typename Serialize = serialize::native::binary::create::Output>
            struct Device
            {
               using connector_type = Connector;

               using blocking_policy = typename connector_type::blocking_policy;
               using non_blocking_policy = typename connector_type::non_blocking_policy;

               using serialize_type = Serialize;

               using error_type = std::function<void()>;

               template< typename... Args>
               Device( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

               constexpr blocking_policy policy_blocking() const { return blocking_policy{};}
               constexpr non_blocking_policy policy_non_blocking() const { return non_blocking_policy{};}

               connector_type& connector() { return m_connector;}
               const connector_type& connector() const { return m_connector;}


               template< typename Policy>
               Uuid put( const message::Complete& complete, Policy&& policy, const error_type& handler = nullptr)
               {
                  return apply( policy, complete, handler);
               }

               //! Tries to send a message to the connector @p message
               //!
               //! @return true if we found one, and message is deserialized. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               template< typename M, typename P>
               Uuid send( M&& message, P&& policy, const error_type& handler = nullptr)
               {
                  if( ! message.execution)
                  {
                     message.execution = execution::id();
                  }

                  message::Complete complete( 
                     common::message::type( message), 
                     message.correlation ? message.correlation : uuid::make());

                  auto serialize = serialize_type()( complete.payload);
                  serialize << message;

                  return apply(
                        std::forward< P>( policy),
                        complete,
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

               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               {
                  CASUAL_SERIALIZE_NAME( m_connector, "connector");
               })

            private:

               template< typename Policy>
               Uuid apply( Policy&& policy, const message::Complete& complete, const error_type& handler)
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
                     catch( const exception::signal::Pipe&)
                     {
                        m_connector.reconnect();
                     }
                     catch( ...)
                     {
                        // Delegate the errors to the handler, if provided
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


