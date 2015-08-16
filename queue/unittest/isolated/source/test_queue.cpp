//!
//! test_queue.cpp
//!
//! Created on: Aug 15, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "queue/group/group.h"

#include "common/queue.h"

namespace casual
{

   namespace queue
   {
      namespace local
      {
         namespace
         {

            struct Group
            {
               Group() : name{ create_name()}, state{ ":memory:", name, queue}
               {
                  for( auto index = 0; index < 10; ++index)
                  {
                     group::Queue queue;
                     queue.name = name + "_q" + std::to_string( index);

                     state.queuebase.create( std::move( queue));
                  }

                  m_thread = std::thread{ &group::message::pump, std::ref( state)};
               }

               Group( Group&&) = default;
               Group& operator = ( Group&&) = default;

               ~Group()
               {
                  if( m_thread.joinable())
                  {
                     common::queue::blocking::Send send;
                     send( queue.id(), common::message::shutdown::Request{});
                     m_thread.join();
                  }
               }

               std::string name;
               common::ipc::receive::Queue queue;
               group::State state;

            private:

               static std::string create_name()
               {
                  static int index = 1;
                  return "group_" + std::to_string( index++);
               }

               std::thread m_thread;
            };

            struct Broker
            {

            };

            struct Domain
            {


            };
         } // <unnamed>
      } // local



      TEST( casual_queue, someTestCase)
      {

      }

   } // queue
} // casual
