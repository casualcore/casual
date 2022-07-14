//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "domain/common.h"

#include "common/pimpl.h"
#include "common/strong/id.h"
#include "common/message/event.h"
#include "common/functional.h"
#include "common/algorithm/container.h"

#include <queue>
#include <memory>

namespace casual
{
   namespace domain::manager
   {

      struct State;

      namespace task
      {
         namespace id
         {
            using type = common::strong::correlation::id;
         } // id

         struct Context
         {
            task::id::type id;
            std::string description;

            CASUAL_LOG_SERIALIZE({
               CASUAL_SERIALIZE( id);
               CASUAL_SERIALIZE( description);
            })
         };

         namespace message
         {
            namespace domain
            {
               using base_information= common::message::basic_request< common::message::Type::event_domain_information>;
               struct Information : base_information
               {
                  using base_information::base_information;

                  common::domain::Identity domain;

                  CASUAL_CONST_CORRECT_SERIALIZE({
                     base_information::serialize( archive);
                     CASUAL_SERIALIZE( domain);
                  })
               };
            } // domain
         } // message

         namespace event
         {
            struct Callback
            {
               template< typename C>
               Callback( C callback)
                  : Callback{ model< C>( std::move( callback))} {}

               template< typename M>
               bool operator () ( const M& message)
               {
                  assert( m_type == common::message::type( message));
                  return m_callback( &message);
               }

               auto type() const { return m_type;}

               inline friend bool operator == ( const Callback& lhs, common::message::Type rhs) { return lhs.type() == rhs;}

               CASUAL_LOG_SERIALIZE({
                  CASUAL_SERIALIZE_NAME( type(), "type");
               })
               
            private:

               using function_type = common::unique_function< bool( const void* memory)>;

               template< typename I> 
               struct model
               {
                  model( I invocable) : invocable{ std::move( invocable)} {}

                  using traits_type = common::traits::function< I>;
                  static constexpr auto valid_signature = traits_type::arguments() == 1;
                  static_assert( valid_signature, "signature has to be callback::Result( const <message>&)");
                  
                  using message_type = common::traits::remove_cvref_t< typename traits_type:: template argument< 0>::type>;

                  bool operator () ( const void* memory)
                  {
                     auto& message = *static_cast< const message_type*>( memory);
                     return invocable( message);
                  }

                  I invocable;
               };

               template< typename I>
               Callback( model< I> callback)
                  : m_type{ model< I>::message_type::type()}, m_callback{ std::move( callback)} {}

               common::message::Type m_type;
               function_type m_callback;
            };

         } // event
         
      } // task

      struct Task
      {
         struct Property
         {
            enum class Execution : short
            {
               concurrent,
               sequential
            };
            friend std::string_view description( Execution value) noexcept;

            enum class Completion : short
            {
               //! has to run regardless
               mandatory,
               //! non started can be removed
               removable, 
               //! can be removed and started will be aborted.
               abortable
            };
            friend std::string_view description( Completion value) noexcept;

            Execution execution = Execution::sequential;
            Completion completion = Completion::mandatory;

            friend std::ostream& operator << ( std::ostream& out, Property value);
         };

         template< typename T>
         explicit Task( task::id::type id, std::string description, T&& task, Property property) 
            : m_context{ std::move( id), std::move( description)},
            m_task{ std::forward< T>( task)},
            m_property( property)
         {
            common::log::line( verbose::log, "task created: ", *this);
         }

         template< typename T>
         explicit Task( std::string description, T&& task, Property property) 
            : Task{ task::id::type::emplace( common::uuid::make()), 
               std::move( description), std::forward< T>( task), property}
         {}

         
         Task( Task&&) = default; // noexcept is deduced
         Task& operator = ( Task&&) = default; // noexcept is deduced

         inline auto& context() const { return m_context;}
         inline auto property() const { return m_property;}

         inline std::vector< task::event::Callback> operator() ( State& state) { return m_task( state, context());}

         inline friend bool operator == ( const Task& lhs, task::id::type rhs) { return lhs.m_context.id == rhs;}
         inline friend bool operator != ( const Task& lhs, task::id::type rhs) { return ! ( lhs.m_context.id == rhs);}


         CASUAL_LOG_SERIALIZE({
            CASUAL_SERIALIZE_NAME( m_context, "context");
            CASUAL_SERIALIZE_NAME( m_property, "property");
         })

      private:

         using task_function_type = common::unique_function< std::vector< task::event::Callback>( State&, const task::Context&)>;

         task::Context m_context;
         task_function_type m_task;
         Property m_property;
      };

      namespace task
      {
         //! running task
         struct Running : Task
         {
            //! construct and start the running task
            Running( State& state, Task&& task);

            template< typename M>
            bool operator () ( const M& message) 
            {
               Trace trace{ "domain::manager::task::Running::operator()"};
               common::log::line( verbose::log, "message: ", message);

               if( auto found = common::algorithm::find( m_callbacks, common::message::type( message)))
               {
                  common::log::line( verbose::log, "callback: ", *found);
                  return common::range::front( found)( message);
               }

               return false;
            }

            auto empty() const { return m_callbacks.empty();}

            friend bool operator == ( const Running& lhs, common::message::Type rhs);

            CASUAL_LOG_SERIALIZE({
               Task::serialize( archive);
               CASUAL_SERIALIZE_NAME( m_callbacks, "callbacks");
            })

            //! base class has done it's thing, make sure we don't "start again"
            std::vector< task::event::Callback> operator() ( State& state) = delete;

         private:
            std::vector< task::event::Callback> m_callbacks;
         };


         struct Queue
         {
            //! start pending tasks if possible.
            void idle( State& state);

            //! adds tasks to queue
            //! @{
            task::id::type add( Task task);
            std::vector< task::id::type> add( std::vector< Task> tasks);
            //! @}

            inline bool empty() const { return m_running.empty() && m_pending.empty();}

            //! dispatch event to event callbacks
            //! @attention this can/should only be called from within a message
            //! handler, to mitigate possible recursive callback invocations. 
            template< typename E>
            void event( State& state, const E& event)
            {
               // second range holds tasks that are 'done'
               auto [ keep, done] = common::algorithm::partition( m_running, [&event]( auto& task)
               {
                  return ! task( event);
               });

               // we handle and remove the 'done' tasks
               common::algorithm::for_each( done, [&]( auto& task)
               {
                  Queue::done( state, std::move( task));
               });

               if( common::algorithm::container::trim( m_running, keep).empty())
                  Queue::next( state);
            }

            //! @returns true any running task listen to this event message
            bool active( common::message::Type type) const;

            //! aborts all abortable tasks, regardless if they're running or not.
            void abort( State& state);

            //! removes all abortable task
            void remove( State& state);

            inline const auto& running() const { return m_running;}
            inline const auto& pending() const { return m_pending;}


            CASUAL_LOG_SERIALIZE({
               CASUAL_SERIALIZE_NAME( m_running, "running");
               CASUAL_SERIALIZE_NAME( m_pending, "pending");
            })

         private:
            void start( State& state, Task&& task);
            void done( State& state, Task&& task);
            void next( State& state);
            
            std::vector< task::Running> m_running;
            std::deque< Task> m_pending;
            
         };

      } // task

   } // domain::manager
} // casual


