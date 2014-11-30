//!
//! client.cpp
//!
//! Created on: Oct 4, 2014
//!     Author: Lazan
//!

#include "queue/broker/admin/queuevo.h"

#include "queue/api/queue.h"

#include "common/arguments.h"

#include "sf/xatmi_call.h"
#include "sf/log.h"

#include "xatmi.h"

namespace casual
{
   namespace queue
   {


      void listGroups()
      {

            sf::xatmi::service::binary::Sync service( ".casual.queue.list.groups");
            auto reply = service();

            std::vector< broker::admin::GroupVO> serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            auto& groups = serviceReply;

            std::cout << CASUAL_MAKE_NVP( groups);
      }

      void listQueues( const std::vector< std::string>& groups)
      {

            sf::xatmi::service::binary::Sync service( ".casual.queue.list.queues");

            service << CASUAL_MAKE_NVP( groups);

            auto reply = service();

            std::vector< broker::admin::verbose::GroupVO> serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            auto& queues = serviceReply;

            std::cout << CASUAL_MAKE_NVP( queues);
      }

      void peekQueue( const std::string& queue)
      {
         auto messages = peek::queue( queue);

         std::cout << CASUAL_MAKE_NVP( messages);

      }

      void enqueue_( const std::string& queue)
      {

         queue::Message message;

         message.attribues.reply = queue;
         message.payload.type = 42;

         while( std::cin)
         {
            message.payload.data.push_back( std::cin.get());
         }

         auto id = queue::enqueue( queue, message);

         std::cout << id << std::endl;
      }

      void dequeue_( const std::string& queue)
      {

         const auto message = queue::dequeue( queue);

         //std::cout << CASUAL_MAKE_NVP( message);

         if( ! message.empty())
         {
            for( const auto& c : message.front().payload.data)
            {
               std::cout.put( c);
            }
            std::cout << std::endl;
         }

      }


   } // queue


   common::Arguments parser()
   {
      common::Arguments parser;
      parser.add(
            common::argument::directive( {"-g", "--list-groups"}, "list all servers", &queue::listGroups),
            common::argument::directive( {"-q", "--list-queues"}, "list queues", &queue::listQueues),
            common::argument::directive( {"-p", "--peek-queue"}, "peek queue", &queue::listQueues),
            common::argument::directive( {"-e", "--enqueue"}, "enqueue", &queue::enqueue_),
            common::argument::directive( {"-d", "--dequeue"}, "dequeue", &queue::dequeue_)
      );

      return parser;
   }


} // casual

int main( int argc, char **argv)
{
   try
   {
      auto parser = casual::parser();

      parser.parse( argc, argv);

   }
   catch( const std::exception& exception)
   {
      std::cerr << "exception: " << exception.what() << std::endl;
   }


   return 0;
}



