//!
//! casual
//!

#include "queue/forward/common.h"
#include "queue/common/log.h"
#include "queue/api/queue.h"
#include "queue/common/transform.h"

#include "common/arguments.h"
#include "common/exception.h"

#include "common/buffer/pool.h"
#include "common/service/call/context.h"


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
               dispatch_t( std::string queue, std::string service, std::string reply)
                  : queue( std::move( queue)), service( std::move( service)), reply( std::move( reply)) {}

               dispatch_t( std::string queue, std::string service)
                  : dispatch_t( std::move( queue), std::move( service), std::string{}) {}

               std::string queue;
               std::string service;
               std::string reply;
            };

            void setForward( const std::vector< std::string>& values)
            {
               switch( values.size())
               {
                  case 2: dispatch.emplace_back( values.at( 0), values.at( 1)); break;
                  case 3: dispatch.emplace_back( values.at( 0), values.at( 1), values.at( 2)); break;
               }
            }

            std::vector< dispatch_t> dispatch;
         };

         struct Caller
         {
            Caller( std::string service, std::string reply) : m_service( std::move( service)), m_reply( std::move( reply)) {}

            void operator () ( queue::Message&& message)
            {
               Trace trace{ "queue::forward::Caller::operator()"};

               //
               // Prepare the xatmi-buffer
               //
               common::buffer::Payload payload{
                  std::move( message.payload.type),
                  std::move( message.payload.data)};

               log << "payload: " << payload << std::endl;

               long size = payload.memory.size();

               auto buffer = common::buffer::pool::Holder::instance().insert( std::move( payload));

               common::service::call::Context::instance().sync( m_service, buffer, size, buffer, size, TPNOTIME);

               const auto& replyqueue = m_reply.empty() ? message.attributes.reply : m_reply;

               if( ! replyqueue.empty())
               {
                  auto data = common::buffer::pool::Holder::instance().release( buffer);

                  queue::Message reply;
                  reply.payload.data = std::move( data.memory);
                  reply.payload.type = std::move( data.type);

                  queue::enqueue( replyqueue, reply);

               }
            }

         private:
            std::string m_service;
            std::string m_reply;
         };

         std::vector< forward::Task> tasks( Settings settings)
         {
            std::vector< forward::Task> tasks;

            for( auto& task : settings.dispatch)
            {
               tasks.emplace_back( task.queue, Caller{ task.service, task.reply});
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
                  common::Arguments parser{ {
                     common::argument::directive( {"-f", "--forward"}, "--forward  <queue> <service> [<reply>]", settings, &Settings::setForward)
                  }};

                  parser.parse( argc, argv);
               }

               start( std::move( settings));

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




