//!
//! client.cpp
//!
//! Created on: Oct 4, 2014
//!     Author: Lazan
//!

#include "queue/broker/admin/queuevo.h"

#include "queue/api/queue.h"

#include "common/arguments.h"
#include "common/message/queue.h"
#include "common/terminal.h"
#include "common/chronology.h"

#include "sf/xatmi_call.h"
#include "sf/archive/terminal.h"
#include "sf/log.h"

#include "xatmi.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace queue
   {
      namespace normalized
      {
         struct Queue
         {
            std::string group;
            std::size_t id;
            std::string name;
            std::size_t type;
            std::size_t retries;
            std::string error;

            broker::admin::Queue::message_t message;

            std::string updated;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( name);

               // size and stuff
               archive & sf::makeNameValuePair( "msg #", message.counts);
               archive & sf::makeNameValuePair( "total", message.size.total);
               archive & sf::makeNameValuePair( "avg", message.size.average);
               archive & sf::makeNameValuePair( "min", message.size.min);
               archive & sf::makeNameValuePair( "max", message.size.max);

               archive & CASUAL_MAKE_NVP( updated);


               archive & CASUAL_MAKE_NVP( retries);
               archive & sf::makeNameValuePair( "error queue", error);


               archive & CASUAL_MAKE_NVP( group);
            })

            friend bool operator < ( const Queue& lhs, const Queue& rhs)
            {
               if( lhs.type > rhs.type)
                  return true;
               if( lhs.type < rhs.type)
                  return false;

               return lhs.name < rhs.name;
            }

            static std::vector< sf::archive::terminal::Directive> directive()
            {
               return {
                  { "name", terminal::color::Solid{ terminal::color::yellow}},
                  { "msg #", sf::archive::terminal::Directive::Align::right},
                  { "total", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::cyan}},
                  { "min", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::cyan}},
                  { "max", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::cyan}},
                  { "avg", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::cyan}},
                  { "retries", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::blue}},
                  { "error queue", sf::archive::terminal::Directive::Align::left, terminal::color::Solid{ common::terminal::color::blue}},
               };
            }

         };


         struct Group
         {
            std::string name;

            broker::admin::Queue::message_t message;

            std::string updated;
            std::string queuebase;

            platform::queue_id_type ipc;


            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( name);
               archive & sf::makeNameValuePair( "msg #", message.counts);
               archive & sf::makeNameValuePair( "total", message.size.total);
               archive & sf::makeNameValuePair( "avg", message.size.average);
               archive & sf::makeNameValuePair( "min", message.size.min);
               archive & sf::makeNameValuePair( "max", message.size.max);

               archive & CASUAL_MAKE_NVP( updated);
               archive & CASUAL_MAKE_NVP( ipc);
               archive & CASUAL_MAKE_NVP( queuebase);
            })

            friend bool operator < ( const Group& lhs, const Group& rhs)
            {
               return lhs.name < rhs.name;
            }

            static std::vector< sf::archive::terminal::Directive> directive()
            {
               auto directive = Queue::directive();
               directive.push_back( { "ipc", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::grey}});
               return directive;
            }
         };


         struct Message
         {
            sf::platform::Uuid id;
            std::string state;

            std::size_t size;

            std::string trid;
            std::size_t rd;

            std::string type;
            std::string reply;

            std::string timestamp;
            std::string avalible;


            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( id);
               archive & sf::makeNameValuePair( "s", state);

               archive & CASUAL_MAKE_NVP( size);

               archive & CASUAL_MAKE_NVP( trid);
               archive & CASUAL_MAKE_NVP( rd);
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( reply);
               archive & CASUAL_MAKE_NVP( timestamp);
               archive & CASUAL_MAKE_NVP( avalible);
            })

            static std::vector< sf::archive::terminal::Directive> directive()
            {
               return {
                  { "id", terminal::color::Solid{ terminal::color::yellow}},
                  { "state", sf::archive::terminal::Directive::Align::right},
                  { "size", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::cyan}},
                  { "trid", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::blue}},
                  { "rd", sf::archive::terminal::Directive::Align::right},
                  { "type", sf::archive::terminal::Directive::Align::left},
                  { "reply", sf::archive::terminal::Directive::Align::left},
                  { "timestamp", sf::archive::terminal::Directive::Align::right, terminal::color::Solid{ common::terminal::color::blue}},
                  { "avalible", sf::archive::terminal::Directive::Align::right},
               };
            }

         };

      } // normalized

      namespace normalize
      {

         std::vector< normalized::Queue> queues( const broker::admin::State& state)
         {
            std::vector< normalized::Queue> result;

            for( auto& queue : state.queues)
            {
               normalized::Queue value;

               value.id = queue.id;
               value.type = queue.type;

               value.name = queue.name;
               value.group = common::range::find_if( state.groups, [&]( const broker::admin::Group& g){
                  return g.id.pid == queue.group;}).at( 0).name;

               value.message = queue.message;
               if( queue.message.timestamp != sf::platform::time_point::min())
               {
                  value.updated = chronology::local( queue.message.timestamp);
               }


               value.retries = queue.retries;

               value.error = common::range::find_if( state.queues, [&]( const broker::admin::Queue& q){
                  return q.id == queue.error && q.group == queue.group;}).at( 0).name;

               result.push_back( std::move( value));
            }

            return range::sort( result);
         }

         std::vector< normalized::Group> groups( broker::admin::State state)
         {
            std::vector< normalized::Group> result;

            auto queues = range::make( state.queues);

            for( auto& group : state.groups)
            {
               normalized::Group value;

               value.message.timestamp = platform::time_point::min();
               value.name = group.name;
               value.queuebase = group.queuebase;
               value.ipc = group.id.queue;

               auto split = range::partition( queues, [&]( const broker::admin::Queue& q)
                     {
                        return q.group == group.id.pid;
                     });

               range::for_each( std::get< 0>( split), [&]( const broker::admin::Queue& q)
                     {
                        if( value.message.size.min > q.message.size.min ) { value.message.size.min = q.message.size.min;}
                        if( value.message.size.max < q.message.size.max ) { value.message.size.max = q.message.size.max;}
                        if( value.message.timestamp < q.message.timestamp ) { value.message.timestamp = q.message.timestamp;}
                        value.message.counts += q.message.counts;
                        value.message.size.total += q.message.size.total;
                     });

               if( value.message.counts > 0)
               {
                  value.message.size.average = value.message.size.total / value.message.counts;
               }

               if( value.message.timestamp != sf::platform::time_type::min())
               {
                  value.updated = chronology::local( value.message.timestamp);
               }

               result.push_back( std::move( value));

               queues = std::get< 1>( split);
            }

            return range::sort( result);
         }

         struct Message
         {
            normalized::Message operator () ( const broker::admin::Message& message)
            {
               normalized::Message result;

               result.id = message.id;
               auto state = []( std::size_t s){
                  switch( s)
                  {
                     case 1: return "E";
                     case 2: return "C";
                     case 3: return "D";
                  }
                  return "?";
               };

               result.state = state( message.state);
               result.size = message.size;

               result.trid = common::transcode::hex::encode( message.trid);
               result.type = message.type.main + ":" + message.type.sub;

               result.rd = message.redelivered;

               result.timestamp = chronology::local( message.timestamp);

               if( message.avalible != std::chrono::time_point_cast< std::chrono::microseconds>( sf::platform::time_type::min()))
               {
                  result.avalible = chronology::local( message.avalible);
               }


               return result;
            }
         };

         std::vector< normalized::Message> messages( std::vector< broker::admin::Message> messages)
         {
            return common::range::transform( messages, Message{});
         }

      } // normalize

      namespace call
      {

         broker::admin::State state()
         {
            sf::xatmi::service::binary::Sync service( ".casual.queue.list.queues");

            auto reply = service();

            broker::admin::State serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }

         std::vector< broker::admin::Message> messages( const std::string& queue)
         {
            sf::xatmi::service::binary::Sync service( ".casual.queue.list.messages");
            service << CASUAL_MAKE_NVP( queue);

            auto reply = service();

            std::vector< broker::admin::Message> serviceReply;

            reply >> CASUAL_MAKE_NVP( serviceReply);

            return serviceReply;
         }


      } // call


      namespace global
      {
         bool porcelain = false;

         bool header = true;
         bool color = true;

         void no_color() { color = false;}
         void no_header() { header = false;}

      } // global


      void listQueues()
      {
         auto queues = normalize::queues( call::state());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( queues);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Queue::directive(), global::header, global::color};
            writer << CASUAL_MAKE_NVP( queues);
         }
      }

      void listGroups()
      {
         auto groups = normalize::groups( call::state());

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( groups);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Group::directive(), global::header, global::color};
            writer << CASUAL_MAKE_NVP( groups);
         }
      }

      void listMessages( const std::string& queue)
      {
         auto temp = call::messages( queue);
         //std::cout << CASUAL_MAKE_NVP( temp);

         auto messages = normalize::messages( temp);

         if( global::porcelain)
         {
            sf::archive::terminal::percelain::Writer writer{ std::cout};
            writer << CASUAL_MAKE_NVP( messages);

         }
         else
         {
            sf::archive::terminal::Writer writer{ std::cout, normalized::Message::directive(), global::header, global::color};
            writer << CASUAL_MAKE_NVP( messages);
         }


      }

      void enqueue_( const std::string& queue)
      {

         queue::Message message;

         message.attributes.reply = queue;
         message.payload.type.type = "X_OCTET";
         message.payload.type.subtype = "binary";

         while( std::cin)
         {
            message.payload.data.push_back( std::cin.get());
         }

         auto id = queue::enqueue( queue, message);

         std::cout << id << std::endl;
      }

      struct Empty : public std::runtime_error
      {
         using std::runtime_error::runtime_error;
      };

      void dequeue_( const std::string& queue)
      {

         const auto message = queue::dequeue( queue);

         //std::cout << CASUAL_MAKE_NVP( message);

         if( message.empty())
         {
            throw Empty{ "queue is empty"};
         }
         else
         {
            for( auto c : message.front().payload.data)
            {
               std::cout.put( c);
            }
         }
         std::cout << std::endl;
      }


   } // queue


   common::Arguments parser()
   {
      common::Arguments parser;
      parser.add(
            common::argument::directive( {"--no-header"}, "do not print headers", &queue::global::no_header),
            common::argument::directive( {"--no-color"}, "do not use color", &queue::global::no_color),
            common::argument::directive( {"--porcelain"}, "Easy to parse format", queue::global::porcelain),
            common::argument::directive( {"-q", "--list-queues"}, "list information of all queues in current domain", &queue::listQueues),
            common::argument::directive( {"-g", "--list-groups"}, "list information of all groups in current domain", &queue::listGroups),
            common::argument::directive( {"-m", "--list-messages"}, "list information of all messages of a queue", &queue::listMessages),
            common::argument::directive( {"-e", "--enqueue"}, "enqueue to a queue from stdin\n  cat somefile.bin | casual-admin queue --enqueue <queue-name>\n  note: should not be used with rest of casual", &queue::enqueue_),
            common::argument::directive( {"-d", "--dequeue"}, "dequeue from a queue to stdout\n  casual-admin queue --dequeue <queue-name> > somefile.bin\n  note: should not be used with rest of casual", &queue::dequeue_)

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
   catch( const casual::queue::Empty& exception)
   {
      //std::cerr << exception.what() << std::endl;
      return 10;
   }
   catch( const std::exception& exception)
   {
      std::cerr << "exception: " << exception.what() << std::endl;
      return 20;
   }


   return 0;
}



