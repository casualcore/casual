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
      namespace message
      {
         using Conclude = common::message::basic_message< common::message::Type::task_conclude>;
      } // message

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
      struct Unit
      {
         template< typename... Hs>
         Unit( Hs&&... handlers) 
            : m_handlers( initialize_handlers( std::forward< Hs>( handlers)...))
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
               return found->dispatch( unit::erased::Type{ message});

            return unit::Dispatch::pending;
         }

         inline bool empty() const noexcept { return m_handlers.empty();}
         explicit operator bool() const noexcept { return ! empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_handlers);
         );

      private:

         struct Concept
         {
            template< typename M>
            Concept( M model) : type{ M::message_type::type()}, dispatch{ std::move( model)}
            {}

            common::message::Type type;
            common::unique_function< unit::Dispatch( unit::erased::Type)> dispatch;

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

            static_assert( function_traits::arguments() == 1 &&
               std::same_as< common::message::Type, decltype( message_type::type())> &&
               common::message::like< message_type>,
               "handler need ot have the signature: unit::Dispatch( const <casual message>& message)");

            Model( Handler&& handler) : m_handler{ std::forward< Handler>( handler)}
            {}

            inline unit::Dispatch operator()( unit::erased::Type erased) 
            { 
               return m_handler( erased.manifest< message_type>());
            }
         private:
            Handler m_handler;
         };

         template< typename... Ts>
         static std::vector< Concept> initialize_handlers( Ts&&... ts)
         {
            std::vector< Concept> result;
            result.reserve( sizeof...(Ts));

            ( result.emplace_back( Model{ std::forward< Ts>( ts)}), ...);
            return result;
         }

         std::vector< Concept> m_handlers;
      };

      struct Coordinator
      {
         Coordinator() = default; 

         Coordinator( Coordinator&&) = default;
         Coordinator& operator = ( Coordinator&&) = default;

         void add( concurrent::Unit task)
         {
            m_units.push_back( std::move( task));
         };


         template< typename M>
         void operator() ( M& message)
         {
            auto unit_done = [ &message]( auto& unit) { return unit( message) == unit::Dispatch::done;};
            common::algorithm::container::erase_if( m_units, unit_done);
         }

         inline auto& tasks() const noexcept { return m_units;}
         inline auto empty() const noexcept { return m_units.empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( m_units);
         )

      private:
         std::vector< concurrent::Unit> m_units;
      };
      
      
   } // task::concurrent
} // casual
