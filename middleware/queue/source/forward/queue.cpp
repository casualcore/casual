//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/forward/common.h"
#include "queue/api/queue.h"
#include "queue/common/log.h"

#include "common/argument.h"
#include "common/exception/handle.h"

#include "common/buffer/pool.h"


namespace casual
{
   namespace queue
   {
      namespace forward
      {
         struct Settings
         {
            std::string source;
            std::string destination;

            auto tie() { return std::tie( source, destination);}
         };

         struct Enqueuer
         {
            Enqueuer( std::string queue) : m_queue( std::move( queue)) {}

            void operator () ( queue::Message&& message)
            {
               Trace trace{ "queue::forward::Enqueuer::operator()"};
               queue::enqueue( m_queue, message);
            }

         private:
            std::string m_queue;
         };

         std::vector< forward::Task> tasks( Settings settings)
         {
            std::vector< forward::Task> tasks;

            tasks.emplace_back( settings.source, Enqueuer{ settings.destination});

            return tasks;
         }

         void start( Settings settings)
         {
            Dispatch dispatch( tasks( std::move( settings)));

            dispatch.execute();
         }

         void main( int argc, char **argv)
         {
            Settings settings;

            using namespace casual::common::argument;
            Parse{ "queue forward to queue",
               Option( settings.tie(), {"-f", "--forward"}, "--forward  <from-queue> <to-queue>")
            }( argc, argv);

            start( std::move( settings));
         }

      } // forward
   } // queue
} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::guard( [=]()
   {
      casual::queue::forward::main( argc, argv);
   });
}




