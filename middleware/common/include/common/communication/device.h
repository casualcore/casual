//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/communication/log.h"

#include "common/serialize/native/binary.h"
#include "common/serialize/native/complete.h"
#include "common/signal.h"
#include "common/predicate.h"
#include "common/traits.h"
#include "common/algorithm/container.h"

#include "common/code/casual.h"


namespace casual
{
   namespace common::communication::device
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


      template< typename Connector>
      struct Inbound
      {
         using connector_type = Connector;

         using cache_type = typename Connector::cache_type;
         using cache_range_type = range::type_t< cache_type>;
         using complete_type = traits::iterable::value_t< cache_type>;
         using correlation_type = strong::correlation::id;

         template< typename... Args>
         Inbound( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

         ~Inbound()
         {
            if( ! m_cache.empty() && log::category::warning)
            {
               log::Stream warning{ "warning"};
               log::line( warning, "pending messages in cache - ", *this);
            }
            else if( verbose::log)
            {
               log::Stream debug{ "casual.communication.verbose"};
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
         [[nodiscard]] complete_type next( P&& policy)
         {
            return select(
               []( auto& m){ return true;},
               std::forward< P>( policy));
         }

         //! @return the first complete message from the cache
         [[nodiscard]] complete_type cached()
         {
            auto is_complete = []( auto& message){ return message.complete();};

            if( auto found = algorithm::find_if( m_cache, is_complete))
               return algorithm::container::extract( m_cache, std::begin( found));

            return {};
         }

         //! @return true if message with `type` exists in the cache, and is a complete message.
         bool cached( common::message::Type type)
         {
            auto has_type = [type]( auto& message) { return message.complete() && message.type() == type;};

            return predicate::boolean( algorithm::find_if( m_cache, has_type));
         }

         //! Tries to find the first logic complete message with a specific type
         //!
         //! @return a logical complete message if there is one,
         //!         otherwise the message has absent_message as type
         template< typename P>
         [[nodiscard]] complete_type next( common::message::Type type, P&& policy)
         {
            return select(
               [type]( auto& complete){ return complete.type() == type;},
               std::forward< P>( policy));
         }

         //! Tries to find the first logic complete message with any of the types in @p types
         //! `types` has to be an _iterateable_ (container) that holds message::Type's
         //!
         //! @return a logical complete message if there is one,
         //!         otherwise the message has absent_message as type
         template< typename R, typename P>
         [[nodiscard]] auto next( R&& types, P&& policy) 
            // `types` is a temple to enable other forms of containers than std::vector
            -> std::enable_if_t< traits::is::same_v< traits::remove_cvref_t< decltype( *std::begin( types))>, common::message::Type>, complete_type>
         {
            return select(
               [&types]( auto& complete){ return ! common::algorithm::find( types, complete.type()).empty();},
               std::forward< P>( policy));
         }

         //! Tries to find the logic complete message with correlation @p correlation
         //!
         //! @return a logical complete message if there is one,
         //!         otherwise the message has absent_message as type
         template< typename P>
         [[nodiscard]] complete_type next( const correlation_type& correlation, P&& policy)
         {
            return select(
               [&correlation]( auto& complete){ return complete.correlation() == correlation;},
               std::forward< P>( policy));
         }

         //! Tries to find a logic complete message a specific type and correlation
         //!
         //! @return a logical complete message if there is one,
         //!         otherwise the message has absent_message as type
         template< typename P>
         [[nodiscard]] complete_type next( common::message::Type type, const correlation_type& correlation, P&& policy)
         {
            return select(
               [type, &correlation]( auto& complete){ return complete.type() == type && complete.correlation() == correlation;},
               std::forward< P>( policy));
         }

         //! Tries to find a logic complete message that fulfills the predicate
         //!
         //! @return a logical complete message if there is one,
         //!         otherwise the message has absent_message as type
         template< typename Predicate, typename Policy>
         [[nodiscard]] complete_type select( Predicate&& predicate, Policy&& policy)
         {
            auto found = find(
                  std::forward< Policy>( policy),
                  predicate::make_and(
                        []( const auto& message){ return message.complete();},
                        std::forward< Predicate>( predicate)));

            if( found)
               return algorithm::container::extract( m_cache, std::begin( found));

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
         bool receive( M& message, const correlation_type& correlation, P&& policy)
         {
            return deserialize(
                  this->next(
                        common::message::type( message),
                        correlation,
                        std::forward< P>( policy)),
                  message);
         }

         //! Discards any message that correlates.
         void discard( const correlation_type& correlation)
         {
            flush();

            auto complete = algorithm::find_if( m_cache, [&]( auto& complete){ return complete.correlation() == correlation;});

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

         //! push a message (or complete) to the cache
         template< typename M>
         correlation_type push( M&& message)
         {
            // Make sure we consume messages from the real queue first.
            flush();

            if constexpr( std::is_same_v< std::decay_t< M>, complete_type>)
            {
               static_assert( std::is_rvalue_reference_v< M>, "complete_type needs to be a rvalue");
               m_cache.push_back( std::move( message));
            }
            else
               m_cache.push_back( serialize::native::complete< complete_type>( std::forward< M>( message)));

             return m_cache.back().correlation();
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

         //! @returns the number of messages complete or incomplete in the cache
         inline platform::size::type size() const noexcept { return m_cache.size();}

         //! @returns the number of complete messages in the cache
         inline platform::size::type complete() const noexcept 
         { 
            return algorithm::accumulate( m_cache, platform::size::type{}, []( auto result, auto& message)
            {
               if( message.complete())
                  return ++result;
               return result;
            });
         }

         //! @returns the number of incomplete messages in the cache
         inline platform::size::type incomplete() const noexcept
         { 
            return algorithm::accumulate( m_cache, platform::size::type{}, []( auto result, auto& message)
            {
               if( ! message.complete())
                  return ++result;
               return result;
            });
         }

         Connector& connector() { return m_connector;}
         const Connector& connector() const { return m_connector;}


         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_connector, "connector");
            CASUAL_SERIALIZE_NAME( m_cache, "cache");
            CASUAL_SERIALIZE_NAME( m_discarded, "discarded");
         )

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
               serialize::native::complete( complete, message);
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

         bool discard( const complete_type& complete)
         {
            auto found = algorithm::find( m_discarded, complete.correlation());

            if( found && complete.complete())
            {
               m_discarded.erase( std::begin( found));
               return true;
            }
            return false;
         }

         cache_type m_cache;
         std::vector< correlation_type> m_discarded;
         Connector m_connector;
      };

      template< typename Inbound>
      auto operator == ( const Inbound& lhs, strong::file::descriptor::id rhs) noexcept -> decltype( lhs.connector().descriptor() == rhs)
      { 
         return lhs.connector().descriptor() == rhs;
      }
      template< typename Inbound>
      auto operator == ( strong::file::descriptor::id lhs, const Inbound& rhs) noexcept -> decltype( lhs == rhs.connector().descriptor())
      { 
         return lhs == rhs.connector().descriptor();
      }

      template< typename Connector>
      struct base_connector
      {
         using connector_type = Connector;

         template< typename... Args>
         base_connector( Args&&... args) : m_connector{ std::forward< Args>( args)...} {}

         Connector& connector() { return m_connector;}
         const Connector& connector() const { return m_connector;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_connector, "connector");
         )

      private:
         Connector m_connector;
      };

      //! Doesn't do much. More for symmetry with inbound
      template< typename Connector, typename Base = device::base_connector< Connector>>
      struct Outbound : Base
      {
         using complete_type = typename Connector::complete_type;

         using Base::Base;

         //! Tries to send a message to the connector @p message
         //!
         template< typename M, typename P>
         auto send( M&& message, P&& policy)
         {
            if constexpr ( std::is_same_v< std::decay_t< M>, complete_type>)
               return apply( std::forward< P>( policy), std::forward< M>( message));
            else
            {
               if( ! message.execution)
                  message.execution = execution::id();

               return apply(
                  std::forward< P>( policy),
                  serialize::native::complete< complete_type>( message));
            }
         }

      private:

         template< typename C>
         static void reconnect( C& connector, traits::priority::tag< 0>)
         {
            throw;
         }
         template< typename C>
         static auto reconnect( C& connector, traits::priority::tag< 1>)
            -> decltype( void( connector.reconnect()), void())
         {
            connector.reconnect();
         }

         template< typename Policy, typename C>
         auto apply( Policy&& policy, C&& complete)
         {
            while( true)
            {
               try
               {
                  // Delegate the invocation to the policy
                  return policy.send( Base::connector(), std::forward< C>( complete));
               }
               catch( ...)
               {
                  auto error = exception::capture();
                  if( error.code() == code::casual::communication_unavailable || error.code() == code::casual::invalid_argument)
                  {
                     // Let connector take a crack at resolving this problem, if implemented...
                     reconnect( Base::connector(), traits::priority::tag< 1>{});
                  }
                  else if( error.code() == code::casual::interupted)
                     signal::dispatch();
                  else
                     throw;
               }
            }
         }
      };

      //! duplex device - inbound and outbound
      template< typename Connector>
      struct Duplex : device::Outbound< Connector, device::Inbound< Connector>>
      {
         using base_type = device::Outbound< Connector, device::Inbound< Connector>>;
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

      //! To enable specific devices to customize calls
      //! @{ 
      template< typename D, typename... Ts>
      auto send( D&& device, Ts&&... ts) 
         -> decltype( customization_point< std::decay_t< D>>::send( std::forward< D>( device), std::forward< Ts>( ts)...))
      {
         return customization_point< std::decay_t< D>>::send( std::forward< D>( device), std::forward< Ts>( ts)...);
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


         namespace optional
         {
            //! blocked send of the message, if callee is unreachable (for example. the process has died)
            //! `false` is returned
            //! @returns true if sent, false if Unavailable
            template< typename... Ts>
            auto send( Ts&&... ts) -> decltype( blocking::send( std::forward< Ts>( ts)...))
            {
               try 
               {
                  return blocking::send( std::forward< Ts>( ts)...);
               }
               catch( ...)
               {
                  if( exception::capture().code() == code::casual::communication_unavailable)
                  {
                     log::line( communication::log, code::casual::communication_unavailable, " failed to send message - action: ignore");
                     return {};
                  }
                  // propagate other errors
                  throw;
               }
            }
         } // optional
      } // blocking

      namespace non::blocking
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

         namespace optional
         {
            //! non blocked send of the message, if not possible to send or if callee is unreachable 
            //! (for example. the process has died) `false` is returned
            //! @returns true if sent, false otherwise or if Unavailable
            template< typename... Ts>
            auto send( Ts&&... ts) -> decltype( blocking::send( std::forward< Ts>( ts)...))
            {
               try 
               {
                  return non::blocking::send( std::forward< Ts>( ts)...);
               }
               catch( ...)
               {
                  if( exception::capture().code() == code::casual::communication_unavailable)
                  {
                     log::line( communication::log, code::casual::communication_unavailable, " failed to send message - action: ignore");
                     return {};
                  }
                  // propagate other errors
                  throw;
               }
            }
         } // optional
      } // non::blocking

      namespace async
      {
         template< typename R>
         struct future
         {
            future( R&& reply) : m_reply{ std::move( reply)} {}

            template< typename D>
            auto get( D&& device)
            {
               blocking::receive( device, m_reply, m_reply.correlation);
               return std::exchange( m_reply, {});
            }
            
         private:
            R m_reply;
         };

         template< typename D, typename M>
         auto call( D&& destination, M&& message)
         {
            auto reply = common::message::reverse::type( message);
            reply.correlation = blocking::send( std::forward< D>( destination), std::forward< M>( message));

            return future< std::decay_t< decltype( reply)>>{ std::move( reply)};
         }
      } // async

      template< typename D, typename M, typename Device>
      auto call(
            D&& destination,
            M&& message,
            Device& device)
      {
         auto reply = common::message::reverse::type( message);
         auto correlation = blocking::send( std::forward< D>( destination), std::forward< M>( message));

         blocking::receive( device, reply, correlation);
         return reply;
      }

   } // common::communication::device
} // casual



