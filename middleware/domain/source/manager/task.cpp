//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/task.h"
#include "domain/common.h"


#include "common/stream.h"
#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace domain
   {

      namespace manager
      {
         Task::~Task() = default;

         void Task::start()
         {
            m_holder->start();
         }

         bool Task::started() const
         {
            return m_holder->started();
         }

         bool Task::done() const
         {
            return m_holder->done();
         }

         std::ostream& operator << ( std::ostream& out, const Task& task)
         {
            task.m_holder->print( out);
            return out;
         }

         namespace task
         {
            void Queue::execute()
            {

               while( ! empty())
               {
                  Trace trace{ "domain::manager::task::Queue::execute"};

                  if( ! m_tasks.front().started())
                  {
                     log::line( log, "task start: ", m_tasks.front());
                     m_tasks.front().start();
                  }

                  if( m_tasks.front().done())
                  {
                     log::line( log, "task done: ", m_tasks.front());
                     m_tasks.pop_front();
                  }
                  else
                  {
                     log::line( log, "task NOT done: ", m_tasks.front());
                     return;
                  }
               }
            }


            std::ostream& operator << ( std::ostream& out, const Queue& queue)
            {
               return out << "{ tasks: " << queue.m_tasks << '}';
            }

         } // task


      } // manager
   } // domain


} // casual
