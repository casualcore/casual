//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"
#include "common/algorithm.h"
#include "common/algorithm/container.h"
#include "common/functional.h"
#include "common/message/type.h"
#include "common/strong/id.h"
#include "common/log.h"

#include "casual/assert.h"

namespace casual
{
   namespace task
   {
      namespace unit
      {
         namespace detail
         {
            struct policy
            {
               inline static int generate() 
               {
                  static int value{};
                  return value++;
               }
            };
         } // detail

         using id = common::strong::Type< int, detail::policy>;
         
         namespace erased
         {
            //! Used for _transport_ type erased messages
            struct Type
            {
               template< typename T>
               Type( const T& value) : m_value{ static_cast< const void*>( &value)}
               {}

               template< typename T>
               const T& manifest() const noexcept { return *static_cast< const T*>( m_value);}
            private:
               const void* m_value;
            };
         } // erased

         namespace action
         {
            enum struct Outcome : std::uint8_t
            {
               success,
               abort,
            };

            constexpr std::string_view description( Outcome value) noexcept
            {
               switch( value)
               {
                  case Outcome::success: return "success";
                  case Outcome::abort: return "abort";
               }
               return "<unknown>";
            }
         } // action

         // A callback that's invoked when a Unit starts
         struct Action
         {
            template< typename C>
            Action( C callable) : m_action{ std::move( callable)}
            {}

            Action( Action&&) = default;
            Action& operator = ( Action&&) = default;

            //! invokes (and consumes) the action, one and only one
            inline action::Outcome operator () ( task::unit::id id) 
            {
               if( m_action) 
                  return std::exchange( m_action, {})( id);
               return action::Outcome::success;
            }

            inline explicit operator bool() const noexcept { return common::predicate::boolean( m_action);}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( common::predicate::boolean( m_action), "active");
               CASUAL_SERIALIZE_NAME( this, "address");
            );

         private:
            common::unique_function< action::Outcome( task::unit::id id)> m_action;
         };

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

      //! Holds an action, and a set of _message handlers_.
      //! a message gets dispatched to a _handler_ (for the message type), and
      //! it's up to the _handler_ to decide what to do.
      struct Unit
      {
         template< typename... Hs>
         Unit( unit::Action action, Hs&&... handlers) 
            : m_handlers( initialize_handlers( std::forward< Hs>( handlers)...)),
               m_action{ std::move( action)}
         {}

         Unit( Unit&&) = default;
         Unit& operator = ( Unit&&) = default;

         template< typename M>
         unit::Dispatch operator() ( const M& message)
         {
            if( auto found = common::algorithm::find( m_handlers, message.type()))
               return found->dispatch( m_id, unit::erased::Type{ message});

            return unit::Dispatch::pending;
         }

         inline auto operator() () { return m_action( m_id);}
         inline auto id() const noexcept { return m_id;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_id, "id");
            CASUAL_SERIALIZE_NAME( m_handlers, "handlers");
            CASUAL_SERIALIZE_NAME( m_action, "action");
         );

      private:
         
         struct Concept
         {
            template< typename M>
            Concept( M model) : m_type{ M::message_type::type()}, m_dispatch{ std::move( model)}
            {}

            inline unit::Dispatch dispatch( task::unit::id id, unit::erased::Type value) { return m_dispatch( id, value);}

            inline friend bool operator == ( const Concept& lhs, common::message::Type rhs) { return lhs.m_type == rhs;}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_type, "type");
               CASUAL_SERIALIZE_NAME( this, "address");
            );

         private:
            common::message::Type m_type;
            common::unique_function< unit::Dispatch( task::unit::id id, unit::erased::Type)> m_dispatch;
         };

         template< typename Handler>
         struct Model
         {
            using function_traits = common::traits::function< Handler>;
            //! helper to deduce _handler argument
            using id_type = std::decay_t< typename function_traits::template argument< 0>::type>;
            using message_type = std::decay_t< typename function_traits::template argument< 1>::type>;

            static_assert( 
               std::is_same_v< task::unit::id, id_type> &&
               std::is_same_v< common::message::Type, decltype( message_type::type())>,
               "handler need ot have the signature: bool( task::unit::id, const <casual message>& message");

            Model( Handler&& handler) : m_handler{ std::forward< Handler>( handler)}
            {}

            inline unit::Dispatch operator()( task::unit::id id, unit::erased::Type erased) 
            { 
               return m_handler( id, erased.manifest< message_type>());
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
         unit::Action m_action;
         task::unit::id m_id = task::unit::id::generate();
      };

      //! Hols a set of `task::Unit`. One group starts all _units_
      //! asynchronous/parallel, and is done when all units are done (or have
      //! failed).
      struct Group
      {
         Group() = default;
         Group( std::vector< task::Unit> units) : m_tasks{ std::move( units)} {}
         Group( task::Unit unit) : Group{ common::algorithm::container::emplace::initialize< std::vector< task::Unit>>( std::move( unit))} {}

         Group( Group&&) = default;
         Group& operator = ( Group&&) = default;

         template< typename M>
         unit::Dispatch operator() ( const M& message)
         {
            auto unit_done = [ &message]( auto& unit) { return unit( message) == unit::Dispatch::done;};
            return common::range::empty( common::algorithm::container::erase_if( m_tasks, unit_done)) ? unit::Dispatch::done : unit::Dispatch::pending;
         }

         unit::action::Outcome operator() ()
         {
            auto successful_action = []( auto& unit){ return unit() == decltype( unit())::success;}; 
            common::algorithm::container::erase_if( m_tasks, common::predicate::negate( successful_action));
            return common::range::empty( m_tasks) ? unit::action::Outcome::abort : unit::action::Outcome::success;
         }

         //! @returns id's of the current tasks. 
         inline auto ids() const noexcept { return common::algorithm::transform( m_tasks, []( auto& task){ return task.id();});}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_tasks, "tasks");
         )

      private:
         std::vector< task::Unit> m_tasks;
      };

      //! Holds a set of `task::Group`. Each group is started
      //! synchronous. When a _group_ is done, the next in line starts (if any).
      //! The API provides a _.then( ...) semantics_ to further make it clear
      //! for the user that the "added _group/unit_" is invoked later (unless
      //! there are no current or pending, then the _unit/group_ starts
      //! directly)
      struct Coordinator
      {
         Coordinator() = default;

         Coordinator( Coordinator&&) = default;
         Coordinator& operator = ( Coordinator&&) = default;

         template< typename M>
         void operator() ( const M& message)
         {
            if( common::range::empty( m_continuations))
               return;

            if( common::range::front( m_continuations)( message) == unit::Dispatch::pending)
               return;

            pop_front();

            // invoke 'action' on the next, if any.
            if( ! common::range::empty( m_continuations))
               common::range::front( m_continuations)();

            // next continuation might be interested in current message, 
            operator()( message);
         }

         void operator() ()
         {
            if( empty())
               return;

            if( common::range::front( m_continuations)() == unit::action::Outcome::abort)
               pop_front();
         }

         //! adds the group as a continuation
         //! @returns "this", hence it's possible to chain then-expressions.
         inline Coordinator& then( Group group)
         {
            m_continuations.push_back( std::move( group));

            // check if we need to kickstart the task coordinator.
            if( m_continuations.size() == 1)
               Coordinator::operator()();

            return *this;
         }

         //! adds the `unit` as as a continuation, wrapped in a task::Group
         //! @returns "this", hence it's possible to chain then-expressions.
         inline Coordinator& then( Unit unit)
         { 
            return then( Group{ std::move( unit)});
         }

         inline bool empty() const noexcept { return m_continuations.empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_continuations, "continuations");
         )

      private:
         void pop_front() noexcept
         {
            m_continuations.erase( std::begin( m_continuations));
         }

         std::vector< Group> m_continuations;
      };

      namespace create
      {
         template< typename A>
         auto action( A&& action) { return unit::Action{ std::forward< A>( action)};}

         template< typename... Ts>
         Unit unit( unit::Action action, Ts&&... ts)
         {
            return Unit{ std::move( action), std::forward< Ts>( ts)...};
         }

         template< typename... Ts>
         Group group( Ts&&... ts)
         {
            return Group{ common::algorithm::container::emplace::initialize< std::vector< task::Unit>>( std::forward< Ts>( ts)...)};
         }

      } // create
      
   } // task
} // casual