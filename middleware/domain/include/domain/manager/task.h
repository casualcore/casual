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
               {}

            template< typename T>
            explicit Task( std::string description, T&& task) 
               : m_task{ std::forward< T>( task)},
               m_description{ std::move( description)} {}
            
            Task( Task&&) = default; // noexcept is deduced
            Task& operator = ( Task&&) = default; // noexcept is deduced

            inline auto id() const { return m_id;}
            inline auto property() const { return m_property;}
            inline auto& description() const { return m_description;}

            inline std::vector< task::event::Callback> operator() ( State& state) { return m_task( state, id());}

            inline friend bool operator == ( const Task& lhs, task::id::type rhs) { return lhs.m_id == rhs;}
            inline friend bool operator != ( const Task& lhs, task::id::type rhs) { return ! ( lhs.m_id == rhs);}

            

            CASUAL_CONST_CORRECT_SERIALIZE_WRITE({
               CASUAL_SERIALIZE_NAME( m_id, "id");
               CASUAL_SERIALIZE_NAME( m_property, "property");
               CASUAL_SERIALIZE_NAME( m_description, "description");
            })

         private:
            using task_function_type = std::function< std::vector< task::event::Callback>( State&, task::id::type)>;

            task::id::type m_id = common::value::id::sequence< task::id::type>::next();
            task_function_type m_task;
            Property m_property;
            std::string m_description;
         };


         namespace task
         {
            struct Queue
            {
               //! 
               //! will start directly, if no pending, or the task is concurrent-friendly
               //! @{
               task::id::type add( State& state, Task&& task);
               std::vector< task::id::type> add( State& state, std::vector< Task>&& tasks);
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

               //! removes all abortable task
               void remove();

               inline const auto& running() const { return m_running;}
               inline const auto& pending() const { return m_pending;}


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


