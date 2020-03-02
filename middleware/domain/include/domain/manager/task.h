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

#include <queue>
#include <memory>

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         struct State;

         namespace task
         {
            namespace id
            {
               struct tag{};
               using native = platform::size::type;
               using type = common::strong::task::id;
            } // id

            namespace event
            {
               struct Callback
               {
                  template< typename C>
                  Callback( task::id::type id, C&& callback)
                     : Callback{ id, std::make_unique< model< C>>( std::forward< C>( callback))} {}

                  template< typename M>
                  void operator () ( const M& message) const 
                  {
                     assert( m_type == common::message::type( message));
                     m_concept->dispatch( &message);
                  }

                  auto type() const { return m_type;}

                  inline friend bool operator == ( const Callback& lhs, common::message::Type rhs) { return lhs.type() == rhs;}
                  inline friend bool operator == ( const Callback& lhs, task::id::type rhs) { return lhs.m_id == rhs;}
                  inline friend bool operator != ( const Callback& lhs, task::id::type rhs) { return ! ( lhs == rhs);}

                  CASUAL_LOG_SERIALIZE({
                     CASUAL_SERIALIZE_NAME( type(), "type");
                     CASUAL_SERIALIZE_NAME( m_id, "id");
                  })
                  
               private:
                  template< typename Model>
                  Callback( task::id::type id, std::unique_ptr< Model>&& model)
                     : m_type{ Model::message_type::type()}, m_concept{ std::move( model)}, m_id{ id} {}
                  
                  struct concept 
                  {
                     virtual ~concept() = default;
                     virtual void dispatch( const void* memory) = 0;
                  };

                  template< typename I> 
                  struct model : concept
                  {
                     model( I invocable) : invocable{ std::move( invocable)} {}

                     using traits_type = common::traits::function< I>;
                     static constexpr auto valid_signature = traits_type::arguments() == 1;
                     static_assert( valid_signature, "signature has to be void callable( const <message>&)");
                     
                     using message_type = common::traits::remove_cvref_t< typename traits_type:: template argument< 0>::type>;

                     void dispatch( const void* memory) override
                     {
                        auto& message = *static_cast< const message_type*>( memory);
                        invocable( message);
                     }

                     I invocable;
                  };

                  common::message::Type m_type;
                  std::unique_ptr< concept> m_concept;
                  task::id::type m_id;
               };




            } // event

            //! called from a task event callback, when the task is done.
            //! (will trigger a message to the main message pump -> removal of the task and callbacks)
            void done( const State& state, task::id::type id, 
               common::message::event::domain::task::State outcome = common::message::event::domain::task::State::ok);
            
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
               friend std::ostream& operator << ( std::ostream& out, Execution value);

               enum class Completion : short
               {
                  //! has to run regardless
                  mandatory,
                  //! non started can be removed
                  removable, 
                  //! can be removed and started will be aborted.
                  abortable
               };
               friend std::ostream& operator << ( std::ostream& out, Completion value);

               Execution execution = Execution::sequential;
               Completion completion = Completion::mandatory;

               friend std::ostream& operator << ( std::ostream& out, Property value);
            };

            template< typename T>
            explicit Task( std::string description, T&& task, Property property) 
               : m_task{ std::forward< T>( task)},
               m_property( property),
               m_description{ std::move( description)}
            {
               common::log::line( verbose::log, "task created: ", *this);
            }

            
            Task( Task&&) = default; // noexcept is deduced
            Task& operator = ( Task&&) = default; // noexcept is deduced

            inline auto id() const { return m_id;}
            inline auto property() const { return m_property;}
            inline auto& description() const { return m_description;}

            inline std::vector< task::event::Callback> operator() ( State& state) { return m_task( state, id());}

            inline friend bool operator == ( const Task& lhs, task::id::type rhs) { return lhs.m_id == rhs;}
            inline friend bool operator != ( const Task& lhs, task::id::type rhs) { return ! ( lhs.m_id == rhs);}


            CASUAL_LOG_SERIALIZE({
               CASUAL_SERIALIZE_NAME( m_id, "id");
               CASUAL_SERIALIZE_NAME( m_property, "property");
               CASUAL_SERIALIZE_NAME( m_description, "description");
            })

         private:

            using task_function_type = common::unique_function< std::vector< task::event::Callback>( State&, task::id::type)>;

            task::id::type m_id = common::value::id::sequence< task::id::type>::next();
            task_function_type m_task;
            Property m_property;
            std::string m_description;
         };

         namespace task
         {
            //! running task
            struct Running : Task
            {
               //! construct and start the running task
               Running( State& state, Task&& task);

               template< typename M>
               void operator () ( const M& event) 
               {
                  dispatch( event);
               }

               //! also removes callbacks corresponding to the ended task
               void operator() ( const common::message::event::domain::task::End& event);

               friend bool operator == ( const Running& lhs, common::message::Type rhs);

               CASUAL_LOG_SERIALIZE({
                  Task::serialize( archive);
                  CASUAL_SERIALIZE_NAME( m_callbacks, "callbacks");
               })

               //! base class has done it's thing, make sure we don't "start again"
               std::vector< task::event::Callback> operator() ( State& state) = delete;

            private:

               template< typename M>
               void dispatch( const M& message) const
               {
                  Trace trace{ "domain::manager::task::Running::dispatch"};
                  common::log::line( verbose::log, "running: ", *this);

                  common::algorithm::for_each_equal( m_callbacks, common::message::type( message), [&]( auto& callback)
                  {
                     common::log::line( verbose::log, "callback: ", callback, " - message: ", message);
                     callback( message);
                  });
               }
               std::vector< task::event::Callback> m_callbacks;
            };


            struct Queue
            {
               //! start pending tasks if possible.
               void idle( State& state);

               //! adds tasks to queue
               //! @{
               task::id::type add( State& state, Task&& task);
               std::vector< task::id::type> add( State& state, std::vector< Task>&& tasks);
               //! @}

               inline bool empty() const { return m_running.empty() && m_pending.empty();}

               //! dispatch event to event callbacks
               template< typename E>
               void event( const E& event)
               {
                  common::algorithm::for_each( m_running, [&event]( auto& task)
                  {
                     task( event);
                  });
               }

               //! removes the corresponding finnished task (that sent this message)
               void event( const common::message::event::domain::task::End& event);

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
               task::id::type start( State& state, Task&& task);
               
               std::vector< task::Running> m_running;
               std::deque< Task> m_pending;
               
            };

         } // task
      } // manager
   } // domain
} // casual


