//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/communication/message.h"
#include "common/communication/log.h"

#include "common/serialize/native/binary.h"
#include "common/serialize/native/complete.h"
#include "common/signal.h"
#include "common/predicate.h"
#include "common/traits.h"

#include "common/code/casual.h"


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

            template< typename D>
            struct customization_point;

            namespace policy
            {
               template< typename D>
               constexpr auto blocking( D&& device) -> decltype( typename std::decay_t< D>::connector_type::blocking_policy{})
               {
                  return typename std::decay_t< D>::connector_type::blocking_policy{};
               }

               template< typename D>
               constexpr auto blocking( D&& device) -> decltype( typename customization_point< std::decay_t< D>>::blocking_policy{})
               {
                  return typename customization_point< std::decay_t< D>>::blocking_policy{};
               }

               namespace non
               {
                  template< typename D>
                  constexpr auto blocking( D&& device) -> decltype( typename std::decay_t< D>::connector_type::non_blocking_policy{})
                  {
                     return typename std::decay_t< D>::connector_type::non_blocking_policy{};
                  }

                  template< typename D>
                  constexpr auto blocking( D&& device) -> decltype( typename customization_point< std::decay_t< D>>::non_blocking_policy{})
                  {
                     return typename customization_point< std::decay_t< D>>::non_blocking_policy{};
                  }
               } // non
               
            } // policy



            namespace inbound
            {
               using cache_type = std::vector< message::Complete>;
               using cache_range_type = range::type_t< cache_type>;
            }

            template< typename Connector, typename Deserialize = serialize::native::binary::create::Reader>
            struct Inbound
            {
               using connector_type = Connector;
               using deserialize_type = Deserialize;

               template< typename... Args>
               Inbound( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

               ~Inbound()
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

               Inbound( Inbound&&) = default;
               Inbound& operator = ( Inbound&&) = default;

               //! Tries to find the first logic complete message
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               message::Complete next( P&& policy)
               {
                  return select(
                     []( auto& m){ return true;},
                     std::forward< P>( policy));
               }

               //! Tries to find the first logic complete message with a specific type
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               message::Complete next( common::message::Type type, P&& policy)
               {
                  return select(
                     [=]( const message::Complete& m){ return m.type == type;},
                     std::forward< P>( policy));
               }

               //! Tries to find the first logic complete message with any of the types in @p types
               //! `types` has to be an _iterateable_ (container) that holds message::Type's
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename R, typename P>
               auto next( R&& types, P&& policy) 
                  // `types` is a temple to enable other forms of containers than std::vector
                  -> std::enable_if_t< traits::concrete::is_same< decltype( *std::begin( types)), common::message::Type>::value, message::Complete>
               {
                  return select(
                     [&]( const message::Complete& m){ return ! common::algorithm::find( types, m.type).empty();},
                     std::forward< P>( policy));
               }

               //! Tries to find the logic complete message with correlation @p correlation
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               message::Complete next( const Uuid& correlation, P&& policy)
               {
                  return select(
                     [&]( const message::Complete& m){ return m.correlation == correlation;},
                     std::forward< P>( policy));
               }

               //! Tries to find a logic complete message a specific type and correlation
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename P>
               message::Complete next( common::message::Type type, const Uuid& correlation, P&& policy)
               {
                  return select(
                     [&]( const message::Complete& m){ return m.type == type && m.correlation == correlation;},
                     std::forward< P>( policy));
               }

               //! Tries to find a logic complete message that fulfills the predicate
               //!
               //! @return a logical complete message if there is one,
               //!         otherwise the message has absent_message as type
               template< typename Predicate, typename Policy>
               message::Complete select( Predicate&& predicate, Policy&& policy)
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
                  return put( serialize::native::complete( std::forward< M>( message), serialize::native::create::reverse_t< Deserialize>{}));
               }

               //! flushes the messages on the device into cache. (ie, make the device writable if it was full), if implemented.
               void flush()
               {
                  flush( *this, traits::priority::tag< 1>{});
               }

               //! Clear and discard all messages in cache and on the device.
               void clear()
               {
                  flush();
                  std::exchange( m_cache, {});
               }

               Connector& connector() { return m_connector;}
               const Connector& connector() const { return m_connector;}


               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( m_connector, "connector");
                  CASUAL_SERIALIZE_NAME( m_cache, "cache");
                  CASUAL_SERIALIZE_NAME( m_discarded, "discarded");
               })

            private:

               template< typename D>
               static void flush( D& device, traits::priority::tag< 0>)
               {
                  // no-op - can't flush if we haven't got non-blocking
               }

               template< typename D>
               static auto flush( D& device, traits::priority::tag< 1>) -> decltype( policy::non::blocking(device), void())
               {
                  // We don't want to handle any signals while we're flushing
                  signal::thread::scope::Block block;

                  auto count = platform::batch::flush;

                  while( device.next( common::message::Type::flush_ipc, policy::non::blocking( device)) && --count > 0)
                     ; // no op
               }

               template< typename C, typename M>
               bool deserialize( C&& complete, M& message)
               {
                  if( complete)
                  {
                     serialize::native::complete( complete, message, Deserialize{});
                     return true;
                  }
                  return false;
               }


               template< typename Policy, typename Predicate>
               inbound::cache_range_type find( Policy&& policy, Predicate&& predicate)
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

               inbound::cache_type m_cache;
               std::vector< Uuid> m_discarded;
               Connector m_connector;
            };

            template< typename Connector>
            struct base_connector
            {
               using connector_type = Connector;

               template< typename... Args>
               base_connector( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

               Connector& connector() { return m_connector;}
               const Connector& connector() const { return m_connector;}

               CASUAL_LOG_SERIALIZE(
               {
                  CASUAL_SERIALIZE_NAME( m_connector, "connector");
               })

            private:
               Connector m_connector;
            };

            //! Doesn't do much. More for symmetry with inbound
            template< typename Connector, typename Serialize = serialize::native::binary::create::Writer, typename Base = device::base_connector< Connector>>
            struct Outbound : Base
            {
               using serialize_type = Serialize;

               using Base::Base;

               template< typename Policy>
               Uuid put( const message::Complete& complete, Policy&& policy)
               {
                  return apply( policy, complete);
               }

               //! Tries to send a message to the connector @p message
               //!
               //! @return true if we found one, and message is unserialized. false otherwise.
               //! @note depending on the policy it may not ever return false (ie with a blocking policy)
               template< typename M, typename P>
               Uuid send( M&& message, P&& policy)
               {
                  if( ! message.execution)
                     message.execution = execution::id();

                  auto writer = Serialize{}();
                  writer << message;

                  message::Complete complete{
                     common::message::type( message), 
                     message.correlation ? message.correlation : uuid::make(),
                     writer.consume()};

                  return apply(
                     std::forward< P>( policy),
                     complete);
               }

            private:

               template< typename C>
               static void reconnect( C& connector, traits::priority::tag< 0>)
               {
                  // rethrow the exception::system::communication::Unavailable 
                  throw;
               }
               template< typename C>
               static auto reconnect( C& connector, traits::priority::tag< 1>)
                  -> decltype( void( connector.reconnect()), void())
               {
                  connector.reconnect();
               }

               template< typename Policy>
               Uuid apply( Policy&& policy, const message::Complete& complete)
               {
                  while( true)
                  {
                     try
                     {
                        // Delegate the invocation to the policy
                        return policy.send( Base::connector(), complete);
                     }
                     catch( ...)
                     {
                        auto code = exception::code();
                        if( code == code::casual::communication_unavailable)
                        {
                           // Let connector take a crack at resolving this problem, if implemented...
                           reconnect( Base::connector(), traits::priority::tag< 1>{});
                        }
                        else if( code == code::casual::interupted)
                           signal::dispatch();
                        else
                           throw;
                     }
                  }
               }
            };

            //! duplex device - inbound and outbound
            template< typename Connector, typename Serialize = serialize::native::binary::create::Writer>
            struct Duplex : device::Outbound< Connector, Serialize, device::Inbound< Connector, common::serialize::native::create::reverse_t< Serialize>>>
            {
               using base_type = device::Outbound< Connector, Serialize, device::Inbound< Connector, common::serialize::native::create::reverse_t< Serialize>>>;
               using base_type::base_type;
            };


            template< typename D, typename... Ts>
            auto receive( D& device, Ts&&... ts)
               -> decltype( device.receive( std::forward< Ts>( ts)...))
            {
               return device.receive( std::forward< Ts>( ts)...);
            }

            template< typename D, typename... Ts>
            auto next( D& device, Ts&&... ts) 
               -> decltype( device.next( std::forward< Ts>( ts)...))
            {
               return device.next( std::forward< Ts>( ts)...);
            }

            template< typename D, typename... Ts>
            auto select( D& device, Ts&&... ts) 
               -> decltype( device.select( std::forward< Ts>( ts)...))
            {
               return device.select( std::forward< Ts>( ts)...);
            }

            template< typename D, typename... Ts>
            auto send( D&& device, Ts&&... ts) 
               -> decltype( device.send( std::forward< Ts>( ts)...))
            {
               return device.send( std::forward< Ts>( ts)...);
            }

            template< typename D, typename... Ts>
            auto put( D&& device, Ts&&... ts) 
               -> decltype( device.put( std::forward< Ts>( ts)...))
            {
               return device.put( std::forward< Ts>( ts)...);
            }

            //! To enable specific devices to customize calls
            //! @{ 
            template< typename D, typename... Ts>
            auto send( D&& device, Ts&&... ts) 
               -> decltype( customization_point< std::decay_t< D>>::send( std::forward< D>( device), std::forward< Ts>( ts)...))
            {
               return customization_point< std::decay_t< D>>::send( std::forward< D>( device), std::forward< Ts>( ts)...);
            }

            template< typename D, typename... Ts>
            auto put( D&& device, Ts&&... ts) 
               -> decltype( customization_point< std::decay_t< D>>::put( std::forward< D>( device), std::forward< Ts>( ts)...))
            {
               return customization_point< std::decay_t< D>>::put( std::forward< D>( device), std::forward< Ts>( ts)...);
            }
            //! @}

            
            namespace blocking
            {
               template< typename D, typename... Ts>
               auto receive( D& device, Ts&&... ts)
                  -> decltype( device::receive( device, std::forward< Ts>( ts)... , policy::blocking( device)))
               {
                  return device::receive( device, std::forward< Ts>( ts)... , policy::blocking( device));
               }

               template< typename D, typename... Ts>
               auto next( D& device, Ts&&... ts) 
                  -> decltype( device::next( device, std::forward< Ts>( ts)..., policy::blocking( device)))
               {
                  return device::next( device, std::forward< Ts>( ts)..., policy::blocking( device));
               }

               template< typename D, typename... Ts>
               auto select( D& device, Ts&&... ts) 
                  -> decltype( device.select( std::forward< Ts>( ts)..., policy::blocking( device)))
               {
                  return device.select( std::forward< Ts>( ts)..., policy::blocking( device));
               }

               template< typename D, typename... Ts>
               auto send( D&& device, Ts&&... ts) 
                  -> decltype( device::send( std::forward< D>( device), std::forward< Ts>( ts)..., policy::blocking( device)))
               {
                  return device::send( std::forward< D>( device), std::forward< Ts>( ts)..., policy::blocking( device));
               }

               template< typename D, typename... Ts>
               auto put( D&& device, Ts&&... ts) 
                  -> decltype( device::put( std::forward< D>( device), std::forward< Ts>( ts)..., policy::blocking( device)))
               {
                  return device::put( std::forward< D>( device), std::forward< Ts>( ts)..., policy::blocking( device));
               }

               namespace optional
               {
                  //! blocked send of the message, if callee is unreachable (for example. the process has died)
                  //! `false` is returned
                  //! @returns true if sent, false if Unavailable
                  template< typename... Ts>
                  auto send( Ts&&... ts) -> decltype( void( blocking::send( std::forward< Ts>( ts)...)), bool())
                  {
                     try 
                     {
                        blocking::send( std::forward< Ts>( ts)...);
                        return true;
                     }
                     catch( ...)
                     {
                        if( exception::code() == code::casual::communication_unavailable)
                        {
                           log::line( communication::log, code::casual::communication_unavailable, " failed to send message - action: ignore");
                           return false;
                        }
                        // propagate other errors
                        throw;
                     }
                  }

                  //! blocked put of the message, if callee is unreachable (for example. the process has died)
                  //! `false` is returned
                  //! @returns true if sent, false if Unavailable
                  template< typename... Ts>
                  auto put( Ts&&... ts) -> decltype( void( blocking::put( std::forward< Ts>( ts)...)), bool())
                  {
                     try 
                     {
                        blocking::put( std::forward< Ts>( ts)...);
                        return true;
                     }
                     catch( ...)
                     {
                        if( exception::code() == code::casual::communication_unavailable)
                        {
                           log::line( communication::log, code::casual::communication_unavailable, " failed to put message - action: ignore");
                           return false;
                        }
                        // propagate other errors
                        throw;
                     }
                  }
               } // optional
            }

            namespace non
            {
               namespace blocking
               {
                  template< typename D, typename... Ts>
                  auto receive( D& device, Ts&&... ts)
                     -> decltype( device::receive( device, std::forward< Ts>( ts)... , policy::non::blocking( device)))
                  {
                     return device::receive( device, std::forward< Ts>( ts)... , policy::non::blocking( device));
                  }

                  template< typename D, typename... Ts>
                  auto next( D& device, Ts&&... ts) 
                     -> decltype( device::next( device, std::forward< Ts>( ts)..., policy::non::blocking( device)))
                  {
                     return device::next( device, std::forward< Ts>( ts)..., policy::non::blocking( device));
                  }

                  template< typename D, typename... Ts>
                  auto send( D&& device, Ts&&... ts) 
                     -> decltype( device::send( std::forward< D>( device), std::forward< Ts>( ts)..., policy::non::blocking( device)))
                  {
                     return device::send( std::forward< D>( device), std::forward< Ts>( ts)..., policy::non::blocking( device));
                  }

                  template< typename D, typename... Ts>
                  auto put( D&& device, Ts&&... ts) 
                     -> decltype( device::put( std::forward< D>( device), std::forward< Ts>( ts)..., policy::non::blocking( device)))
                  {
                     return device::put( std::forward< D>( device), std::forward< Ts>( ts)..., policy::non::blocking( device));
                  }

                  namespace optional
                  {
                     //! non blocked send of the message, if not possible to send or if callee is unreachable 
                     //! (for example. the process has died) `false` is returned
                     //! @returns true if sent, false otherwise or if Unavailable
                     template< typename... Ts>
                     auto send( Ts&&... ts) -> decltype( void( blocking::send( std::forward< Ts>( ts)...)), bool())
                     {
                        try 
                        {
                           return ! non::blocking::send( std::forward< Ts>( ts)...).empty();
                        }
                        catch( ...)
                        {
                           if( exception::code() == code::casual::communication_unavailable)
                           {
                              log::line( communication::log, code::casual::communication_unavailable, " failed to send message - action: ignore");
                              return false;
                           }
                           // propagate other errors
                           throw;
                        }
                     }
               } // optional

               } // blocking
            } // non         
         } // device
      } // communication
   } // common
} // casual



