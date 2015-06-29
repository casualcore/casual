//!
//! template-server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!
//!
//! Only to get a feel for the abstractions needed.
//! That is, to go "from the outside -> in" instead of the opposite
//!



#include "common/process.h"

#include "common/arguments.h"
#include "common/string.h"
#include "common/process.h"
#include "common/trace.h"

#include "sf/log.h"
#include "sf/archive/archive.h"
#include "sf/namevaluepair.h"
#include "sf/buffer.h"


#include "queue/api/rm/queue.h"



#include <unistd.h>




#include "xatmi.h"
#include "tx.h"

extern "C"
{





void casual_test1( TPSVCINFO *serviceContext)
{


   //char* buffer = tpalloc( "STRING", 0, 500);
   //buffer = tprealloc( buffer, 3000);

   {
      casual::common::log::debug << "transb->name: " << serviceContext->name << std::endl;
      casual::common::log::debug << "transb->cd: " << serviceContext->cd << std::endl;
      casual::common::log::debug << "transb->data: " << serviceContext->data << std::endl;



//      std::string test = "bla bla bla";
//      std::copy( test.begin(), test.end(), buffer);
//      buffer[ test.size()] = '\0';
   }


	tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);


}

void casual_test2( TPSVCINFO *serviceContext)
{
   {


      std::string argumentString;

      {
         casual::sf::buffer::binary::Stream stream{ casual::sf::buffer::raw( serviceContext)};
         stream >> argumentString;
         stream.release();
      }

      struct Task
      {
         std::size_t sleep = 0;

      } task;

      casual::common::Arguments parser;

      parser.add(
            casual::common::argument::directive( { "-ms", "--ms-sleep"}, "sleep time", task.sleep)
      );

      parser.parse( casual::common::process::path(), casual::common::string::split( argumentString));

      if( task.sleep > 0)
      {
         casual::sf::log::debug << "casual_test2 called - sleep for  " << task.sleep << "ms"<< std::endl;

         casual::common::process::sleep(  std::chrono::milliseconds( task.sleep));
      }
   }

	tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}


void casual_test3( TPSVCINFO *serviceContext)
{

   auto outcome = TPSUCCESS;
   try
   {
      casual::common::trace::Scope trace{ "casual_test3 called"};


      std::string argumentString;

      {
         casual::sf::buffer::binary::Stream stream{ casual::sf::buffer::raw( serviceContext)};
         stream >> argumentString;
         stream.release();
      }

      struct Task
      {
         std::string message;

         std::string enqueue;
         std::string dequeue;

         bool rollback = false;

      } task;

      casual::common::Arguments parser;

      parser.add(
            casual::common::argument::directive( { "-m"}, "message", task.message),
            casual::common::argument::directive( { "-e", "--enqueue"}, "queue", task.enqueue),
            casual::common::argument::directive( { "-d", "--dequeue"}, "queue", task.dequeue),
            casual::common::argument::directive( { "-rb", "--rollback"}, "queue", task.rollback)
      );

      parser.parse( casual::common::process::path(), casual::common::string::split( argumentString));

      if( ! task.enqueue.empty())
      {

         casual::queue::Message message;

         message.attributes.reply = task.enqueue;
         auto type = casual::sf::buffer::type::get( serviceContext->data);
         message.payload.type.type = type.name;
         message.payload.type.subtype = type.subname;
         casual::common::range::copy( task.message, std::back_inserter( message.payload.data));

         auto id = casual::queue::rm::enqueue( task.enqueue, message);

         casual::sf::log::debug << CASUAL_MAKE_NVP( id);
      }
      else if( ! task.dequeue.empty())
      {
         auto dequeued = casual::queue::rm::dequeue( task.dequeue);

         if( ! dequeued.empty())
         {
            auto& message = dequeued.front();

            casual::sf::log::debug << CASUAL_MAKE_NVP( message);
            /*
            casual::sf::log::debug << CASUAL_MAKE_NVP( message.attributes.properties);
            casual::sf::log::debug << CASUAL_MAKE_NVP( message.attributes.reply);
            casual::sf::log::debug << CASUAL_MAKE_NVP( message.payload.type);
            casual::sf::log::debug << CASUAL_MAKE_NVP( message.payload.data.size());
            */

            std::string data;
            casual::common::range::copy( message.payload.data, std::back_inserter( data));

            casual::sf::log::debug << CASUAL_MAKE_NVP( data);

         }
      }

      if( task.rollback)
      {
         outcome = TPFAIL;
      }

   }
   catch( ...)
   {
      casual::common::error::handler();
      outcome = TPFAIL;
   }
   tpreturn( outcome, 0, serviceContext->data, serviceContext->len, 0);
}


void casual_echo( TPSVCINFO *info)
{
   tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
}



int tpsvrinit(int argc, char **argv)
{
   casual::common::log::debug << "USER tpsvrinit called" << std::endl;

   return tx_open();
}

}






