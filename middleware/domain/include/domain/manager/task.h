//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_TASK_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_TASK_H_


#include "domain/common.h"

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
            Task( T&& task) : m_holder{ make( std::forward< T>( task))} {}

           ~Task();

           Task( Task&&) noexcept = default;
           Task& operator = ( Task&&) noexcept = default;

            void start();
            bool done() const;

            friend std::ostream& operator << ( std::ostream& out, const Task& task);


         private:

            struct base_task
            {
               virtual ~base_task() = default;
               virtual void start() = 0;
               virtual bool done() const = 0;
               virtual void print( std::ostream& out) const = 0;
            };

            template< typename T>
            struct basic_task : base_task
            {
               basic_task( T task) : m_task( std::move( task)) {}
               ~basic_task() = default;

               void start() override { m_task.start();}
               bool done() const override { return m_task.done();}
               void print( std::ostream& out) const override { out << m_task;}

               T m_task;
            };

            template< typename T>
            static std::unique_ptr< base_task> make( T&& task)
            {
               return std::unique_ptr< base_task>{ new basic_task< T>( std::forward< T>( task))};
            }

            std::unique_ptr< base_task> m_holder;

         };


         namespace task
         {
            struct Queue
            {

               template< typename T>
               void add( T&& task)
               {
                  domain::log << "added task\n";

                  m_tasks.push_back( std::forward< T>( task));

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

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_TASK_H_
