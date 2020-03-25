//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/admin/cli.h"

#include "queue/manager/admin/model.h"
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
#include "common/serialize/create.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace queue
   {

      namespace local
      {
         namespace
         {
            namespace normalize
            {
               std::string timestamp( const platform::time::point::type& time)
               {
                  if( time != platform::time::point::limit::zero())
                  {
                     return chronology::local( time);
                  }
                  return "-";;
               }

            } // normalize

            namespace call
            {
               manager::admin::model::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::state);

                  manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

               std::vector< manager::admin::model::Message> messages( const std::string& queue)
               {
                  serviceframework::service::protocol::binary::Call call;
                  call << CASUAL_NAMED_VALUE( queue);
                  auto reply = call( manager::admin::service::name::messages::list);

                  std::vector< manager::admin::model::Message> result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

            } // call


            namespace format
            {
               auto messages()
               {
                  auto format_state = []( const manager::admin::model::Message& v)
                  {
                     using Enum = decltype( v.state);
                     switch( v.state)
                     {
                        case Enum::enqueued: return 'E';
                        case Enum::committed: return 'C';
                        case Enum::dequeued: return 'D';
                     }
                     return '?';
                  };

                  auto format_trid = []( const manager::admin::model::Message& v) { return transcode::hex::encode( v.trid);};
                  auto format_type = []( const manager::admin::model::Message& v) { return v.type;};
                  auto format_timestamp = []( const manager::admin::model::Message& v) { return normalize::timestamp( v.timestamp);};
                  auto format_available = []( const manager::admin::model::Message& v) { return normalize::timestamp( v.available);};

                  return terminal::format::formatter< manager::admin::model::Message>::construct(
                     terminal::format::column( "id", std::mem_fn( &manager::admin::model::Message::id), terminal::color::yellow),
                     terminal::format::column( "S", format_state, terminal::color::no_color),
                     terminal::format::column( "size", std::mem_fn( &manager::admin::model::Message::size), terminal::color::cyan, terminal::format::Align::right),
                     terminal::format::column( "trid", format_trid, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rd", std::mem_fn( &manager::admin::model::Message::redelivered), terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "type", format_type, terminal::color::no_color),
                     terminal::format::column( "reply", std::mem_fn( &manager::admin::model::Message::reply), terminal::color::no_color),
                     terminal::format::column( "available", format_available, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "timestamp", format_timestamp, terminal::color::blue, terminal::format::Align::right)
                  );
               }

               auto groups()
               {
                  auto format_pid = []( const manager::admin::model::Group& g) { return g.process.pid;};
                  auto format_ipc = []( const manager::admin::model::Group& g) { return g.process.ipc;};

                  return terminal::format::formatter< manager::admin::model::Group>::construct(
                     terminal::format::column( "name", std::mem_fn( &manager::admin::model::Group::name), terminal::color::yellow),
                     terminal::format::column( "pid", format_pid, terminal::color::grey, terminal::format::Align::right),
                     terminal::format::column( "ipc", format_ipc, terminal::color::grey, terminal::format::Align::right),
                     terminal::format::column( "queuebase", std::mem_fn( &manager::admin::model::Group::queuebase))
                  );
               }

               auto affected()
               {
                  auto format_name = []( const queue::restore::Affected& a) { return a.queue;};

                  return terminal::format::formatter< queue::restore::Affected>::construct(
                     terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::right),
                     terminal::format::column( "count", std::mem_fn( &queue::restore::Affected::count), terminal::color::green)
                  );
               }


               auto queues( const manager::admin::model::State& state)
               {
                  using q_type = manager::admin::model::Queue;

                  auto format_retry_delay = []( const q_type& q)
                  {
                     using second_t = std::chrono::duration< double>;
                     return std::chrono::duration_cast< second_t>( q.retry.delay).count();
                  };
                  
                  auto format_group = [&]( const q_type& q){
                     return algorithm::find_if( state.groups, [&]( const manager::admin::model::Group& g){ return q.group == g.process.pid;}).at( 0).name;
                  };

                  return terminal::format::formatter< manager::admin::model::Queue>::construct(
                     terminal::format::column( "name", std::mem_fn( &q_type::name), terminal::color::yellow),
                     terminal::format::column( "count", []( const auto& q){ return q.count;}, terminal::color::green, terminal::format::Align::right),
                     terminal::format::column( "size", []( const auto& q){ return q.size;}, common::terminal::color::cyan, terminal::format::Align::right),
                     terminal::format::column( "avg", []( const auto& q){ return q.count == 0 ? 0 : q.size / q.count;}, common::terminal::color::cyan, terminal::format::Align::right),
                     terminal::format::column( "uc", []( const auto& q){ return q.uncommitted;}, common::terminal::color::grey, terminal::format::Align::right),
                     terminal::format::column( "rc", []( const auto& q){ return q.retry.count;}, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rd", format_retry_delay, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "group", format_group),
                     terminal::format::column( "updated", []( const q_type& q){ return normalize::timestamp( q.timestamp);}, common::terminal::color::blue)
                  );
               }

               namespace remote
               {
                  auto queues( const manager::admin::model::State& state)
                  {
                     auto format_pid = [&]( const auto& q){
                        return q.pid;
                     };
         
         
                     return terminal::format::formatter< manager::admin::model::remote::Queue>::construct(
                        terminal::format::column( "name", std::mem_fn( &manager::admin::model::remote::Queue::name), terminal::color::yellow),
                        terminal::format::column( "pid", format_pid, common::terminal::color::blue)
                     );
                  }
                  
               } // remote

            } // format


            namespace legend
            {
               namespace list
               {
                  constexpr auto queues = R"(legend: list queues 
   name:
      name of the queue
   count:
      number of messages on the queue
   size:
      the current size of the queue, aggregated message sizies
   avg:
      avarage message size in the queue
   uc:
      number of uncommit messages
   rc:
      the retry count of the queue. (error queues has 0 as retry count, hence has to be consumed to be removed)
   rd:
      the retry delay of the queue, if rolled backed available will be `now + retry delay`.
   group:
      which group the queue is hosted on.
   updated:
      the last time the queue was updated
)";

               constexpr auto messages =  R"(legend: list messages 
   id:
      the id of the message
   S:
      the state of the message
         E: enqueued - not visable until commit
         C: committed - visable
         D: dequeued - not visable, removed on commit, back to state 'committed' if rolled back
   size:
      the size of the message
   trid:
      transaction trid
   rd:
      number of 'redeliver' (dequeues that has been rollbacked)
   type:
      type of the payload
   reply:
      the reply queue
   available:
      when the message is available for dequeue
   timestamp:
      when the message was enqueued
      
)";
               } // list            

               void invoke( const std::string& option)
               {
                  if( option == "list-queues")
                     std::cout << legend::list::queues;
                  else if( option == "list-messages")
                     std::cout << legend::list::messages;
               }

               auto complete = []( auto& values, bool help) -> std::vector< std::string>
               {     
                  return { "list-queues", "list-messages"};
               };

               constexpr auto description = R"(provide legend for the output for some of the options

to view legend for --list-queues use casual queue --legend list-queues.

use auto-complete to help which options has legends
)";
            } // legend 


            auto queues() 
            {
               auto state = call::state();

               return algorithm::transform( state.queues, []( auto& q)
               {
                  return std::move( q.name);
               });
            }

            namespace list
            {
               namespace queues
               {
                  void invoke()
                  {
                     auto state = call::state();

                     auto formatter = format::queues( state);

                     formatter.print( std::cout, algorithm::sort( state.queues));
                  }

                  constexpr auto description = R"(list information of all queues in current domain)";
               } // queues

               namespace remote
               {
                  namespace queues
                  {
                     void invoke()
                     {
                        auto state = call::state();
                        
                        auto formatter = format::remote::queues( state);

                        formatter.print( std::cout, algorithm::sort( state.remote.queues));
                     }
                     constexpr auto description = R"(list all remote discovered queues)";
                  } // queues
               } // remote

               namespace groups
               {
                  void invoke()
                  {
                     auto state = call::state();

                     auto formatter = format::groups();

                     formatter.print( std::cout, state.groups);
                  }
                  constexpr auto description = R"(list information of all groups in current domain)";
               } // groups

               namespace messages
               {
                  void invoke( const std::string& queue)
                  {
                     auto messages = call::messages( queue);

                     auto formatter = format::messages();

                     formatter.print( std::cout, messages);
                  }

                  constexpr auto description = R"(list information of all messages of a queue)";
               } // messages
            } // list

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

               constexpr auto description = R"(enqueue buffer(s) to a queue from stdin

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
               constexpr auto description = R"(dequeue buffer from a queue to stdout

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

               constexpr auto description = R"(consumes a queue to stdout, dequeues until the queue is empty

Example:
casual queue --consume <queue-name> > somefile.bin 

note: operation is atomic)";
               
            } // consume

            namespace peek
            {
               void invoke( const std::string& queue, const std::vector< queue::Message::id_type>& ids)
               {
                  auto messages = queue::peek::messages( queue, ids);

                  algorithm::for_each( messages, []( auto& message)
                  {
                     auto& payload = message.payload;
                     common::buffer::payload::binary::stream( 
                        common::buffer::Payload{ std::move( payload.type), std::move( payload.data)}, 
                        std::cout);
                  });
               }

               auto complete = []( auto& values, bool help) -> std::vector< std::string>
               { 
                  if( help) 
                     return { "<queue>", "<id>"};

                  if( values.empty())
                     return local::queues();

                  // complete on id
                  return algorithm::transform( 
                     call::messages( range::front( values)),
                     []( auto& message){ return uuid::string( message.id);});
               };

               constexpr auto description = R"(peeks messages from the give queue and streams them to stdout

Example:
casual queue --peek <queue-name> <id1> <id2> > somefile.bin 

note: operation is atomic)";
               
            } // consume

            namespace restore
            {
               void invoke( const std::vector< std::string>& queues)
               {
                  format::affected().print( std::cout, queue::restore::queue( queues));
               }

               constexpr auto description = R"(restores messages to queue

Messages will be restored to the queue they first was enqueued to (within the same queue-group)

Example:
casual queue --restore <queue-name>)";
               
            } // restore

            namespace messages
            {
               namespace remove
               {
                  void invoke( const std::string& queue, Uuid id, std::vector< Uuid> ids)
                  {
                     ids.insert( std::begin( ids), std::move( id));

                     auto removed = queue::messages::remove( queue, ids);

                     algorithm::for_each( removed, []( auto& id)
                     {
                        std::cout << id << '\n';
                     });
                  }

                  auto complete = []( auto& values, bool help) -> std::vector< std::string>
                  { 
                     if( help) 
                        //return { "<queue>", "<message-id>[1..*]"};
                        return { "<queue>", "<id>"};
                     
                     if( values.empty())
                        return local::queues();
                     
                     return { "<value>"};
                  };

                  constexpr auto description = R"(removes specific messages from a given queue)";
               } // remove

            } // messages

            namespace clear
            {
               void invoke( std::vector< std::string> queues)
               {
                  format::affected().print( std::cout, queue::clear::queue( queues));
               }

               constexpr auto description = R"(clears all messages from provided queues

Example:
casual queue --clear queue-a queue-b)";

               auto complete = []( auto& values, bool help) -> std::vector< std::string>
               { 
                  if( help) 
                     return { "<queue>"};
                     
                  return local::queues();
               };

            } // clear
            
            namespace state
            {
               void invoke( const common::optional< std::string>& format)
               {
                  auto state = call::state();

                  auto archive = common::serialize::create::writer::from( format.value_or( ""));
                  archive << CASUAL_NAMED_VALUE( state);
                  archive.consume( std::cout);
               } 

               auto complete = []( auto values, bool) -> std::vector< std::string>
               {
                  return { "json", "yaml", "xml", "ini"};
               };
               
            } // state

            namespace information
            {
               auto call() -> std::vector< std::tuple< std::string, std::string>> 
               {
                  auto state = local::call::state();

                  auto accumulate = []( auto extract)
                  {
                     return [extract]( auto& queues)
                     {
                        decltype( extract( range::front( queues))) initial{};
                        return algorithm::accumulate( queues, initial, [extract]( auto count, auto& queue){ return count + extract( queue);});
                     };
                  };

                  auto message_count = []( auto& queue){ return queue.count;};
                  auto message_size = []( auto& queue){ return queue.size;};

                  auto split = algorithm::partition( state.queues, []( auto& queue){ return queue.type() == decltype( queue.type())::queue;});
                  auto queues = std::get< 0>( split);
                  auto errors = std::get< 1>( split);

                  return {
                     { "queue.manager.group.count", string::compose( state.groups.size())},
                     { "queue.manager.queue.count", string::compose( queues.size())},
                     { "queue.manager.queue.message.count", string::compose( accumulate( message_count)( queues))},
                     { "queue.manager.queue.message.size", string::compose( accumulate( message_size)( queues))},
                     { "queue.manager.error.queue.message.count", string::compose( accumulate( message_count)( errors))},
                     { "queue.manager.error.queue.message.size", string::compose( accumulate( message_size)( errors))},
                     { "queue.manager.remote.domain.count", string::compose( state.remote.domains.size())},
                     { "queue.manager.remote.queue.count", string::compose( state.remote.queues.size())},
                  };
               } 
               
               void invoke()
               {
                  terminal::formatter::key::value().print( std::cout, call());
               }

               constexpr auto description = R"(collect aggregated information about queues in this domain)";

            } // information


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

                  auto complete_queues = []( auto& values, bool help) -> std::vector< std::string>
                  { 
                     if( help) 
                        return { "<queue>"};
                        
                     return local::queues();
                  };

                  return argument::Group{ [](){}, { "queue"}, "queue related administration",
                     argument::Option( &local::list::queues::invoke, { "-q", "--list-queues"}, local::list::queues::description),
                     argument::Option( &local::list::remote::queues::invoke, { "-r", "--list-remote"}, local::list::remote::queues::description),
                     argument::Option( &local::list::groups::invoke, { "-g", "--list-groups"}, local::list::groups::description),
                     argument::Option( &local::list::messages::invoke, complete_queues, { "-m", "--list-messages"}, local::list::messages::description),
                     argument::Option( &local::restore::invoke, complete_queues, { "--restore"}, local::restore::description),
                     argument::Option( &local::enqueue::invoke, complete_queues, { "-e", "--enqueue"}, local::enqueue::description),
                     argument::Option( &local::dequeue::invoke, complete_queues, { "-d", "--dequeue"}, local::dequeue::description),
                     argument::Option( &local::peek::invoke, local::peek::complete, { "-p", "--peek"}, local::peek::description),
                     argument::Option( &local::consume::invoke, complete_queues, { "--consume"}, local::consume::description),
                     argument::Option( argument::option::one::many( &local::clear::invoke), local::clear::complete, { "--clear"}, local::clear::description),
                     argument::Option( &local::messages::remove::invoke, local::messages::remove::complete, { "--remove-messages"}, local::messages::remove::description),
                     argument::Option( &local::legend::invoke, local::legend::complete, {"--legend"}, local::legend::description),
                     argument::Option( &local::information::invoke, {"--information"}, local::information::description),
                     argument::Option( &local::state::invoke, local::state::complete, {"--state"}, "queue state"),
                  };
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }

            std::vector< std::tuple< std::string, std::string>> cli::information() &
            {
               return local::information::call();
            }
            
         } // admin
      } // manager
   } // queue
} // casual



