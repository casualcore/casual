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
               using native = common::platform::size::type;
               using type = common::strong::task::id;
            } // id

            namespace event
            {
               struct Callback
               {
                  template< typename C>
                  Callback( C&& callback)
                     : Callback{ std::make_unique< model< C>>( std::forward< C>( callback))} {}

                  Callback( Callback&&) noexcept = default;
                  Callback& operator = ( Callback&&) noexcept = default;

                  template< typename M>
                  void operator () ( const M& message) const 
                  {
                     assert( m_type == common::message::type( message));
                     m_concept->dispatch( &message);
                  }

                  auto type() const { return m_type;}

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
                     CASUAL_SERIALIZE_NAME( type(), "type");
                  })
                  
               private:
                  template< typename Model>
                  Callback( std::unique_ptr< Model>&& model)
                     : m_type{ Model::message_type::type()}, m_concept{ std::move( model)} {}
                  
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

                     /*  static constexpr auto valid_signature = traits_type::arguments() == 3 
                        && std::is_same< State, common::traits::remove_cvref_t< typename traits_type:: template argument< 0>::type>>::value
                        && std::is_same< task::id::type, common::traits::remove_cvref_t< typename traits_type:: template argument< 1>::type>>::value;
                     */
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
               };

               struct Dispatch 
               {
                  template< typename M>
                  void operator() ( const M& message) const 
                  {
                     Trace trace{ "domain::manager::task::event::Dispatch operator ()"};
                     common::algorithm::for_each_equal( m_invocables, common::message::type( message), [&]( auto& invocable)
                     {
                        common::log::line( verbose::log, "invoke: ", invocable, " - message: ", message);
                        invocable( message);
                     });
                  }

                  bool active( common::message::Type type) const;

                  void add( task::id::type id, std::vector< Callback> callbacks);
                  void remove( task::id::type id);

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
                     CASUAL_SERIALIZE_NAME( m_invocables, "invocables");
                  })

               private:
                  struct Invocable
                  {
                     inline Invocable( task::id::type id, event::Callback&& callback) : id{ id}, callback{ std::move( callback)} {}

                     template< typename M>
                     void operator () ( const M& message) const 
                     {
                        callback( message);
                     }

                     task::id::type id;
                     event::Callback callback;

                     inline friend bool operator == ( const Invocable& lhs, task::id::type rhs) { return lhs.id == rhs;}
                     inline friend bool operator == ( const Invocable& lhs, common::message::Type rhs) { return lhs.callback.type() == rhs;}

                     CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
                        CASUAL_SERIALIZE( id);
                        CASUAL_SERIALIZE( callback);
                     })
                  };

                  std::vector< Invocable> m_invocables;
               };
               
            } // event

            //! called from a task event callback, when the task is done.
            //! (will trigger a message to the main message pump -> removal of the task and callbacks)
            void done( const State& state, task::id::type id);
            
         } // task

         struct Task
         {
            enum class Type : short
            {
               mandatory,
               abortable
            };

            template< typename T>
            explicit Task( std::string description, Type type, T&& task) 
               : m_concept{ std::make_unique< model< T>>( std::forward< T>( task))},
               m_description{ std::move( description)},
               m_type( type) {}

            template< typename T>
            explicit Task( std::string description, T&& task) 
               : m_concept{ std::make_unique< model< T>>( std::forward< T>( task))},
               m_description{ std::move( description)} {}
            
            ~Task();

            Task( Task&&) noexcept = default;
            Task& operator = ( Task&&) noexcept = default;

            std::vector< task::event::Callback> operator() ( State& state);

            inline auto id() const { return m_id;}
            inline auto type() const { return m_type;}
            inline auto& description() const { return m_description;}

            inline friend bool operator == ( const Task& lhs, task::id::type rhs) { return lhs.m_id == rhs;}
            inline friend bool operator != ( const Task& lhs, task::id::type rhs) { return ! ( lhs.m_id == rhs);}

            friend std::ostream& operator << ( std::ostream& out, Type value);

            CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
               CASUAL_SERIALIZE_NAME( m_id, "id");
               CASUAL_SERIALIZE_NAME( m_type, "type");
               CASUAL_SERIALIZE_NAME( m_description, "description");
            })



         private:

            struct concept
            {
               virtual ~concept() = default;
               virtual std::vector< task::event::Callback> start( State& state, task::id::type id) = 0;
            };

            template< typename T>
            struct model : concept
            {
               explicit model( T task) : m_task( std::move( task)) {}
               ~model() = default;

               std::vector< task::event::Callback> start( State& state, task::id::type id) override
               {
                  return m_task( state, id);
               }

               T m_task;
            };

            task::id::type m_id = common::value::id::sequence< task::id::type>::next();
            std::unique_ptr< concept> m_concept;
            std::string m_description;
            Type m_type = Type::mandatory;
         };


         namespace task
         {
            struct Queue
            {
               //! tasks that can be executed in a concurrent manner.
               //! will start directly
               //! @{
               task::id::type concurrent( State& state, Task&& task);
               //! @}

               //! tasks that needs to be executed in a sequential manner.
               //! will start directly iff no running tasks is present
               //! @{
               task::id::type sequential( State& state, Task&& task);
               //! @}

               inline bool empty() const { return m_running.empty() && m_pending.empty();}

               //! dispatch event to event callbacks
               template< typename E>
               void event( State& state, const E& event)
               {
                  m_events( event);
               }

               //! removes the corresponding finnished task (that sent this message)
               void event( State& state, const common::message::event::domain::task::End& event);

               //! @returns true if event callback type is active
               inline bool active( common::message::Type type) const { return m_events.active( type);}

               //! aborts all abortable tasks, regardless if they're running or not.
               void abort();


               CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
                  CASUAL_SERIALIZE_NAME( m_events, "events");
                  CASUAL_SERIALIZE_NAME( m_running, "running");
                  CASUAL_SERIALIZE_NAME( m_pending, "pending");
               })

            private:
               task::id::type start( State& state, Task&& task);

               event::Dispatch m_events;
               std::vector< Task> m_running;
               std::deque< Task> m_pending;
               
            };

         } // task
      } // manager
   } // domain
} // casual


