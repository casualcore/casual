//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "domain/common.h"

#include "common/pimpl.h"

#include <queue>
#include <memory>

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         struct State;

         struct Task
         {
            template< typename T>
            explicit Task( T&& task) : m_holder{ make( std::forward< T>( task))} {}
            ~Task();

            void start();

            bool started() const;
            bool done() const;

            friend std::ostream& operator << ( std::ostream& out, const Task& task);

         private:

            struct concept
            {
               virtual ~concept() = default;
               virtual void start() = 0;
               virtual bool done() const = 0;
               virtual bool started() const = 0;
               virtual void print( std::ostream& out) const = 0;
            };


            template< typename T>
            struct model : concept
            {
               explicit model( T task) : m_task( std::move( task)) {}
               ~model() = default;

               enum class State : short
               {
                  not_started,
                  started,
                  done
               };

               void start() override
               {
                  if( m_state == State::not_started)
                  {
                     m_task.start();
                     m_state = State::started;
                  }
               }

               bool started() const override { return m_state != State::not_started;}
               bool done() const override
               {
                  if( m_state == State::started)
                  {
                     m_state = m_task.done() ? State::done : State::started;
                  }
                  return m_state == State::done;
               }


               void print( std::ostream& out) const override { out << m_task;}

               T m_task;
               mutable State m_state = State::not_started;
            };

            template< typename T>
            static std::unique_ptr< concept> make( T&& task)
            {
               return std::make_unique< model< T>>( std::forward< T>( task));
            }

            std::unique_ptr< concept> m_holder;

         };


         namespace task
         {
            struct Queue
            {

               template< typename T>
               void add( T&& task)
               {

                  m_tasks.emplace_back( std::forward< T>( task));

                  common::log::line( domain::log, "added task: ", m_tasks.back());

                  if( m_tasks.size() == 1)
                  {
                     m_tasks.front().start();
                  }
               }

               inline bool empty() const { return m_tasks.empty();}

               void execute();

               friend std::ostream& operator << ( std::ostream& out, const Queue& queue);


            private:
               std::deque< Task> m_tasks;
            };



         } // task

      } // manager
   } // domain


} // casual


