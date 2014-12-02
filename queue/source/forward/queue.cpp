//!
//! service.cpp
//!
//! Created on: Nov 30, 2014
//!     Author: Lazan
//!

#include "queue/forward/common.h"

#include "common/arguments.h"
#include "common/exception.h"

#include "common/buffer/pool.h"
#include "common/call/context.h"


namespace casual
{
   namespace queue
   {
      namespace forward
      {
         struct Settings
         {

            struct dispatch_t
            {
               dispatch_t( std::string source, std::string destination)
                  : source( std::move( source)), destination( std::move( destination)) {}

               std::string source;
               std::string destination;
            };

            void setForward( const std::vector< std::string>& values)
            {
               if( values.size() == 2)
               {
                  dispatch.emplace_back( values.at( 0), values.at( 1));
               }
            }

            std::vector< dispatch_t> dispatch;
         };

         struct Enqueuer
         {
            Enqueuer( std::string queue) : m_queue( std::move( queue)) {}

            void operator () ( common::message::queue::dequeue::Reply::Message& message)
            {
               //
               // Prepare the xatmi-buffer
               //
               common::buffer::Payload payload;
               payload.type = std::move( message.type);
               payload.memory = std::move( message.payload);

               long size = payload.memory.size();

               //auto buffer = common::buffer::pool::Holder::instance().insert( std::move( payload));

               //common::call::Context::instance().sync( m_service, buffer, 0, buffer, size, 0);
            }

         private:
            std::string m_queue;
         };

         std::vector< forward::Task> tasks( Settings settings)
         {
            std::vector< forward::Task> tasks;

            for( auto& task : settings.dispatch)
            {
               tasks.emplace_back( task.source, Enqueuer{ task.destination});
            }

            return tasks;
         }

         void start( Settings settings)
         {

            Dispatch dispatch( tasks( std::move( settings)));

            dispatch.execute();

         }

         int main( int argc, char **argv)
         {
            try
            {

               Settings settings;

               {
                  common::Arguments parser;

                  parser.add(
                        common::argument::directive( {"-f", "--forward"}, "forward  <queue> <queue>", settings, &Settings::setForward)
                  );

                  parser.parse( argc, argv);

                  common::process::path( parser.processName());
               }

               start( std::move( settings));

            }
            catch( const common::exception::signal::Terminate&)
            {
               return 0;
            }
            catch( ...)
            {
               return common::error::handler();

            }
            return 0;

         }

      } // forward
   } // queue
} // casual


int main( int argc, char **argv)
{
   return casual::queue::forward::main( argc, argv);
}




