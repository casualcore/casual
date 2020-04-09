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
#include "queue/common/queue.h"

#include "common/argument.h"
#include "common/message/queue.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"
#include "common/execute.h"
#include "common/exception/handle.h"
#include "common/serialize/create.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"


#include "casual/cli/pipe.h"
#include "casual/cli/message.h"

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
            namespace global
            {
               struct
               {
                  transaction::ID trid;
               } state;

            } // global
            namespace normalize
            {
               std::string timestamp( const platform::time::point::type& time)
               {
                  if( time != platform::time::point::limit::zero())
                     return chronology::utc::offset( time);

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
                  auto format_state = []( auto& message)
                  {
                     using Enum = decltype( message.state);
                     switch( message.state)
                     {
                        case Enum::enqueued: return 'E';
                        case Enum::committed: return 'C';
                        case Enum::dequeued: return 'D';
                     }
                     return '?';
                  };

                  auto format_trid = []( auto& message) { return transcode::hex::encode( message.trid);};
                  auto format_type = []( auto& message) { return message.type;};
                  auto format_timestamp = []( auto& message) { return normalize::timestamp( message.timestamp);};
                  auto format_available = []( auto& message) { return normalize::timestamp( message.available);};

                  return terminal::format::formatter< manager::admin::model::Message>::construct(
                     terminal::format::column( "id", []( auto& message) { return message.id;}, terminal::color::yellow),
                     terminal::format::column( "S", format_state, terminal::color::no_color),
                     terminal::format::column( "size", []( auto& message) { return message.size;}, terminal::color::cyan, terminal::format::Align::right),
                     terminal::format::column( "trid", format_trid, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rd", []( auto& message) { return message.redelivered;}, terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "type", format_type, terminal::color::no_color),
                     terminal::format::column( "reply", []( auto& message) { return message.reply;}, terminal::color::no_color),
                     terminal::format::column( "available", format_available, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "timestamp", format_timestamp, terminal::color::blue, terminal::format::Align::right)
                  );
               }

               auto groups()
               {
                  auto format_pid = []( auto& group) { return group.process.pid;};
                  auto format_ipc = []( auto& group) { return group.process.ipc;};

                  return terminal::format::formatter< manager::admin::model::Group>::construct(
                     terminal::format::column( "name", []( auto& group){ return group.name;}, terminal::color::yellow),
                     terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "ipc", format_ipc, terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "queuebase", []( auto& group){ return group.queuebase;}, terminal::color::cyan)
                  );
               }

               auto affected()
               {
                  auto format_name = []( auto& value) { return value.queue;};

                  return terminal::format::formatter< queue::restore::Affected>::construct(
                     terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::right),
                     terminal::format::column( "count", std::mem_fn( &queue::restore::Affected::count), terminal::color::green)
                  );
               }


               auto queues( const manager::admin::model::State& state)
               {
                  using second_t = std::chrono::duration< double>;

                  auto format_retry_delay = []( auto& queue)
                  {
                     return std::chrono::duration_cast< second_t>( queue.retry.delay).count();
                  };
                  
                  auto format_group = [&]( auto& queue)
                  {
                     return algorithm::find_if( state.groups, [&]( auto& group){ return queue.group == group.process.pid;}).at( 0).name;
                  };

                  auto avg_size = []( auto& queue)
                  {
                     return queue.count == 0 ? 0 : queue.size / queue.count;
                  };

                  return terminal::format::formatter< manager::admin::model::Queue>::construct(
                     terminal::format::column( "name", []( const auto& q){ return q.name;}, terminal::color::yellow),
                     terminal::format::column( "group", format_group),
                     terminal::format::column( "rc", []( const auto& q){ return q.retry.count;}, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rd", format_retry_delay, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "count", []( const auto& q){ return q.count;}, terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "size", []( const auto& q){ return q.size;}, common::terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "avg", avg_size, common::terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "EQ", []( auto& q){ return q.metric.enqueued;}, common::terminal::color::cyan, terminal::format::Align::right),
                     terminal::format::column( "DQ", []( auto& q){ return q.metric.dequeued;}, common::terminal::color::cyan, terminal::format::Align::right),
                     terminal::format::column( "UC", []( const auto& q){ return q.uncommitted;}, common::terminal::color::magenta, terminal::format::Align::right),
                     terminal::format::column( "last", []( auto& q){ return normalize::timestamp( q.last);}, common::terminal::color::blue)
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

               namespace forward
               {
                  auto column_alias = []()
                  {
                     return terminal::format::column( "alias", []( auto& forward){ return forward.alias;}, terminal::color::yellow);
                  };
                  
                  auto column_configure_instances = []()
                  { 
                     return terminal::format::column( "CI", []( auto& forward){ return forward.instances.configured;}, terminal::color::white);
                  };
                  auto column_running_instances = []()
                  { 
                     return terminal::format::column( "I", []( auto& forward){ return forward.instances.running;}, terminal::color::white);
                  };

                  auto column_source = []()
                  {
                     return terminal::format::column( "source", []( auto& forward){ return forward.source;}, terminal::color::white);
                  };

                  auto column_commit = []()
                  {
                     return terminal::format::column( "commit", []( auto& forward)
                     { 
                        return forward.metric.commit.count;
                     }, terminal::color::cyan, terminal::format::Align::right);
                  };

                  auto column_rollback = []()
                  {
                     return terminal::format::column( "rollback", []( auto& forward)
                     { 
                        return forward.metric.rollback.count;
                     }, terminal::color::cyan, terminal::format::Align::right);
                  };

                  auto column_last = []()
                  {
                     return terminal::format::column( "last", []( auto& forward)
                     {
                        return local::normalize::timestamp( std::max( forward.metric.commit.last, forward.metric.rollback.last));
                     }, terminal::color::blue);
                  };

                  using time_type = std::chrono::duration< double>;

                  auto services()
                  {
                     auto column_target = []()
                     {
                        return terminal::format::column( "target", []( auto& forward){ return forward.target.service;}, terminal::color::white);
                     };

                     auto column_reply_name = []()
                     {
                        return terminal::format::column( "reply", []( auto& forward)
                        { 
                           if( forward.reply)
                              return forward.reply.value().queue;
                           
                           return std::string{ "-"};
                        }, terminal::color::cyan);
                     };

                     auto column_reply_delay = []()
                     {
                        return terminal::format::column( "delay", []( auto& forward)
                        { 
                           if( ! forward.reply)
                              return std::string{ "-"};

                           return std::to_string( std::chrono::duration_cast< time_type>( forward.reply.value().delay).count());
                        }, terminal::color::cyan, terminal::format::Align::right);
                     };

                     return terminal::format::formatter< manager::admin::model::Forward::Service>::construct(
                        column_alias(),
                        column_source(),
                        column_target(),
                        column_reply_name(),
                        column_reply_delay(),
                        column_configure_instances(),
                        column_running_instances(),
                        column_commit(),
                        column_rollback(),
                        column_last()
                     );
                  }
   
                  auto queues()
                  {
                     auto column_target = []()
                     {
                        return terminal::format::column( "target", []( auto& forward){ return forward.target.queue;}, terminal::color::white);
                     };

                     auto column_target_delay = []()
                     {
                        return terminal::format::column( "delay", []( auto& forward)
                        { 
                           return std::chrono::duration_cast< time_type>( forward.target.delay).count();
                        }, terminal::color::white, terminal::format::Align::right);
                     };

                     return terminal::format::formatter< manager::admin::model::Forward::Queue>::construct(
                        column_alias(),
                        column_source(),
                        column_target(),
                        column_target_delay(),
                        column_configure_instances(),
                        column_running_instances(),
                        column_commit(),
                        column_rollback(),
                        column_last()
                     );
                  }

               } // forward

            } // format

            auto queues() 
            {
               auto state = call::state();

               return algorithm::transform( state.queues, []( auto& q)
               {
                  return std::move( q.name);
               });
            }

            namespace complete
            {
               auto queues = []( auto& values, bool help) -> std::vector< std::string>
               { 
                  if( help) 
                     return { "<queue>"};
                     
                  return local::queues();
               };
            } // complete


            namespace legend
            {
               namespace list
               {
                  constexpr auto queues = R"(legend: list queues 
   name:
      name of the queue
   group:
      which group the queue is hosted on.
   rc:
      retry-count - the retry count of the queue. (error queues has 0 as retry count, hence has to be consumed to be removed)
   rd:
      retry-delay - the retry delay of the queue, if rolled backed available will be 'now + retry delay'.
   count:
      number of messages on the queue
   size:
      the current size of the queue, aggregated message sizies
   avg:
      avarage message size in the queue
   EQ: 
      enqueued - total number of successfully enqueued messages on the queue (committet), over time.
      note: this also include moved and restored messages.
   DQ: 
      dequeued - total number of successfully dequeued messages on the queue (committet), over time. 
      note: this also includes when a message is moved to an error queue if a retry-count is reached.   
   UC:
      number of currently uncommitted messages.
   last:
      the timetamp of the newest message on the queue, or has been on the queue if the queue is empty.
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

               auto option()
               {
                  auto invoke = []( const std::string& option)
                  {
                     if( option == "list-queues")
                        std::cout << legend::list::queues;
                     else if( option == "list-messages")
                        std::cout << legend::list::messages;
                  };

                  auto complete = []( auto& values, bool help) -> std::vector< std::string>
                  {     
                     return { "list-queues", "list-messages"};
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete,
                     { "--legend"},
                     R"(provide legend for the output for some of the options

to view legend for --list-queues use casual queue --legend list-queues.

use auto-complete to help which options has legends)"
                  };
               }

            } // legend 


            namespace list
            {
               namespace queues
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();

                        auto formatter = format::queues( state);

                        formatter.print( std::cout, algorithm::sort( state.queues));
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "-q", "--list-queues"},
                        R"(list information of all queues in current domain)"
                     };
                  }
                  
               } // queues

               namespace remote
               {
                  namespace queues
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           auto formatter = format::remote::queues( state);
                           formatter.print( std::cout, algorithm::sort( state.remote.queues));
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           { "-r", "--list-remote"},
                           R"(list all remote discovered queues)"
                        };
                     }
                  } // queues
               } // remote

               namespace groups
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        auto state = call::state();
                        format::groups().print( std::cout, state.groups);
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        {  "-g", "--list-groups"},
                        "list information of all groups in current domain"
                     };
                  }
               } // groups

               namespace messages
               {
                  auto option()
                  {
                     auto invoke = []( const std::string& queue)
                     {
                        auto messages = call::messages( queue);
                        auto formatter = format::messages();
                        formatter.print( std::cout, messages);
                     };
                     
                     return argument::Option{
                        std::move( invoke),
                        complete::queues,
                        {  "-m", "--list-messages"},
                        "list information of all messages of the provided queue"
                     };
                  }
               } // messages

               namespace forward
               {
                  namespace services
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           format::forward::services().print( std::cout, state.forward.services);
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           {  "--list-forward-services"},
                           "list information of all service forwards"
                        };
                     }
                  } // services

                  namespace queues
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::state();
                           format::forward::queues().print( std::cout, state.forward.queues);
                        };
                        
                        return argument::Option{
                           std::move( invoke),
                           {  "--list-forward-queues"},
                           "list information of all queue forwards"
                        };
                     }
                  } // services

               } // groups
            } // list

            namespace enqueue
            {
               auto option()
               {
                  auto invoke = []( std::string name)
                  {
                     Trace trace{ "queue::local::enqueue::invoke"};

                     auto destination = queue::Lookup{ name}();
                     log::line( verbose::log, "queue: ", destination);
                     
                     communication::stream::inbound::Device in{ std::cin};

                     cli::pipe::transaction::Association associate;

                     auto enqueue = [&name, &destination, &associate]( cli::message::Payload& payload)
                     {
                        associate( payload);

                        common::message::queue::enqueue::Request request{ process::handle()};
                        request.trid = payload.transaction.trid;
                        request.name = name;
                        request.queue = destination.queue;
                        std::swap( request.message.payload, payload.payload.memory);
                        std::swap( request.message.type, payload.payload.type);

                        cli::message::queue::message::ID id{ process::handle()};
                        id.id = communication::ipc::call( destination.process.ipc, request).id;
                        id.transaction = payload.transaction;
                        cli::pipe::forward::message( id);
                     };

                     bool done = false;

                     auto handler = common::message::dispatch::handler( in, 
                        cli::pipe::forward::handle::defaults(),
                        casual::cli::pipe::handle::done( done),
                        cli::pipe::transaction::handle::directive( associate),
                        std::move( enqueue)
                     );

                     // start the pump
                     common::message::dispatch::pump( cli::pipe::condition::done( done), handler, in);

                     cli::pipe::done();
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete::queues,
                     { "-e", "--enqueue"},
                     R"(enqueue buffer(s) to a queue from stdin

Assumes a conformant buffer(s)

Example:
cat somefile.bin | casual queue --enqueue <queue-name>

@note: part of casual-pipe)"
                  };

               }
               
            } // enqueue



            struct Empty : public std::runtime_error
            {
               using std::runtime_error::runtime_error;
            };

            namespace dequeue
            {
               auto forward = []( auto& destination, auto& associate, const std::optional< Uuid>& id)
               {
                  // we 'start' a new execution 'context'
                  common::execution::reset();

                  Trace trace{ "queue::local::dequeue::action"};
                  log::line( verbose::log, "destination: ", destination);
                  log::line( verbose::log, "associate: ", associate);

                  common::message::queue::dequeue::Request request{ process::handle()};
                  // associate the request to 'transaction', if no ongoing directive -> no-op
                  associate( request);
                  request.name = destination.name;
                  request.queue = destination.queue;

                  if( id)
                     request.selector.id = id.value();

                  auto reply = communication::ipc::call( destination.process.ipc, request);

                  if( ! reply.message.empty())
                  {
                     cli::message::Payload payload{ process::handle()};
                     std::swap( reply.message.front().payload, payload.payload.memory);
                     std::swap( reply.message.front().type, payload.payload.type);
                     payload.transaction.trid = request.trid;
                     cli::pipe::forward::message( payload);

                     return true;
                  }

                  // if associated with a transaction, send the transaction downstream, 
                  // since associate has sent the association of the transaction upstream already
                  if( request.trid)
                  {
                     cli::message::transaction::Propagate message{ process::handle()};
                     message.transaction.trid = request.trid;
                     cli::pipe::forward::message( message);
                  }

                  return false;
               };

               auto option()
               {
                  auto invoke = []( std::string queue, const std::optional< Uuid>& id)
                  {
                     Trace trace{ "queue::local::dequeue::invoke"};

                     auto destination = queue::Lookup{ std::move( queue)}();
                     
                     communication::stream::inbound::Device in{ std::cin};
                     cli::pipe::transaction::Association associate;

                     // we need to consume to see if we've got an ongoing transaction

                     bool done = false;

                     auto handler = common::message::dispatch::handler( in, 
                        cli::pipe::forward::handle::defaults(),
                        cli::pipe::handle::done( done),
                        cli::pipe::transaction::handle::directive( associate)
                     );

                     // consume the pipe
                     common::message::dispatch::pump( cli::pipe::condition::done( done), handler, in);

                     dequeue::forward( destination, associate, id);

                     cli::pipe::done();
                  };

                  auto complete = []()
                  {
                     return []( auto, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<queue>", "[<id>]"};

                        return local::queues();
                     };
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete(),
                     { "-d", "--dequeue"},
                     R"(dequeue buffer from a queue to stdout

if id is absent the oldest available message is dequeued. 

Example:
casual queue --dequeue <queue> | <some other part in casual-pipe> | ... | <casual-pipe termination>
casual queue --dequeue <queue> <id> | <some other part in casual-pipe> | ... | <casual-pipe termination>

@note: part of casual-pipe
)"

                  };
               }
            } // dequeue



            namespace consume
            {
               auto option()
               {
                  auto invoke = []( std::string queue, std::optional< platform::size::type> count)
                  {
                     Trace trace{ "queue::local::consume::invoke"};
                     auto destination = queue::Lookup{ std::move( queue)}();

                     communication::stream::inbound::Device in{ std::cin};
                     cli::pipe::transaction::Association associate;

                     // we need to consume to see if we've got a ongoing transaction

                     bool done = false;

                     auto handler = common::message::dispatch::handler( in, 
                        cli::pipe::forward::handle::defaults(),
                        cli::pipe::handle::done( done),
                        cli::pipe::transaction::handle::directive( associate)
                     );

                     // consume the pipe
                     common::message::dispatch::pump( cli::pipe::condition::done( done), handler, in);

                     auto remaining = count.value_or( std::numeric_limits< platform::size::type>::max());

                     while( remaining > 0 && dequeue::forward( destination, associate, {}))
                        --remaining;

                     cli::pipe::done();
                  };

                  return argument::Option{
                     std::move( invoke),
                     []( auto& values, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<queue>", "<count>"};
                        
                        if( values.empty())
                           return local::queues();
                        else 
                           return { "<value>"};

                     },
                     { "--consume"},
                      R"(consumes up to `count` messages from the provided `queue` and send it downstream

Example:
casual queue --consume <queue-name> [<count>] | <some other part of casual-pipe> | ... | <casual-pipe-termination>

@note: part of casual-pipe
)"

                  };
               }
               
            } // consume

            namespace peek
            {
               auto option()
               {
                  auto invoke = [](const std::string& queue, const std::vector< queue::Message::id_type>& ids)
                  {
                     // consume from casual-pipe
                     communication::stream::inbound::Device in{ std::cin};

                     // we need to consume to see if we've got a ongoing transaction

                     bool done = false;

                     auto handler = common::message::dispatch::handler( in, 
                        cli::pipe::forward::handle::defaults(),
                        cli::pipe::handle::done( done)
                     );

                     // consume the pipe
                     common::message::dispatch::pump( cli::pipe::condition::done( done), handler, in);

                     auto messages = queue::peek::messages( queue, ids);

                     algorithm::for_each( messages, []( auto& message)
                     {
                        cli::message::Payload pipemessage;
                        pipemessage.payload.type = std::move( message.payload.type);
                        pipemessage.payload.memory = std::move( message.payload.data);

                        cli::pipe::forward::message( pipemessage);
                     });

                     cli::pipe::done();
                  };

                  auto complete = []( auto& values, bool help) -> std::vector< std::string>
                  { 
                     if( help) 
                        return { "<queue>", "[<id>]"};

                     if( values.empty())
                        return local::queues();

                     // complete on id
                     return algorithm::transform( 
                        call::messages( range::front( values)),
                        []( auto& message){ return uuid::string( message.id);});
                  };

                  return argument::Option{
                     std::move( invoke),
                     std::move( complete),
                     { "-p", "--peek"},
                     R"(peeks messages from the give queue and streams them to casual-pipe

Example:
casual queue --peek <queue-name> <id1> <id2> | <some other part of casual-pipe> | ... | <casual-pipe-termination>

@note: part of casual-pipe)"
                  };
               }
               
            } // consume

            namespace restore
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     format::affected().print( std::cout, queue::restore::queue( queues));
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete::queues,
                     {  "--restore"},
                     R"(restores messages to queue

Messages will be restored to the queue they first was enqueued to (within the same queue-group)

Example:
casual queue --restore <queue-name>)"
                  };
               }
            } // restore

            namespace messages
            {
               namespace remove
               {
                  auto option()
                  {
                     auto invoke = []( const std::string& queue, Uuid id, std::vector< Uuid> ids)
                     {
                        ids.insert( std::begin( ids), std::move( id));
                        auto removed = queue::messages::remove( queue, ids);

                        algorithm::for_each( removed, []( auto& id)
                        {
                           std::cout << id << '\n';
                        });
                     };

                     auto complete = []( auto& values, bool help) -> std::vector< std::string>
                     { 
                        if( help) 
                           return { "<queue>", "<id>"};
                        
                        if( values.empty())
                           return local::queues();
                        
                        return { "<value>"};
                     };

                     return argument::Option{
                        std::move( invoke),
                        complete,
                        {  "--remove-messages"},
                        R"(removes specific messages from a given queue)"
                     };
                  }
               } // remove

            } // messages

            namespace clear
            {
               auto option()
               {
                  auto invoke = []( const std::vector< std::string>& queues)
                  {
                     format::affected().print( std::cout, queue::clear::queue( queues));
                  };

                  return argument::Option{
                     argument::option::one::many( std::move( invoke)),
                     complete::queues,
                     {  "--clear"},
                     R"(clears all messages from provided queues

Example:
casual queue --clear a b c)"
                  };
               }

            } // clear

            namespace forward
            {
               namespace scale
               {
                  namespace aliases
                  {
                     auto option()
                     {
                        auto invoke = []( std::vector< std::tuple< std::string, platform::size::type>> values)
                        {
                           auto aliases = algorithm::transform( values, []( auto& value)
                           {
                              manager::admin::model::scale::Alias result;
                              result.name = std::move( std::get< 0>( value));
                              result.instances = std::get< 1>( value);
                              return result;
                           });

                           serviceframework::service::protocol::binary::Call call;
                           call << CASUAL_NAMED_VALUE( aliases);
                           call( manager::admin::service::name::forward::scale::aliases);
                        };

                        auto complete = []( auto&& values, bool help) -> std::vector< std::string>
                        {
                           if( help)
                              return { "<alias>", "<# instances>"};

                           if( range::size( values) % 2 == 1)
                              return { "<value>"};

                           auto state = call::state();

                           auto transform_alias = []( auto& forward){ return forward.alias;};

                           auto aliases = algorithm::transform( state.forward.services, transform_alias);
                           algorithm::transform( state.forward.queues, aliases, transform_alias);

                           return aliases;
                        };

                        return argument::Option{
                           argument::option::one::many( std::move( invoke)),
                           complete,
                           {  "--forward-scale-aliases"},
                           R"(scales forward aliases to the requested number of instances

Example:
casual queue --forward-scale-aliases a 2 b 0 c 10)"
                        };
                     };  
                  } // aliases

               } // scale
            } // forward

            namespace metric
            {
               namespace reset
               {
                  auto option()
                  {
                     auto invoke = []( const std::vector< std::string>& queues)
                     {
                        serviceframework::service::protocol::binary::Call call;
                        call << CASUAL_NAMED_VALUE( queues);
                        auto reply = call( manager::admin::service::name::metric::reset);
                     };

                     return argument::Option{
                        argument::option::one::many( std::move( invoke)),
                        complete::queues,
                        {  "--metric-reset"},
                        R"(resets metrics for the provided queues

if no queues are provided, metrics for all queues are reset.

Example:
casual queue --metric-reset a b)"
                     };
                  }
                  
               } // reset
            } // metric
            
            namespace state
            {
               auto option()
               {
                  auto invoke = []( const std::optional< std::string>& format)
                  {
                     auto state = call::state();

                     auto archive = common::serialize::create::writer::from( format.value_or( ""));
                     archive << CASUAL_NAMED_VALUE( state);
                     archive.consume( std::cout);
                  };

                  auto complete = []( auto values, bool) -> std::vector< std::string>
                  {
                     return { "json", "yaml", "xml", "ini"};
                  };

                  return argument::Option{
                     std::move( invoke),
                     complete,
                     {  "--state"},
                     "queue state"
                  };
               }
               
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

                  auto max = []( auto extract)
                  {
                     return [extract]( auto& range)
                     {
                        decltype( extract( range::front( range))) initial{};
                        return algorithm::accumulate( range, initial, [extract]( auto value, auto& element){ return std::max( value, extract( element));});
                     };
                  };

                  auto message_count = []( auto& queue){ return queue.count;};
                  auto message_size = []( auto& queue){ return queue.size;};

                  auto metric_enqueued = []( auto& queue){ return queue.metric.enqueued;};
                  auto metric_dequeued = []( auto& queue){ return queue.metric.dequeued;};

                  auto split = algorithm::partition( state.queues, []( auto& queue){ return queue.type() == decltype( queue.type())::queue;});
                  auto queues = std::get< 0>( split);
                  auto errors = std::get< 1>( split);

                  auto commit_count = []( auto& forward){ return forward.metric.commit.count;};
                  auto commit_last = []( auto& forward){ return forward.metric.commit.last;};
                  auto rollback_count = []( auto& forward){ return forward.metric.rollback.count;};
                  auto rollback_last = []( auto& forward){ return forward.metric.rollback.last;};

                  return {
                     { "queue.manager.group.count", string::compose( state.groups.size())},
                     { "queue.manager.queue.count", string::compose( queues.size())},
                     { "queue.manager.queue.message.count", string::compose( accumulate( message_count)( queues))},
                     { "queue.manager.queue.message.size", string::compose( accumulate( message_size)( queues))},
                     { "queue.manager.queue.metric.enqueued", string::compose( accumulate( metric_enqueued)( queues))},
                     { "queue.manager.queue.metric.dequeued", string::compose( accumulate( metric_dequeued)( queues))},
                     { "queue.manager.error.queue.message.count", string::compose( accumulate( message_count)( errors))},
                     { "queue.manager.error.queue.message.size", string::compose( accumulate( message_size)( errors))},
                     { "queue.manager.error.queue.metric.enqueued", string::compose( accumulate( metric_enqueued)( errors))},
                     { "queue.manager.error.queue.metric.dequeued", string::compose( accumulate( metric_dequeued)( errors))},
                     { "queue.manager.remote.domain.count", string::compose( state.remote.domains.size())},
                     { "queue.manager.remote.queue.count", string::compose( state.remote.queues.size())},
                     { "queue.manager.forward.services.count", string::compose( state.forward.services.size())},
                     { "queue.manager.forward.services.metric.commit.count", string::compose( accumulate( commit_count)( state.forward.services))},
                     { "queue.manager.forward.services.metric.commit.last", string::compose( max( commit_last)( state.forward.services))},
                     { "queue.manager.forward.services.metric.rollback.count", string::compose( accumulate( rollback_count)( state.forward.services))},
                     { "queue.manager.forward.services.metric.rollback.last", string::compose( max( rollback_last)( state.forward.services))},
                     { "queue.manager.forward.queues.count", string::compose( state.forward.queues.size())},
                     { "queue.manager.forward.queues.metric.commit.count", string::compose( accumulate( commit_count)( state.forward.queues))},
                     { "queue.manager.forward.queues.metric.commit.last", string::compose( max( commit_last)( state.forward.queues))},
                     { "queue.manager.forward.queues.metric.rollback.count", string::compose( accumulate( rollback_count)( state.forward.queues))},
                     { "queue.manager.forward.queues.metric.rollback.last", string::compose( max( rollback_last)( state.forward.queues))},
                  };
               };

               auto option()
               {
                  auto invoke = []()
                  {
                     terminal::formatter::key::value().print( std::cout, information::call());
                  };

                  return argument::Option{
                     std::move( invoke),
                     {  "--information"},
                     "collect aggregated information about queues in this domain"};
                  }

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

                  return argument::Group{ [](){}, { "queue"}, "queue related administration",
                     local::list::queues::option(),
                     local::list::remote::queues::option(),
                     local::list::groups::option(),
                     local::list::messages::option(),
                     local::list::forward::services::option(),
                     local::list::forward::queues::option(),
                     local::restore::option(),
                     local::enqueue::option(),
                     local::dequeue::option(),
                     local::peek::option(),
                     local::consume::option(),
                     local::clear::option(),
                     local::messages::remove::option(),
                     local::forward::scale::aliases::option(),
                     local::metric::reset::option(),
                     local::legend::option(),
                     local::information::option(),
                     local::state::option(),
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



