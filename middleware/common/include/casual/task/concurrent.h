//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
#include "common/strong/id.h"
#include "common/message/type.h"
#include "common/functional.h"

#include <string_view>

namespace casual
{
   namespace task::concurrent
   {
      namespace message::task
      {
         //! A message to indicate failed `key`
         using Failed = common::message::basic_message< common::message::Type::task_failed>;
      } // message::task

      namespace unit
      {

         namespace erased
         {
            //! Used for _transport_ type erased messages
            struct Type
            {
               template< typename T>
               Type( T& value) : m_value{ static_cast< void*>( &value)}
               {}

               template< typename T>
               T& manifest() const noexcept { return *static_cast< T*>( m_value);}
            private:
               void* m_value;
            };
         } // erased
         

         enum struct Dispatch : std::uint8_t
         {
            done,
            pending,
         };

         constexpr std::string_view description( Dispatch value) noexcept
         {
            switch( value)
            {
               case Dispatch::done: return "done";
               case Dispatch::pending: return "pending";
            }
            return "<unknown>";
         }
      } // unit

      //! Holds a set of _message handlers_.
      //! a message gets dispatched to a _handler_ (for the message type), and
      //! it's up to the _handler_ to decide what to do.
      template< typename Key>
      struct Unit
      {
         template< typename... Hs>
         Unit( const Key& key, const common::strong::correlation::id& correlation, Hs&&... handlers) 
            : m_key{ key}, m_correlation{ correlation}, m_handlers( initialize_handlers( std::forward< Hs>( handlers)...))
         {}

         Unit() = default;

         Unit( Unit&&) = default;
         Unit& operator = ( Unit&&) = default;

         template< typename M>
         unit::Dispatch operator() ( M& message)
         {
            if( m_handlers.empty())
               return unit::Dispatch::done;

            if( auto found = common::algorithm::find( m_handlers, message.type()))
               return found->dispatch( unit::erased::Type{ message}, m_key);

            return unit::Dispatch::pending;
         }

         inline bool empty() const noexcept { return m_handlers.empty();}
         explicit operator bool() const noexcept { return ! empty();}

         inline friend bool operator == ( const Unit& lhs, const common::strong::correlation::id& rhs) noexcept { return lhs.m_correlation == rhs;}
         inline friend bool operator == ( const Unit& lhs, const Key& rhs) noexcept { return lhs.m_key == rhs;}

         const auto& correlation() const noexcept { return m_correlation;}
         const auto& key() const noexcept { return m_key;}
         auto types() const noexcept { return common::algorithm::transform( m_handlers, []( auto& handler){ return handler.type;});}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_key);
            CASUAL_SERIALIZE( m_correlation);
            CASUAL_SERIALIZE( m_handlers);
         );

      private:

         struct Concept
         {
            template< typename M>
            Concept( M model) : type{ M::message_type::type()}, dispatch{ std::move( model)}
            {}

            common::message::Type type;
            common::unique_function< unit::Dispatch( unit::erased::Type, const Key& key)> dispatch;

            inline friend bool operator == ( const Concept& lhs, common::message::Type rhs) { return lhs.type == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( type);
               CASUAL_SERIALIZE_NAME( this, "address");
            );
         };

         template< typename Handler>
         struct Model 
         {
            using function_traits = common::traits::function< Handler>;
         
            //! helper to deduce _handler argument
            using message_type = std::decay_t< typename function_traits::template argument< 0>::type>;

            static_assert( function_traits::arguments() <= 2 &&
               std::same_as< common::message::Type, decltype( message_type::type())> &&
               common::message::like< message_type>,
               "handler need ot have the signature: unit::Dispatch( const <casual message>& message)");

            Model( Handler&& handler) : m_handler{ std::forward< Handler>( handler)}
            {}

            inline unit::Dispatch operator()( unit::erased::Type erased, const Key& key) 
            {
               // Just to make it easier to make task-units that always are done.
               if constexpr( std::same_as< decltype( m_handler( erased.manifest< message_type>(), key)), void>)
               {
                  m_handler( erased.manifest< message_type>(), key);
                  return unit::Dispatch::done;
               }
               else
                  return m_handler( erased.manifest< message_type>(), key);
            }
         private:
            Handler m_handler;
         };

         template< typename... Ts>
         static std::vector< Concept> initialize_handlers( Ts&&... ts)
         {
            std::vector< Concept> result;
            result.reserve( sizeof...(Ts));

            ( result.emplace_back( Model< std::remove_cvref_t< Ts>>{ std::forward< Ts>( ts)}), ...);
            return result;
         }

         Key m_key;
         common::strong::correlation::id m_correlation;
         std::vector< Concept> m_handlers;
      };

      template< typename Key>
      struct Coordinator
      {
         using unit_type = concurrent::Unit< Key>;

         Coordinator() = default; 

         Coordinator( Coordinator&&) = default;
         Coordinator& operator = ( Coordinator&&) = default;

         void add( unit_type task)
         {
            m_units.push_back( std::move( task));
         };


         template< typename M>
         void operator() ( M& message)
         {
            if( auto found = common::algorithm::find( m_units, message.correlation))
               if( std::invoke( *found, message) == unit::Dispatch::done)
                  common::algorithm::container::erase( m_units, std::begin( found));
         }

         bool contains( const common::strong::correlation::id& correlation) const noexcept
         {
            return common::algorithm::contains( m_units, correlation);
         }

         bool contains( const Key& key) const noexcept
         {
            return common::algorithm::contains( m_units, key);
         }

         //! Find all task-units that correspond to the `key` and invoke them with message::task::Failed, 
         //! then remove the unit. It's optional for the unit to have a handler for message::task::Failed.
         void failed( const Key& key)
         {
            common::algorithm::container::erase_if( m_units, [ &key]( auto& unit)
            {
               if( unit != key)
                  return false;

               message::task::Failed message;
               message.correlation = unit.correlation();
               std::invoke( unit, message);

               return true;
            });
         }

         void remove( const common::strong::correlation::id& correlation)
         {
            common::algorithm::container::erase( m_units, correlation);
         }

         inline auto& tasks() const noexcept { return m_units;}
         inline auto empty() const noexcept { return m_units.empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_units);
         )

      private:
         std::vector< unit_type> m_units;
      };
      
      
   } // task::concurrent
} // casual
