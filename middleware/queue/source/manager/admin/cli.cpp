//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/admin/cli.h"

#include "queue/manager/admin/queuevo.h"
#include "queue/manager/admin/services.h"

#include "queue/api/queue.h"
#include "queue/common/transform.h"

#include "common/argument.h"
#include "common/message/queue.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"
#include "common/execute.h"
#include "common/exception/handle.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/archive/create.h"
#include "serviceframework/log.h"


#include <iostream>

namespace casual
{
   using namespace common;

   namespace queue
   {
      namespace normalize
      {

         std::string timestamp( const platform::time::point::type& time)
         {
            if( time != platform::time::point::type::min())
            {
               return chronology::local( time);
            }
            return {};
         }


      } // normalize

      namespace call
      {

         manager::admin::State state()
         {
            serviceframework::service::protocol::binary::Call call;
            auto reply = call( manager::admin::service::name::state());

            manager::admin::State result;
            reply >> CASUAL_MAKE_NVP( result);

            return result;
         }

         std::vector< manager::admin::Message> messages( const std::string& queue)
         {
            serviceframework::service::protocol::binary::Call call;
            call << CASUAL_MAKE_NVP( queue);
            auto reply = call( manager::admin::service::name::list_messages());

            std::vector< manager::admin::Message> result;
            reply >> CASUAL_MAKE_NVP( result);

            return result;
         }

      } // call


      namespace format
      {
         auto messages()
         {
            auto format_state = []( const manager::admin::Message& v)
               {
                  switch( v.state)
                  {
                     case 1: return 'E';
                     case 2: return 'C';
                     case 3: return 'D';
                     default: return '?';
                  }
               };

            auto format_trid = []( const manager::admin::Message& v) { return transcode::hex::encode( v.trid);};
            auto format_type = []( const manager::admin::Message& v) { return v.type;};
            auto format_timestamp = []( const manager::admin::Message& v) { return normalize::timestamp( v.timestamp);};
            auto format_available = []( const manager::admin::Message& v) { return normalize::timestamp( v.available);};

            return terminal::format::formatter< manager::admin::Message>::construct(
               terminal::format::column( "id", std::mem_fn( &manager::admin::Message::id), terminal::color::yellow),
               terminal::format::column( "S", format_state, terminal::color::no_color),
               terminal::format::column( "size", std::mem_fn( &manager::admin::Message::size), terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "trid", format_trid, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "rd", std::mem_fn( &manager::admin::Message::redelivered), terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "type", format_type, terminal::color::no_color),
               terminal::format::column( "reply", std::mem_fn( &manager::admin::Message::reply), terminal::color::no_color),
               terminal::format::column( "timestamp", format_timestamp, terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "available", format_available, terminal::color::blue, terminal::format::Align::right)
            );
         }

         auto groups()
         {
            auto format_pid = []( const manager::admin::Group& g) { return g.process.pid;};
            auto format_ipc = []( const manager::admin::Group& g) { return g.process.ipc;};

            return terminal::format::formatter< manager::admin::Group>::construct(
               terminal::format::column( "name", std::mem_fn( &manager::admin::Group::name), terminal::color::yellow),
               terminal::format::column( "pid", format_pid, terminal::color::grey, terminal::format::Align::right),
               terminal::format::column( "ipc", format_ipc, terminal::color::grey, terminal::format::Align::right),
               terminal::format::column( "queuebase", std::mem_fn( &manager::admin::Group::queuebase))
            );
         }

         auto restored()
         {
            auto format_name = []( const queue::restore::Affected& a) { return a.queue;};

            return terminal::format::formatter< queue::restore::Affected>::construct(
               terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::right),
               terminal::format::column( "restored", std::mem_fn( &queue::restore::Affected::restored), terminal::color::green)
            );
         }


         auto queues( const manager::admin::State& state)
         {
            using q_type = manager::admin::Queue;


            /*
            auto format_error = [&]( const q_type& q){
               return algorithm::find_if( state.queues, [&]( const q_type& e){ return e.id == q.error && e.group == q.group;}).at( 0).name;
            };
            */

            auto format_type = [&]( const q_type& q){
               switch( q.type)
               {
                  case q_type::Type::group_error_queue: return 'g';
                  case q_type::Type::error_queue: return 'e';
                  case q_type::Type::queue: return 'q';
               }
               return '-';
            };

            auto format_group = [&]( const q_type& q){
               return algorithm::find_if( state.groups, [&]( const manager::admin::Group& g){ return q.group == g.process.pid;}).at( 0).name;
            };


            return terminal::format::formatter< manager::admin::Queue>::construct(
               terminal::format::column( "name", std::mem_fn( &q_type::name), terminal::color::yellow),
               terminal::format::column( "count", []( const auto& q){ return q.count;}, terminal::color::green, terminal::format::Align::right),
               terminal::format::column( "size", []( const auto& q){ return q.size;}, common::terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "avg", []( const auto& q){ return q.count == 0 ? 0 : q.size / q.count;}, common::terminal::color::cyan, terminal::format::Align::right),
               terminal::format::column( "uc", []( const auto& q){ return q.uncommitted;}, common::terminal::color::grey, terminal::format::Align::right),
               terminal::format::column( "updated", []( const q_type& q){ return normalize::timestamp( q.timestamp);}),
               terminal::format::column( "r", []( const auto& q){ return q.retries;}, common::terminal::color::blue, terminal::format::Align::right),
               terminal::format::column( "t", format_type, common::terminal::color::blue),
               terminal::format::column( "group", format_group)
            );
         }

         namespace remote
         {
            auto queues( const manager::admin::State& state)
            {
               auto format_pid = [&]( const auto& q){
                  return q.pid;
               };
   
   
               return terminal::format::formatter< manager::admin::remote::Queue>::construct(
                  terminal::format::column( "name", std::mem_fn( &manager::admin::remote::Queue::name), terminal::color::yellow),
                  terminal::format::column( "pid", format_pid, common::terminal::color::blue)
               );
            }
            
         } // remote

      } // format


      void list_queues()
      {

         auto state = call::state();

         auto formatter = format::queues( state);

         formatter.print( std::cout, algorithm::sort( state.queues));
      }

      void list_remote_queues()
      {
         auto state = call::state();
         
         auto formatter = format::remote::queues( state);

         formatter.print( std::cout, algorithm::sort( state.remote.queues));

      }

      void list_groups()
      {
         auto state = call::state();

         auto formatter = format::groups();

         formatter.print( std::cout, state.groups);
      }

      void list_messages( const std::string& queue)
      {
         auto messages = call::messages( queue);

         auto formatter = format::messages();

         formatter.print( std::cout, messages);
      }

      namespace local
      {
         namespace
         {
            auto queues() 
            {
               auto state = call::state();

               return algorithm::transform( state.queues, []( auto& q)
               {
                  return std::move( q.name);
               });
            }

            namespace enqueue
            {
               void invoke( const std::string& queue)
               {
                  auto dispatch = [&queue]( auto&& payload)
                  {
                     queue::Message message;
                     std::swap( message.payload.data, payload.memory);
                     std::swap( message.payload.type, payload.type);

                     auto id = queue::enqueue( queue, message);
                     std::cout << id << '\n';
                  };

                  common::buffer::payload::binary::stream( std::cin, dispatch);
               }

               constexpr auto description = R"(
enqueue buffer(s) to a queue from stdin

Assumes a conformant buffer(s)

Example:
cat somefile.bin | casual queue --enqueue <queue-name>

note: operation is atomic)";
               
            } // enqueue



            struct Empty : public std::runtime_error
            {
               using std::runtime_error::runtime_error;
            };

            namespace dequeue
            {
               void invoke( const std::string& queue)
               {
                  const auto message = queue::dequeue( queue);

                  if( ! message.empty())
                  {
                     auto& payload = message.front().payload;
                     common::buffer::payload::binary::stream( 
                        common::buffer::Payload{ std::move( payload.type), std::move( payload.data)}, 
                        std::cout);
                  }
                  else
                  {
                     throw Empty{ "queue is empty"};
                  }
               }
               constexpr auto description = R"(
dequeue buffer from a queue to stdout

Example:
casual queue --dequeue <queue-name> > somefile.bin

note: operation is atomic)";

            } // dequeue



            namespace consume
            {
               void invoke( const std::string& queue)
               {
                  auto message = queue::dequeue( queue);

                  while( ! message.empty())
                  {
                     auto& payload = message.front().payload;
                     common::buffer::payload::binary::stream( 
                        common::buffer::Payload{ std::move( payload.type), std::move( payload.data)}, 
                        std::cout);

                     message = queue::dequeue( queue);
                  }
               }

               constexpr auto description = R"(
consumes a queue to stdout, dequeues until the queue is empty

Example:
casual queue --consume <queue-name> > somefile.bin 

note: operation is atomic)";
               
            } // consume

            namespace restore
            {
               void invoke( const std::vector< std::string>& queues)
               {
                  auto affected = queue::restore::queue( queues);

                  auto formatter = format::restored();
                  formatter.print( std::cout, affected);
               }

               constexpr auto description = R"("restores messages to queue

Messages will be restored to the queue they first was enqueued to (within the same queue-group)

Example:
casual queue --restore <queue-name>)";
               
            } // restore  
            


            void state( const common::optional< std::string>& format)
            {
               auto state = call::state();

               auto archive = serviceframework::archive::create::writer::from( format.value(), std::cout);
               archive << CASUAL_MAKE_NVP( state);
            }
         } // <unnamed>
      } // local

      namespace manager
      {
         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  auto complete_state = []( auto values, bool){
                     return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                  };

                  auto complete_queues = []( auto& values, bool help) -> std::vector< std::string>
                  { 
                     if( help) 
                        return { "<queue>"};
                        
                     return local::queues();
                  };

                  return common::argument::Group{ [](){}, { "queue"}, "queue related administration",
                     common::argument::Option( &queue::list_queues, { "-q", "--list-queues"}, "list information of all queues in current domain"),
                     common::argument::Option( &queue::list_remote_queues, { "-r", "--list-remote"}, "list all remote discovered queues"),
                     common::argument::Option( &queue::list_groups, { "-g", "--list-groups"}, "list information of all groups in current domain"),
                     common::argument::Option( &queue::list_messages, complete_queues, { "-m", "--list-messages"}, "list information of all messages of a queue"),
                     common::argument::Option( local::restore::invoke, complete_queues, { "--restore"}, local::restore::description),
                     common::argument::Option( &local::enqueue::invoke, complete_queues, { "-e", "--enqueue"}, local::enqueue::description),
                     common::argument::Option( &local::dequeue::invoke, complete_queues, { "-d", "--dequeue"}, local::dequeue::description),
                     common::argument::Option( &local::consume::invoke, complete_queues, { "--consume"}, local::consume::description),
                     common::argument::Option( &queue::local::state, complete_state, {"--state"}, "queue state"),
                  };
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }
            
         } // admin
      } // manager
   } // queue
} // casual



